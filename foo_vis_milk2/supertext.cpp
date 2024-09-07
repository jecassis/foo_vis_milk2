/*
 * supertext.cpp - Interactive 3-D text implementation file.
 *
 * Copyright (c) Microsoft Corporation
 * SPDX-License-Identifier: MIT
 */

#include "pch.h"
#include "supertext.h"
namespace
{
#include "extrusionps.inc"
#include "extrusionvs.inc"
}

using namespace DirectX;
using Microsoft::WRL::ComPtr;

static const UINT sc_maxMsaaSampleCount = 4;
static const float sc_flatteningTolerance = .1f;
static const float sc_cursorWidth = 5.0f;

static const XMFLOAT3 sc_eyeLocation(0.0f, -100.0f, 400.0f);
static const XMFLOAT3 sc_eyeAt(0.0f, -50.0f, 0.0f);
static const XMFLOAT3 sc_eyeUp(0.0f, -1.0f, 0.0f);

// Template that creates dummy `IUnknown` method implementations.
// This is useful in situations where one is absolutely sure
// none of the `IUnknown` methods will be called (neither D2D nor
// DWrite will call these methods on a `ID2D1SimplifiedGeometrySink`
// or `IDWriteTextRenderer`). Using this technique also allows one
// to stack allocate such objects.
template <class T>
class NoRefComObject : public T
{
  public:
    template <typename U>
    NoRefComObject(U u) : T(u)
    {
    }

    template <typename U, typename V>
    NoRefComObject(U u, V v) : T(u, v)
    {
    }

    template <typename U, typename V, typename W>
    NoRefComObject(U u, V v, W w) : T(u, v, w)
    {
    }

    STDMETHOD_(ULONG, AddRef)(THIS) { return 0; }

    STDMETHOD_(ULONG, Release)(THIS) { return 0; }

    STDMETHOD(QueryInterface)(THIS_ REFIID /*riid*/, void** /*ppvObj*/) { return E_UNEXPECTED; }
};

// Returns true if the bounds are empty.
// Direct2D uses the convention that "right > left" is empty.
static bool IsEmptyBounds(D2D1_RECT_F& bounds)
{
    return bounds.left > bounds.right;
}

// Creates a path geometry containing no figures.
static HRESULT CreateEmptyGeometry(ID2D1Factory* pFactory, ID2D1Geometry** ppGeometry)
{
    HRESULT hr;
    ID2D1PathGeometry* pGeometry = NULL;

    hr = pFactory->CreatePathGeometry(&pGeometry);
    if (SUCCEEDED(hr))
    {
        ID2D1GeometrySink* pSink = NULL;
        hr = pGeometry->Open(&pSink);

        if (SUCCEEDED(hr))
        {
            hr = pSink->Close();

            if (SUCCEEDED(hr))
            {
                *ppGeometry = pGeometry;
                (*ppGeometry)->AddRef();
            }
            pSink->Release();
        }
        pGeometry->Release();
    }

    return hr;
}

// Utility class for snapping points and vertices to grid points.
// This is useful when preparing and consuming tessellation data
// (see notes further on in the code).
// Grid-points are spaced at 1/16 intervals and aligned on
class PointSnapper
{
  public:
    static HRESULT SnapGeometry(ID2D1Geometry* pGeometry, ID2D1Geometry** ppGeometry)
    {
        HRESULT hr;

        ID2D1Factory* pFactory = NULL;
        pGeometry->GetFactory(&pFactory);

        ID2D1PathGeometry* pPathGeometry = NULL;
        hr = pFactory->CreatePathGeometry(&pPathGeometry);

        if (SUCCEEDED(hr))
        {
            ID2D1GeometrySink* pSink = NULL;
            hr = pPathGeometry->Open(&pSink);

            if (SUCCEEDED(hr))
            {
                NoRefComObject<PointSnappingSink> snapper(pSink);

                hr = pGeometry->Simplify(
                    D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
                    NULL, // world transform
                    &snapper
                );

                if (SUCCEEDED(hr))
                {
                    hr = snapper.Close();

                    if (SUCCEEDED(hr))
                    {
                        *ppGeometry = pPathGeometry;
                        (*ppGeometry)->AddRef();
                    }
                }

                pSink->Release();
            }
            pPathGeometry->Release();
        }
        pFactory->Release();

        return hr;
    }

    static float SnapCoordinate(float x) { return floorf(16.0f * x + .5f) / 16.0f; }

    static D2D1_POINT_2F SnapPoint(D2D1_POINT_2F pt) { return D2D1::Point2F(SnapCoordinate(pt.x), SnapCoordinate(pt.y)); }

    static XMFLOAT2 SnapPoint(XMFLOAT2 pt) { return XMFLOAT2(SnapCoordinate(pt.x), SnapCoordinate(pt.y)); }

  private:
    // PointSnappingSink
    // Internal sink used to implement SnapGeometry.
    // Note: This class makes certain assumptions about it's usage
    // (e.g., no Beziers), which is why it's a private class.
    class PointSnappingSink : public ID2D1SimplifiedGeometrySink
    {
      public:
        PointSnappingSink(ID2D1SimplifiedGeometrySink* pSink) : m_pSinkNoRef(pSink) {}

        STDMETHOD_(void, AddBeziers)(const D2D1_BEZIER_SEGMENT* /*beziers*/, UINT /*beziersCount*/)
        {
            // Users should be sure to flatten their geometries prior to passing
            // through a PointSnappingSink. It makes little sense snapping
            // the control points of a Bezier, as the vertices from the
            // flattened Bezier will almost certainly not be snapped.
        }

        STDMETHOD_(void, AddLines)(const D2D1_POINT_2F* points, UINT pointsCount)
        {
            for (UINT i = 0; i < pointsCount; ++i)
            {
                D2D1_POINT_2F pt = SnapPoint(points[i]);

                m_pSinkNoRef->AddLines(&pt, 1);
            }
        }

        STDMETHOD_(void, BeginFigure)(D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin)
        {
            D2D1_POINT_2F pt = SnapPoint(startPoint);

            m_pSinkNoRef->BeginFigure(pt, figureBegin);
        }

        STDMETHOD_(void, EndFigure)(D2D1_FIGURE_END figureEnd) { m_pSinkNoRef->EndFigure(figureEnd); }

