/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
  Copyright 2021-2024 Jimmy Cassis
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of Nullsoft nor the names of its contributors may be used to
      endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "pch.h"
#include "textmgr.h"

using namespace DX;
using namespace D2D1;
using Microsoft::WRL::ComPtr;

#ifndef _FOOBAR
#define MAX_MSG_CHARS (65536 * 2)
wchar_t g_szMsgPool[2][MAX_MSG_CHARS];
#endif

#pragma region TextStyle
TextStyle::TextStyle(std::wstring fontName,
                     float fontSize,
                     DWRITE_FONT_WEIGHT fontWeight,
                     DWRITE_FONT_STYLE fontStyle,
                     DWRITE_TEXT_ALIGNMENT textAlignment,
                     DWRITE_TRIMMING_GRANULARITY trimmingGranularity) :
    m_fontName(fontName),
    m_fontSize(fontSize),
    m_fontWeight(fontWeight),
    m_fontStyle(fontStyle),
    m_textAlignment(textAlignment),
    m_paragraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR),
    m_wordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP),
    m_trimmingGranularity(trimmingGranularity)
{
}

void TextStyle::SetFontName(std::wstring fontName)
{
    if (m_fontName != fontName)
    {
        m_fontName = fontName;
        m_textFormat = nullptr;
    }
}

void TextStyle::SetFontSize(float fontSize)
{
    if (m_fontSize != fontSize)
    {
        m_fontSize = fontSize;
        m_textFormat = nullptr;
    }
}

void TextStyle::SetFontWeight(DWRITE_FONT_WEIGHT fontWeight)
{
    if (m_fontWeight != fontWeight)
    {
        m_fontWeight = fontWeight;
        m_textFormat = nullptr;
    }
}

void TextStyle::SetFontStyle(DWRITE_FONT_STYLE fontStyle)
{
    if (m_fontStyle != fontStyle)
    {
        m_fontStyle = fontStyle;
        m_textFormat = nullptr;
    }
}

void TextStyle::SetTextAlignment(DWRITE_TEXT_ALIGNMENT textAlignment)
{
    if (m_textAlignment != textAlignment)
    {
        m_textAlignment = textAlignment;
        m_textFormat = nullptr;
    }
}

IDWriteTextFormat* TextStyle::GetTextFormat(IDWriteFactory* dwriteFactory)
{
    if (m_textFormat == nullptr)
    {
        ThrowIfFailed(dwriteFactory->CreateTextFormat(m_fontName.data(), // family
                                                      nullptr, // collection
                                                      m_fontWeight, // weight
                                                      m_fontStyle, // style
                                                      DWRITE_FONT_STRETCH_NORMAL, // stretch
                                                      m_fontSize, // size
                                                      L"en-US", // locale
                                                      &m_textFormat // text format object
                                                      ));

        ThrowIfFailed(m_textFormat->SetTextAlignment(m_textAlignment));
        ThrowIfFailed(m_textFormat->SetWordWrapping(m_wordWrapping));

        ThrowIfFailed(m_textFormat->SetParagraphAlignment(m_paragraphAlignment));

        ComPtr<IDWriteInlineObject> spInlineObject;
        ThrowIfFailed(dwriteFactory->CreateEllipsisTrimmingSign(m_textFormat.Get(), &spInlineObject));
        DWRITE_TRIMMING trimming = {m_trimmingGranularity, 0, 0};
        ThrowIfFailed(m_textFormat->SetTrimming(&trimming, spInlineObject.Get()));
    }

    return m_textFormat.Get();
}
#pragma endregion

#pragma region ElementBase
ElementBase::ElementBase() : m_container{}, m_size{}, m_visible(false)
{
    m_alignment.horizontal = AlignCenter;
    m_alignment.vertical = AlignCenter;
}

void ElementBase::SetAlignment(AlignType horizontal, AlignType vertical)
{
    m_alignment.horizontal = horizontal;
    m_alignment.vertical = vertical;
}

void ElementBase::SetContainer(const D2D1_RECT_F& container)
{
    m_container = container;
}

void ElementBase::SetVisible(bool visible)
{
    m_visible = visible;
}

D2D1_RECT_F ElementBase::GetBounds(IDWriteFactory* dwriteFactory)
{
    CalculateSize(dwriteFactory);

    D2D1_RECT_F bounds = RectF();

    switch (m_alignment.horizontal)
    {
        case AlignNear:
            bounds.left = m_container.left;
            bounds.right = bounds.left + m_size.width;
            break;

        case AlignCenter:
            bounds.left = m_container.left + (m_container.right - m_container.left - m_size.width) / 2.0f;
            bounds.right = bounds.left + m_size.width;
            break;

        case AlignFar:
            bounds.right = m_container.right;
            bounds.left = bounds.right - m_size.width;
            break;
    }

    switch (m_alignment.vertical)
    {
        case AlignNear:
            bounds.top = m_container.top;
            bounds.bottom = bounds.top + m_size.height;
            break;

        case AlignCenter:
            bounds.top = m_container.top + (m_container.bottom - m_container.top - m_size.height) / 2.0f;
            bounds.bottom = bounds.top + m_size.height;
            break;

        case AlignFar:
            bounds.bottom = m_container.bottom;
            bounds.top = bounds.bottom - m_size.height;
            break;
    }

    return bounds;
}
#pragma endregion

