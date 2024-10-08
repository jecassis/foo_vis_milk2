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

#ifndef GEISS_TEXT_DRAWING_MANAGER
#define GEISS_TEXT_DRAWING_MANAGER

#include <set>
#include "dxcontext.h"

#define MAX_MSGS 4096

enum AlignType
{
    AlignNear,
    AlignCenter,
    AlignFar,
};

struct Alignment
{
    AlignType horizontal;
    AlignType vertical;
};

class TextStyle
{
  public:
    TextStyle(std::wstring fontName = L"Segoe UI",
              float fontSize = 24.0f,
              DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_NORMAL,
              DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL,
              DWRITE_TEXT_ALIGNMENT textAlignment = DWRITE_TEXT_ALIGNMENT_LEADING,
              DWRITE_TRIMMING_GRANULARITY trimmingGranularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER);

    void SetFontName(std::wstring fontName);
    void SetFontSize(float fontSize);
    void SetFontWeight(DWRITE_FONT_WEIGHT fontWeight);
    void SetFontStyle(DWRITE_FONT_STYLE fontStyle);
    void SetTextAlignment(DWRITE_TEXT_ALIGNMENT textAlignment);

    IDWriteTextFormat* GetTextFormat(IDWriteFactory* dwriteFactory);

    bool HasTextFormatChanged() const { return (m_textFormat == nullptr); }

  private:
    std::wstring m_fontName;
    float m_fontSize;
    DWRITE_FONT_WEIGHT m_fontWeight;
    DWRITE_FONT_STYLE m_fontStyle;
    DWRITE_TEXT_ALIGNMENT m_textAlignment;
    DWRITE_PARAGRAPH_ALIGNMENT m_paragraphAlignment;
    DWRITE_WORD_WRAPPING m_wordWrapping;
    DWRITE_TRIMMING_GRANULARITY m_trimmingGranularity;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
};

class ElementBase
{
  public:
    virtual void Initialize(ID2D1RenderTarget* d2dRenderTarget) { UNREFERENCED_PARAMETER(d2dRenderTarget); }
    virtual void Initialize(ID2D1DeviceContext* d2dContext) { UNREFERENCED_PARAMETER(d2dContext); }
    virtual void Update(float timeTotal, float timeDelta) { UNREFERENCED_PARAMETER(timeTotal); UNREFERENCED_PARAMETER(timeDelta); }
    virtual void Render(ID2D1RenderTarget* d2dRenderTarget, IDWriteFactory* dwriteFactory) { UNREFERENCED_PARAMETER(d2dRenderTarget); UNREFERENCED_PARAMETER(dwriteFactory); }
    virtual void Render(ID2D1DeviceContext* d2dContext, IDWriteFactory* dwriteFactory) { UNREFERENCED_PARAMETER(d2dContext); UNREFERENCED_PARAMETER(dwriteFactory); }
    virtual void ReleaseDeviceDependentResources() {}

    void SetAlignment(AlignType horizontal, AlignType vertical);
    virtual void SetContainer(const D2D1_RECT_F& container);
    void SetVisible(bool visible);

    D2D1_RECT_F GetBounds(IDWriteFactory* dwriteFactory);

    bool IsVisible() const { return m_visible; }

  protected:
    ElementBase();

    virtual void CalculateSize(IDWriteFactory* dwriteFactory) { UNREFERENCED_PARAMETER(dwriteFactory); }

    Alignment m_alignment;
    D2D1_RECT_F m_container;
    D2D1_SIZE_F m_size;
    bool m_visible;
};

typedef std::set<ElementBase*> ElementSet;

class TextElement : public ElementBase
{
  public:
    TextElement();
    virtual ~TextElement() {}

    virtual void Initialize(ID2D1RenderTarget* d2dRenderTarget);
    virtual void Initialize(ID2D1DeviceContext* d2dContext);
    virtual void Update(float timeTotal, float timeDelta);
    virtual void Render(ID2D1RenderTarget* d2dRenderTarget, IDWriteFactory* dwriteFactory);
    virtual void Render(ID2D1DeviceContext* d2dContext, IDWriteFactory* dwriteFactory);
    virtual void ReleaseDeviceDependentResources();