        STDMETHOD_(void, SetFillMode)(D2D1_FILL_MODE fillMode) { m_pSinkNoRef->SetFillMode(fillMode); }

        STDMETHOD_(void, SetSegmentFlags)(D2D1_PATH_SEGMENT vertexFlags) { m_pSinkNoRef->SetSegmentFlags(vertexFlags); }

        STDMETHOD(Close)() { return m_pSinkNoRef->Close(); }

      private:
        ID2D1SimplifiedGeometrySink* m_pSinkNoRef;
    };
};

// Helper function that performs "flattening" -- transforms a geometry with Beziers into one
// containing only line segments.
static HRESULT D2DFlatten(ID2D1Geometry* pGeometry, float flatteningTolerance, ID2D1Geometry** ppGeometry)
{
    HRESULT hr;
    ID2D1Factory* pFactory = NULL;
    pGeometry->GetFactory(&pFactory);

    ID2D1PathGeometry* pPathGeometry = NULL;
    hr = pFactory->CreatePathGeometry(&pPathGeometry);

    if (SUCCEEDED(hr))
    {
        ID2D1GeometrySink* pSink = NULL;
        hr = pPathGeometry->Open(&pSink);

        if (SUCCEEDED(hr))
        {
            hr = pGeometry->Simplify(
                D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
                NULL, // world transform
                flatteningTolerance,
                pSink
            );

            if (SUCCEEDED(hr))
            {
                hr = pSink->Close();

                if (SUCCEEDED(hr))
                {
                    *ppGeometry = pPathGeometry;
                    (*ppGeometry)->AddRef();
                }
            }
            pSink->Release();
        }
        pPathGeometry->Release();
    }

    pFactory->Release();

    return hr;
}

// Helper function that performs "outlining" -- constructing an equivalent geometry with no
// self-intersections.
// Note: Uses the default flattening tolerance and hence should not be used
// with very small geometries.
static HRESULT D2DOutline(ID2D1Geometry* pGeometry, ID2D1Geometry** ppGeometry)
{
    HRESULT hr;
    ID2D1Factory* pFactory = NULL;
    pGeometry->GetFactory(&pFactory);

    ID2D1PathGeometry* pPathGeometry = NULL;
    hr = pFactory->CreatePathGeometry(&pPathGeometry);

    if (SUCCEEDED(hr))
    {
        ID2D1GeometrySink* pSink = NULL;
        hr = pPathGeometry->Open(&pSink);

        if (SUCCEEDED(hr))
        {
            hr = pGeometry->Outline(NULL, pSink);

            if (SUCCEEDED(hr))
            {
                hr = pSink->Close();

                if (SUCCEEDED(hr))
                {
                    *ppGeometry = pPathGeometry;
                    (*ppGeometry)->AddRef();
                }
            }
            pSink->Release();
        }
        pPathGeometry->Release();
    }

    pFactory->Release();

    return hr;
}

// Helper function for Boolean operations.
// Note: Uses the default flattening tolerance and hence
// should not be used with very small geometries.
static HRESULT D2DCombine(D2D1_COMBINE_MODE combineMode, ID2D1Geometry* pGeometry1, ID2D1Geometry* pGeometry2, ID2D1Geometry** ppGeometry)
{
    HRESULT hr;
    ID2D1Factory* pFactory = NULL;
    pGeometry1->GetFactory(&pFactory);

    ID2D1PathGeometry* pPathGeometry = NULL;
    hr = pFactory->CreatePathGeometry(&pPathGeometry);

    if (SUCCEEDED(hr))
    {
        ID2D1GeometrySink* pSink = NULL;
        hr = pPathGeometry->Open(&pSink);

        if (SUCCEEDED(hr))
        {
            hr = pGeometry1->CombineWithGeometry(
                pGeometry2,
                combineMode,
                NULL, // world transform
                pSink
            );

            if (SUCCEEDED(hr))
            {
                hr = pSink->Close();

                if (SUCCEEDED(hr))
                {
                    *ppGeometry = pPathGeometry;
                    (*ppGeometry)->AddRef();
                }
            }
            pSink->Release();
        }
        pPathGeometry->Release();
    }

    pFactory->Release();

    return hr;
}

// Class for extruding an ID2D1Geometry into 3-D.
class Extruder
{
  public:
    static HRESULT ExtrudeGeometry(ID2D1Geometry* pGeometry, float height, std::vector<PNVertex>& vertices)
    {
        HRESULT hr;

        // The basic idea here is to generate the side faces by walking the
        // geometry and constructing quads, and use `ID2D1Geometry::Tessellate`
        // to generate the front and back faces.
        //
        // There are two things to be careful of here:
        //
        // 1) Must not produce overlapping triangles, as this can cause
        //    "depth-buffer fighting".
        // 2) The vertices on the front and back faces must perfectly align with
        //    the vertices on the side faces.
        //
        // Thankfully, D2D correctly handles self-intersections, which makes
        // solving issue 1 easy.
        //
        // Issue 2 is more complicated, since the D2D tessellation algorithm
        // will jitter vertices slightly. To get around this, snap vertices
        // to grid-points. To ensure that the tessellation jittering does cause
        // a vertex to be snapped to a neighboring grid-point, actually snap
        // the  vertices twice: once prior to tessellating and once after
        // tessellating. As long as the grid points are spaced further than
        // twice max-jitter distance, one can be sure the vertices will snap
        // to the right spot.
        ID2D1Geometry* pFlattenedGeometry = NULL;

        // Flatten the geometry first so there is no need to worry about stitching
        // together seams of Beziers.
        hr = D2DFlatten(pGeometry, sc_flatteningTolerance, &pFlattenedGeometry);

        if (SUCCEEDED(hr))
        {
            ID2D1Geometry* pOutlinedGeometry = NULL;

            // D2DOutline will remove any self-intersections. This is important to
            // ensure that the tessellator doesn't introduce new vertices (which
            // can cause T-junctions).
            hr = D2DOutline(pFlattenedGeometry, &pOutlinedGeometry);

            if (SUCCEEDED(hr))
            {
                ID2D1Geometry* pSnappedGeometry = NULL;
                hr = PointSnapper::SnapGeometry(pOutlinedGeometry, &pSnappedGeometry);

                if (SUCCEEDED(hr))
                {
                    NoRefComObject<ExtrudingSink> helper(height, &vertices);

                    hr = pSnappedGeometry->Tessellate(NULL, &helper);

                    if (SUCCEEDED(hr))
                    {
                        // Simplify is a convenient API for extracting the data out of a geometry.
                        hr = pOutlinedGeometry->Simplify(
                            D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
                            NULL, // world transform
                            &helper
                        );

                        if (SUCCEEDED(hr))
                        {
                            // This `Close()` call is a little ambiguous, since it refers both to the
                            // ID2D1TessellationSink and to the ID2D1SimplifiedGeometrySink.
                            // Thankfully, it really doesn't matter with our ExtrudingSink.
                            hr = helper.Close();
                        }
                    }
                    pSnappedGeometry->Release();
                }
                pOutlinedGeometry->Release();
            }

            pFlattenedGeometry->Release();
        }

        return hr;
    }