#pragma region TextElement
TextElement::TextElement() :
    m_boxRect{},
    m_hasBox(false),
    m_hasShadow(false),
    /*m_isFadingOut(false),*/ m_textExtents{},
    m_textStyle(nullptr)
{
}

void TextElement::Initialize(ID2D1DeviceContext* d2dContext)
{
    ThrowIfFailed(d2dContext->CreateSolidColorBrush(ColorF(ColorF::White), m_textColorBrush.ReleaseAndGetAddressOf()));
    ThrowIfFailed(d2dContext->CreateSolidColorBrush(ColorF(ColorF::Black), m_shadowColorBrush.ReleaseAndGetAddressOf()));
    ThrowIfFailed(d2dContext->CreateSolidColorBrush(ColorF(ColorF::LightSlateGray), m_boxColorBrush.ReleaseAndGetAddressOf()));
}

void TextElement::Update(/* float timeTotal, float timeDelta */)
{
    //if (m_isFadingOut)
    //{
    //    m_fadeOutTimeElapsed += timeDelta;

    //    float delta = std::min(1.0f, m_fadeOutTimeElapsed / m_fadeOutTime);
    //    SetTextOpacity((1.0f - delta) * m_fadeStartingOpacity);

    //    if (m_fadeOutTimeElapsed >= m_fadeOutTime)
    //    {
    //        m_isFadingOut = false;
    //        SetVisible(false);
    //    }
    //}
}