    void SetTextColor(const D2D1_COLOR_F& textColor);
    void SetTextOpacity(float textOpacity);
    void SetTextShadow(bool hasShadow);
    void SetTextBox(const D2D1_COLOR_F boxColor, const D2D1_RECT_F boxRect);

    void SetText(__nullterminated WCHAR* text);
    void SetText(std::wstring text);

    TextStyle* GetTextStyle() { return m_textStyle; }
    void SetTextStyle(TextStyle* textStyle) { m_textStyle = textStyle; }

    void FadeOut(float fadeOutTime);
    void FadeIn(float fadeOutTime);

  protected:
    virtual void CalculateSize(IDWriteFactory* dwriteFactory);
    void CreateTextLayout(IDWriteFactory* dwriteFactory);

    std::wstring m_text;
    D2D1_RECT_F m_textExtents;

    TextStyle* m_textStyle;

    bool m_hasShadow;
    bool m_hasBox;
    D2D1_RECT_F m_boxRect;

    bool m_isFadingOut;
    bool m_isFadingIn;
    float m_fadeStartingOpacity;
    float m_fadeOutTime;
    float m_fadeOutTimeElapsed;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_textColorBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_shadowColorBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_boxColorBrush;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> m_textLayout;
};

#ifndef _FOOBAR
typedef struct
{
    wchar_t* msg; // points to some character in `g_szMsgPool[2][]`
    TextStyle* font; // note: if this string is really a dark box, font will be NULL!
    D2D1_RECT_F rect;
    DWORD flags;
    DWORD color;
    DWORD bgColor;
    int added, deleted; // temporary; used during `DrawNow()`
    void* prev_dark_box_ptr; // temporary; used during `DrawNow()`
} td_string;
#endif

class CTextManager
{
  public:
    CTextManager() :
#ifndef _FOOBAR
        m_b(0), m_nMsg{0, 0}, m_next_msg_start_ptr(nullptr), m_blit_additively(0),
#endif
        m_lpDX(nullptr), m_dwriteFactory(nullptr), m_d2dDevice(nullptr), m_d2dContext(nullptr) {};
    ~CTextManager() {};

    // Note: If unable to create `lpTextSurface` full size, do not create it at all!
    //       OK if `lpTextSurface == NULL`; in that case, text will be drawn directly to screen (but not until end anyway).
    void Init(DXContext* lpDX
#ifndef _FOOBAR
              , ID3D11Texture2D* lpTextSurface, int bAdditive
#endif
    );
    void Finish();

    // Note: `pFont` must persist until `DrawNow()` is called!
    int DrawD2DText(TextStyle* pFont, TextElement* pElement, const wchar_t* szText, D2D1_RECT_F* pRect, DWORD flags, DWORD color, bool bBox, DWORD boxColor = 0xFF000000); // actually queues the text!
#ifndef _FOOBAR
    void DrawBox(D2D1_RECT_F* pRect, DWORD boxColor);
    void DrawDarkBox(D2D1_RECT_F* pRect) { DrawBox(pRect, 0xFF000000); }
#endif
    void DrawNow();
    void ClearAll(); // automatically called at end of `DrawNow()`

    void Update(/* float timeTotal, float timeDelta */);
    void Render(D2D1::Matrix3x2F orientation2D = D2D1::Matrix3x2F::Identity());

    void RegisterElement(ElementBase* element);
    void UnregisterElement(ElementBase* element);

  protected:
#ifndef _FOOBAR
    int m_blit_additively;
    int m_nMsg[2];
    td_string m_msg[2][MAX_MSGS];
    wchar_t* m_next_msg_start_ptr;
    int m_b;
#endif

  private:
    void ReleaseDeviceDependentResources();
    void Release()
    {
        m_stateBlock.Reset();
        m_d2dContext.Reset();
        m_d2dFactory.Reset();
        m_d2dDevice.Reset();
        m_dwriteFactory.Reset();
    }

    DXContext* m_lpDX;
    Microsoft::WRL::ComPtr<ID2D1Factory1> m_d2dFactory;
    Microsoft::WRL::ComPtr<ID2D1Device> m_d2dDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_d2dContext;
    Microsoft::WRL::ComPtr<IDWriteFactory1> m_dwriteFactory;
    Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock> m_stateBlock;

    ElementSet m_elements;
};

#endif