  private:
    // Internal sink used to implement Extruder.
    // Note: This class makes certain assumptions about it's usage
    // (e.g. no Beziers), which is why it's a private class.
    // Note 2: Both ID2D1SimplifiedGeometrySink and
    // ID2D1TessellationSink define a Close() method, which is
    // bending the rules a bit. This is another reason why we are
    // significantly limiting its usage.
    class ExtrudingSink : public ID2D1SimplifiedGeometrySink, public ID2D1TessellationSink
    {
      public:
        ExtrudingSink(float height, std::vector<PNVertex>* pVertices) : m_height(height), m_vertices(*pVertices), m_startPoint({0.0f, 0.0f}), m_lastPoint({0.0f, 0.0f}), m_hr(S_OK) {}

        STDMETHOD_(void, AddBeziers)(const D2D1_BEZIER_SEGMENT* /*beziers*/, UINT /*beziersCount*/)
        {
            // ExtrudingSink only handles line segments. Users should flatten
            // their geometry prior to passing through an ExtrudingSink.
        }

        STDMETHOD_(void, AddLines)(const D2D1_POINT_2F* points, UINT pointsCount)
        {
            if (SUCCEEDED(m_hr))
            {
                for (UINT i = 0; SUCCEEDED(m_hr) && i < pointsCount; ++i)
                {
                    Vertex2D v{};

                    v.pt = XMFLOAT2(points[i].x, points[i].y);

                    // Take care to ignore degenerate segments, as we will be
                    // unable to compute proper normals for them.
                    // Note: This doesn't handle near-degenerate segments, which
                    // should probably also be removed. The one complication here
                    // is that the segments should be removed from both the outline
                    // and the front/back tessellations.
                    if ((m_figureVertices.empty()) ||
                        (v.pt.x != m_figureVertices.back().pt.x) ||
                        (v.pt.y != m_figureVertices.back().pt.y))
                    {
                        m_figureVertices.push_back(v);
                        m_hr = S_OK;
                    }
                }
            }
        }

        STDMETHOD_(void, BeginFigure)(D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN /*figureBegin*/)
        {
            if (SUCCEEDED(m_hr))
            {
                m_figureVertices.clear();

                Vertex2D v = {
                    XMFLOAT2(startPoint.x, startPoint.y),
                    XMFLOAT2(0.0f, 0.0f), // dummy
                    XMFLOAT2(0.0f, 0.0f), // dummy
                    XMFLOAT2(0.0f, 0.0f)  // dummy
                };

                m_figureVertices.push_back(v);
                m_hr = S_OK;
            }
        }

        STDMETHOD(Close)() { return m_hr; }

        STDMETHOD_(void, EndFigure)(D2D1_FIGURE_END /*figureEnd*/)
        {
            if (SUCCEEDED(m_hr))
            {
                XMFLOAT2 front = m_figureVertices.front().pt;
                XMFLOAT2 back = m_figureVertices.back().pt;

                if (front.x == back.x && front.y == back.y)
                {
                    m_figureVertices.pop_back();
                }

                // If we only have one vertex, then there is nothing to draw!
                if (m_figureVertices.size() > 1)
                {
                    // Construct the triangles corresponding to the sides of
                    // the extruded object in 3 steps:
                    //
                    // Step 1: Snap vertices and calculate normals.
                    //         Note: It is important that we compute normals *before*
                    //         snapping the vertices, otherwise, the normals will become
                    //         discretized, which will manifest itself as faceting.
                    for (UINT i = 0; i < static_cast<UINT>(m_figureVertices.size()); ++i)
                    {
                        m_figureVertices[i].norm = GetNormal(i);
                        m_figureVertices[i].pt = PointSnapper::SnapPoint(m_figureVertices[i].pt);
                    }

                    // Step 2: Interpolate normals as appropriate.
                    for (UINT i = 0; i < static_cast<UINT>(m_figureVertices.size()); ++i)
                    {
                        UINT h = static_cast<int64_t>(i + static_cast<UINT>(m_figureVertices.size()) - 1) % m_figureVertices.size();

                        XMFLOAT2 n1 = m_figureVertices[h].norm;
                        XMFLOAT2 n2 = m_figureVertices[i].norm;

                        // Take a dot-product to determine if the angle between
                        // the normals is small. If it is, then average them so we
                        // get a smooth transition from one face to the next.
                        if ((n1.x * n2.x + n1.y * n2.y) > .5f)
                        {
                            XMFLOAT2 sum;
                            XMVECTOR vN1 = XMLoadFloat2(&m_figureVertices[h].norm);
                            XMVECTOR vN2 = XMLoadFloat2(&m_figureVertices[i].norm);
                            XMVECTOR vSum = vN2 + vN1;
                            XMStoreFloat2(&sum, vSum);

                            m_figureVertices[i].interpNorm1 = m_figureVertices[i].interpNorm2 = Normalize(sum);
                        }
                        else
                        {
                            m_figureVertices[i].interpNorm1 = m_figureVertices[h].norm;
                            m_figureVertices[i].interpNorm2 = m_figureVertices[i].norm;
                        }
                    }

                    // Step 3: Output the triangles.
                    // interpNorm1 == end normal of previous segment
                    // interpNorm2 == begin normal of next segment
                    for (UINT i = 0; i < static_cast<UINT>(m_figureVertices.size()); ++i)
                    {
                        UINT j = static_cast<int64_t>(i + 1) % m_figureVertices.size();

                        XMFLOAT2 pt = m_figureVertices[i].pt;
                        XMFLOAT2 nextPt = m_figureVertices[j].pt;

                        XMFLOAT2 ptNorm3 = m_figureVertices[i].interpNorm2;
                        XMFLOAT2 nextPtNorm2 = m_figureVertices[j].interpNorm1;

                        // These 6 vertices define two adjacent triangles that
                        // together form a quad.
                        PNVertex newVertices[6] = {
                            {XMFLOAT3(pt.x,     pt.y,      m_height / 2), XMFLOAT3(ptNorm3.x,     ptNorm3.y,     0.0f)},
                            {XMFLOAT3(pt.x,     pt.y,     -m_height / 2), XMFLOAT3(ptNorm3.x,     ptNorm3.y,     0.0f)},
                            {XMFLOAT3(nextPt.x, nextPt.y, -m_height / 2), XMFLOAT3(nextPtNorm2.x, nextPtNorm2.y, 0.0f)},
                            {XMFLOAT3(nextPt.x, nextPt.y, -m_height / 2), XMFLOAT3(nextPtNorm2.x, nextPtNorm2.y, 0.0f)},
                            {XMFLOAT3(nextPt.x, nextPt.y,  m_height / 2), XMFLOAT3(nextPtNorm2.x, nextPtNorm2.y, 0.0f)},
                            {XMFLOAT3(pt.x,     pt.y,      m_height / 2), XMFLOAT3(ptNorm3.x,     ptNorm3.y,     0.0f)},
                        };

                        for (UINT n = 0; SUCCEEDED(m_hr) && n < ARRAYSIZE(newVertices); ++n)
                        {
                            m_vertices.push_back(newVertices[n]);
                            m_hr = S_OK;
                        }
                    }
                }
            }
        }