void TextElement::Render(ID2D1DeviceContext* d2dContext, IDWriteFactory* dwriteFactory)
{
    D2D1_RECT_F bounds = GetBounds(dwriteFactory);
    D2D1_POINT_2F origin = Point2F(bounds.left - m_textExtents.left, bounds.top - m_textExtents.top);

    if (m_hasBox)
    {
        //D2D1_RECT_F r3 = bounds;
        //r3.left -= m_boxMargin.left;
        //r3.top -= m_boxMargin.top;
        //r3.right += m_boxMargin.right;
        //r3.bottom += m_boxMargin.bottom;
        //m_boxColorBrush->SetOpacity(m_textColorBrush->GetOpacity() * 0.5f);
        d2dContext->FillRectangle(m_boxRect, m_boxColorBrush.Get()); // draw a filled rectangle
    }

    if (m_hasShadow)
    {
        m_shadowColorBrush->SetOpacity(m_textColorBrush->GetOpacity() * 0.5f);
        d2dContext->DrawTextLayout(Point2F(origin.x + 1.0f, origin.y + 1.0f), m_textLayout.Get(), m_shadowColorBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
    }

    d2dContext->DrawTextLayout(origin, m_textLayout.Get(), m_textColorBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NO_SNAP);
}

void TextElement::ReleaseDeviceDependentResources()
{
    m_visible = false;

    m_textColorBrush.Reset();
    m_shadowColorBrush.Reset();
    m_boxColorBrush.Reset();
    m_textLayout.Reset();
}

void TextElement::SetTextColor(const D2D1_COLOR_F& textColor)
{
    m_textColorBrush->SetColor(textColor);
}

void TextElement::SetTextOpacity(float textOpacity)
{
    m_textColorBrush->SetOpacity(textOpacity);
}

void TextElement::SetTextShadow(bool hasShadow)
{
    m_hasShadow = hasShadow;
}

void TextElement::SetTextBox(const D2D1_COLOR_F boxColor, const D2D1_RECT_F boxRect)
{
    m_boxColorBrush->SetColor(boxColor);
    m_boxColorBrush->SetOpacity(boxColor.a);
    m_boxRect = boxRect;
    m_hasBox = true;
}

void TextElement::SetText(__nullterminated WCHAR* text)
{
    SetText(std::wstring(text));
}

void TextElement::SetText(std::wstring text)
{
    if (m_text != text)
    {
        m_text = text;
        m_textLayout = nullptr;
    }
}

//void TextElement::FadeOut(float fadeOutTime)
//{
//    m_fadeStartingOpacity = m_textColorBrush->GetOpacity();
//    m_fadeOutTime = fadeOutTime;
//    m_fadeOutTimeElapsed = 0.0f;
//    m_isFadingOut = true;
//}

void TextElement::CalculateSize(IDWriteFactory* dwriteFactory)
{
    CreateTextLayout(dwriteFactory);

    DWRITE_TEXT_METRICS metrics;
    DWRITE_OVERHANG_METRICS overhangMetrics;
    ThrowIfFailed(m_textLayout->GetMetrics(&metrics));
    ThrowIfFailed(m_textLayout->GetOverhangMetrics(&overhangMetrics));

    m_textExtents = RectF(-overhangMetrics.left,
                          -overhangMetrics.top,
                          overhangMetrics.right + metrics.layoutWidth,
                          overhangMetrics.bottom + metrics.layoutHeight);

    m_size = SizeF(m_textExtents.right - m_textExtents.left, m_textExtents.bottom - m_textExtents.top);
}

void TextElement::CreateTextLayout(IDWriteFactory* dwriteFactory)
{
    if ((m_textLayout == nullptr) || m_textStyle->HasTextFormatChanged())
    {
        m_textLayout = nullptr;

        ThrowIfFailed(dwriteFactory->CreateTextLayout(m_text.data(),
                                                      static_cast<UINT32>(m_text.size()),
                                                      m_textStyle->GetTextFormat(dwriteFactory),
                                                      m_container.right - m_container.left,
                                                      m_container.bottom - m_container.top,
                                                      m_textLayout.ReleaseAndGetAddressOf()));
    }
}
#pragma endregion

#pragma region CTextManager
void CTextManager::ReleaseDeviceDependentResources()
{
    //Finish();

    ElementSet elements = m_elements;
    for (auto iterator = elements.begin(); iterator != elements.end(); iterator++)
    {
        (*iterator)->ReleaseDeviceDependentResources();
    }
}

void CTextManager::Init(DXContext* lpDX
#ifndef _FOOBAR
                        , ID3D11Texture2D* lpTextSurface, int bAdditive
#endif
)
{
    m_lpDX = lpDX;
    m_dwriteFactory = m_lpDX->GetDWriteFactory();
    m_d2dDevice = m_lpDX->GetD2DDevice();
    m_d2dContext = m_lpDX->GetD2DDeviceContext();
#ifndef _FOOBAR
    m_lpTextSurface = lpTextSurface;
    m_blit_additively = bAdditive;
#endif

    ComPtr<ID2D1Factory> factory;
    m_d2dDevice->GetFactory(&factory);

    ThrowIfFailed(factory.As(&m_d2dFactory));
    ThrowIfFailed(m_d2dFactory->CreateDrawingStateBlock(&m_stateBlock));

#ifndef _FOOBAR
    m_b = 0;
    m_nMsg[0] = 0;
    m_nMsg[1] = 0;
    m_next_msg_start_ptr = g_szMsgPool[m_b];
#endif
}

void CTextManager::Finish()
{
    for (auto it = m_elements.begin(); it != m_elements.end();)
    {
        (*it)->ReleaseDeviceDependentResources();
        it = m_elements.erase(it);
    }
    ReleaseDeviceDependentResources();

    m_stateBlock.Reset();
    //m_d2dContext.Reset();
    m_d2dFactory.Reset();
    //m_d2dDevice.Reset();
    //m_dwriteFactory.Reset();
}

void CTextManager::ClearAll()
{
#ifndef _FOOBAR
    m_nMsg[m_b] = 0;
    m_next_msg_start_ptr = g_szMsgPool[m_b];
#else
    //for (auto it = m_elements.begin(); it != m_elements.end(); ++it)
    //{
    //    if ((*it)->IsVisible())
    //        (*it)->SetVisible(false);
    //}
#endif
}

void CTextManager::DrawBox(D2D1_RECT_F* pRect, DWORD boxColor)
{
    if (!pRect)
        return;

#ifdef _FOOBAR
    UNREFERENCED_PARAMETER(boxColor);
#else
    if ((m_nMsg[m_b] < MAX_MSGS) && (static_cast<DWORD>(m_next_msg_start_ptr - g_szMsgPool[m_b]) + 1 < MAX_MSG_CHARS))
    {
        *m_next_msg_start_ptr = 0;

        m_msg[m_b][m_nMsg[m_b]].msg = m_next_msg_start_ptr;
        m_msg[m_b][m_nMsg[m_b]].font = NULL;
        m_msg[m_b][m_nMsg[m_b]].rect = *pRect;
        m_msg[m_b][m_nMsg[m_b]].flags = 0;
        m_msg[m_b][m_nMsg[m_b]].color = 0xFFFFFFFF;
        m_msg[m_b][m_nMsg[m_b]].bgColor = boxColor;
        m_nMsg[m_b]++;
        m_next_msg_start_ptr += 1;
    }
#endif
}

// Returns height of the text in logical units.
int CTextManager::DrawD2DText(TextStyle* pFont, TextElement* pElement, const wchar_t* szText, D2D1_RECT_F* pRect, DWORD flags, DWORD color, bool bBox, DWORD boxColor)
{
    if (!(pFont && pElement && pRect && szText))
        return 0;

    if (flags & DT_CALCRECT)
    {
        *pRect = pElement->GetBounds(m_lpDX->GetDWriteFactory());
        return static_cast<int>(ceilf(pRect->bottom - pRect->top));
    }

#ifdef _FOOBAR
    UNREFERENCED_PARAMETER(boxColor);
    UNREFERENCED_PARAMETER(bBox);
    UNREFERENCED_PARAMETER(color);
#else
    if (!m_d2dDevice)
        return 0;

    size_t len = wcslen(szText);

    if ((m_nMsg[m_b] < MAX_MSGS) && (reinterpret_cast<ULONG_PTR>(m_next_msg_start_ptr) - reinterpret_cast<ULONG_PTR>(g_szMsgPool[m_b]) + len + 1 < MAX_MSG_CHARS))
    {
        wcscpy_s(m_next_msg_start_ptr, len + 1, szText);

        m_msg[m_b][m_nMsg[m_b]].msg = m_next_msg_start_ptr;
        m_msg[m_b][m_nMsg[m_b]].font = pFont;
        m_msg[m_b][m_nMsg[m_b]].rect = *pRect;
        m_msg[m_b][m_nMsg[m_b]].flags = flags;
        m_msg[m_b][m_nMsg[m_b]].color = color;
        m_msg[m_b][m_nMsg[m_b]].bgColor = boxColor;

        // Shrink rectangles on new frame's text strings; important for deletions.
        m_msg[m_b][m_nMsg[m_b]].rect = pElement->GetBounds(m_lpDX->GetDWriteFactory());
        int h = static_cast<int>(ceilf(m_msg[m_b][m_nMsg[m_b]].rect.bottom - m_msg[m_b][m_nMsg[m_b]].rect.top));

        m_nMsg[m_b]++;
        m_next_msg_start_ptr += len + 1;

        if (bBox)
        {
            // Adds a message with no text, but the rectangle is the same as
            // the text, so it creates a black box.
            DrawBox(&m_msg[m_b][m_nMsg[m_b] - 1].rect, boxColor);
            // Swap the box with the text that precedes it, so it draws first
            // and becomes a background.
            td_string x = m_msg[m_b][m_nMsg[m_b] - 1];
            m_msg[m_b][m_nMsg[m_b] - 1] = m_msg[m_b][m_nMsg[m_b] - 2];
            m_msg[m_b][m_nMsg[m_b] - 2] = x;
            //pElement->SetTextBox(D2D1::ColorF(boxColor, static_cast<FLOAT>(((boxColor & 0xFF000000) >> 24) / 255.0f)));
        }
        return h;
    }
#endif

    // No room for more text? Return accurate information.
    D2D1_RECT_F r2 = pElement->GetBounds(m_lpDX->GetDWriteFactory());
    int h = static_cast<int>(ceilf(r2.bottom - r2.top));
    return h;
}

#define MATCH(i, j) \
    (m_msg[m_b][i].font == m_msg[1 - m_b][j].font && \
     m_msg[m_b][i].flags == m_msg[1 - m_b][j].flags && \
     m_msg[m_b][i].color == m_msg[1 - m_b][j].color && \
     m_msg[m_b][i].bgColor == m_msg[1 - m_b][j].bgColor && \
     memcmp(&m_msg[m_b][i].rect, &m_msg[1 - m_b][j].rect, sizeof(D2D1_RECT_F)) == 0 && \
     wcscmp(m_msg[m_b][i].msg, m_msg[1 - m_b][j].msg) == 0)

void CTextManager::DrawNow()
{
    if (!m_d2dDevice)
        return;

#ifndef _FOOBAR
    if (m_nMsg[m_b] > 0)
    {
#ifdef DX9_MILKDROP
        XMMATRIX Ortho2D = XMMatrixOrthographicLH(2.0f, -2.0f, 0.0f, 1.0f);
        m_lpDX->m_lpDevice->SetTransform(D3DTS_PROJECTION, &Ortho2D);

        constexpr size_t NUM_DIRTY_RECTS = 3;
        D2D1_RECT_F dirty_rect[NUM_DIRTY_RECTS];
        int dirty_rects_ready = 0;

        int bRTT = (m_lpTextSurface == NULL) ? 0 : 1;
        ID3D11Texture2D* pBackBuffer = NULL;
        D3D11_TEXTURE2D_DESC desc_backbuf, desc_text_surface;
#else
        int bRTT = 0;
#endif

        // Clear added/deleted flags.
        void* last_dark_box = NULL;
        for (int i = 0; i < m_nMsg[m_b]; i++)
        {
            m_msg[m_b][i].deleted = m_msg[m_b][i].added = 0;
            m_msg[m_b][i].prev_dark_box_ptr = last_dark_box;
            last_dark_box = (m_msg[m_b][i].font) ? last_dark_box : (void*)&m_msg[m_b][i];
        }

        last_dark_box = NULL;
        for (int j = 0; j < m_nMsg[1 - m_b]; j++)
        {
            m_msg[1 - m_b][j].deleted = m_msg[1 - m_b][j].added = 0;
            m_msg[1 - m_b][j].prev_dark_box_ptr = last_dark_box;
            last_dark_box = (m_msg[1 - m_b][j].font) ? last_dark_box : (void*)&m_msg[1 - m_b][j];
        }

        int bRedrawText = 0;
        if (!bRTT || (m_nMsg[m_b] > 0 && m_nMsg[1 - m_b] == 0))
        {
            bRedrawText = 2; // redraw ALL
        }
        else
        {
            // Try to synchronize the text strings from last frame plus this frame,
            // and label additions and deletions. Algorithm will catch:
            //  - Insertion of any number of items in one spot.
            //  - Deletion of any number of items from one spot.
            //  - Changes to 1 item.
            //  - Changes to 2 consecutive items.
            //    (provided that the 2 text strings immediately bounding the
            //     additions/deletions/change(s) are left unchanged)
            // In any other case, all the text is just re-rendered.
            int i = 0, j = 0;
            while (i < m_nMsg[m_b] && j < m_nMsg[1 - m_b])
            {
                // MATCH macro: first index is record number for current stuff;
                //              second index is record number for previous frame stuff.
                if (MATCH(i, j))
                {
                    i++;
                    j++;
                }
                else
                {
                    int continue_now = 0;

                    // Scan to see if something was added.
                    for (int i2 = i + 1; i2 < m_nMsg[m_b]; i2++)
                        if (MATCH(i2, j))
                        {
                            for (int i3 = i; i3 < i2; i3++)
                                m_msg[m_b][i3].added = 1;
                            i = i2;
                            bRedrawText = 1;
                            continue_now = 1;
                            break;
                        }
                    if (continue_now)
                        continue;

                    // Scan to see if something was deleted.
                    for (int j2 = j + 1; j2 < m_nMsg[1 - m_b]; j2++)
                        if (MATCH(i, j2))
                        {
                            for (int j3 = j; j3 < j2; j3++)
                                m_msg[1 - m_b][j3].deleted = 1;
                            j = j2;
                            bRedrawText = 1;
                            continue_now = 1;
                            break;
                        }
                    if (continue_now)
                        continue;

                    // Scan to see if just a small group of 1-4 items were changed
                    // [and are followed by two identical items again].
                    int break_now = 0;
                    for (int chgd = 1; chgd <= 4; chgd++)
                    {
                        if (i >= m_nMsg[m_b] - chgd || j >= m_nMsg[1 - m_b] - chgd)
                        {
                            // Only a few items left in one of the lists -> just finish it.
                            bRedrawText = 1;
                            break_now = 1;
                            break;
                        }
                        if (i < m_nMsg[m_b] - chgd && j < m_nMsg[1 - m_b] - chgd && MATCH(i + chgd, j + chgd))
                        {
                            for (int k = 0; k < chgd; k++)
                            {
                                m_msg[m_b][i + k].added = 1;
                                m_msg[1 - m_b][j + k].deleted = 1;
                            }
                            i += chgd;
                            j += chgd;

                            bRedrawText = 1;
                            continue_now = 1;
                            break;
                        }
                    }
                    if (break_now)
                        break;
                    if (continue_now)
                        continue;

                    // Otherwise, nontrivial case -> just re-render whole thing.
                    bRedrawText = 2; // redraw ALL
                    break;
                }
            }

            if (bRedrawText < 2)
            {
                while (i < m_nMsg[m_b])
                {
                    m_msg[m_b][i].added = 1;
                    bRedrawText = 1;
                    i++;
                }

                while (j < m_nMsg[1 - m_b])
                {
                    m_msg[1 - m_b][j].deleted = 1;
                    bRedrawText = 1;
                    j++;
                }
            }
        }

        // 0. Remember old render target and get surface descriptions.
#ifdef DX9_MILKDROP
        m_lpDX->m_lpDevice->GetRenderTarget(&pBackBuffer);
        pBackBuffer->GetDesc(&desc_backbuf);

        if (bRTT)
        {
            //if (m_lpDX->m_lpDevice->GetDepthStencilSurface(&pZBuffer) != S_OK)
            //    pZBuffer = NULL; // OK if return value != S_OK - just means there is no Z-buffer.
            if (m_lpTextSurface->GetLevelDesc(0, &desc_text_surface) != D3D_OK)
                bRTT = 0;

            m_lpDX->m_lpDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            m_lpDX->m_lpDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            m_lpDX->m_lpDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
            m_lpDX->m_lpDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

            m_lpDX->m_lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            m_lpDX->m_lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
            m_lpDX->m_lpDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

            m_lpDX->m_lpDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        }
        else
        {
            desc_text_surface = desc_backbuf;
        }
#endif

        if (bRTT && bRedrawText)
            do
            {
                // 1. Change render target
#ifdef DX9_MILKDROP
                m_lpDX->m_lpDevice->SetTexture(0, NULL);

                ID3D11Texture2D* pNewTarget = NULL;
                if (m_lpTextSurface->GetSurfaceLevel(0, &pNewTarget) != S_OK)
                {
                    bRTT = 0;
                    break;
                }
                m_lpDX->m_lpDevice->SetRenderTarget(pNewTarget);

                //m_lpDevice->SetDepthStencilSurface(???);
                pNewTarget->Release();

                m_lpDX->m_lpDevice->SetTexture(0, NULL);
#endif

                // 2. Clear to black.
#ifdef DX9_MILKDROP
                //m_lpDX->m_lpDevice->SetTexture(0, NULL);
                m_lpDX->m_lpDevice->SetBlendState(false);
                m_lpDX->m_lpDevice->SetVertexShader(NULL);
                //m_lpDX->m_lpDevice->SetFVF(WFVERTEX_FORMAT);
                m_lpDX->m_lpDevice->SetPixelShader(NULL, NULL);

                WFVERTEX v3[4];
#endif
                if (bRedrawText == 2)
                {
#ifdef DX9_MILKDROP
                    DWORD clearcolor = m_msg[m_b][j].bgColor;
                    for (int i = 0; i < 4; i++)
                    {
                        v3[i].x = -1.0f + 2.0f * (i % 2);
                        v3[i].y = -1.0f + 2.0f * (i / 2);
                        v3[i].z = 0;
                        v3[i].a = static_cast<int>(((clearcolor >> 24) & 0xFF) * 255) / 255.0f;
                        v3[i].r = static_cast<int>((clearcolor >> 16) & 0xFF) / 255.0f;
                        v3[i].g = static_cast<int>((clearcolor >> 8) & 0xFF) / 255.0f;
                        v3[i].b = static_cast<int>(clearcolor & 0xFF) / 255.0f;
                    }
                    m_lpDX->m_lpDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 2, v3, sizeof(WFVERTEX));
#else
                    ;
#endif
                }
                else
                {
                    // 1. Erase (draw black box over) any old text items deleted.
                    //    Also, update the dirty rects; stuff that was ABOVE/BELOW these guys will need redrawn!
                    //    (..picture them staggered)
                    for (int j = 0; j < m_nMsg[1 - m_b]; j++)
                    {
                        // erase text from PREV frame if it was deleted.
                        if (m_msg[1 - m_b][j].deleted)
                        {
#ifdef DX9_MILKDROP
                            float x0 = -1.0f + 2.0f * m_msg[1 - m_b][j].rect.left / (float)desc_text_surface.Width;
                            float x1 = -1.0f + 2.0f * m_msg[1 - m_b][j].rect.right / (float)desc_text_surface.Width;
                            float y0 = -1.0f + 2.0f * m_msg[1 - m_b][j].rect.top / (float)desc_text_surface.Height;
                            float y1 = -1.0f + 2.0f * m_msg[1 - m_b][j].rect.bottom / (float)desc_text_surface.Height;
                            DWORD bgcolor = m_msg[m_b][j].bgColor;
                            for (int k = 0; k < 4; k++)
                            {
                                v3[k].x = (k % 2) ? x0 : x1;
                                v3[k].y = (k / 2) ? y0 : y1;
                                v3[k].z = 0;
                                v3[k].a = static_cast<int>(((bgcolor >> 24) & 0xFF) * 255) / 255.0f;
                                v3[k].r = static_cast<int>((bgcolor >> 16) & 0xFF) / 255.0f;
                                v3[k].g = static_cast<int>((bgcolor >> 8) & 0xFF) / 255.0f;
                                v3[k].b = static_cast<int>(bgcolor & 0xFF) / 255.0f;
                            }
                            m_lpDX->m_lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, v3, sizeof(WFVERTEX));

                            //----------------------------------

                            // Special case:
                            // If something is erased, but it's completely inside a dark box,
                            // then don't add it to the dirty rectangle.
                            td_string* pDarkBox = (td_string*)m_msg[1 - m_b][j].prev_dark_box_ptr;
                            int add_to_dirty_rect = 1;
                            while (pDarkBox && add_to_dirty_rect)
                            {
                                RECT t;
                                UnionRect(&t, &pDarkBox->rect, &m_msg[1 - m_b][j].rect);
                                if (EqualRect(&t, &pDarkBox->rect))
                                    add_to_dirty_rect = 0;
                                pDarkBox = (td_string*)pDarkBox->prev_dark_box_ptr;
                            }

                            // Also, update dirty rectangles.
                            // First, check to see if this shares area or a border with
                            // any of the going dirty rects, and if so, expand that dirty rect.
                            if (add_to_dirty_rect)
                            {
                                int done = 0;
                                RECT t;
                                RECT r1 = m_msg[1 - m_b][j].rect;
                                RECT r2 = m_msg[1 - m_b][j].rect;
                                r2.top -= 1;
                                r2.left -= 1;
                                r2.right += 1;
                                r2.bottom += 1;
                                for (int i = 0; i < dirty_rects_ready; i++)
                                {
                                    if (IntersectRect(&t, &r2, &dirty_rect[i]))
                                    {
                                        // Expand the dirty rect to include `r1`.
                                        UnionRect(&t, &r1, &dirty_rect[i]);
                                        dirty_rect[i] = t;
                                        done = 1;
                                        break;
                                    }
                                }
                                if (done == 1)
                                    continue;

                                // If it is in a new spot, and there are still unused
                                // dirty rectangles, use those.
                                if (dirty_rects_ready < NUM_DIRTY_RECTS)
                                {
                                    dirty_rect[dirty_rects_ready] = r1;
                                    dirty_rects_ready++;
                                    continue;
                                }

                                // Otherwise, find the closest dirty rectangle...
                                float nearest_dist = 0.0f;
                                int nearest_id = 0;
                                for (int i = 0; i < NUM_DIRTY_RECTS; i++)
                                {
                                    int dx = 0, dy = 0;

                                    if (r1.left > dirty_rect[i].right)
                                        dx = r1.left - dirty_rect[i].right;
                                    else if (dirty_rect[i].left > r1.right)
                                        dx = dirty_rect[i].left - r1.right;

                                    if (r1.top > dirty_rect[i].bottom)
                                        dy = r1.top - dirty_rect[i].bottom;
                                    else if (dirty_rect[i].top > r1.bottom)
                                        dy = dirty_rect[i].top - r1.bottom;

                                    float dist = sqrtf((float)(dx * dx + dy * dy));
                                    if (i == 0 || dist < nearest_dist)
                                    {
                                        nearest_dist = dist;
                                        nearest_id = i;
                                    }
                                }
                                //...and expand it to include this one.
                                UnionRect(&t, &r1, &dirty_rect[nearest_id]);
                                dirty_rect[nearest_id] = t;
                            }
#else
                            ;
#endif
                        }
                    }

                    // 2. Erase AND REDRAW any of *this* frame's text that falls in
                    //    dirty rectangles from erasures of *previous* frame's deleted text.
                    for (int j = 0; j < m_nMsg[m_b]; j++)
                    {
#ifdef DX9_MILKDROP
                        RECT t;
#else
                        int dirty_rects_ready = 0;
#endif
                        // Note: None of these could be "deleted" status yet.
                        if (!m_msg[m_b][j].added)
                        {
                            // Check vs. dirty rectangles so far; if intersects any, erase and redraw this one.
                            for (int i = 0; i < dirty_rects_ready; i++)
                                if (m_msg[m_b][j].font
#ifdef DX9_MILKDROP
                                    && // exclude dark boxes... //fixme?
                                    IntersectRect(&t, &dirty_rect[i], &m_msg[m_b][j].rect)
#endif
                                )
                                {
#ifdef DX9_MILKDROP
                                    float x0 = -1.0f + 2.0f * m_msg[m_b][j].rect.left / (float)desc_text_surface.Width;
                                    float x1 = -1.0f + 2.0f * m_msg[m_b][j].rect.right / (float)desc_text_surface.Width;
                                    float y0 = -1.0f + 2.0f * m_msg[m_b][j].rect.top / (float)desc_text_surface.Height;
                                    float y1 = -1.0f + 2.0f * m_msg[m_b][j].rect.bottom / (float)desc_text_surface.Height;
                                    DWORD bgcolor = m_msg[m_b][j].bgColor;
                                    for (int i = 0; i < 4; i++)
                                    {
                                        v3[i].x = (i % 2) ? x0 : x1;
                                        v3[i].y = (i / 2) ? y0 : y1;
                                        v3[i].z = 0;
                                        v3[i].a = static_cast<int>(((bgcolor >> 24) & 0xFF) * 255) / 255.0f;
                                        v3[i].r = static_cast<int>((bgcolor >> 16) & 0xFF) / 255.0f;
                                        v3[i].g = static_cast<int>((bgcolor >> 8) & 0xFF) / 255.0f;
                                        v3[i].b = static_cast<int>(bgcolor & 0xFF) / 255.0f;
                                    }
                                    m_lpDX->m_lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, v3, sizeof(WFVERTEX));
#endif
                                    m_msg[m_b][j].deleted = 1;
                                    m_msg[m_b][j].added = 1;
                                    bRedrawText = 1;
                                }
                        }
                    }
                }
            } while (0);

        // 3. Render text to TEXT surface.
        if (bRedrawText)
        {
#ifdef DX9_MILKDROP
            m_lpDX->m_lpDevice->SetTexture(0, NULL);
            m_lpDX->m_lpDevice->SetTexture(1, NULL);
            m_lpDX->m_lpDevice->SetBlendState(false);
            m_lpDX->m_lpDevice->SetVertexShader(NULL);
            m_lpDX->m_lpDevice->SetPixelShader(NULL);
            m_lpDX->m_lpDevice->SetFVF(WFVERTEX_FORMAT);
#endif
            for (int i = 0; i < m_nMsg[m_b]; i++)
                if (bRedrawText == 2 || m_msg[m_b][i].added == 1)
                    if (m_msg[m_b][i].font) // dark boxes have "font == NULL"
#ifdef DX9_MILKDROP
                        m_msg[m_b][i].font->DrawTextW(NULL, m_msg[m_b][i].msg, -1, &m_msg[m_b][i].rect, m_msg[m_b][i].flags, m_msg[m_b][i].color); // warning: in DX9, the DT_WORD_ELLIPSIS and DT_NOPREFIX flags cause no text to render!!
#else
                        ;
#endif
                    else if (m_msg[m_b][i].added || bRedrawText == 2 || !bRTT)
                    {
#ifdef DX9_MILKDROP
                        WFVERTEX v3[4];
                        float x0 = -1.0f + 2.0f * m_msg[m_b][i].rect.left / (float)desc_text_surface.Width;
                        float x1 = -1.0f + 2.0f * m_msg[m_b][i].rect.right / (float)desc_text_surface.Width;
                        float y0 = -1.0f + 2.0f * m_msg[m_b][i].rect.top / (float)desc_text_surface.Height;
                        float y1 = -1.0f + 2.0f * m_msg[m_b][i].rect.bottom / (float)desc_text_surface.Height;
                        for (int k = 0; k < 4; k++)
                        {
                            v3[k].x = (k % 2) ? x0 : x1;
                            v3[k].y = (k / 2) ? y0 : y1;
                            v3[k].z = 0;
                            v3[k].Diffuse = m_msg[m_b][i].bgColor; //0xFF303000;
                        }
                        m_lpDX->m_lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, v3, sizeof(WFVERTEX));
#else
                        ;
#endif
                    }
        }

        if (bRTT)
        {
            // 4. Restore render target.
            if (bRedrawText)
            {
#ifdef DX9_MILKDROP
                m_lpDX->m_lpDevice->SetTexture(0, NULL);
                m_lpDX->m_lpDevice->SetRenderTarget(0, pBackBuffer); //, pZBuffer);
                //m_lpDevice->SetDepthStencilSurface(pZBuffer);
#else
                ;
#endif
            }

            // 5. Blit text surface to backbuffer.
#ifdef DX9_MILKDROP
            m_lpDX->m_lpDevice->SetTexture(0, m_lpTextSurface);
            m_lpDX->m_lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, m_blit_additively ? TRUE : FALSE);
            m_lpDX->m_lpDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            m_lpDX->m_lpDevice->SetRenderState(D3DRS_DESTBLEND, m_blit_additively ? D3DBLEND_ONE : D3DBLEND_ZERO);
            m_lpDX->m_lpDevice->SetVertexShader(NULL);
            m_lpDX->m_lpDevice->SetPixelShader(NULL);
            m_lpDX->m_lpDevice->SetFVF(SPRITEVERTEX_FORMAT);

            SPRITEVERTEX v3[4];
            ZeroMemory(v3, sizeof(SPRITEVERTEX) * 4);
            float fx = desc_text_surface.Width / (float)desc_backbuf.Width;
            float fy = desc_text_surface.Height / (float)desc_backbuf.Height;
            for (int i = 0; i < 4; i++)
            {
                v3[i].x = (i % 2 == 0) ? -1 : -1 + 2 * fx;
                v3[i].y = (i / 2 == 0) ? -1 : -1 + 2 * fy;
                v3[i].z = 0;
                v3[i].tu = ((i % 2 == 0) ? 0.0f : 1.0f) + 0.5f / desc_text_surface.Width; // FIXES BLURRY TEXT even when bilinear interp. is on (which can't be turned off on all cards!)
                v3[i].tv = ((i / 2 == 0) ? 0.0f : 1.0f) + 0.5f / desc_text_surface.Height; // FIXES BLURRY TEXT even when bilinear interp. is on (which can't be turned off on all cards!)
                v3[i].Diffuse = 0xFFFFFFFF;
            }

            DWORD oldblend[3];
            //m_lpDevice->GetTextureStageState(0, D3DTSS_MAGFILTER, &oldblend[0]);
            //m_lpDevice->GetTextureStageState(1, D3DTSS_MINFILTER, &oldblend[1]);
            //m_lpDevice->GetTextureStageState(2, D3DTSS_MIPFILTER, &oldblend[2]);
            m_lpDX->m_lpDevice->GetSamplerState(0, D3DSAMP_MAGFILTER, &oldblend[0]);
            m_lpDX->m_lpDevice->GetSamplerState(1, D3DSAMP_MINFILTER, &oldblend[1]);
            m_lpDX->m_lpDevice->GetSamplerState(2, D3DSAMP_MIPFILTER, &oldblend[2]);
            m_lpDX->m_lpDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
            m_lpDX->m_lpDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
            m_lpDX->m_lpDevice->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

            m_lpDX->m_lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, v3, sizeof(SPRITEVERTEX));

            m_lpDX->m_lpDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, oldblend[0]);
            m_lpDX->m_lpDevice->SetSamplerState(1, D3DSAMP_MINFILTER, oldblend[1]);
            m_lpDX->m_lpDevice->SetSamplerState(2, D3DSAMP_MIPFILTER, oldblend[2]);

            m_lpDX->m_lpDevice->SetBlendState(false);