        STDMETHOD_(void, SetFillMode)(D2D1_FILL_MODE /*fillMode*/)
        {
            // Do nothing
        }
        STDMETHOD_(void, SetSegmentFlags)(D2D1_PATH_SEGMENT /*vertexFlags*/)
        {
            // Do nothing
        }
        STDMETHOD_(void, AddTriangles)(const D2D1_TRIANGLE* triangles, UINT trianglesCount)
        {
            if (SUCCEEDED(m_hr))
            {
                // These triangles represent the front and back faces of the extrusion.
                for (UINT i = 0; i < trianglesCount; ++i)
                {
                    D2D1_TRIANGLE tri = triangles[i];

                    D2D1_POINT_2F d1 = {tri.point2.x - tri.point1.x, tri.point2.y - tri.point1.y};
                    D2D1_POINT_2F d2 = {tri.point3.x - tri.point2.x, tri.point3.y - tri.point2.y};

                    tri.point1 = PointSnapper::SnapPoint(tri.point1);
                    tri.point2 = PointSnapper::SnapPoint(tri.point2);
                    tri.point3 = PointSnapper::SnapPoint(tri.point3);

                    // Currently, tessellate does not guarantee the orientation
                    // of the triangles it produces, so we must check here.
                    float cross = d1.x * d2.y - d1.y * d2.x;
                    if (cross < 0)
                    {
                        D2D1_POINT_2F tmp = tri.point1;

                        tri.point1 = tri.point2;
                        tri.point2 = tmp;
                    }

                    PNVertex newVertices[] = {
                        {XMFLOAT3(tri.point1.x, tri.point1.y, m_height / 2),  XMFLOAT3(0.0f, 0.0f, 1.0f) },
                        {XMFLOAT3(tri.point2.x, tri.point2.y, m_height / 2),  XMFLOAT3(0.0f, 0.0f, 1.0f) },
                        {XMFLOAT3(tri.point3.x, tri.point3.y, m_height / 2),  XMFLOAT3(0.0f, 0.0f, 1.0f) },

                        // Note: these points are listed in a different order
                        // since the orientation of the back face should be
                        // the opposite of the front face.
                        {XMFLOAT3(tri.point2.x, tri.point2.y, -m_height / 2), XMFLOAT3(0.0f, 0.0f, -1.0f)},
                        {XMFLOAT3(tri.point1.x, tri.point1.y, -m_height / 2), XMFLOAT3(0.0f, 0.0f, -1.0f)},
                        {XMFLOAT3(tri.point3.x, tri.point3.y, -m_height / 2), XMFLOAT3(0.0f, 0.0f, -1.0f)},
                    };

                    for (UINT j = 0; SUCCEEDED(m_hr) && j < ARRAYSIZE(newVertices); ++j)
                    {
                        m_vertices.push_back(newVertices[j]);
                        m_hr = S_OK;
                    }
                }
            }
        }

      private:
        XMFLOAT2 GetNormal(UINT i)
        {
            UINT j = static_cast<int64_t>(i + 1) % m_figureVertices.size();

            XMFLOAT2 vecij;
            XMVECTOR pti = XMLoadFloat2(&m_figureVertices[i].pt);
            XMVECTOR ptj = XMLoadFloat2(&m_figureVertices[j].pt);
            XMVECTOR vij = ptj - pti;
            XMStoreFloat2(&vecij, vij);

            return Normalize(XMFLOAT2(vecij.y, vecij.x));
        }

        XMFLOAT2 Normalize(XMFLOAT2 pt)
        {
            XMVECTOR vt = XMLoadFloat2(&pt);
            XMVECTOR vRet = vt / sqrtf(pt.x * pt.x + pt.y * pt.y);
            XMFLOAT2 pRet;
            XMStoreFloat2(&pRet, vRet);

            return pRet;
        }

        struct Vertex2D
        {
            XMFLOAT2 pt;
            XMFLOAT2 norm;
            XMFLOAT2 interpNorm1;
            XMFLOAT2 interpNorm2;
        };

        HRESULT m_hr;

        float m_height;
        D2D1_POINT_2F m_lastPoint;
        D2D1_POINT_2F m_startPoint;

        std::vector<PNVertex>& m_vertices;

        std::vector<Vertex2D> m_figureVertices;
    };
};

// A "TextRenderer" that extracts the glyph runs of a text
// layout object and turns them into a geometry.
class OutlineRenderer : public IDWriteTextRenderer
{
  public:
    OutlineRenderer(ID2D1Factory* pFactory) : m_pFactory(pFactory)
    {
        m_pFactory->AddRef();
        m_pGeometry = NULL;
    }

    ~OutlineRenderer()
    {
        SafeReleaseT(&m_pFactory);
        SafeReleaseT(&m_pGeometry);
    }