#else
            ;
#endif
        }

#ifdef DX9_MILKDROP
        SafeRelease(pBackBuffer);
        //SafeRelease(pZBuffer);

        m_lpDX->m_lpDevice->SetTexture(0, NULL);
        m_lpDX->m_lpDevice->SetTexture(1, NULL);
        m_lpDX->m_lpDevice->SetBlendState(false);
        m_lpDX->m_lpDevice->SetVertexShader(NULL);
        m_lpDX->m_lpDevice->SetPixelShader(NULL);
        m_lpDX->m_lpDevice->SetFVF(SPRITEVERTEX_FORMAT);

        //D3DXMATRIX ident;
        //D3DXMatrixIdentity(&ident);
        //m_lpDevice->SetTransform(D3DTS_PROJECTION, &ident);
#endif
    }
#endif

    Render();

#ifndef _FOOBAR
    // Flip.
    m_b = 1 - m_b;
#endif

    ClearAll();
}

void CTextManager::Update(/* float timeTotal, float timeDelta */)
{
    //for (auto iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    //{
    //    (*iter)->Update(timeTotal, timeDelta);
    //}
}

void CTextManager::Render(Matrix3x2F orientation2D)
{
    m_d2dContext->SaveDrawingState(m_stateBlock.Get());
    m_d2dContext->BeginDraw();
    m_d2dContext->SetTransform(orientation2D);

    m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
    //FLOAT x, y;
    //m_d2dContext->GetDpi(&x, &y);
    //D2D1_PIXEL_FORMAT pixel_format = m_d2dContext->GetPixelFormat();

    for (auto iter = m_elements.begin(); iter != m_elements.end(); ++iter)
    {
        if ((*iter)->IsVisible())
            (*iter)->Render(m_lpDX->GetD2DDeviceContext(), m_lpDX->GetDWriteFactory());
    }

    // Ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
    // is lost. It will be handled during the next call to `Present()`.
    HRESULT hr = m_d2dContext->EndDraw();
    if (hr != D2DERR_RECREATE_TARGET)
    {
        ThrowIfFailed(hr);
    }

    m_d2dContext->RestoreDrawingState(m_stateBlock.Get());
}

void CTextManager::RegisterElement(ElementBase* element)
{
    m_elements.insert(element);
}

void CTextManager::UnregisterElement(ElementBase* element)
{
    auto iter = m_elements.find(element);
    if (iter != m_elements.end())
    {
        (*iter)->ReleaseDeviceDependentResources();
        m_elements.erase(iter);
    }
}
#pragma endregion