    STDMETHOD(DrawGlyphRun)(
        void* /*clientDrawingContext*/,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE /*measuringMode*/,
        DWRITE_GLYPH_RUN const* glyphRun,
        DWRITE_GLYPH_RUN_DESCRIPTION const* /*glyphRunDescription*/,
        IUnknown* /*clientDrawingEffect*/
    )
    {
        HRESULT hr;

        ID2D1PathGeometry* pPathGeometry = NULL;

        hr = m_pFactory->CreatePathGeometry(&pPathGeometry);

        if (SUCCEEDED(hr))
        {
            ID2D1GeometrySink* pSink = NULL;

            hr = pPathGeometry->Open(&pSink);

            if (SUCCEEDED(hr))
            {
                hr = glyphRun->fontFace->GetGlyphRunOutline(
                    glyphRun->fontEmSize,
                    glyphRun->glyphIndices,
                    glyphRun->glyphAdvances,
                    glyphRun->glyphOffsets,
                    glyphRun->glyphCount,
                    glyphRun->isSideways,
                    (glyphRun->bidiLevel % 2) == 1,
                    pSink
                );

                if (SUCCEEDED(hr))
                {
                    hr = pSink->Close();

                    if (SUCCEEDED(hr))
                    {
                        ID2D1TransformedGeometry* pTransformedGeometry = NULL;
                        hr = m_pFactory->CreateTransformedGeometry(pPathGeometry, D2D1::Matrix3x2F::Translation(baselineOriginX, baselineOriginY), &pTransformedGeometry);

                        if (SUCCEEDED(hr))
                        {
                            hr = AddGeometry(pTransformedGeometry);

                            pTransformedGeometry->Release();
                        }
                    }
                }
                pSink->Release();
            }
            pPathGeometry->Release();
        }

        return hr;
    }

    STDMETHOD(DrawUnderline)(
        void* /*clientDrawingContext*/,
        FLOAT /*baselineOriginX*/,
        FLOAT /*baselineOriginY*/,
        DWRITE_UNDERLINE const* /*underline*/,
        IUnknown* /*clientDrawingEffect*/
    )
    {
        // Implement this to add support for underlines. See the
        // CustomTextRender in the DWrite PDC hands-on lab for
        // an example on how to do this.
        return E_NOTIMPL;
    }

    STDMETHOD(DrawStrikethrough)(
        void* /*clientDrawingContext*/,
        FLOAT /*baselineOriginX*/,
        FLOAT /*baselineOriginY*/,
        DWRITE_STRIKETHROUGH const* /*strikethrough*/,
        IUnknown* /*clientDrawingEffect*/
    )
    {
        // Implement this to add support for strikethroughs. See the
        // CustomTextRender in the DWrite PDC hands-on lab for
        // an example on how to do this.
        return E_NOTIMPL;
    }

    STDMETHOD(DrawInlineObject)(
        void* /*clientDrawingContext*/,
        FLOAT /*originX*/,
        FLOAT /*originY*/,
        IDWriteInlineObject* /*inlineObject*/,
        BOOL /*isSideways*/,
        BOOL /*isRightToLeft*/,
        IUnknown* /*clientDrawingEffect*/
    )
    {
        // Implement this to add support for inline objects.
        // See the CustomTextRender in the DWrite PDC hands-on lab for an
        // example on how to do this.
        return E_NOTIMPL;
    }

    STDMETHOD(IsPixelSnappingDisabled)(void* /*clientDrawingContext*/, BOOL* isDisabled)
    {
        *isDisabled = TRUE;
        return S_OK;
    }

    STDMETHOD(GetCurrentTransform)(void* /*clientDrawingContext*/, DWRITE_MATRIX* transform)
    {
        DWRITE_MATRIX matrix = {1, 0, 0, 1, 0, 0};

        *transform = matrix;

        return S_OK;
    }

    STDMETHOD(GetPixelsPerDip)(void* /*clientDrawingContext*/, FLOAT* pixelsPerDip)
    {
        *pixelsPerDip = 1.0f;

        return S_OK;
    }

    HRESULT GetGeometry(ID2D1Geometry** ppGeometry)
    {
        HRESULT hr = S_OK;

        if (m_pGeometry)
        {
            *ppGeometry = m_pGeometry;
            (*ppGeometry)->AddRef();
        }
        else
        {
            ID2D1PathGeometry* pGeometry = NULL;

            hr = m_pFactory->CreatePathGeometry(&pGeometry);

            if (SUCCEEDED(hr))
            {
                ID2D1GeometrySink* pSink = NULL;
                hr = pGeometry->Open(&pSink);

                if (SUCCEEDED(hr))
                {
                    hr = pSink->Close();

                    if (SUCCEEDED(hr))
                    {
                        *ppGeometry = pGeometry;
                        (*ppGeometry)->AddRef();
                    }

                    pSink->Release();
                }
                pGeometry->Release();
            }
        }

        return hr;
    }

  protected:
    HRESULT AddGeometry(ID2D1Geometry* pGeometry)
    {
        HRESULT hr = S_OK;

        if (m_pGeometry)
        {
            ID2D1Geometry* pCombinedGeometry = NULL;

            hr = D2DCombine(D2D1_COMBINE_MODE_UNION, m_pGeometry, pGeometry, &pCombinedGeometry);
            if (SUCCEEDED(hr))
            {
                SafeReplace(&m_pGeometry, pCombinedGeometry);
                pCombinedGeometry->Release();
            }
        }
        else
        {
            SafeReplace(&m_pGeometry, pGeometry);
        }

        return hr;
    }

  private:
    ID2D1Factory* m_pFactory;
    ID2D1Geometry* m_pGeometry;
};

// Static Data
/*static*/ const D3D11_INPUT_ELEMENT_DESC SuperText::sc_PNVertexLayout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

/* static*/ const UINT SuperText::sc_vertexBufferCount = 3 * 1000; // must be a multiple of 3

// Constructor -- initializes member data.
SuperText::SuperText(DXContext* lpDX) :
    m_pD2DFactory(nullptr),
    m_pDWriteFactory(nullptr),
    m_pDevice(nullptr),
    m_pContext(nullptr),
    m_pSwapChain(nullptr),
    m_pState(nullptr),
    m_pVertexBuffer(nullptr),
    m_pVertexLayout(nullptr),
    m_pTextGeometry(nullptr),
    m_pTextLayout(nullptr)
{
    m_characters = L"> ";
    m_fontFace = L"Gabriola";
    m_fontSize = 96.0f;

    XMStoreFloat4x4(&m_ProjectionMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&m_ViewMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());

    CreateDeviceIndependentResources(lpDX->GetD2DFactory(), lpDX->GetDWriteFactory());
    CreateDeviceDependentResources(lpDX->GetD3DDevice(), lpDX->GetDeviceResources()->GetD3DDeviceContext());
    SetSwapChain(lpDX->GetDeviceResources()->GetSwapChain());
}

// Destructor -- tears down member data.
SuperText::~SuperText()
{
    m_pState.Reset();
    m_pVertexBuffer.Reset();
    m_pVertexLayout.Reset();
    SafeReleaseT(&m_pTextGeometry);
    SafeReleaseT(&m_pTextLayout);
}

// Creates resources which are not bound
// to any device. Their lifetime effectively extends for the
// duration of the app. These resources include the D2D,
// DWrite, and WIC factories; and a DWrite Text Format object
// (used for identifying particular font characteristics) and
// a D2D geometry.
HRESULT SuperText::CreateDeviceIndependentResources(ID2D1Factory1* pD2DFactory, IDWriteFactory1* pDWriteFactory)
{
    HRESULT hr = S_OK;

    // Create D2D factory and DWrite factory.
    m_pD2DFactory = pD2DFactory;
    m_pDWriteFactory = pDWriteFactory;

    // Add command-prompt-ish like text to the start of the string.
    if (SUCCEEDED(hr))
    {
        m_characters = L"> ";
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        hr = UpdateTextGeometry();
    }

    return hr;
}

// Generates a layout object and a geometric outline
// corresponding to the current text string and stores the
// results in m_pTextLayout and m_pTextGeometry.
HRESULT SuperText::UpdateTextGeometry()
{
    HRESULT hr;

    IDWriteTextFormat* pFormat = NULL;

    hr = m_pDWriteFactory->CreateTextFormat(
        m_fontFace,
        NULL,
        DWRITE_FONT_WEIGHT_EXTRA_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        m_fontSize,
        L"", // locale name
        &pFormat
    );
    if (SUCCEEDED(hr))
    {
        IDWriteTextLayout* pLayout = NULL;
        hr = m_pDWriteFactory->CreateTextLayout(
            m_characters.data(),
            static_cast<UINT32>(m_characters.length()),
            pFormat,
            0.0f, // lineWidth (ignored because of NO_WRAP)
            m_fontSize, // lineHeight
            &pLayout
        );
        if (SUCCEEDED(hr))
        {
            hr = pLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            if (SUCCEEDED(hr))
            {
                IDWriteTypography* pTypography = NULL;
                hr = m_pDWriteFactory->CreateTypography(&pTypography);
                if (SUCCEEDED(hr))
                {
                    DWRITE_FONT_FEATURE fontFeature = {DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_7, 1};
                    hr = pTypography->AddFontFeature(fontFeature);
                    if (SUCCEEDED(hr))
                    {
                        DWRITE_TEXT_RANGE textRange = {0, static_cast<UINT32>(m_characters.length())};
                        hr = pLayout->SetTypography(pTypography, textRange);
                    }

                    pTypography->Release();
                }

                NoRefComObject<OutlineRenderer> renderer(m_pD2DFactory.Get());

                hr = pLayout->Draw(
                    NULL, // clientDrawingContext
                    &renderer,
                    0.0f, // originX
                    0.0f // originY
                );
                if (SUCCEEDED(hr))
                {
                    ID2D1Geometry* pGeometry = NULL;
                    hr = renderer.GetGeometry(&pGeometry);

                    if (SUCCEEDED(hr))
                    {
                        SafeReplace(&m_pTextGeometry, pGeometry);
                        SafeReplace(&m_pTextLayout, pLayout);
                        pGeometry->Release();
                    }
                }
            }
            pLayout->Release();
        }
        pFormat->Release();
    }

    return hr;
}

// Returns a version of the text geometry that is horizontally centered and
// vertically positioned so (0,0) is on the base line.
HRESULT SuperText::GenerateTextOutline(bool includeCursor, ID2D1Geometry** ppGeometry)
{
    HRESULT hr;

    DWRITE_LINE_METRICS lineMetrics;
    DWRITE_TEXT_METRICS textMetrics;

    UINT actualNumLines;

    // Assuming here that the text doesn't wrap and doesn't contain newlines.
    hr = m_pTextLayout->GetLineMetrics(
        &lineMetrics,
        1, // maxLineCount
        &actualNumLines // ignored
    );
    if (SUCCEEDED(hr))
    {
        hr = m_pTextLayout->GetMetrics(&textMetrics);
        if (SUCCEEDED(hr))
        {
            float offsetY = -lineMetrics.baseline;
            float offsetX = -textMetrics.widthIncludingTrailingWhitespace / 2;
            ID2D1TransformedGeometry* pTransformedGeometry = NULL;

            hr = m_pD2DFactory->CreateTransformedGeometry(m_pTextGeometry, D2D1::Matrix3x2F::Translation(offsetX, offsetY), &pTransformedGeometry);
            if (SUCCEEDED(hr))
            {
                ID2D1Geometry* pGeometry = NULL;
                pGeometry = pTransformedGeometry;
                pGeometry->AddRef();

                if (includeCursor)
                {
                    float x, y;
                    DWRITE_HIT_TEST_METRICS hitTestMetrics;

                    hr = m_pTextLayout->HitTestTextPosition(
                        lineMetrics.length,
                        TRUE, // isTrailingHit
                        &x,
                        &y,
                        &hitTestMetrics
                    );
                    if (SUCCEEDED(hr))
                    {
                        float width = sc_cursorWidth;
                        float left = x + offsetX;
                        ID2D1RectangleGeometry* pCursorGeometry = NULL;

                        hr = m_pD2DFactory->CreateRectangleGeometry(D2D1::RectF(left, hitTestMetrics.top + offsetY, left + width, 0.0f), &pCursorGeometry);
                        if (SUCCEEDED(hr))
                        {
                            ID2D1Geometry* pCombinedGeometry = NULL;
                            hr = D2DCombine(D2D1_COMBINE_MODE_UNION, pGeometry, pCursorGeometry, &pCombinedGeometry);
                            if (SUCCEEDED(hr))
                            {
                                SafeReplace(&pGeometry, pCombinedGeometry);
                                pCombinedGeometry->Release();
                            }
                            pCursorGeometry->Release();
                        }
                    }
                }

                // transfer reference
                *ppGeometry = pGeometry;
                pGeometry = NULL;
            }
            pTransformedGeometry->Release();
        }
    }

    return hr;
}

// Creates the D3D device-bound resources that are required to render.
HRESULT SuperText::CreateDeviceDependentResources(ID3D11Device1* pDevice, ID3D11DeviceContext1* pContext)
{
    HRESULT hr = E_FAIL;

    // Create D3D device and context.
    m_pDevice = pDevice;
    m_pContext = pContext;

    if (m_pDevice)
    {
        hr = S_OK;
        UINT msaaQuality = 0;
        UINT sampleCount = 0;

        if (SUCCEEDED(hr))
        {
            msaaQuality = 0;
            for (sampleCount = sc_maxMsaaSampleCount; SUCCEEDED(hr) && sampleCount > 0; --sampleCount)
            {
                hr = m_pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_B8G8R8A8_UNORM, sampleCount, &msaaQuality);

                if (SUCCEEDED(hr))
                {
                    if (msaaQuality > 0)
                    {
                        break;
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            // Create rasterizer state object
            D3D11_RASTERIZER_DESC rsDesc{};
            rsDesc.AntialiasedLineEnable = FALSE;
            rsDesc.CullMode = D3D11_CULL_BACK;
            rsDesc.DepthBias = 0;
            rsDesc.DepthBiasClamp = 0;
            rsDesc.DepthClipEnable = TRUE;
            rsDesc.FillMode = D3D11_FILL_SOLID;
            rsDesc.FrontCounterClockwise = FALSE; // must be FALSE for 10on9
            rsDesc.MultisampleEnable = TRUE;
            rsDesc.ScissorEnable = FALSE;
            rsDesc.SlopeScaledDepthBias = 0;

            hr = m_pDevice->CreateRasterizerState(&rsDesc, m_pState.ReleaseAndGetAddressOf());
        }

        if (SUCCEEDED(hr))
        {
            m_pContext->RSSetState(m_pState.Get());

            msaaQuality = 0;
            for (sampleCount = sc_maxMsaaSampleCount; SUCCEEDED(hr) && sampleCount > 0; --sampleCount)
            {
                hr = m_pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_B8G8R8A8_UNORM, sampleCount, &msaaQuality);

                if (SUCCEEDED(hr))
                {
                    if (msaaQuality > 0)
                    {
                        break;
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT SuperText::CreateWindowSizeDependentResources(int nWidth, int nHeight)
{
    HRESULT hr = S_OK;

    if (m_pDevice && m_pSwapChain)
    {
        if (SUCCEEDED(hr))
        {
            D3D11_BUFFER_DESC bd{};

            // Create the constant buffers.
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bd.CPUAccessFlags = 0;
            bd.ByteWidth = (sizeof(ConstantBufferNeverChanges) + 15) / 16 * 16;
            hr = m_pDevice->CreateBuffer(&bd, nullptr, &m_constantBufferNeverChanges);

            bd.ByteWidth = (sizeof(ConstantBufferChangeOnResize) + 15) / 16 * 16;
            hr |= m_pDevice->CreateBuffer(&bd, nullptr, &m_constantBufferChangeOnResize);

            bd.ByteWidth = (sizeof(ConstantBufferChangesEveryFrame) + 15) / 16 * 16;
            hr |= m_pDevice->CreateBuffer(&bd, nullptr, &m_constantBufferChangesEveryFrame);
        }

        if (SUCCEEDED(hr))
        {
            D3D11_SAMPLER_DESC sampDesc{};
            sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            sampDesc.MinLOD = 0;
            sampDesc.MaxLOD = FLT_MAX;
            hr = m_pDevice->CreateSamplerState(&sampDesc, m_samplerLinear.GetAddressOf());
        }

        if (SUCCEEDED(hr))
        {
            // Create the input layout.
            hr = m_pDevice->CreateInputLayout(sc_PNVertexLayout, ARRAYSIZE(sc_PNVertexLayout), extrusionvsCode, sizeof(extrusionvsCode), &m_pVertexLayout);

            // Load the shaders [and textures].
            hr |= m_pDevice->CreateVertexShader(extrusionvsCode, sizeof(extrusionvsCode), nullptr, &m_vertexShader);
            hr |= m_pDevice->CreatePixelShader(extrusionpsCode, sizeof(extrusionpsCode), nullptr, &m_pixelShader);
        }

        if (SUCCEEDED(hr))
        {
            // Set the input layout.
            m_pContext->IASetInputLayout(m_pVertexLayout.Get());

            D3D11_BUFFER_DESC bd{};
            bd.Usage = D3D11_USAGE_DYNAMIC;
            bd.ByteWidth = sc_vertexBufferCount * sizeof(PNVertex);
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            bd.MiscFlags = 0;

            hr = m_pDevice->CreateBuffer(&bd, NULL, m_pVertexBuffer.GetAddressOf());
        }

        if (SUCCEEDED(hr))
        {
            // Set the vertex buffer.
            UINT stride = sizeof(PNVertex);
            UINT offset = 0;
            ID3D11Buffer* pVertexBuffer = m_pVertexBuffer.Get();

            m_pContext->IASetVertexBuffers(
                0, // StartSlot
                1, // NumBuffers
                &pVertexBuffer,
                &stride,
                &offset
            );

            // Set primitive topology.
            m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // Initialize the world matrices.
            XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());

            // Initialize the view matrix.
            XMStoreFloat4x4(&m_ViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&sc_eyeLocation), XMLoadFloat3(&sc_eyeAt), XMLoadFloat3(&sc_eyeUp)));

            // Initialize the projection matrix.
            XMStoreFloat4x4(&m_ProjectionMatrix, XMMatrixPerspectiveFovLH(
                    static_cast<float>(XM_PI) * 0.24f, // fovy
                    nWidth / static_cast<float>(nHeight), // aspect
                    0.1f, // zn
                    800.0f // zf
                )
            );
            // 0-degree Z-rotation
            static const XMFLOAT4X4 Rotation0(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            );

            // Update variables that never change.
            ConstantBufferNeverChanges constantBufferNeverChanges{};
            XMStoreFloat4x4(&constantBufferNeverChanges.View, XMMatrixTranspose(XMLoadFloat4x4(&m_ViewMatrix)));
            XMStoreFloat4(&constantBufferNeverChanges.LightPosition[0], XMVectorSet(1200.0f, -20.0f, 400.0f, 0.0f));
            XMStoreFloat4(&constantBufferNeverChanges.LightColor[0], XMVectorSet(0.9f, 0.0f, 0.0f, 1.0f));
            m_pContext->UpdateSubresource(m_constantBufferNeverChanges.Get(), 0, nullptr, &constantBufferNeverChanges, 0, 0);

            ConstantBufferChangeOnResize changesOnResize{};
            XMStoreFloat4x4(&changesOnResize.Projection, XMMatrixTranspose(XMLoadFloat4x4(&m_ProjectionMatrix)));
            m_pContext->UpdateSubresource(m_constantBufferChangeOnResize.Get(), 0, nullptr, &changesOnResize, 0, 0);

            ConstantBufferChangesEveryFrame changesEveryFrame{};
            XMStoreFloat4x4(&changesEveryFrame.World, XMMatrixTranspose(XMLoadFloat4x4(&m_WorldMatrix)));
            m_pContext->UpdateSubresource(m_constantBufferChangesEveryFrame.Get(), 0, nullptr, &changesEveryFrame, 0, 0);
        }
    }

    return hr;
}

// Certain resources (e.g., device, swap chain, RT) are bound to a
// particular D3D device. Under certain conditions (eg. change
// display mode, remoting, removing a video adapter), it is
// necessary to discard device-specific resources. This method
// just releases all of the device-bound resources that we're
// holding onto.
void SuperText::DiscardDeviceResources()
{
    m_pState.Reset();
    m_pVertexBuffer.Reset();
    m_pVertexLayout.Reset();
}

//void SuperText::OnChar(SHORT key)
//{
//    if (key == '\r')
//    {
//        // Black hole
//    }
//    else if (key == '\b')
//    {
//        if (m_characters.length() > 2)
//        {
//            m_characters.pop_back();
//        }
//    }
//    else
//    {
//        // In the case of failure keep the previous characters, so can
//        // safely ignore the return value.
//        m_characters.push_back(key);
//    }
//
//    // In the case of failure keep the previous text geometry, so can
//    // safely ignore the return value.
//    UpdateTextGeometry();
//}

HRESULT SuperText::SetTextFont(const std::wstring& str, const PCWSTR face, float size)
{
    m_characters = str;
    m_fontFace = face;
    m_fontSize = size;

    return UpdateTextGeometry();
}

// This method is called when the app needs to paint the window. It uses a D2D
// RT to draw a gradient background into the swap chain buffer. Then, it uses
// a separate D2D RT to draw a 2D scene into a D3D texture. This texture is
// mapped onto a simple planar 3D model and displayed using D3D.
HRESULT SuperText::OnRender()
{
    HRESULT hr;
    static float t = 0.0f;
    static ULONGLONG dwTimeStart = 0;

    if (m_pContext)
    {
        ULONGLONG dwTimeCur = GetTickCount64();
        if (dwTimeStart == 0)
        {
            dwTimeStart = dwTimeCur;
        }
        t = (dwTimeCur - dwTimeStart) / 3000.0f;

        float a = (static_cast<float>(XM_PI)) / 4 * std::sin(2 * t);

        // A silly way to get a blinking cursor, but it works!
        bool showCursor = false; //std::sin(20 * t) < 0;

        ID2D1Geometry* pGeometry = NULL;

        // Note: We are dynamically extruding the geometry every frame here.
        // This is somewhat wasteful, but allows us to introduce more complex
        // geometry animations in the future.
        hr = GenerateTextOutline(showCursor, &pGeometry);

        if (SUCCEEDED(hr))
        {
            std::vector<PNVertex> vertices;

            hr = Extruder::ExtrudeGeometry(
                pGeometry,
                24.0f, // height
                vertices
            );

            if (SUCCEEDED(hr))
            {
                XMStoreFloat4x4(&m_WorldMatrix, XMMatrixRotationY(a));

                // Setup the graphics pipeline.
                m_pContext->IASetInputLayout(m_pVertexLayout.Get());
                m_pContext->VSSetConstantBuffers(0, 1, m_constantBufferNeverChanges.GetAddressOf());
                m_pContext->VSSetConstantBuffers(1, 1, m_constantBufferChangeOnResize.GetAddressOf());
                m_pContext->VSSetConstantBuffers(2, 1, m_constantBufferChangesEveryFrame.GetAddressOf());

                m_pContext->PSSetConstantBuffers(0, 1, m_constantBufferNeverChanges.GetAddressOf());
                //m_pContext->PSSetConstantBuffers(1, 1, m_constantBufferChangeOnResize.GetAddressOf());
                //m_pContext->PSSetConstantBuffers(2, 1, m_constantBufferChangesEveryFrame.GetAddressOf());
                m_pContext->PSSetSamplers(0, 1, m_samplerLinear.GetAddressOf());

                // Update variables that change once per frame.
                ConstantBufferChangesEveryFrame constantBufferChangesEveryFrame{};
                XMStoreFloat4x4(&constantBufferChangesEveryFrame.World, XMMatrixTranspose(XMLoadFloat4x4(&m_WorldMatrix)));
                m_pContext->UpdateSubresource(m_constantBufferChangesEveryFrame.Get(), 0, nullptr, &constantBufferChangesEveryFrame, 0, 0);

                // Render the scene.
                m_pContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
                m_pContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

                UINT verticesLeft = static_cast<UINT>(vertices.size());

                UINT index = 0;

                while (verticesLeft > 0)
                {
                    UINT verticesToCopy = std::min(verticesLeft, sc_vertexBufferCount);

                    D3D11_MAPPED_SUBRESOURCE resource;
                    hr = m_pContext->Map(m_pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

                    if (SUCCEEDED(hr))
                    {
                        memcpy(resource.pData, &vertices[index], verticesToCopy * sizeof(PNVertex));

                        m_pContext->Unmap(m_pVertexBuffer.Get(), 0);

                        m_pContext->Draw(verticesToCopy, 0);

                        verticesLeft -= verticesToCopy;
                        index += verticesToCopy;
                    }
                }
            }
            pGeometry->Release();
        }
    }

    return hr;
}