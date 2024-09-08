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
#include "utility.h"
#include "plugin.h"
#include "support.h"
#include "d3d11shim.h"

#define COLOR_NORM(x) (((int)(x * 255) & 0xFF) / 255.0f)
#define COPY_COLOR(x, y) { x.a = y.a; x.r = y.r; x.g = y.g; x.b = y.b; }

#define VERT_CLIP 0.75f // warning: top/bottom can get clipped if less than 0.65!

// This function evaluates whether the floating-point
// control word is set to single precision/round to nearest/
// exceptions disabled. If not, the function changes the
// control word to set them and returns TRUE, putting the
// old control word value in the passback location pointed
// to by pwOldCW.
static void MungeFPCW(WORD* /* pwOldCW */)
{
#if 0
    BOOL ret = FALSE;
    WORD wTemp, wSave;

    __asm fstcw wSave
    if (wSave & 0x300 ||            // Not single mode
        0x3f != (wSave & 0x3f) ||   // Exceptions enabled
        wSave & 0xC00)              // Not round to nearest mode
    {
        __asm
        {
            mov ax, wSave
            and ax, not 300h    ;; single mode
            or  ax, 3fh         ;; disable all exceptions
            and ax, not 0xC00   ;; round to nearest mode
            mov wTemp, ax
            fldcw   wTemp
        }
        ret = TRUE;
    }
    if (pwOldCW) *pwOldCW = wSave;
  //  return ret;
#else
    unsigned int current_word = 0;
#ifndef _WIN64
    _controlfp_s(&current_word, _PC_24, _MCW_PC); // single precision (not supported on x64 see MSDN)
#endif // !_WIN64
    _controlfp_s(&current_word, _RC_NEAR, _MCW_RC); // round to nearest mode
    _controlfp_s(&current_word, _EM_ZERODIVIDE, _EM_ZERODIVIDE); // disable divide-by-zero exception interrupt
#endif
}

#ifdef _M_IX86
void RestoreFPCW(WORD wSave)
{
    __asm fldcw wSave
}
#endif

// PARAMETERS
// ------------
// fTime:       sum of all fDeltaT's so far (excluding this one)
// fDeltaT:     time window for this frame
// fRate:       avg. rate (spawns per second) of generation
// fRegularity: regularity of generation
//               0.0: totally chaotic
//               0.2: getting chaotic / very jittered
//               0.4: nicely jittered
//               0.6: slightly jittered
//               0.8: almost perfectly regular
//               1.0: perfectly regular
// iNumSpawnedSoFar: the total number of spawnings so far
//
// RETURN VALUE
// ------------
// The number to spawn for this frame (add this to your net count!).
//
// COMMENTS
// --------
// The spawn values returned will, over time, match
// (within 1%) the theoretical totals expected based on the
// amount of time passed and the average generation rate.
//
// UNRESOLVED ISSUES
// -----------------
// Actual results of mixed gen. (0 < reg < 1) are about 1% too low
// in the long run (vs. analytical expectations). Decided not
// to bother fixing it since it's only 1% (and VERY consistent).
int GetNumToSpawn(float fTime, float fDeltaT, float fRate, float fRegularity, int iNumSpawnedSoFar)
{
    float fNumToSpawnReg;
    float fNumToSpawnIrreg;
    float fNumToSpawn;

    // Compute number spawned based on regular generation.
    fNumToSpawnReg = ((fTime + fDeltaT) * fRate) - iNumSpawnedSoFar;

    // Compute number spawned based on irregular (random) generation.
    if (fDeltaT <= 1.0f / fRate)
    {
        // case 1: avg. less than 1 spawn per frame
        if ((warand() % 16384) / 16384.0f < fDeltaT * fRate)
            fNumToSpawnIrreg = 1.0f;
        else
            fNumToSpawnIrreg = 0.0f;
    }
    else
    {
        // Case 2: average more than 1 spawn per frame.
        fNumToSpawnIrreg = fDeltaT * fRate;
        fNumToSpawnIrreg *= 2.0f * (warand() % 16384) / 16384.0f;
    }

    // Get linear combination of regular and irregular.
    fNumToSpawn = fNumToSpawnReg * fRegularity + fNumToSpawnIrreg * (1.0f - fRegularity);

    // Round to nearest integer for result.
    return static_cast<int>(fNumToSpawn + 0.49f);
}

// Clear the window contents to avoid a 1-pixel thick border of noise that
// sometimes sticks around.
void CPlugin::ClearGraphicsWindow()
{
    /*
    RECT rect;
    GetClientRect(GetPluginWindow(), &rect);

    HDC hdc = GetDC(GetPluginWindow());
    FillRect(hdc, &rect, m_hBlackBrush);
    ReleaseDC(GetPluginWindow(), hdc);
    */
}

bool CPlugin::RenderStringToTitleTexture()
{
    if (m_supertext.szText[0] == L'\0')
        return false;

    wchar_t szTextToDraw[512];
    swprintf_s(szTextToDraw, L" %s ", m_supertext.szText); // add a space at end for italicized fonts and at start, too, because it's centered!

    if (!m_lpDDSTitle)
        return false;

    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return false;
    
    int g_title_font_sizes[] = {
        6,   8,   10,  12,  14,  16,  20,  26,  32,  38,  44,  50,  56,  64,  72,  80,
        88,  96,  104, 112, 120, 128, 136, 144, 160, 192, 224, 256, 288, 320, 352, 384,
        416, 448, 480, 512
    }; // NOTE: DO NOT EXCEED 64 FONTS

    // Remember the original backbuffer and Z-buffer.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;
    //Microsoft::WRL::ComPtr<ID3D11Texture2D> pZBuffer;
    lpDevice->GetRenderTarget(&pBackBuffer);
    //lpDevice->GetDepthStencilSurface(&pZBuffer);

    // Set render target to m_lpDDSTitle.
    ID3D11DepthStencilView* origDSView = nullptr;
    ID3D11DepthStencilView* emptyDSView = nullptr;
    lpDevice->GetDepthView(&origDSView);
    lpDevice->SetRenderTarget(m_lpDDSTitle, &emptyDSView);
    lpDevice->SetTexture(0, nullptr);

    // Clear the texture to black.
    {
        lpDevice->SetVertexShader(NULL, NULL);
        //lpDevice->SetFVF(WFVERTEX_FORMAT);
        lpDevice->SetTexture(0, NULL);

        lpDevice->SetBlendState(false);

        // Set up a quad.
        WFVERTEX verts[4];
        for (int i = 0; i < 4; i++)
        {
            verts[i].x = (i % 2 == 0) ? -1.0f : 1.0f;
            verts[i].y = (i / 2 == 0) ? -1.0f : 1.0f;
            verts[i].z = 0.0f;
            verts[i].a = 1.0f; verts[i].r = 0.0f; verts[i].g = 0.0f; verts[i].b = 0.0f; // diffuse color; also acts as filler to align structure to 16 bytes (good for random access/indexed prims)
        }

        lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, verts, sizeof(WFVERTEX));
    }

    // 1. Clip title if too many characters.
    /*if (m_supertext.bIsSongTitle)
    {
        // Truncate song title if too long; don't clip custom messages, though!
        int clip_chars = 32;
        int user_title_size = GetFontHeight(SONGTITLE_FONT);

        constexpr int MIN_CHARS = 8; // maximum clip characters *for BIG FONTS*
        constexpr int MAX_CHARS = 64; // maximum clip chars *for tiny fonts*
        float t = (user_title_size - 10) / static_cast<float>(128 - 10);
        t = std::min(1.0f, std::max(0.0f, t));
        clip_chars = static_cast<int>(MAX_CHARS - (MAX_CHARS - MIN_CHARS) * t);

        if (static_cast<int>(wcsnlen_s(szTextToDraw, 512)) > clip_chars + 3)
            wcscpy_s(&szTextToDraw[clip_chars], 512 - clip_chars, L"...");
    }*/

    bool ret = false;

    // Use 2 lines; must leave room for bottom of 'g' characters and such!
    D2D1_RECT_F rect{};
    rect.left = 0.0f;
    rect.right = static_cast<FLOAT>(m_nTitleTexSizeX); // now allow text to go all the way over, since we're actually drawing!
    rect.top = static_cast<FLOAT>(m_nTitleTexSizeY * 1 / 21); // otherwise, top of '%' could be cut off (1/21 seems safe)
    rect.bottom = static_cast<FLOAT>(m_nTitleTexSizeY * 17 / 21); // otherwise, bottom of 'g' could be cut off (18/21 seems safe, but we want some leeway)

    DWORD textColor = 0xFFFFFFFF;
    DWORD backColor = 0xD0000000;
    D2D1_COLOR_F fTextColor = D2D1::ColorF(textColor, static_cast<FLOAT>(((textColor & 0xFF000000) >> 24) / 255.0f));
    D2D1_COLOR_F fBackColor = D2D1::ColorF(backColor, static_cast<FLOAT>(((backColor & 0xFF000000) >> 24) / 255.0f));

    if (!m_supertext.bIsSongTitle)
    {
        // Custom message -> pick font to use that will best fill the texture.
        std::unique_ptr<TextStyle> gdi_font;

        int lo = 0;
        int hi = sizeof(g_title_font_sizes) / sizeof(int) - 1;

        // Limit the size of the font used.
        //int user_title_size = GetFontHeight(SONGTITLE_FONT);
        //while (g_title_font_sizes[hi] > user_title_size * 2 && hi > 4)
        //    hi--;

        D2D1_RECT_F temp{};
        Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
        while (1) //(lo < hi-1)
        {
            int mid = (lo + hi) / 2;

            // Create new DirectWrite font at 'mid' size.
            gdi_font = std::make_unique<TextStyle>(
                m_supertext.nFontFace,
                static_cast<float>(g_title_font_sizes[mid]),
                m_supertext.bBold ? DWRITE_FONT_WEIGHT_BLACK : DWRITE_FONT_WEIGHT_REGULAR,
                m_supertext.bItal ? DWRITE_FONT_STYLE_ITALIC: DWRITE_FONT_STYLE_NORMAL,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_TRIMMING_GRANULARITY_NONE
            );

            if (gdi_font)
            {
                if (lo == hi - 1)
                    break; // DONE; but the 'lo'-size font is ready for use!

                temp = rect;
                if (!m_ddsTitle.IsVisible())
                {
                    m_ddsTitle.Initialize(m_lpDX->GetD2DDeviceContext());
                }
                m_ddsTitle.SetAlignment(AlignCenter, AlignCenter);
                m_ddsTitle.SetTextColor(fTextColor);
                m_ddsTitle.SetTextOpacity(fTextColor.a);
                m_ddsTitle.SetContainer(temp);
                m_ddsTitle.SetText(szTextToDraw);
                m_ddsTitle.SetTextStyle(gdi_font.get());
                m_ddsTitle.SetTextShadow(true);

                // Compute size of text if drawn with font of this size.
                temp = m_ddsTitle.GetBounds(m_lpDX->GetDWriteFactory());
                float h = temp.bottom - temp.top;

                // Adjust and prepare to reiterate.
                if (temp.right >= rect.right || h > rect.bottom - rect.top)
                    hi = mid;
                else
                    lo = mid;

                gdi_font.reset();
            }
        }

        if (gdi_font)
        {
            // Do actual drawing and set `m_supertext.nFontSizeUsed`; use 'lo' size.
            int h = m_text.DrawD2DText(gdi_font.get(), &m_ddsTitle, szTextToDraw, &temp, /*DT_NOPREFIX |*/ DT_SINGLELINE | DT_CENTER | DT_CALCRECT, textColor, false, backColor);
            temp.left = static_cast<FLOAT>(0);
            temp.right = static_cast<FLOAT>(m_nTitleTexSizeX); // now allow text to go all the way over, since actually drawing!
            temp.top = static_cast<FLOAT>(m_nTitleTexSizeY / 2 - h / 2);
            temp.bottom = static_cast<FLOAT>(m_nTitleTexSizeY / 2 + h / 2);
            m_ddsTitle.SetContainer(temp);
            m_supertext.nFontSizeUsed = m_text.DrawD2DText(gdi_font.get(), &m_ddsTitle, szTextToDraw, &temp, /*DT_NOPREFIX |*/ DT_SINGLELINE | DT_CENTER, textColor, false, backColor);

            ret = true;
        }
        else
        {
            ret = false;
        }

        // Clean up font.
        gdi_font.release();
    }
    else // Song title
    {
        D2D1_RECT_F temp{};
        std::unique_ptr<TextStyle> m_gdi_title_font_doublesize;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
        wchar_t* str = m_supertext.szText;

        // Create `m_gdi_title_font_doublesize`.
        int songtitle_font_size = m_fontinfo[SONGTITLE_FONT].nSize * m_nTitleTexSizeX / 256;
        if (songtitle_font_size < 6)
            songtitle_font_size = 6;

        m_gdi_title_font_doublesize = std::make_unique<TextStyle>(
            m_fontinfo[SONGTITLE_FONT].szFace,
            static_cast<float>(songtitle_font_size),
            m_fontinfo[SONGTITLE_FONT].bBold ? DWRITE_FONT_WEIGHT_BLACK : DWRITE_FONT_WEIGHT_REGULAR,
            m_fontinfo[SONGTITLE_FONT].bItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_TRIMMING_GRANULARITY_NONE
        );
        //if (!m_gdi_title_font_doublesize)
        //{
        //    MessageBox(NULL, WASABI_API_LNGSTRINGW(IDS_ERROR_CREATING_DOUBLE_SIZED_GDI_TITLE_FONT), WASABI_API_LNGSTRINGW_BUF(IDS_MILKDROP_ERROR, title, sizeof(title)), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
        //    return false;
        //}

        // Clip the text manually...
        // NOTE: DT_END_ELLIPSIS CAUSES NOTHING TO DRAW.
        int h = 0;
        int max_its = 6;
        int it = 0;
        while (it < max_its)
        {
            it++;

            temp = rect;
            if (!m_ddsTitle.IsVisible())
            {
                m_ddsTitle.Initialize(m_lpDX->GetD2DDeviceContext());
            }
            m_ddsTitle.SetAlignment(AlignCenter, AlignCenter);
            m_ddsTitle.SetTextColor(fTextColor);
            m_ddsTitle.SetTextOpacity(fTextColor.a);
            m_ddsTitle.SetContainer(temp);
            m_ddsTitle.SetText(str);
            m_ddsTitle.SetTextStyle(m_gdi_title_font_doublesize.get());
            m_ddsTitle.SetTextShadow(true);
            h = m_text.DrawD2DText(m_gdi_title_font_doublesize.get(), &m_ddsTitle, str, &temp, /*DT_NOPREFIX | DT_END_ELLIPSIS*/ DT_SINGLELINE | DT_CALCRECT, textColor, false, backColor);
            if (static_cast<int>(temp.right - temp.left) <= m_nTitleTexSizeX)
                break;

            // Manually clip the text... chop segments off the front.
            /*wchar_t* p = wcsstr(str, L" - ");
            if (p)
            {
                str = p + 3;
                continue;
            }*/

            // No more stuff to chop off the front; chop off the end with "...".
            size_t len = wcsnlen_s(str, 256);
            float fPercentToKeep = 0.91f * m_nTitleTexSizeX / (temp.right - temp.left);
            if (len > 8)
                lstrcpy(&str[(int)(len * fPercentToKeep)], L"...");
            break;
        }

        // Now actually draw it.
        temp.left = static_cast<FLOAT>(0);
        temp.right = static_cast<FLOAT>(m_nTitleTexSizeX); // now allow text to go all the way over, since actually drawing!
        temp.top = static_cast<FLOAT>(m_nTitleTexSizeY / 2 - h / 2);
        temp.bottom = static_cast<FLOAT>(m_nTitleTexSizeY / 2 + h / 2);
        m_ddsTitle.SetContainer(temp);
        m_supertext.nFontSizeUsed = m_text.DrawD2DText(m_gdi_title_font_doublesize.get(), &m_ddsTitle, str, &temp, DT_SINGLELINE | DT_CENTER /*| DT_NOPREFIX | DT_END_ELLIPSIS*/, textColor, false, backColor);

        ret = true;
    }

    // Change the render target back to the original setup.
    lpDevice->SetTexture(0, NULL);
    lpDevice->SetRenderTarget(pBackBuffer.Get(), &origDSView);
    //lpDevice->SetDepthStencilSurface(pZBuffer.Get());
    SafeRelease(pBackBuffer);
    SafeRelease(origDSView);
    //SafeRelease(pZBuffer);

    if (ret && m_supertext.fStartTime > 0.0f && !m_ddsTitle.IsVisible())
    {
        m_ddsTitle.SetVisible(true);
        m_text.RegisterElement(&m_ddsTitle);
    }

    return ret;
}

// Loads the `var_pf_*` variables in this CState object with the correct values.
// for vars that affect pixel motion, that means evaluating them at time==-1,
// (i.e. no blending with blend to value); the blending of the file dx/dy
// will be done *after* execution of the per-vertex code.
// for vars that do NOT affect pixel motion, evaluate them at the current time,
// so that if they're blending, both states see the blended value.
// clang-format off
void CPlugin::LoadPerFrameEvallibVars(CState* pState)
{
    // 1. Vars that affect pixel motion (eval at time == -1).
    *pState->var_pf_zoom        = (double)pState->m_fZoom.eval(-1.0f); // GetTime());
    *pState->var_pf_zoomexp     = (double)pState->m_fZoomExponent.eval(-1.0f); // GetTime());
    *pState->var_pf_rot         = (double)pState->m_fRot.eval(-1.0f); //GetTime());
    *pState->var_pf_warp        = (double)pState->m_fWarpAmount.eval(-1.0f); //GetTime());
    *pState->var_pf_cx          = (double)pState->m_fRotCX.eval(-1.0f); //GetTime());
    *pState->var_pf_cy          = (double)pState->m_fRotCY.eval(-1.0f); //GetTime());
    *pState->var_pf_dx          = (double)pState->m_fXPush.eval(-1.0f); //GetTime());
    *pState->var_pf_dy          = (double)pState->m_fYPush.eval(-1.0f); //GetTime());
    *pState->var_pf_sx          = (double)pState->m_fStretchX.eval(-1.0f); //GetTime());
    *pState->var_pf_sy          = (double)pState->m_fStretchY.eval(-1.0f); //GetTime());
    // Read-only.
    *pState->var_pf_time        = (double)(GetTime() - m_fStartTime);
    *pState->var_pf_fps         = (double)GetFps();
    *pState->var_pf_bass        = (double)mdsound.imm_rel[0];
    *pState->var_pf_mid         = (double)mdsound.imm_rel[1];
    *pState->var_pf_treb        = (double)mdsound.imm_rel[2];
    *pState->var_pf_bass_att    = (double)mdsound.avg_rel[0];
    *pState->var_pf_mid_att     = (double)mdsound.avg_rel[1];
    *pState->var_pf_treb_att    = (double)mdsound.avg_rel[2];
    *pState->var_pf_frame       = (double)GetFrame();
    //*pState->var_pf_monitor     = 0; -leave this as it was set in the per-frame INIT code!
    for (int vi=0; vi<NUM_Q_VAR; vi++)
        *pState->var_pf_q[vi]   = pState->q_values_after_init_code[vi]; //0.0f;
    *pState->var_pf_monitor     = pState->monitor_after_init_code;
    *pState->var_pf_progress    = (GetTime() - m_fPresetStartTime) / (m_fNextPresetTime - m_fPresetStartTime);

    // 2. Vars that do NOT affect pixel motion (eval at time == now).
    *pState->var_pf_decay         = (double)pState->m_fDecay.eval(GetTime());
    *pState->var_pf_wave_a        = (double)pState->m_fWaveAlpha.eval(GetTime());
    *pState->var_pf_wave_r        = (double)pState->m_fWaveR.eval(GetTime());
    *pState->var_pf_wave_g        = (double)pState->m_fWaveG.eval(GetTime());
    *pState->var_pf_wave_b        = (double)pState->m_fWaveB.eval(GetTime());
    *pState->var_pf_wave_x        = (double)pState->m_fWaveX.eval(GetTime());
    *pState->var_pf_wave_y        = (double)pState->m_fWaveY.eval(GetTime());
    *pState->var_pf_wave_mystery  = (double)pState->m_fWaveParam.eval(GetTime());
    *pState->var_pf_wave_mode     = (double)pState->m_nWaveMode; //?!?! -why won't it work if set to pState->m_nWaveMode???
    *pState->var_pf_ob_size       = (double)pState->m_fOuterBorderSize.eval(GetTime());
    *pState->var_pf_ob_r          = (double)pState->m_fOuterBorderR.eval(GetTime());
    *pState->var_pf_ob_g          = (double)pState->m_fOuterBorderG.eval(GetTime());
    *pState->var_pf_ob_b          = (double)pState->m_fOuterBorderB.eval(GetTime());
    *pState->var_pf_ob_a          = (double)pState->m_fOuterBorderA.eval(GetTime());
    *pState->var_pf_ib_size       = (double)pState->m_fInnerBorderSize.eval(GetTime());
    *pState->var_pf_ib_r          = (double)pState->m_fInnerBorderR.eval(GetTime());
    *pState->var_pf_ib_g          = (double)pState->m_fInnerBorderG.eval(GetTime());
    *pState->var_pf_ib_b          = (double)pState->m_fInnerBorderB.eval(GetTime());
    *pState->var_pf_ib_a          = (double)pState->m_fInnerBorderA.eval(GetTime());
    *pState->var_pf_mv_x          = (double)pState->m_fMvX.eval(GetTime());
    *pState->var_pf_mv_y          = (double)pState->m_fMvY.eval(GetTime());
    *pState->var_pf_mv_dx         = (double)pState->m_fMvDX.eval(GetTime());
    *pState->var_pf_mv_dy         = (double)pState->m_fMvDY.eval(GetTime());
    *pState->var_pf_mv_l          = (double)pState->m_fMvL.eval(GetTime());
    *pState->var_pf_mv_r          = (double)pState->m_fMvR.eval(GetTime());
    *pState->var_pf_mv_g          = (double)pState->m_fMvG.eval(GetTime());
    *pState->var_pf_mv_b          = (double)pState->m_fMvB.eval(GetTime());
    *pState->var_pf_mv_a          = (double)pState->m_fMvA.eval(GetTime());
    *pState->var_pf_echo_zoom     = (double)pState->m_fVideoEchoZoom.eval(GetTime());
    *pState->var_pf_echo_alpha    = (double)pState->m_fVideoEchoAlpha.eval(GetTime());
    *pState->var_pf_echo_orient   = (double)pState->m_nVideoEchoOrientation;
    *pState->var_pf_wave_usedots  = (double)pState->m_bWaveDots;
    *pState->var_pf_wave_thick    = (double)pState->m_bWaveThick;
    *pState->var_pf_wave_additive = (double)pState->m_bAdditiveWaves;
    *pState->var_pf_wave_brighten = (double)pState->m_bMaximizeWaveColor;
    *pState->var_pf_darken_center = (double)pState->m_bDarkenCenter;
    *pState->var_pf_gamma         = (double)pState->m_fGammaAdj.eval(GetTime());
    *pState->var_pf_wrap          = (double)pState->m_bTexWrap;
    *pState->var_pf_invert        = (double)pState->m_bInvert;
    *pState->var_pf_brighten      = (double)pState->m_bBrighten;
    *pState->var_pf_darken        = (double)pState->m_bDarken;
    *pState->var_pf_solarize      = (double)pState->m_bSolarize;
    *pState->var_pf_meshx         = (double)m_nGridX;
    *pState->var_pf_meshy         = (double)m_nGridY;
    *pState->var_pf_pixelsx       = (double)GetWidth();
    *pState->var_pf_pixelsy       = (double)GetHeight();
    *pState->var_pf_aspectx       = (double)m_fInvAspectX;
    *pState->var_pf_aspecty       = (double)m_fInvAspectY;
    *pState->var_pf_blur1min      = (double)pState->m_fBlur1Min.eval(GetTime());
    *pState->var_pf_blur2min      = (double)pState->m_fBlur2Min.eval(GetTime());
    *pState->var_pf_blur3min      = (double)pState->m_fBlur3Min.eval(GetTime());
    *pState->var_pf_blur1max      = (double)pState->m_fBlur1Max.eval(GetTime());
    *pState->var_pf_blur2max      = (double)pState->m_fBlur2Max.eval(GetTime());
    *pState->var_pf_blur3max      = (double)pState->m_fBlur3Max.eval(GetTime());
    *pState->var_pf_blur1_edge_darken = (double)pState->m_fBlur1EdgeDarken.eval(GetTime());
}

// Run per-frame calculations.
void CPlugin::RunPerFrameEquations(int code)
{
    /*
      Code is only valid when blending.
          OLDcomp ~ blend-from preset has a composite shader.
          NEWwarp ~ blend-to preset has a warp shader, etc.

      code OLDcomp NEWcomp OLDwarp NEWwarp
        0
        1            1
        2                            1
        3            1               1
        4     1
        5     1      1
        6     1                      1
        7     1      1               1
        8                    1
        9            1       1
        10                   1       1
        11           1       1       1
        12    1              1
        13    1      1       1
        14    1              1       1
        15    1      1       1       1
    */

    // When blending booleans (like darken, invert, etc) for pre-shader presets,
    // if blending to/from a pixel-shader preset, it is possible to tune the snap
    // point (when it changes during the blend) for a less jumpy transition.
    m_fSnapPoint = 0.5f;
    if (m_pState->m_bBlending)
    {
        switch (code)
        {
            case 4:
            case 6:
            case 12:
            case 14:
                // Old preset (only) had a comp shader.
                m_fSnapPoint = -0.01f;
                break;
            case 1:
            case 3:
            case 9:
            case 11:
                // New preset (only) has a comp shader.
                m_fSnapPoint = 1.01f;
                break;
            case 0:
            case 2:
            case 8:
            case 10:
                // Neither old or new preset had a comp shader.
                m_fSnapPoint = 0.5f;
                break;
            case 5:
            case 7:
            case 13:
            case 15:
                // Both old and new presets use a comp shader - so it won't matter.
                m_fSnapPoint = 0.5f;
                break;
        }
    }

    int num_reps = (m_pState->m_bBlending) ? 2 : 1;
    for (int rep = 0; rep < num_reps; rep++)
    {
        CState* pState;

        if (rep == 0)
            pState = m_pState;
        else
            pState = m_pOldState;

        // Values that will affect the pixel motion (and will be automatically blended
        // LATER, when the results of 2 sets of these params creates 2 different U/V
        // meshes that get blended together).
        LoadPerFrameEvallibVars(pState);

        // Also do just a once-per-frame init for the *per-**VERTEX*** *READ-ONLY* variables
        // (the non-read-only ones will be reset/restored at the start of each vertex)
        *pState->var_pv_time     = *pState->var_pf_time;
        *pState->var_pv_fps      = *pState->var_pf_fps;
        *pState->var_pv_frame    = *pState->var_pf_frame;
        *pState->var_pv_progress = *pState->var_pf_progress;
        *pState->var_pv_bass     = *pState->var_pf_bass;
        *pState->var_pv_mid      = *pState->var_pf_mid;
        *pState->var_pv_treb     = *pState->var_pf_treb;
        *pState->var_pv_bass_att = *pState->var_pf_bass_att;
        *pState->var_pv_mid_att  = *pState->var_pf_mid_att;
        *pState->var_pv_treb_att = *pState->var_pf_treb_att;
        *pState->var_pv_meshx    = (double)m_nGridX;
        *pState->var_pv_meshy    = (double)m_nGridY;
        *pState->var_pv_pixelsx  = (double)GetWidth();
        *pState->var_pv_pixelsy  = (double)GetHeight();
        *pState->var_pv_aspectx  = (double)m_fInvAspectX;
        *pState->var_pv_aspecty  = (double)m_fInvAspectY;
        //*pState->var_pv_monitor = *pState->var_pf_monitor;

#ifndef _NO_EXPR_
        // Execute once-per-frame expressions.
        if (pState->m_pf_codehandle)
        {
            NSEEL_code_execute(pState->m_pf_codehandle);
        }
#endif

        // Save some things for next frame.
        pState->monitor_after_init_code = *pState->var_pf_monitor;

        // Save some things for per-vertex code.
        for (int vi = 0; vi < NUM_Q_VAR; vi++)
            *pState->var_pv_q[vi] = *pState->var_pf_q[vi];

        // Range checks.
        *pState->var_pf_gamma = std::max(0.0, std::min(double(8), *pState->var_pf_gamma));
        *pState->var_pf_echo_zoom = std::max(0.001, std::min(double(1000), *pState->var_pf_echo_zoom));

        /*
        if (m_pState->m_bRedBlueStereo || m_bAlways3D)
        {
            // Override wave colors.
            *pState->var_pf_wave_r = 0.35f * (*pState->var_pf_wave_r) + 0.65f;
            *pState->var_pf_wave_g = 0.35f * (*pState->var_pf_wave_g) + 0.65f;
            *pState->var_pf_wave_b = 0.35f * (*pState->var_pf_wave_b) + 0.65f;
        }
        */
    }

    if (m_pState->m_bBlending)
    {
        // For all variables that do NOT affect pixel motion, blend them NOW,
        // so later the user can just access m_pState->m_pf_whatever.
        double mix = (double)CosineInterp(m_pState->m_fBlendProgress);
        double mix2 = 1.0 - mix;
        *m_pState->var_pf_decay        = mix * (*m_pState->var_pf_decay)        + mix2 * (*m_pOldState->var_pf_decay);
        *m_pState->var_pf_wave_a       = mix * (*m_pState->var_pf_wave_a)       + mix2 * (*m_pOldState->var_pf_wave_a);
        *m_pState->var_pf_wave_r       = mix * (*m_pState->var_pf_wave_r)       + mix2 * (*m_pOldState->var_pf_wave_r);
        *m_pState->var_pf_wave_g       = mix * (*m_pState->var_pf_wave_g)       + mix2 * (*m_pOldState->var_pf_wave_g);
        *m_pState->var_pf_wave_b       = mix * (*m_pState->var_pf_wave_b)       + mix2 * (*m_pOldState->var_pf_wave_b);
        *m_pState->var_pf_wave_x       = mix * (*m_pState->var_pf_wave_x)       + mix2 * (*m_pOldState->var_pf_wave_x);
        *m_pState->var_pf_wave_y       = mix * (*m_pState->var_pf_wave_y)       + mix2 * (*m_pOldState->var_pf_wave_y);
        *m_pState->var_pf_wave_mystery = mix * (*m_pState->var_pf_wave_mystery) + mix2 * (*m_pOldState->var_pf_wave_mystery);
        // wave_mode: exempt (integer)
        *m_pState->var_pf_ob_size       = mix * (*m_pState->var_pf_ob_size)    + mix2 * (*m_pOldState->var_pf_ob_size);
        *m_pState->var_pf_ob_r          = mix * (*m_pState->var_pf_ob_r)       + mix2 * (*m_pOldState->var_pf_ob_r);
        *m_pState->var_pf_ob_g          = mix * (*m_pState->var_pf_ob_g)       + mix2 * (*m_pOldState->var_pf_ob_g);
        *m_pState->var_pf_ob_b          = mix * (*m_pState->var_pf_ob_b)       + mix2 * (*m_pOldState->var_pf_ob_b);
        *m_pState->var_pf_ob_a          = mix * (*m_pState->var_pf_ob_a)       + mix2 * (*m_pOldState->var_pf_ob_a);
        *m_pState->var_pf_ib_size       = mix * (*m_pState->var_pf_ib_size)    + mix2 * (*m_pOldState->var_pf_ib_size);
        *m_pState->var_pf_ib_r          = mix * (*m_pState->var_pf_ib_r)       + mix2 * (*m_pOldState->var_pf_ib_r);
        *m_pState->var_pf_ib_g          = mix * (*m_pState->var_pf_ib_g)       + mix2 * (*m_pOldState->var_pf_ib_g);
        *m_pState->var_pf_ib_b          = mix * (*m_pState->var_pf_ib_b)       + mix2 * (*m_pOldState->var_pf_ib_b);
        *m_pState->var_pf_ib_a          = mix * (*m_pState->var_pf_ib_a)       + mix2 * (*m_pOldState->var_pf_ib_a);
        *m_pState->var_pf_mv_x          = mix * (*m_pState->var_pf_mv_x)       + mix2 * (*m_pOldState->var_pf_mv_x);
        *m_pState->var_pf_mv_y          = mix * (*m_pState->var_pf_mv_y)       + mix2 * (*m_pOldState->var_pf_mv_y);
        *m_pState->var_pf_mv_dx         = mix * (*m_pState->var_pf_mv_dx)      + mix2 * (*m_pOldState->var_pf_mv_dx);
        *m_pState->var_pf_mv_dy         = mix * (*m_pState->var_pf_mv_dy)      + mix2 * (*m_pOldState->var_pf_mv_dy);
        *m_pState->var_pf_mv_l          = mix * (*m_pState->var_pf_mv_l)       + mix2 * (*m_pOldState->var_pf_mv_l);
        *m_pState->var_pf_mv_r          = mix * (*m_pState->var_pf_mv_r)       + mix2 * (*m_pOldState->var_pf_mv_r);
        *m_pState->var_pf_mv_g          = mix * (*m_pState->var_pf_mv_g)       + mix2 * (*m_pOldState->var_pf_mv_g);
        *m_pState->var_pf_mv_b          = mix * (*m_pState->var_pf_mv_b)       + mix2 * (*m_pOldState->var_pf_mv_b);
        *m_pState->var_pf_mv_a          = mix * (*m_pState->var_pf_mv_a)       + mix2 * (*m_pOldState->var_pf_mv_a);
        *m_pState->var_pf_echo_zoom     = mix * (*m_pState->var_pf_echo_zoom)  + mix2 * (*m_pOldState->var_pf_echo_zoom);
        *m_pState->var_pf_echo_alpha    = mix * (*m_pState->var_pf_echo_alpha) + mix2 * (*m_pOldState->var_pf_echo_alpha);
        *m_pState->var_pf_echo_orient   = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_echo_orient   : *m_pState->var_pf_echo_orient;
        *m_pState->var_pf_wave_usedots  = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_wave_usedots  : *m_pState->var_pf_wave_usedots;
        *m_pState->var_pf_wave_thick    = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_wave_thick    : *m_pState->var_pf_wave_thick;
        *m_pState->var_pf_wave_additive = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_wave_additive : *m_pState->var_pf_wave_additive;
        *m_pState->var_pf_wave_brighten = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_wave_brighten : *m_pState->var_pf_wave_brighten;
        *m_pState->var_pf_darken_center = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_darken_center : *m_pState->var_pf_darken_center;
        *m_pState->var_pf_gamma         = mix * (*m_pState->var_pf_gamma) + mix2 * (*m_pOldState->var_pf_gamma);
        *m_pState->var_pf_wrap          = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_wrap     : *m_pState->var_pf_wrap;
        *m_pState->var_pf_invert        = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_invert   : *m_pState->var_pf_invert;
        *m_pState->var_pf_brighten      = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_brighten : *m_pState->var_pf_brighten;
        *m_pState->var_pf_darken        = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_darken   : *m_pState->var_pf_darken;
        *m_pState->var_pf_solarize      = (mix < m_fSnapPoint) ? *m_pOldState->var_pf_solarize : *m_pState->var_pf_solarize;
        *m_pState->var_pf_blur1min      = mix * (*m_pState->var_pf_blur1min) + mix2 * (*m_pOldState->var_pf_blur1min);
        *m_pState->var_pf_blur2min      = mix * (*m_pState->var_pf_blur2min) + mix2 * (*m_pOldState->var_pf_blur2min);
        *m_pState->var_pf_blur3min      = mix * (*m_pState->var_pf_blur3min) + mix2 * (*m_pOldState->var_pf_blur3min);
        *m_pState->var_pf_blur1max      = mix * (*m_pState->var_pf_blur1max) + mix2 * (*m_pOldState->var_pf_blur1max);
        *m_pState->var_pf_blur2max      = mix * (*m_pState->var_pf_blur2max) + mix2 * (*m_pOldState->var_pf_blur2max);
        *m_pState->var_pf_blur3max      = mix * (*m_pState->var_pf_blur3max) + mix2 * (*m_pOldState->var_pf_blur3max);
        *m_pState->var_pf_blur1_edge_darken = mix * (*m_pState->var_pf_blur1_edge_darken) + mix2 * (*m_pOldState->var_pf_blur1_edge_darken);
    }
}
// clang-format on

void CPlugin::RenderFrame(int bRedraw)
{
    float fDeltaT = 1.0f / GetFps();

    if (bRedraw)
    {
        // Pre-un-flip buffers to not repeat the same work done last frame...
        ID3D11Texture2D* pTemp = m_lpVS[0];
        m_lpVS[0] = m_lpVS[1];
        m_lpVS[1] = pTemp;
    }

    // Update time.
    /*
    float fDeltaT = (GetFrame() == 0) ? 1.0f / 30.0f : GetTime() - m_prev_time;
    DWORD dwTime = GetTickCount();
    float fDeltaT = (dwTime - m_dwPrevTickCount) * 0.001f;
    if (GetFrame() > 64)
    {
        fDeltaT = (fDeltaT)*0.2f + 0.8f * (1.0f / m_fps);
        if (fDeltaT > 2.0f / m_fps)
        {
            char buf[64];
            sprintf_s(buf, "fixing time gap of %5.3f seconds", fDeltaT);
            DumpDebugMessage(buf);

            fDeltaT = 1.0f / m_fps;
        }
    }
    m_dwPrevTickCount = dwTime;
    GetTime() += fDeltaT;
    */

    if (GetFrame() == 0)
    {
        m_fStartTime = GetTime();
        m_fPresetStartTime = GetTime();
    }

    if (m_fNextPresetTime < 0)
    {
        float dt = m_fTimeBetweenPresetsRand * (warand() % 1000) * 0.001f;
        m_fNextPresetTime = GetTime() + m_fBlendTimeAuto + m_fTimeBetweenPresets + dt;
    }

    // If the user has the preset LOCKED, or if they're in the middle of
    // saving it, then keep extending the time at which the auto-switch will occur
    // (by the length of this frame).
    if (m_bPresetLockedByUser || m_bPresetLockedByCode)
    {
        m_fPresetStartTime += fDeltaT;
        m_fNextPresetTime += fDeltaT;
    }

    // Update FPS.
    /*
    if (GetFrame() < 4)
    {
        m_fps = 0.0f;
    }
    else if (GetFrame() <= 64)
    {
        m_fps = GetFrame() / (float)(GetTime() - m_fTimeHistory[0]);
    }
    else
    {
        m_fps = 64.0f / (float)(GetTime() - m_fTimeHistory[m_nTimeHistoryPos]);
    }
    m_fTimeHistory[m_nTimeHistoryPos] = GetTime();
    m_nTimeHistoryPos = (m_nTimeHistoryPos + 1) % 64;
    */

    // Limit FPS, if necessary.
    /*
    if (m_nFpsLimit > 0 && (GetFrame() % 64) == 0 && GetFrame() > 64)
    {
        float spf_now = 1.0f / m_fps;
        float spf_desired = 1.0f / (float)m_nFpsLimit;

        float new_sleep = m_fFPSLimitSleep + (spf_desired - spf_now) * 1000.0f;

        if (GetFrame() <= 128)
            m_fFPSLimitSleep = new_sleep;
        else
            m_fFPSLimitSleep = m_fFPSLimitSleep * 0.8f + 0.2f * new_sleep;

        if (m_fFPSLimitSleep < 0)
            m_fFPSLimitSleep = 0;
        if (m_fFPSLimitSleep > 100)
            m_fFPSLimitSleep = 100;

        //sprintf_s(m_szUserMessage, "sleep=%f", m_fFPSLimitSleep);
        //m_fShowUserMessageUntilThisTime = GetTime() + 3.0f;
    }

    static float deficit;
    if (GetFrame() == 0)
        deficit = 0;
    float ideal_sleep = (m_fFPSLimitSleep + deficit);
    int actual_sleep = (int)ideal_sleep;
    if (actual_sleep > 0)
        Sleep(actual_sleep);
    deficit = ideal_sleep - actual_sleep;
    if (deficit < 0)
        deficit = 0; // just in case
    if (deficit > 1)
        deficit = 1; // just in case
    */

    if (!bRedraw)
    {
        m_rand_frame = XMFLOAT4(FRAND, FRAND, FRAND, FRAND);

        // Randomly change the preset, if it's time.
        if (m_fNextPresetTime < GetTime())
        {
            if (m_nLoadingPreset == 0) // don't start a load if one is already underway!
                LoadRandomPreset(m_fBlendTimeAuto);
        }

        // Randomly spawn song title, if time.
        if (m_fTimeBetweenRandomSongTitles > 0 &&
            !m_supertext.bRedrawSuperText &&
            GetTime() >= m_supertext.fStartTime + m_supertext.fDuration + 1.0f / GetFps())
        {
            int n = GetNumToSpawn(GetTime(), fDeltaT, 1.0f / m_fTimeBetweenRandomSongTitles, 0.5f, m_nSongTitlesSpawned);
            if (n > 0)
            {
                LaunchSongTitleAnim();
                m_nSongTitlesSpawned += n;
            }
        }

        // Randomly spawn custom message, if time.
        if (m_fTimeBetweenRandomCustomMsgs > 0 &&
            !m_supertext.bRedrawSuperText &&
            GetTime() >= m_supertext.fStartTime + m_supertext.fDuration + 1.0f / GetFps())
        {
            int n = GetNumToSpawn(GetTime(), fDeltaT, 1.0f / m_fTimeBetweenRandomCustomMsgs, 0.5f, m_nCustMsgsSpawned);
            if (n > 0)
            {
                LaunchCustomMessage(-1);
                m_nCustMsgsSpawned += n;
            }
        }

        // Update `m_fBlendProgress`.
        if (m_pState->m_bBlending)
        {
            m_pState->m_fBlendProgress = (GetTime() - m_pState->m_fBlendStartTime) / m_pState->m_fBlendDuration;
            if (m_pState->m_fBlendProgress > 1.0f)
            {
                m_pState->m_bBlending = false;
            }
        }

        // Handle hard cuts here (just after new sound analysis).
        //static float m_fHardCutThresh;
        if (GetFrame() == 0)
            m_fHardCutThresh = m_fHardCutLoudnessThresh * 2.0f;
        if (GetFps() > 1.0f && !m_bHardCutsDisabled && !m_bPresetLockedByUser && !m_bPresetLockedByCode)
        {
            if (mdsound.imm_rel[0] + mdsound.imm_rel[1] + mdsound.imm_rel[2] > m_fHardCutThresh * 3.0f)
            {
                if (m_nLoadingPreset == 0) // don't start a load if one is already underway!
                    LoadRandomPreset(0.0f);
                m_fHardCutThresh *= 2.0f;
            }
            else
            {
                /*
                float halflife_modified = m_fHardCutHalflife * 0.5f;
                //thresh = (thresh - 1.5f) * 0.99f + 1.5f;
                float k = -0.69315f / halflife_modified;
                */
                float k = -1.3863f / (m_fHardCutHalflife * GetFps());
                //float single_frame_multiplier = powf(2.7183f, k / GetFps());
                float single_frame_multiplier = expf(k);
                m_fHardCutThresh = (m_fHardCutThresh - m_fHardCutLoudnessThresh) * single_frame_multiplier + m_fHardCutLoudnessThresh;
            }
        }

        // Smooth and scale the audio data, according to m_state, for display purposes.
        float scale = m_pState->m_fWaveScale.eval(GetTime()) / 128.0f;
        mdsound.fWave[0][0] *= scale;
        mdsound.fWave[1][0] *= scale;
        float mix2 = m_pState->m_fWaveSmoothing.eval(GetTime());
        float mix1 = scale * (1.0f - mix2);
        for (int i = 1; i < NUM_AUDIO_BUFFER_SAMPLES; i++)
        {
            mdsound.fWave[0][i] = mdsound.fWave[0][i] * mix1 + mdsound.fWave[0][i - 1] * mix2;
            mdsound.fWave[1][i] = mdsound.fWave[1][i] * mix1 + mdsound.fWave[1][i - 1] * mix2;
        }
    }

    bool bOldPresetUsesWarpShader = (m_pOldState->m_nWarpPSVersion > 0);
    bool bNewPresetUsesWarpShader = (m_pState->m_nWarpPSVersion > 0);
    bool bOldPresetUsesCompShader = (m_pOldState->m_nCompPSVersion > 0);
    bool bNewPresetUsesCompShader = (m_pState->m_nCompPSVersion > 0);

    // Note: `code` is only meaningful if BLENDING.
    int code = (bOldPresetUsesWarpShader ? 8 : 0) |
               (bOldPresetUsesCompShader ? 4 : 0) |
               (bNewPresetUsesWarpShader ? 2 : 0) |
               (bNewPresetUsesCompShader ? 1 : 0);

    RunPerFrameEquations(code);

    // Restore any lost surfaces.
    //m_lpDD->RestoreAllSurfaces();

    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    // Remember the original backbuffer and zbuffer.
    ID3D11Texture2D* pBackBuffer = NULL; //, *pZBuffer = NULL;
    lpDevice->GetRenderTarget(&pBackBuffer);
    //lpDevice->GetDepthStencilSurface(&pZBuffer);
    // Set up render state DX11
    {
        //D3D11_TEXTURE_ADDRESS_MODE texaddr = (*m_pState->var_pf_wrap > m_fSnapPoint) ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
        D3D11_TEXTURE_ADDRESS_MODE texaddr = D3D11_TEXTURE_ADDRESS_WRAP;
        lpDevice->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, texaddr);
        lpDevice->SetRasterizerState(D3D11_CULL_NONE, D3D11_FILL_SOLID);
        lpDevice->SetDepth(false);
        lpDevice->SetShader(0); // note: this texture stage state setup works for 0 or 1 texture
                                //        if a texture is set, it will be modulated with the current diffuse color
                                //        if a texture is not set, it will just use the current diffuse color

        /*
        lpDevice->SetRenderState(D3DRS_WRAP0, 0);//D3DWRAPCOORD_0|D3DWRAPCOORD_1|D3DWRAPCOORD_2|D3DWRAPCOORD_3);
        lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
        lpDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
        lpDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        lpDevice->SetRenderState(D3DRS_COLORVERTEX, TRUE);
        lpDevice->SetRenderState(D3DRS_AMBIENT, 0xFFFFFFFF);  //?
        lpDevice->SetRenderState(D3DRS_CLIPPING, TRUE);

        lpDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        lpDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        lpDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
        lpDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        lpDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        */

        // NOTE: Do not forget to call SetTexture and SetVertexShader before drawing!
        // Examples:
        //      SPRITEVERTEX verts[4]; // has texcoords
        //      lpDevice->SetTexture(0, m_sprite_tex);
        //      lpDevice->SetVertexShader(SPRITEVERTEX_FORMAT);
        //
        //      WFVERTEX verts[4]; // no texcoords
        //      lpDevice->SetTexture(0, NULL);
        //      lpDevice->SetVertexShader(WFVERTEX_FORMAT);
    }

    // Render string to m_lpDDSTitle, if necessary.
    if (m_supertext.bRedrawSuperText)
    {
        if (!RenderStringToTitleTexture())
            m_supertext.fStartTime = -1.0f;
        m_supertext.bRedrawSuperText = false;
    }

    // Set up to render [from NULL] to VS0 (for motion vectors).
    {
        lpDevice->SetTexture(0, NULL);
        lpDevice->SetRenderTarget(m_lpVS[0]);
    }

    // Draw motion vectors to VS0.
    DrawMotionVectors();

    lpDevice->SetTexture(0, NULL);
    lpDevice->SetTexture(1, NULL);

    // On first frame, clear OLD VS.
    if (m_nFramesSinceResize == 0)
    {
        float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        lpDevice->ClearRenderTarget(m_lpVS[0], color);
    }

    // Set up to render [from VS0] to VS1.
    {
        lpDevice->SetRenderTarget(m_lpVS[1]);
    }

    if (m_bAutoGamma && GetFrame() == 0)
    {
        m_n16BitGamma = 0; // skip gamma correction for 32-bit color in Direct3D 11
    }

    ComputeGridAlphaValues();

    // Do the warping for this frame [warp shader].
    if (!m_pState->m_bBlending)
    {
        // No blend.
        if (bNewPresetUsesWarpShader)
            WarpedBlit_Shaders(1, false, false, false, false);
        else
            WarpedBlit_NoShaders(1, false, false, false, false);
    }
    else
    {
        // Blending.
        // WarpedBlit( nPass,  bAlphaBlend, bFlipAlpha, bCullTiles, bFlipCulling )
        // Note: alpha values go from 0..1 during a blend.
        // Note: bFlipCulling==false means tiles with alpha>0 will draw.
        //       bFlipCulling==true  means tiles with alpha<255 will draw.
        if (bOldPresetUsesWarpShader && bNewPresetUsesWarpShader)
        {
            WarpedBlit_Shaders(0, false, false, true, true);
            WarpedBlit_Shaders(1, true, false, true, false);
        }
        else if (!bOldPresetUsesWarpShader && bNewPresetUsesWarpShader)
        {
            WarpedBlit_NoShaders(0, false, false, true, true);
            WarpedBlit_Shaders(1, true, false, true, false);
        }
        else if (bOldPresetUsesWarpShader && !bNewPresetUsesWarpShader)
        {
            WarpedBlit_Shaders(0, false, false, true, true);
            WarpedBlit_NoShaders(1, true, false, true, false);
        }
        else if (!bOldPresetUsesWarpShader && !bNewPresetUsesWarpShader)
        {
            //WarpedBlit_NoShaders(0, false, false, true, true);
            //WarpedBlit_NoShaders(1, true,  false, true, false);

            // Special case - all the blending just happens in the vertex UV's, so just pretend there's no blend.
            WarpedBlit_NoShaders(1, false, false, false, false);
        }
    }

    if (m_nMaxPSVersion > 0)
        BlurPasses();

    // Draw audio data.
    DrawCustomShapes(); // draw these first; better for feedback if the waves draw *over* them
    DrawCustomWaves();
    DrawWave(mdsound.fWave[0].data(), mdsound.fWave[1].data());
    DrawSprites();

    float fProgress = (GetTime() - m_supertext.fStartTime) / m_supertext.fDuration;

    // If song title animation just ended, burn it into the VS.
    if (m_supertext.fStartTime >= 0.0f && fProgress >= 1.0f && !m_supertext.bRedrawSuperText)
    {
        ShowSongTitleAnim(m_nTexSizeX, m_nTexSizeY, 1.0f);
        if (m_ddsTitle.IsVisible())
        {
            m_ddsTitle.SetVisible(false);
            m_text.UnregisterElement(&m_ddsTitle);
        }
    }

    // Change the render target back to the original setup.
    lpDevice->SetTexture(0, NULL);
    m_lpDX->RestoreTarget(); //lpDevice->SetRenderTarget(pBackBuffer);  // BUG!!
    //lpDevice->SetDepthStencilSurface(pZBuffer);
    SafeRelease(pBackBuffer);
    //SafeRelease(pZBuffer);

    // Show it to the user [composite shader].
    if (m_skip_comp_shaders)
    {
        ShowToUser_NoShaders();
    }
    else
    {
        if (!m_pState->m_bBlending)
        {
            // No blend.
            if (bNewPresetUsesCompShader)
                ShowToUser_Shaders(1, false, false, false, false);
            else
                ShowToUser_NoShaders(); //1, false, false, false, false);
        }
        else
        {
            // Blending.
            // `ShowToUser(nPass, bAlphaBlend, bFlipAlpha, bCullTiles, bFlipCulling)`
            // Note: `alpha` values go from 0..1 during a blend.
            // Note: bFlipCulling==false means tiles with alpha>0 will draw.
            //       bFlipCulling==true means tiles with alpha<255 will draw.
            // Note: ShowToUser_NoShaders() must always come before ShowToUser_Shaders(),
            //       because it always draws the full quad (it can't do tile culling or alpha blending).
            if (bOldPresetUsesCompShader && bNewPresetUsesCompShader)
            {
                ShowToUser_Shaders(0, false, false, true, true);
                ShowToUser_Shaders(1, true, false, true, false);
            }
            else if (!bOldPresetUsesCompShader && bNewPresetUsesCompShader)
            {
                ShowToUser_NoShaders();
                ShowToUser_Shaders(1, true, false, true, false);
            }
            else if (bOldPresetUsesCompShader && !bNewPresetUsesCompShader)
            {
                // THA FUNKY REVERSAL.
                //ShowToUser_Shaders(0);
                //ShowToUser_NoShaders(1);
                ShowToUser_NoShaders();
                ShowToUser_Shaders(0, true, true, true, true);
            }
            else if (!bOldPresetUsesCompShader && !bNewPresetUsesCompShader)
            {
                // Special case - all the blending just happens in the blended state vars, so just pretend there's no blend.
                ShowToUser_NoShaders(); //1, false, false, false, false);
            }
        }
    }

    // Finally, render song title animation to back buffer.
    if (m_supertext.fStartTime >= 0.0f && !m_supertext.bRedrawSuperText)
    {
        ShowSongTitleAnim(GetWidth(), GetHeight(), std::min(fProgress, 0.9999f));
        if (fProgress >= 1.0f)
            m_supertext.fStartTime = -1.0f; // "off" state
    }

    DrawUserSprites();

    // Flip buffers.
    ID3D11Texture2D* pTemp = m_lpVS[0];
    m_lpVS[0] = m_lpVS[1];
    m_lpVS[1] = pTemp;

    /*
    // FIXME - remove EnforceMaxFPS() if never used
    //EnforceMaxFPS(!(m_nLoadingPreset==1 || m_nLoadingPreset==2 || m_nLoadingPreset==4 || m_nLoadingPreset==5));  // this call just turns it on or off; doesn't do it now...
    //EnforceMaxFPS(!(m_nLoadingPreset==2 || m_nLoadingPreset==5));  // this call just turns it on or off; doesn't do it now...

    // FIXME - remove this stuff, and change 'm_last_raw_time' in pluginshell (and others) back to private.
    static float fOldTime = 0;
    float fNewTime = (float)((double)m_last_raw_time / (double)m_high_perf_timer_freq.QuadPart);
    float dt = fNewTime - fOldTime;
    if (m_nLoadingPreset != 0)
    {
        char buf[256];
        sprintf_s(buf, "m_nLoadingPreset==%d: dt=%d ms\n", m_nLoadingPreset, (int)(dt * 1000));
        OutputDebugString(buf);
    }
    fOldTime = fNewTime;
    */
}

void CPlugin::DrawMotionVectors()
{
    // FLEXIBLE MOTION VECTOR FIELD
    if ((float)*m_pState->var_pf_mv_a >= 0.001f)
    {
        D3D11Shim* lpDevice = GetDevice();
        if (!lpDevice)
            return;

        lpDevice->SetTexture(0, NULL);
        lpDevice->SetVertexShader(NULL, NULL);
        lpDevice->SetVertexColor(true);
        //lpDevice->SetFVF(WFVERTEX_FORMAT);
        //-------------------------------------------------------

        int nX = (int)(*m_pState->var_pf_mv_x); // + 0.999f);
        int nY = (int)(*m_pState->var_pf_mv_y); // + 0.999f);
        float dx = (float)*m_pState->var_pf_mv_x - nX;
        float dy = (float)*m_pState->var_pf_mv_y - nY;
        if (nX > 64) { nX = 64; dx = 0; }
        if (nY > 48) { nY = 48; dy = 0; }

        if (nX > 0 && nY > 0)
        {
            /*
            float dx2 = m_fMotionVectorsTempDx; //(*m_pState->var_pf_mv_dx) * 0.05f*GetTime(); // 0..1 range
            float dy2 = m_fMotionVectorsTempDy; //(*m_pState->var_pf_mv_dy) * 0.05f*GetTime(); // 0..1 range
            if (GetFps() > 2.0f && GetFps() < 300.0f)
            {
                dx2 += (float)(*m_pState->var_pf_mv_dx) * 0.05f / GetFps();
                dy2 += (float)(*m_pState->var_pf_mv_dy) * 0.05f / GetFps();
            }
            if (dx2 > 1.0f) dx2 -= (int)dx2;
            if (dy2 > 1.0f) dy2 -= (int)dy2;
            if (dx2 < 0.0f) dx2 = 1.0f - (-dx2 - (int)(-dx2));
            if (dy2 < 0.0f) dy2 = 1.0f - (-dy2 - (int)(-dy2));
            // Hack: when there is only 1 motion vector on the screen, to keep it in.
            //       the center, we gradually migrate it toward 0.5.
            dx2 = dx2*0.995f + 0.5f*0.005f;
            dy2 = dy2*0.995f + 0.5f*0.005f;
            // safety catch
            if (dx2 < 0 || dx2 > 1 || dy2 < 0 || dy2 > 1)
            {
                dx2 = 0.5f;
                dy2 = 0.5f;
            }
            m_fMotionVectorsTempDx = dx2;
            m_fMotionVectorsTempDy = dy2;*/
            float dx2 = (float)(*m_pState->var_pf_mv_dx);
            float dy2 = (float)(*m_pState->var_pf_mv_dy);

            float len_mult = (float)*m_pState->var_pf_mv_l;
            if (dx < 0) dx = 0;
            if (dy < 0) dy = 0;
            if (dx > 1) dx = 1;
            if (dy > 1) dy = 1;
            //dx = dx * 1.0f / (float)nX;
            //dy = dy * 1.0f / (float)nY;
            float inv_texsize = 1.0f / (float)m_nTexSizeX;
            float min_len = 1.0f * inv_texsize;

            WFVERTEX v[(64 + 1) * 2];
            ZeroMemory(v, sizeof(WFVERTEX) * (64 + 1) * 2);
            v[0].r = COLOR_NORM((float)*m_pState->var_pf_mv_r);
            v[0].g = COLOR_NORM((float)*m_pState->var_pf_mv_g);
            v[0].b = COLOR_NORM((float)*m_pState->var_pf_mv_b);
            v[0].a = COLOR_NORM((float)*m_pState->var_pf_mv_a);

            for (int x = 1; x < (nX + 1) * 2; x++)
                COPY_COLOR(v[x], v[0]);
            lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);

            for (int y = 0; y < nY; y++)
            {
                float fy = (y + 0.25f) / (float)(nY + dy + 0.25f - 1.0f);

                // now move by offset
                fy -= dy2;

                if (fy > 0.0001f && fy < 0.9999f)
                {
                    int n = 0;
                    for (int x = 0; x < nX; x++)
                    {
                        //float fx = (x + 0.25f) / (float)(nX + dx + 0.25f - 1.0f);
                        float fx = (x + 0.25f) / (float)(nX + dx + 0.25f - 1.0f);

                        // Now move by offset.
                        fx += dx2;

                        if (fx > 0.0001f && fx < 0.9999f)
                        {
                            float fx2, fy2;
                            ReversePropagatePoint(fx, fy, &fx2, &fy2); // NOTE: THIS IS REALLY A REVERSE-PROPAGATION
                            //fx2 = fx*2 - fx2;
                            //fy2 = fy*2 - fy2;
                            //fx2 = fx + 1.0f/(float)m_nTexSize;
                            //fy2 = 1-(fy + 1.0f/(float)m_nTexSize);

                            // Enforce minimum trail lengths.
                            {
                                dx = (fx2 - fx);
                                dy = (fy2 - fy);
                                dx *= len_mult;
                                dy *= len_mult;
                                float len = sqrtf(dx * dx + dy * dy);

                                if (len > min_len)
                                {
                                }
                                else if (len > 0.00000001f)
                                {
                                    len = min_len / len;
                                    dx *= len;
                                    dy *= len;
                                }
                                else
                                {
                                    dx = min_len;
                                    dy = min_len;
                                }

                                fx2 = fx + dx;
                                fy2 = fy + dy;
                            }

                            v[n].x = fx * 2.0f - 1.0f;
                            v[n].y = fy * 2.0f - 1.0f;
                            v[n + 1].x = fx2 * 2.0f - 1.0f;
                            v[n + 1].y = fy2 * 2.0f - 1.0f;

                            // actually, project it in the reverse direction
                            //v[n+1].x = v[n].x*2.0f - v[n+1].x;// + dx*2;
                            //v[n+1].y = v[n].y*2.0f - v[n+1].y;// + dy*2;
                            //v[n].x += dx*2;
                            //v[n].y += dy*2;

                            n += 2;
                        }
                    }

                    // Draw it.
                    lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINELIST, n / 2, v, sizeof(WFVERTEX));
                }
            }

            lpDevice->SetBlendState(false);
        }
    }
}

bool CPlugin::ReversePropagatePoint(float fx, float fy, float* fx2, float* fy2)
{
    //float fy = y / (float)nMotionVectorsY;
    int y0 = (int)(fy * m_nGridY);
    float dy = fy * m_nGridY - y0;

    //float fx = x / (float)nMotionVectorsX;
    int x0 = (int)(fx * m_nGridX);
    float dx = fx * m_nGridX - x0;

    int x1 = x0 + 1;
    int y1 = y0 + 1;

    if (x0 < 0) return false;
    if (y0 < 0) return false;
    //if (x1 < 0) return false;
    //if (y1 < 0) return false;
    //if (x0 > m_nGridX) return false;
    //if (y0 > m_nGridY) return false;
    if (x1 > m_nGridX) return false;
    if (y1 > m_nGridY) return false;

    float tu, tv;
    tu  = m_verts[y0 * (m_nGridX + 1) + x0].tu * (1 - dx) * (1 - dy);
    tv  = m_verts[y0 * (m_nGridX + 1) + x0].tv * (1 - dx) * (1 - dy);
    tu += m_verts[y0 * (m_nGridX + 1) + x1].tu * (dx) * (1 - dy);
    tv += m_verts[y0 * (m_nGridX + 1) + x1].tv * (dx) * (1 - dy);
    tu += m_verts[y1 * (m_nGridX + 1) + x0].tu * (1 - dx) * (dy);
    tv += m_verts[y1 * (m_nGridX + 1) + x0].tv * (1 - dx) * (dy);
    tu += m_verts[y1 * (m_nGridX + 1) + x1].tu * (dx) * (dy);
    tv += m_verts[y1 * (m_nGridX + 1) + x1].tv * (dx) * (dy);

    *fx2 = tu;
    *fy2 = 1.0f - tv;
    return true;
}

void CPlugin::GetSafeBlurMinMax(CState* pState, float* blur_min, float* blur_max)
{
    blur_min[0] = (float)*pState->var_pf_blur1min;
    blur_min[1] = (float)*pState->var_pf_blur2min;
    blur_min[2] = (float)*pState->var_pf_blur3min;
    blur_max[0] = (float)*pState->var_pf_blur1max;
    blur_max[1] = (float)*pState->var_pf_blur2max;
    blur_max[2] = (float)*pState->var_pf_blur3max;

    // check that precision isn't wasted in later blur passes [...min-max gap can't grow!]
    // also, if min-max are close to each other, push them apart:
    const float fMinDist = 0.1f;
    if (blur_max[0] - blur_min[0] < fMinDist)
    {
        float avg = (blur_min[0] + blur_max[0]) * 0.5f;
        blur_min[0] = avg - fMinDist * 0.5f;
        blur_max[0] = avg - fMinDist * 0.5f;
    }
    blur_max[1] = std::min(blur_max[0], blur_max[1]);
    blur_min[1] = std::max(blur_min[0], blur_min[1]);
    if (blur_max[1] - blur_min[1] < fMinDist)
    {
        float avg = (blur_min[1] + blur_max[1]) * 0.5f;
        blur_min[1] = avg - fMinDist * 0.5f;
        blur_max[1] = avg - fMinDist * 0.5f;
    }
    blur_max[2] = std::min(blur_max[1], blur_max[2]);
    blur_min[2] = std::max(blur_min[1], blur_min[2]);
    if (blur_max[2] - blur_min[2] < fMinDist)
    {
        float avg = (blur_min[2] + blur_max[2]) * 0.5f;
        blur_min[2] = avg - fMinDist * 0.5f;
        blur_max[2] = avg - fMinDist * 0.5f;
    }
}

// Note: Blur is currently a little funky. It blurs the *current* frame after warp;
//       this way, it lines up well with the composite pass. However, if switching
//       presets instantly, to one whose *warp* shader uses the blur texture,
//       it will be outdated (just for one frame). Oh well.
//       This also means that when sampling the blurred textures in the warp shader,
//       they are one frame old. This isn't too big a deal. Getting them to match
//       up for the composite pass is probably more important.
void CPlugin::BlurPasses()
{
#if (NUM_BLUR_TEX > 0)
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    int passes = std::min(NUM_BLUR_TEX, m_nHighestBlurTexUsedThisFrame * 2);
    if (passes == 0)
        return;

    ID3D11Texture2D* pBackBuffer = NULL; //, pZBuffer = NULL;
    lpDevice->GetRenderTarget(&pBackBuffer);

    //lpDevice->SetFVF(MDVERTEX_FORMAT);
    lpDevice->SetVertexShader(m_BlurShaders[0].vs.ptr, m_BlurShaders[0].vs.CT);
    //lpDevice->SetVertexDeclaration(m_pMilkDropVertDecl);
    lpDevice->SetBlendState(false);
    lpDevice->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
    //lpDevice->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);

    //ID3D11Texture2D* pNewTarget = NULL;

    // Clear texture bindings.
    for (int i = 0; i < 16; i++)
        lpDevice->SetTexture(i, NULL);

    // Set up fullscreen quad.
    MDVERTEX v[4];

    v[0].x = -1.0f;
    v[0].y = -1.0f;
    v[1].x = 1.0f;
    v[1].y = -1.0f;
    v[2].x = -1.0f;
    v[2].y = 1.0f;
    v[3].x = 1.0f;
    v[3].y = 1.0f;

    v[0].tu = 0.0f; // kiv: upside-down?
    v[0].tv = 0.0f;
    v[1].tu = 1.0f;
    v[1].tv = 0.0f;
    v[2].tu = 0.0f;
    v[2].tv = 1.0f;
    v[3].tu = 1.0f;
    v[3].tv = 1.0f;

    const float w[8] = {4.0f, 3.8f, 3.5f, 2.9f, 1.9f, 1.2f, 0.7f, 0.3f}; // <- user can specify these
    float edge_darken = static_cast<float>(*m_pState->var_pf_blur1_edge_darken);
    float blur_min[3], blur_max[3];
    GetSafeBlurMinMax(m_pState, blur_min, blur_max);

    float fscale[3];
    float fbias[3];

    // Figure out the progressive scale and bias needed, at each step,
    // to go from one [min..max] range to the next.
    float temp_min, temp_max;
    fscale[0] = 1.0f / (blur_max[0] - blur_min[0]);
    fbias[0] = -blur_min[0] * fscale[0];
    temp_min = (blur_min[1] - blur_min[0]) / (blur_max[0] - blur_min[0]);
    temp_max = (blur_max[1] - blur_min[0]) / (blur_max[0] - blur_min[0]);
    fscale[1] = 1.0f / (temp_max - temp_min);
    fbias[1] = -temp_min * fscale[1];
    temp_min = (blur_min[2] - blur_min[1]) / (blur_max[1] - blur_min[1]);
    temp_max = (blur_max[2] - blur_min[1]) / (blur_max[1] - blur_min[1]);
    fscale[2] = 1.0f / (temp_max - temp_min);
    fbias[2] = -temp_min * fscale[2];

    ID3D11DepthStencilView* origDSView = nullptr;
    ID3D11DepthStencilView* emptyDSView = nullptr;
    lpDevice->GetDepthView(&origDSView);

    // Note: Warped blit just rendered from VS0 to VS1.
    for (int i = 0; i < passes; i++)
    {
        // Hook up correct render target.
        lpDevice->SetRenderTarget(m_lpBlur[i], &emptyDSView);

        // Hook up correct source texture; assume there is only one, at stage 0. /????
        lpDevice->SetTexture(0, (i == 0) ? m_lpVS[0] : m_lpBlur[i - 1]);

        // Set constants.
        CConstantTable* pCT = m_BlurShaders[i % 2].ps.CT;
        LPCSTR* h = m_BlurShaders[i % 2].ps.params.const_handles;

        int srcw = (i == 0) ? GetWidth() : m_nBlurTexW[i - 1];
        int srch = (i == 0) ? GetHeight() : m_nBlurTexH[i - 1];
        XMFLOAT4 srctexsize = XMFLOAT4(static_cast<float>(srcw), static_cast<float>(srch), 1.0f / static_cast<float>(srcw), 1.0f / static_cast<float>(srch));

        float fscale_now = fscale[i / 2];
        float fbias_now = fbias[i / 2];

        if (i % 2 == 0)
        {
            // Pass 1 (long horizontal pass).
            //-------------------------------------
            const float w1 = w[0] + w[1];
            const float w2 = w[2] + w[3];
            const float w3 = w[4] + w[5];
            const float w4 = w[6] + w[7];
            const float d1 = 0 + 2 * w[1] / w1;
            const float d2 = 2 + 2 * w[3] / w2;
            const float d3 = 4 + 2 * w[5] / w3;
            const float d4 = 6 + 2 * w[7] / w4;
            const float w_div = 0.5f / (w1 + w2 + w3 + w4);
            //-------------------------------------
            //float4 _c0; // source texsize (.xy), and inverse (.zw)
            //float4 _c1; // w1..w4
            //float4 _c2; // d1..d4
            //float4 _c3; // scale, bias, w_div, 0
            //-------------------------------------
            if (h[0]) { pCT->SetVector(h[0], &srctexsize); }
            if (h[1]) { XMFLOAT4 v4(w1, w2, w3, w4);  pCT->SetVector(h[1], &v4); }
            if (h[2]) { XMFLOAT4 v4(d1, d2, d3, d4);  pCT->SetVector(h[2], &v4); }
            if (h[3]) { XMFLOAT4 v4(fscale_now, fbias_now, w_div, 0.0f); pCT->SetVector(h[3], &v4); }
        }
        else
        {
            // Pass 2 (short vertical pass).
            //-------------------------------------
            const float w1 = w[0] + w[1] + w[2] + w[3];
            const float w2 = w[4] + w[5] + w[6] + w[7];
            const float d1 = 0 + 2 * ((w[2] + w[3]) / w1);
            const float d2 = 2 + 2 * ((w[6] + w[7]) / w2);
            const float w_div = 1.0f / ((w1 + w2) * 2);
            //-------------------------------------
            //float4 _c0; // source texsize (.xy), and inverse (.zw)
            //float4 _c5; // w1, w2, d1, d2
            //float4 _c6; // w_div, edge_darken_c1, edge_darken_c2, edge_darken_c3
            //-------------------------------------
            if (h[0]) { pCT->SetVector( h[0], &srctexsize ); }
            if (h[5]) { XMFLOAT4 v4(w1, w2, d1, d2); pCT->SetVector(h[5], &v4); }
            if (h[6])
            {
                // Note: only do this first time; if you do it many times,
                //       then the super-blurred levels will have big black lines along the top & left sides.
                if (i == 1)
                {
                    XMFLOAT4 v4(w_div, (1 - edge_darken), edge_darken, 5.0f);
                    pCT->SetVector(h[6], &v4); // darken edges
                }
                else
                {
                    XMFLOAT4 v4(w_div, 1.0f, 0.0f, 5.0f);
                    pCT->SetVector(h[6], &v4); // don't darken
                }
            }
        }

        // Set pixel shader.
        lpDevice->SetPixelShader(m_BlurShaders[i % 2].ps.ptr, m_BlurShaders[i % 2].ps.CT);

        // Draw fullscreen quad.
        lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, v, sizeof(MDVERTEX));

        // Clear texture bindings.
        lpDevice->SetTexture(0, nullptr);
    }

    lpDevice->SetRenderTarget(pBackBuffer, &origDSView);
    SafeRelease(pBackBuffer);
    SafeRelease(origDSView);
    lpDevice->SetPixelShader(nullptr, nullptr);
    lpDevice->SetVertexShader(nullptr, nullptr);
    lpDevice->SetTexture(0, nullptr);
#endif

    m_nHighestBlurTexUsedThisFrame = 0;
}

void CPlugin::ComputeGridAlphaValues()
{
    float fBlend = m_pState->m_fBlendProgress; //std::max(0, min(1, (m_pState->m_fBlendProgress * 1.6f - 0.3f)));
    /*
    switch (code) //if (nPassOverride==0)
    {
        //case 8:
        //case 9:
        //case 12:
        //case 13:
            // note - these are the 4 cases where the old preset uses a warp shader, but new preset doesn't.
            fBlend = 1 - fBlend; // <-- THIS IS THE KEY - FLIPS THE ALPHAS AND EVERYTHING ELSE JUST WORKS.
            break;
    }
    */
    //fBlend = 1 - fBlend; // <-- THIS IS THE KEY - FLIPS THE ALPHAS AND EVERYTHING ELSE JUST WORKS.
    //bool bBlending = m_pState->m_bBlending; //(fBlend >= 0.0001f && fBlend <= 0.9999f);

    // Warp.
    float fWarpTime = GetTime() * m_pState->m_fWarpAnimSpeed;
    float fWarpScaleInv = 1.0f / m_pState->m_fWarpScale.eval(GetTime());
    float f[4];
    f[0] = 11.68f + 4.0f * cosf(fWarpTime * 1.413f + 10);
    f[1] =  8.77f + 3.0f * cosf(fWarpTime * 1.113f + 7);
    f[2] = 10.54f + 3.0f * cosf(fWarpTime * 1.233f + 3);
    f[3] = 11.49f + 4.0f * cosf(fWarpTime * 0.933f + 5);

    // Texel alignment.
    float texel_offset_x = 0.5f / (float)m_nTexSizeX;
    float texel_offset_y = 0.5f / (float)m_nTexSizeY;

    int num_reps = (m_pState->m_bBlending) ? 2 : 1;
    int start_rep = 0;

    // FIRST WE HAVE 1-2 PASSES FOR CRUNCHING THE PER-VERTEX EQUATIONS
    for (int rep = start_rep; rep < num_reps; rep++)
    {
        // to blend the two PV equations together, we simulate both to get the final UV coords,
        // then we blend those final UV coords.  We also write out an alpha value so that
        // the second DRAW pass below (which might use a different shader) can do blending.
        CState* pState;

        if (rep == 0)
            pState = m_pState;
        else
            pState = m_pOldState;

        // Cache the doubles as floats so that computations are a bit faster.
        float fZoom    = static_cast<float>(*pState->var_pf_zoom);
        float fZoomExp = static_cast<float>(*pState->var_pf_zoomexp);
        float fRot     = static_cast<float>(*pState->var_pf_rot);
        float fWarp    = static_cast<float>(*pState->var_pf_warp);
        float fCX      = static_cast<float>(*pState->var_pf_cx);
        float fCY      = static_cast<float>(*pState->var_pf_cy);
        float fDX      = static_cast<float>(*pState->var_pf_dx);
        float fDY      = static_cast<float>(*pState->var_pf_dy);
        float fSX      = static_cast<float>(*pState->var_pf_sx);
        float fSY      = static_cast<float>(*pState->var_pf_sy);

        int n = 0;

        for (int y = 0; y <= m_nGridY; y++)
        {
            for (int x = 0; x <= m_nGridX; x++)
            {
                // Note: x, y, z are now set at init. time - no need to mess with them!
                //m_verts[n].x = i / (float)m_nGridX * 2.0f - 1.0f;
                //m_verts[n].y = j / (float)m_nGridY * 2.0f - 1.0f;
                //m_verts[n].z = 0.0f;

                if (pState->m_pp_codehandle)
                {
                    // Restore all the variables to their original states,
                    // run the user-defined equations, then move the
                    // results into local vars for computation as floats
                    *pState->var_pv_x       = (double)(m_verts[n].x * 0.5f * m_fAspectX + 0.5f);
                    *pState->var_pv_y       = (double)(m_verts[n].y * -0.5f * m_fAspectY + 0.5f);
                    *pState->var_pv_rad     = (double)m_vertinfo[n].rad;
                    *pState->var_pv_ang     = (double)m_vertinfo[n].ang;
                    *pState->var_pv_zoom    = *pState->var_pf_zoom;
                    *pState->var_pv_zoomexp = *pState->var_pf_zoomexp;
                    *pState->var_pv_rot     = *pState->var_pf_rot;
                    *pState->var_pv_warp    = *pState->var_pf_warp;
                    *pState->var_pv_cx      = *pState->var_pf_cx;
                    *pState->var_pv_cy      = *pState->var_pf_cy;
                    *pState->var_pv_dx      = *pState->var_pf_dx;
                    *pState->var_pv_dy      = *pState->var_pf_dy;
                    *pState->var_pv_sx      = *pState->var_pf_sx;
                    *pState->var_pv_sy      = *pState->var_pf_sy;
                    //*pState->var_pv_time    = *pState->var_pv_time; // (these are all now initialized
                    //*pState->var_pv_bass    = *pState->var_pv_bass; //  just once per frame)
                    //*pState->var_pv_mid     = *pState->var_pv_mid;
                    //*pState->var_pv_treb    = *pState->var_pv_treb;
                    //*pState->var_pv_bass_att = *pState->var_pv_bass_att;
                    //*pState->var_pv_mid_att = *pState->var_pv_mid_att;
                    //*pState->var_pv_treb_att = *pState->var_pv_treb_att;

#ifndef _NO_EXPR_
                    NSEEL_code_execute(pState->m_pp_codehandle);
#endif

                    fZoom    = static_cast<float>(*pState->var_pv_zoom);
                    fZoomExp = static_cast<float>(*pState->var_pv_zoomexp);
                    fRot     = static_cast<float>(*pState->var_pv_rot);
                    fWarp    = static_cast<float>(*pState->var_pv_warp);
                    fCX      = static_cast<float>(*pState->var_pv_cx);
                    fCY      = static_cast<float>(*pState->var_pv_cy);
                    fDX      = static_cast<float>(*pState->var_pv_dx);
                    fDY      = static_cast<float>(*pState->var_pv_dy);
                    fSX      = static_cast<float>(*pState->var_pv_sx);
                    fSY      = static_cast<float>(*pState->var_pv_sy);
                }

                float fZoom2 = powf(fZoom, powf(fZoomExp, m_vertinfo[n].rad * 2.0f - 1.0f));

                // initial texcoords, w/built-in zoom factor
                float fZoom2Inv = 1.0f / fZoom2;
                float u = m_verts[n].x * m_fAspectX * 0.5f * fZoom2Inv + 0.5f;
                float v = -m_verts[n].y * m_fAspectY * 0.5f * fZoom2Inv + 0.5f;
                //float u_orig = u;
                //float v_orig = v;
                //m_verts[n].tr = u_orig + texel_offset_x;
                //m_verts[n].ts = v_orig + texel_offset_y;

                // Stretch on X, Y.
                u = (u - fCX) / fSX + fCX;
                v = (v - fCY) / fSY + fCY;

                // Warping.
                //if (fWarp > 0.001f || fWarp < -0.001f)
                //{
                u += fWarp * 0.0035f * sinf(fWarpTime * 0.333f + fWarpScaleInv * (m_verts[n].x * f[0] - m_verts[n].y * f[3]));
                v += fWarp * 0.0035f * cosf(fWarpTime * 0.375f - fWarpScaleInv * (m_verts[n].x * f[2] + m_verts[n].y * f[1]));
                u += fWarp * 0.0035f * cosf(fWarpTime * 0.753f - fWarpScaleInv * (m_verts[n].x * f[1] - m_verts[n].y * f[2]));
                v += fWarp * 0.0035f * sinf(fWarpTime * 0.825f + fWarpScaleInv * (m_verts[n].x * f[0] + m_verts[n].y * f[3]));
                //}

                // Rotation.
                float u2 = u - fCX;
                float v2 = v - fCY;

                float cos_rot = cosf(fRot);
                float sin_rot = sinf(fRot);
                u = u2 * cos_rot - v2 * sin_rot + fCX;
                v = u2 * sin_rot + v2 * cos_rot + fCY;

                // translation:
                u -= fDX;
                v -= fDY;

                // undo aspect ratio fix:
                u = (u - 0.5f) * m_fInvAspectX + 0.5f;
                v = (v - 0.5f) * m_fInvAspectY + 0.5f;

                // final half-texel-offset translation:
                u += texel_offset_x;
                v += texel_offset_y;

                if (rep == 0)
                {
                    // UV's for m_pState
                    m_verts[n].tu = u;
                    m_verts[n].tv = v;
                    m_verts[n].a = 1.0f;
                    m_verts[n].r = 1.0f;
                    m_verts[n].g = 1.0f;
                    m_verts[n].b = 1.0f;
                }
                else
                {
                    // Blend to UV's for `m_pOldState`.
                    float mix2 = m_vertinfo[n].a * fBlend + m_vertinfo[n].c; //fCosineBlend2;
                    mix2 = std::max(0.0f, std::min(1.0f, mix2));
                    // if fBlend un-flipped, then mix2 is 0 at the beginning of a blend, 1 at the end...
                    //                       and alphas are 0 at the beginning, 1 at the end.
                    m_verts[n].tu = m_verts[n].tu * (mix2) + u * (1 - mix2);
                    m_verts[n].tv = m_verts[n].tv * (mix2) + v * (1 - mix2);
                    // Set the alpha values for blending between two presets:
                    m_verts[n].a = mix2;
                    m_verts[n].r = 1.0f;
                    m_verts[n].g = 1.0f;
                    m_verts[n].b = 1.0f;
                }

                n++;
            }
        }
    }
}

void CPlugin::WarpedBlit_NoShaders(int /* nPass */, bool bAlphaBlend, bool bFlipAlpha, bool bCullTiles, bool bFlipCulling)
{
    MungeFPCW(NULL); // single-precision mode and disable exceptions

    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    if (!wcscmp(m_pState->m_szDesc, INVALID_PRESET_DESC))
    {
        // If no valid preset loaded, clear the target to black and return.
        // TODO: DirectX 11
        //lpDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
        return;
    }

    lpDevice->SetTexture(0, m_lpVS[0]);
    lpDevice->SetVertexShader(NULL, NULL);
    lpDevice->SetPixelShader(NULL, NULL);
    lpDevice->SetBlendState(false);
    //lpDevice->SetFVF(MDVERTEX_FORMAT);

    // Stages 0 and 1 always just use bilinear filtering.
    D3D11_TEXTURE_ADDRESS_MODE texaddr = (*m_pState->var_pf_wrap > m_fSnapPoint) ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
    lpDevice->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, texaddr);
    //lpDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    //lpDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    //lpDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    //lpDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    //lpDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    //lpDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

    // note: this texture stage state setup works for 0 or 1 texture.
    // if you set a texture, it will be modulated with the current diffuse color.
    // if you don't set a texture, it will just use the current diffuse color.
    lpDevice->SetShader(0);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
    //lpDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
    //lpDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    //DWORD texaddr = (*m_pState->var_pf_wrap > m_fSnapPoint) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
    //lpDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, texaddr);
    //lpDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, texaddr);
    //lpDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, texaddr);

    // Decay.
    float fDecay = COLOR_NORM(static_cast<float>(*m_pState->var_pf_decay));
    float cDecay = fDecay;

    //if (m_pState->m_bBlending)
    //    fDecay = fDecay * (fCosineBlend) + (1.0f - fCosineBlend) * ((float)(*m_pOldState->var_pf_decay));

    // Skip gamma correction for 32-bit color in Direct3D 11.
    /*if (m_n16BitGamma > 0 &&
        (GetBackBufFormat() == D3DFMT_R5G6B5 || GetBackBufFormat() == D3DFMT_X1R5G5B5 || GetBackBufFormat() == D3DFMT_A1R5G5B5 || GetBackBufFormat() == D3DFMT_A4R4G4B4) &&
        fDecay < 0.9999f)
    {
        fDecay = std::min(fDecay, (32.0f - m_n16BitGamma) / 32.0f);
    }

    D3DCOLOR cDecay = D3DCOLOR_RGBA_01(fDecay, fDecay, fDecay, 1);*/

    // Hurl the triangle strips at the video card.
    int poly;
    for (poly = 0; poly < (m_nGridX + 1) * 2; poly++)
    {
        m_verts_temp[poly].a = 1.0f;
        m_verts_temp[poly].r = cDecay;
        m_verts_temp[poly].g = cDecay;
        m_verts_temp[poly].b = cDecay;
    }

    if (bAlphaBlend)
    {
        if (bFlipAlpha)
        {
            lpDevice->SetBlendState(true, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_SRC_ALPHA);
        }
        else
        {
            lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
        }
    }
    else
        lpDevice->SetBlendState(false);

    int nAlphaTestValue = 0;
    if (bFlipCulling)
        nAlphaTestValue = 1 - nAlphaTestValue;

    // Hurl the triangles at the video card.
    // Going to un-index it, to not stress non-performant drivers.
    // If blending, skip any polygon that is all alpha-blended out.
    // This also respects the `MaxPrimCount` limit of the video card.
    MDVERTEX tempv[1024 * 3];
    int max_prims_per_batch = std::min(lpDevice->GetMaxPrimitiveCount(), static_cast<UINT>(ARRAYSIZE(tempv) / 3)) - 4;
    int primCount = m_nGridX * m_nGridY * 2;
    int src_idx = 0;
    //int prims_sent = 0;
    while (src_idx < primCount * 3)
    {
        int prims_queued = 0;
        int i = 0;
        while (prims_queued < max_prims_per_batch && src_idx < primCount * 3)
        {
            // Copy 3 vertices.
            for (int j = 0; j < 3; j++)
            {
                tempv[i++] = m_verts[m_indices_list[src_idx++]];
                // Flip sign on Y and factor in the decay color!
                tempv[i - 1].y *= -1;
                tempv[i - 1].r = cDecay;
                tempv[i - 1].g = cDecay;
                tempv[i - 1].b = cDecay;
            }
            if (bCullTiles)
            {
                DWORD d1 = static_cast<DWORD>(tempv[i - 3].a * 255);
                DWORD d2 = static_cast<DWORD>(tempv[i - 2].a * 255);
                DWORD d3 = static_cast<DWORD>(tempv[i - 1].a * 255);
                bool bIsNeeded;
                if (nAlphaTestValue)
                    bIsNeeded = ((d1 & d2 & d3) < 255); //(d1 < 255) || (d2 < 255) || (d3 < 255);
                else
                    bIsNeeded = ((d1 | d2 | d3) > 0); //(d1 > 0) || (d2 > 0) || (d3 > 0);
                if (!bIsNeeded)
                    i -= 3;
                else
                    prims_queued++;
            }
            else
                prims_queued++;
        }
        if (prims_queued > 0)
            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, prims_queued, tempv, sizeof(MDVERTEX));
    }

    /*if (!bCullTiles)
    {
        assert(!bAlphaBlend); // not handled yet

        // Draw normally - just a full triangle strip for each half-row of cells
        // (even if we are blending, it is between two pre-pixel-shader presets,
        // so the blend all happens exclusively in the per-vertex equations).
        for (int strip = 0; strip < m_nGridY * 2; strip++)
        {
            int index = strip * (m_nGridX + 2);

            for (poly = 0; poly < m_nGridX + 2; poly++)
            {
                int ref_vert = m_indices_strip[index];
                m_verts_temp[poly].x = m_verts[ref_vert].x;
                m_verts_temp[poly].y = -m_verts[ref_vert].y;
                m_verts_temp[poly].z = m_verts[ref_vert].z;
                m_verts_temp[poly].tu = m_verts[ref_vert].tu;
                m_verts_temp[poly].tv = m_verts[ref_vert].tv;
                //m_verts_temp[poly].Diffuse = cDecay; this is done just once; see just above
                index++;
            }
            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, m_nGridX, (LPVOID)m_verts_temp, sizeof(MDVERTEX));
        }
    }
    else
    {
        // Blending to/from a new pixel-shader enabled preset.
        // Only draw the cells needed! (an optimization)
        int nAlphaTestValue = 0;
        if (bFlipCulling)
            nAlphaTestValue = 1 - nAlphaTestValue;

        int idx[2048];
        for (int y=0; y<m_nGridY; y++)
        {
            // Copy vertices and flip sign on Y.
            int ref_vert = y * (m_nGridX + 1);
            for (int i = 0; i < (m_nGridX + 1) * 2; i++)
            {
                m_verts_temp[i].x  =  m_verts[ref_vert].x;
                m_verts_temp[i].y  = -m_verts[ref_vert].y;
                m_verts_temp[i].z  =  m_verts[ref_vert].z;
                m_verts_temp[i].tu =  m_verts[ref_vert].tu;
                m_verts_temp[i].tv =  m_verts[ref_vert].tv;
                m_verts_temp[i].Diffuse = (cDecay & 0x00FFFFFF) | (m_verts[ref_vert].Diffuse & 0xFF000000);
                ref_vert++;
            }

            // Create (smart) indices.
            int count = 0;
            int nVert = 0;
            bool bWasNeeded;
            ref_vert = (y) * (m_nGridX + 1);
            DWORD d1 = (m_verts[ref_vert].Diffuse >> 24);
            DWORD d2 = (m_verts[ref_vert+m_nGridX+1].Diffuse >> 24);
            if (nAlphaTestValue)
                bWasNeeded = (d1 < 255) || (d2 < 255);
            else
                bWasNeeded = (d1 > 0) || (d2 > 0);
            for (int i = 0; i < m_nGridX; i++)
            {
                bool bIsNeeded;
                DWORD d1 = (m_verts[ref_vert + 1].Diffuse >> 24);
                DWORD d2 = (m_verts[ref_vert + 1 + m_nGridX + 1].Diffuse >> 24);
                if (nAlphaTestValue)
                    bIsNeeded = (d1 < 255) || (d2 < 255);
                else
                    bIsNeeded = (d1 > 0) || (d2 > 0);

                if (bIsNeeded || bWasNeeded)
                {
                    idx[count++] = nVert;
                    idx[count++] = nVert + 1;
                    idx[count++] = nVert + m_nGridX + 1;
                    idx[count++] = nVert + m_nGridX + 1;
                    idx[count++] = nVert + 1;
                    idx[count++] = nVert + m_nGridX + 2;
                }
                bWasNeeded = bIsNeeded;

                nVert++;
                ref_vert++;
            }
            lpDevice->DrawIndexedPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, (m_nGridX + 1) * 2, count / 3, (LPVOID)idx, D3DFMT_INDEX32, (LPVOID)m_verts_temp, sizeof(MDVERTEX));
        }
    }*/

    lpDevice->SetBlendState(false);
}

// If nPass==0, it draws old preset (blending 1 of 2).
// If nPass==1, it draws new preset (blending 2 of 2, OR done blending).
void CPlugin::WarpedBlit_Shaders(int nPass, bool bAlphaBlend, bool bFlipAlpha, bool bCullTiles, bool bFlipCulling)
{
    MungeFPCW(NULL); // puts us in single-precision mode & disables exceptions

    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    if (!wcscmp(m_pState->m_szDesc, INVALID_PRESET_DESC))
    {
        // If no valid preset loaded, clear the target to black and return.
        // TODO: DirectX 11
        //lpDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
        return;
    }

    //float fBlend = m_pState->m_fBlendProgress; //max(0, min(1, (m_pState->m_fBlendProgress * 1.6f - 0.3f)));
    //if (nPassOverride == 0)
    //    fBlend = 1 - fBlend; // <-- THIS IS THE KEY - FLIPS THE ALPHAS AND EVERYTHING ELSE JUST WORKS
    //bool bBlending = m_pState->m_bBlending; //(fBlend >= 0.0001f && fBlend <= 0.9999f);

    //lpDevice->SetTexture(0, m_lpVS[0]);
    lpDevice->SetVertexShader(NULL, NULL);
    //lpDevice->SetFVF(MDVERTEX_FORMAT);

    // Texel alignment.
    //float texel_offset_x = 0.5f / static_cast<float>(m_nTexSizeX);
    //float texel_offset_y = 0.5f / static_cast<float>(m_nTexSizeY);

    int nAlphaTestValue = 0;
    if (bFlipCulling)
        nAlphaTestValue = 1 - nAlphaTestValue;

    if (bAlphaBlend)
    {
        if (bFlipAlpha)
        {
            lpDevice->SetBlendState(true, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_SRC_ALPHA);
        }
        else
        {
            lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
        }
    }
    else
        lpDevice->SetBlendState(false);

    int pass = nPass;
    {
        // PASS 0: draw using *blended per-vertex motion vectors*, but with the OLD warp shader.
        // PASS 1: draw using *blended per-vertex motion vectors*, but with the NEW warp shader.
        PShaderInfo* si = (pass == 0) ? &m_OldShaders.warp : &m_shaders.warp;
        CState* state = (pass == 0) ? m_pOldState : m_pState;

        //lpDevice->SetVertexDeclaration(m_pMyVertDecl);
        lpDevice->SetVertexShader(m_fallbackShaders_vs.warp.ptr, m_fallbackShaders_vs.warp.CT);

        ApplyShaderParams(&(si->params), si->CT, state);
        lpDevice->SetPixelShader(si->ptr, si->CT);

        // Hurl the triangles at the video card.
        // Going to un-index it, to not stress non-performant drivers.
        // Divide it into the two halves of the screen (top/bottom) so can hack
        // the 'ang' values along the angle-wrap seam, halfway through the draw.
        // If we're blending, we'll skip any polygon that is all alpha-blended out.
        // This also respects the MaxPrimCount limit of the video card.
        MDVERTEX tempv[1024 * 3];
        int max_prims_per_batch = std::min(lpDevice->GetMaxPrimitiveCount(), static_cast<UINT>(ARRAYSIZE(tempv) / 3)) - 4;
        for (int half = 0; half < 2; half++)
        {
            // hack / restore the ang values along the angle-wrap [0 <-> 2pi] seam...
            float new_ang = half ? 3.1415926535897932384626433832795f : -3.1415926535897932384626433832795f;
            int y_offset = (m_nGridY / 2) * (m_nGridX + 1);
            for (int x = 0; x < m_nGridX / 2; x++)
                m_verts[y_offset + x].ang = new_ang;

            // Send half of the polys.
            int primCount = m_nGridX * m_nGridY * 2 / 2; // in this case, to draw HALF the polys
            int src_idx = 0;
            int src_idx_offset = half * primCount * 3;
            //int prims_sent = 0;
            while (src_idx < primCount * 3)
            {
                int prims_queued = 0;
                int i = 0;
                while (prims_queued < max_prims_per_batch && src_idx < primCount * 3)
                {
                    // copy 3 verts
                    for (int j = 0; j < 3; j++)
                        tempv[i++] = m_verts[m_indices_list[src_idx_offset + src_idx++]];
                    if (bCullTiles)
                    {
                        DWORD d1 = static_cast<DWORD>(tempv[i - 3].a * 255);
                        DWORD d2 = static_cast<DWORD>(tempv[i - 2].a * 255);
                        DWORD d3 = static_cast<DWORD>(tempv[i - 1].a * 255);
                        bool bIsNeeded;
                        if (nAlphaTestValue)
                            bIsNeeded = ((d1 & d2 & d3) < 255); //(d1 < 255) || (d2 < 255) || (d3 < 255);
                        else
                            bIsNeeded = ((d1 | d2 | d3) > 0); //(d1 > 0) || (d2 > 0) || (d3 > 0);
                        if (!bIsNeeded)
                            i -= 3;
                        else
                            prims_queued++;
                    }
                    else
                        prims_queued++;
                }
                if (prims_queued > 0)
                    lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, prims_queued, tempv, sizeof(MDVERTEX));
            }
        }
    }

    lpDevice->SetBlendState(false);

    RestoreShaderParams();
}

void CPlugin::DrawCustomShapes()
{
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    //lpDevice->SetTexture(0, m_lpVS[0]);//NULL);
    //lpDevice->SetVertexShader(SPRITEVERTEX_FORMAT);

    int num_reps = (m_pState->m_bBlending) ? 2 : 1;
    for (int rep = 0; rep < num_reps; rep++)
    {
        CState* pState = (rep == 0) ? m_pState : m_pOldState;
        float alpha_mult = 1;
        if (num_reps == 2)
            alpha_mult = (rep == 0) ? m_pState->m_fBlendProgress : (1 - m_pState->m_fBlendProgress);

        for (int i = 0; i < MAX_CUSTOM_SHAPES; i++)
        {
            if (pState->m_shape[i].enabled)
            {
                /*
                int bAdditive = 0;
                int nSides = 3;//3 + ((int)GetTime() % 8);
                int bThickOutline = 0;
                float x = 0.5f + 0.1f*cosf(GetTime()*0.8f+1);
                float y = 0.5f + 0.1f*sinf(GetTime()*0.8f+1);
                float rad = 0.15f + 0.07f*sinf(GetTime()*1.1f+3);
                float ang = GetTime()*1.5f;

                // inside colors
                float r = 1;
                float g = 0;
                float b = 0;
                float a = 0.4f;//0.1f + 0.1f*sinf(GetTime()*0.31f);

                // outside colors
                float r2 = 0;
                float g2 = 1;
                float b2 = 0;
                float a2 = 0;

                // border colors
                float border_r = 1;
                float border_g = 1;
                float border_b = 1;
                float border_a = 0.5f;
                */

                for (int instance = 0; instance < pState->m_shape[i].instances; instance++)
                {
                    // 1. Execute per-frame code.
                    LoadCustomShapePerFrameEvallibVars(pState, i, instance);

#ifndef _NO_EXPR_
                    if (pState->m_shape[i].m_pf_codehandle)
                    {
                        NSEEL_code_execute(pState->m_shape[i].m_pf_codehandle);
                    }
#endif

                    // save changes to t1-t8 this frame
                    /*
                    pState->m_shape[i].t_values_after_init_code[0] = *pState->m_shape[i].var_pf_t1;
                    pState->m_shape[i].t_values_after_init_code[1] = *pState->m_shape[i].var_pf_t2;
                    pState->m_shape[i].t_values_after_init_code[2] = *pState->m_shape[i].var_pf_t3;
                    pState->m_shape[i].t_values_after_init_code[3] = *pState->m_shape[i].var_pf_t4;
                    pState->m_shape[i].t_values_after_init_code[4] = *pState->m_shape[i].var_pf_t5;
                    pState->m_shape[i].t_values_after_init_code[5] = *pState->m_shape[i].var_pf_t6;
                    pState->m_shape[i].t_values_after_init_code[6] = *pState->m_shape[i].var_pf_t7;
                    pState->m_shape[i].t_values_after_init_code[7] = *pState->m_shape[i].var_pf_t8;
                    */

                    int sides = (int)(*pState->m_shape[i].var_pf_sides);
                    if (sides<3) sides=3;
                    if (sides>100) sides=100;

                    lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, ((int)(*pState->m_shape[i].var_pf_additive) != 0) ? D3D11_BLEND_ONE : D3D11_BLEND_INV_SRC_ALPHA);

                    SPRITEVERTEX v[512]; // for textured shapes (has texcoords)
                    WFVERTEX v2[512];    // for untextured shapes + borders

                    v[0].x = (float)(*pState->m_shape[i].var_pf_x * 2 - 1); // * ASPECT;
                    v[0].y = (float)(*pState->m_shape[i].var_pf_y * -2 + 1);
                    v[0].z = 0;
                    v[0].tu = 0.5f;
                    v[0].tv = 0.5f;
                    v[0].a = COLOR_NORM(*pState->m_shape[i].var_pf_a * alpha_mult);
                    v[0].r = COLOR_NORM(*pState->m_shape[i].var_pf_r);
                    v[0].g = COLOR_NORM(*pState->m_shape[i].var_pf_g);
                    v[0].b = COLOR_NORM(*pState->m_shape[i].var_pf_b);
                    v[1].a = COLOR_NORM(*pState->m_shape[i].var_pf_a2 * alpha_mult);
                    v[1].r = COLOR_NORM(*pState->m_shape[i].var_pf_r2);
                    v[1].g = COLOR_NORM(*pState->m_shape[i].var_pf_g2);
                    v[1].b = COLOR_NORM(*pState->m_shape[i].var_pf_b2);
                    for (int j = 1; j < sides + 1; j++)
                    {
                        float t = (j - 1) / (float)sides;
                        v[j].x = v[0].x + (float)*pState->m_shape[i].var_pf_rad * cosf(t * 3.1415927f * 2 + (float)*pState->m_shape[i].var_pf_ang + 3.1415927f * 0.25f) * m_fAspectY; // DON'T TOUCH!
                        v[j].y = v[0].y + (float)*pState->m_shape[i].var_pf_rad * sinf(t * 3.1415927f * 2 + (float)*pState->m_shape[i].var_pf_ang + 3.1415927f * 0.25f);              // DON'T TOUCH!
                        v[j].z = 0;
                        v[j].tu = 0.5f + 0.5f * cosf(t * 3.1415927f * 2 + (float)*pState->m_shape[i].var_pf_tex_ang + 3.1415927f * 0.25f) / ((float)*pState->m_shape[i].var_pf_tex_zoom) * m_fAspectY; // DON'T TOUCH!
                        v[j].tv = 0.5f + 0.5f * sinf(t * 3.1415927f * 2 + (float)*pState->m_shape[i].var_pf_tex_ang + 3.1415927f * 0.25f) / ((float)*pState->m_shape[i].var_pf_tex_zoom);              // DON'T TOUCH!
                        COPY_COLOR(v[j], v[1]);
                    }
                    v[sides + 1] = v[1];

                    if ((int)(*pState->m_shape[i].var_pf_textured) != 0)
                    {
                        // Draw textured version.
                        lpDevice->SetTexture(0, m_lpVS[0]);
                        lpDevice->SetVertexShader(NULL, NULL);
                        lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP + 1, sides, (LPVOID)v, sizeof(SPRITEVERTEX));
                    }
                    else
                    {
                        // No texture.
                        for (int j = 0; j < sides + 2; j++)
                        {
                            v2[j].x = v[j].x;
                            v2[j].y = v[j].y;
                            v2[j].z = v[j].z;
                            COPY_COLOR(v2[j], v[j]);
                        }
                        lpDevice->SetTexture(0, NULL);
                        lpDevice->SetVertexShader(NULL, NULL);
                        lpDevice->SetVertexColor(true);
                        lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP + 1, sides, (LPVOID)v2, sizeof(WFVERTEX));
                    }

                    // Draw border.
                    if (*pState->m_shape[i].var_pf_border_a > 0)
                    {
                        lpDevice->SetTexture(0, NULL);
                        lpDevice->SetVertexShader(NULL, NULL);
                        lpDevice->SetVertexColor(true);

                        v2[0].a = COLOR_NORM(*pState->m_shape[i].var_pf_border_a * alpha_mult);
                        v2[0].r = COLOR_NORM(*pState->m_shape[i].var_pf_border_r);
                        v2[0].g = COLOR_NORM(*pState->m_shape[i].var_pf_border_g);
                        v2[0].b = COLOR_NORM(*pState->m_shape[i].var_pf_border_b);
                        for (int j = 0; j < sides + 2; j++)
                        {
                            v2[j].x = v[j].x;
                            v2[j].y = v[j].y;
                            v2[j].z = v[j].z;
                            COPY_COLOR(v2[j], v2[0]);
                        }

                        int its = ((int)(*pState->m_shape[i].var_pf_thick) != 0) ? 4 : 1;
                        float x_inc = 2.0f / (float)m_nTexSizeX;
                        float y_inc = 2.0f / (float)m_nTexSizeY;
                        for (int it = 0; it < its; it++)
                        {
                            switch (it)
                            {
                                case 0: break;
                                case 1: for (int j = 0; j < sides + 2; j++) v2[j].x += x_inc; break; // draw fat dots
                                case 2: for (int j = 0; j < sides + 2; j++) v2[j].y += y_inc; break; // draw fat dots
                                case 3: for (int j = 0; j < sides + 2; j++) v2[j].x -= x_inc; break; // draw fat dots
                            }
                            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, sides, (LPVOID)&v2[1], sizeof(WFVERTEX));
                        }
                    }

                    lpDevice->SetTexture(0, m_lpVS[0]);
                    lpDevice->SetVertexShader(NULL, NULL);
                    lpDevice->SetVertexColor(false);
                    //lpDevice->SetFVF(SPRITEVERTEX_FORMAT);
                }
            }
        }
    }

    lpDevice->SetBlendState(false, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
}

void CPlugin::LoadCustomShapePerFrameEvallibVars(CState* pState, int i, int instance)
{
    *pState->m_shape[i].var_pf_time      = (double)(GetTime() - m_fStartTime);
    *pState->m_shape[i].var_pf_frame     = (double)GetFrame();
    *pState->m_shape[i].var_pf_fps       = (double)GetFps();
    *pState->m_shape[i].var_pf_progress  = (GetTime() - m_fPresetStartTime) / (m_fNextPresetTime - m_fPresetStartTime);
    *pState->m_shape[i].var_pf_bass      = (double)mdsound.imm_rel[0];
    *pState->m_shape[i].var_pf_mid       = (double)mdsound.imm_rel[1];
    *pState->m_shape[i].var_pf_treb      = (double)mdsound.imm_rel[2];
    *pState->m_shape[i].var_pf_bass_att  = (double)mdsound.avg_rel[0];
    *pState->m_shape[i].var_pf_mid_att   = (double)mdsound.avg_rel[1];
    *pState->m_shape[i].var_pf_treb_att  = (double)mdsound.avg_rel[2];
    for (int vi = 0; vi < NUM_Q_VAR; vi++)
        *pState->m_shape[i].var_pf_q[vi] = *pState->var_pf_q[vi];
    for (int vi = 0; vi < NUM_T_VAR; vi++)
        *pState->m_shape[i].var_pf_t[vi] = pState->m_shape[i].t_values_after_init_code[vi];
    *pState->m_shape[i].var_pf_x         = pState->m_shape[i].x;
    *pState->m_shape[i].var_pf_y         = pState->m_shape[i].y;
    *pState->m_shape[i].var_pf_rad       = pState->m_shape[i].rad;
    *pState->m_shape[i].var_pf_ang       = pState->m_shape[i].ang;
    *pState->m_shape[i].var_pf_tex_zoom  = pState->m_shape[i].tex_zoom;
    *pState->m_shape[i].var_pf_tex_ang   = pState->m_shape[i].tex_ang;
    *pState->m_shape[i].var_pf_sides     = pState->m_shape[i].sides;
    *pState->m_shape[i].var_pf_additive  = pState->m_shape[i].additive;
    *pState->m_shape[i].var_pf_textured  = pState->m_shape[i].textured;
    *pState->m_shape[i].var_pf_instances = pState->m_shape[i].instances;
    *pState->m_shape[i].var_pf_instance  = instance;
    *pState->m_shape[i].var_pf_thick     = pState->m_shape[i].thickOutline;
    *pState->m_shape[i].var_pf_r         = pState->m_shape[i].r;
    *pState->m_shape[i].var_pf_g         = pState->m_shape[i].g;
    *pState->m_shape[i].var_pf_b         = pState->m_shape[i].b;
    *pState->m_shape[i].var_pf_a         = pState->m_shape[i].a;
    *pState->m_shape[i].var_pf_r2        = pState->m_shape[i].r2;
    *pState->m_shape[i].var_pf_g2        = pState->m_shape[i].g2;
    *pState->m_shape[i].var_pf_b2        = pState->m_shape[i].b2;
    *pState->m_shape[i].var_pf_a2        = pState->m_shape[i].a2;
    *pState->m_shape[i].var_pf_border_r  = pState->m_shape[i].border_r;
    *pState->m_shape[i].var_pf_border_g  = pState->m_shape[i].border_g;
    *pState->m_shape[i].var_pf_border_b  = pState->m_shape[i].border_b;
    *pState->m_shape[i].var_pf_border_a  = pState->m_shape[i].border_a;
}

void CPlugin::LoadCustomWavePerFrameEvallibVars(CState* pState, int i)
{
    *pState->m_wave[i].var_pf_time      = (double)(GetTime() - m_fStartTime);
    *pState->m_wave[i].var_pf_frame     = (double)GetFrame();
    *pState->m_wave[i].var_pf_fps       = (double)GetFps();
    *pState->m_wave[i].var_pf_progress  = (GetTime() - m_fPresetStartTime) / (m_fNextPresetTime - m_fPresetStartTime);
    *pState->m_wave[i].var_pf_bass      = (double)mdsound.imm_rel[0];
    *pState->m_wave[i].var_pf_mid       = (double)mdsound.imm_rel[1];
    *pState->m_wave[i].var_pf_treb      = (double)mdsound.imm_rel[2];
    *pState->m_wave[i].var_pf_bass_att  = (double)mdsound.avg_rel[0];
    *pState->m_wave[i].var_pf_mid_att   = (double)mdsound.avg_rel[1];
    *pState->m_wave[i].var_pf_treb_att  = (double)mdsound.avg_rel[2];
    for (int vi = 0; vi < NUM_Q_VAR; vi++)
        *pState->m_wave[i].var_pf_q[vi] = *pState->var_pf_q[vi];
    for (int vi = 0; vi < NUM_T_VAR; vi++)
        *pState->m_wave[i].var_pf_t[vi] = pState->m_wave[i].t_values_after_init_code[vi];
    *pState->m_wave[i].var_pf_r         = pState->m_wave[i].r;
    *pState->m_wave[i].var_pf_g         = pState->m_wave[i].g;
    *pState->m_wave[i].var_pf_b         = pState->m_wave[i].b;
    *pState->m_wave[i].var_pf_a         = pState->m_wave[i].a;
    *pState->m_wave[i].var_pf_samples   = pState->m_wave[i].samples;
}

// Does a better-than-linear smooth on a wave. Roughly doubles the number of points.
int SmoothWave(WFVERTEX* vi, int nVertsIn, WFVERTEX* vo)
{
    const float c1 = -0.15f;
    const float c2 = 1.15f;
    const float c3 = 1.15f;
    const float c4 = -0.15f;
    const float inv_sum = 1.0f / (c1 + c2 + c3 + c4);

    int j = 0;

    int i_below = 0;
    int i_above;
    int i_above2 = 1;
    for (int i = 0; i < nVertsIn - 1; i++)
    {
        i_above = i_above2;
        i_above2 = std::min(nVertsIn - 1, i + 2);
        vo[j] = vi[i];
        vo[j + 1].x = (c1 * vi[i_below].x + c2 * vi[i].x + c3 * vi[i_above].x + c4 * vi[i_above2].x) * inv_sum;
        vo[j + 1].y = (c1 * vi[i_below].y + c2 * vi[i].y + c3 * vi[i_above].y + c4 * vi[i_above2].y) * inv_sum;
        vo[j + 1].z = 0;
        COPY_COLOR(vo[j + 1], vi[i]);
        i_below = i;
        j += 2;
    }
    vo[j++] = vi[nVertsIn - 1];

    return j;
}

void CPlugin::DrawCustomWaves()
{
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, NULL);
    lpDevice->SetVertexShader(NULL, NULL);
    lpDevice->SetVertexColor(true);

    // note: read in all sound data from CPluginShell's m_sound
    int num_reps = (m_pState->m_bBlending) ? 2 : 1;
    for (int rep = 0; rep < num_reps; rep++)
    {
        CState* pState = (rep == 0) ? m_pState : m_pOldState;
        float alpha_mult = 1;
        if (num_reps == 2)
            alpha_mult = (rep == 0) ? m_pState->m_fBlendProgress : (1 - m_pState->m_fBlendProgress);

        for (int i = 0; i < MAX_CUSTOM_WAVES; i++)
        {
            if (pState->m_wave[i].enabled)
            {
                int nSamples = pState->m_wave[i].samples;
                int max_samples = pState->m_wave[i].bSpectrum ? 512 : NUM_WAVEFORM_SAMPLES;
                if (nSamples > max_samples)
                    nSamples = max_samples;
                nSamples -= pState->m_wave[i].sep;

                // 1. execute per-frame code
                LoadCustomWavePerFrameEvallibVars(pState, i);

                // 2.a. do just a once-per-frame init for the *per-point* *READ-ONLY* variables
                //     (the non-read-only ones will be reset/restored at the start of each vertex)
                *pState->m_wave[i].var_pp_time     = *pState->m_wave[i].var_pf_time;
                *pState->m_wave[i].var_pp_fps      = *pState->m_wave[i].var_pf_fps;
                *pState->m_wave[i].var_pp_frame    = *pState->m_wave[i].var_pf_frame;
                *pState->m_wave[i].var_pp_progress = *pState->m_wave[i].var_pf_progress;
                *pState->m_wave[i].var_pp_bass     = *pState->m_wave[i].var_pf_bass;
                *pState->m_wave[i].var_pp_mid      = *pState->m_wave[i].var_pf_mid;
                *pState->m_wave[i].var_pp_treb     = *pState->m_wave[i].var_pf_treb;
                *pState->m_wave[i].var_pp_bass_att = *pState->m_wave[i].var_pf_bass_att;
                *pState->m_wave[i].var_pp_mid_att  = *pState->m_wave[i].var_pf_mid_att;
                *pState->m_wave[i].var_pp_treb_att = *pState->m_wave[i].var_pf_treb_att;

                NSEEL_code_execute(pState->m_wave[i].m_pf_codehandle);

                for (int vi = 0; vi < NUM_Q_VAR; vi++)
                    *pState->m_wave[i].var_pp_q[vi] = *pState->m_wave[i].var_pf_q[vi];
                for (int vi = 0; vi < NUM_T_VAR; vi++)
                    *pState->m_wave[i].var_pp_t[vi] = *pState->m_wave[i].var_pf_t[vi];

                nSamples = (int)*pState->m_wave[i].var_pf_samples;
                nSamples = std::min(512, nSamples);

                if ((nSamples >= 2) || (pState->m_wave[i].bUseDots && nSamples >= 1))
                {
                    float tempdata[2][512];
                    float mult = ((pState->m_wave[i].bSpectrum) ? 0.15f : 0.004f) * pState->m_wave[i].scaling * pState->m_fWaveScale.eval(-1.0f);
                    float* pdata1 = (pState->m_wave[i].bSpectrum) ? m_sound.fSpectrum[0].data() : m_sound.fWaveform[0].data();
                    float* pdata2 = (pState->m_wave[i].bSpectrum) ? m_sound.fSpectrum[1].data() : m_sound.fWaveform[1].data();

                    // initialize tempdata[2][512]
                    int j0 = (pState->m_wave[i].bSpectrum) ? 0 : (max_samples - nSamples) / 2 /*(1 - pState->m_wave[i].bSpectrum)*/ - pState->m_wave[i].sep / 2;
                    int j1 = (pState->m_wave[i].bSpectrum) ? 0 : (max_samples - nSamples) / 2 /*(1 - pState->m_wave[i].bSpectrum)*/ + pState->m_wave[i].sep / 2;
                    float t = (pState->m_wave[i].bSpectrum) ? (max_samples - pState->m_wave[i].sep) / (float)nSamples : 1.0f;
                    float mix1 = powf(pState->m_wave[i].smoothing * 0.98f, 0.5f); // lower exponent -> more default smoothing
                    float mix2 = 1.0f - mix1;
                    // SMOOTHING:
                    tempdata[0][0] = pdata1[j0];
                    tempdata[1][0] = pdata2[j1];
                    for (int j = 1; j < nSamples; j++)
                    {
                        tempdata[0][j] = pdata1[(int)(j * t) + j0] * mix2 + tempdata[0][j - 1] * mix1;
                        tempdata[1][j] = pdata2[(int)(j * t) + j1] * mix2 + tempdata[1][j - 1] * mix1;
                    }
                    // smooth again, backwards: [this fixes the asymmetry of the beginning & end..]
                    for (int j = nSamples - 2; j >= 0; j--)
                    {
                        tempdata[0][j] = tempdata[0][j] * mix2 + tempdata[0][j + 1] * mix1;
                        tempdata[1][j] = tempdata[1][j] * mix2 + tempdata[1][j + 1] * mix1;
                    }
                    // finally, scale to final size:
                    for (int j = 0; j < nSamples; j++)
                    {
                        tempdata[0][j] *= mult;
                        tempdata[1][j] *= mult;
                    }

                    // 2. For each point, execute per-point code.
                    // to do:
                    //  -add any of the m_wave[i].xxx menu-accessible vars to the code?
                    WFVERTEX v[1024];
                    float j_mult = 1.0f / (float)(nSamples - 1);
                    for (int j = 0; j < nSamples; j++)
                    {
                        float t2 = j * j_mult;
                        float value1 = tempdata[0][j];
                        float value2 = tempdata[1][j];
                        *pState->m_wave[i].var_pp_sample = t2;
                        *pState->m_wave[i].var_pp_value1 = value1;
                        *pState->m_wave[i].var_pp_value2 = value2;
                        *pState->m_wave[i].var_pp_x = 0.5f + value1;
                        *pState->m_wave[i].var_pp_y = 0.5f + value2;
                        *pState->m_wave[i].var_pp_r = *pState->m_wave[i].var_pf_r;
                        *pState->m_wave[i].var_pp_g = *pState->m_wave[i].var_pf_g;
                        *pState->m_wave[i].var_pp_b = *pState->m_wave[i].var_pf_b;
                        *pState->m_wave[i].var_pp_a = *pState->m_wave[i].var_pf_a;

#ifndef _NO_EXPR_
                        NSEEL_code_execute(pState->m_wave[i].m_pp_codehandle);
#endif

                        v[j].x = (float)(*pState->m_wave[i].var_pp_x * 2 - 1) * m_fInvAspectX;
                        v[j].y = (float)(*pState->m_wave[i].var_pp_y * -2 + 1) * m_fInvAspectY;
                        v[j].z = 0;
                        v[j].a = COLOR_NORM(*pState->m_wave[i].var_pp_a * alpha_mult);
                        v[j].r = COLOR_NORM(*pState->m_wave[i].var_pp_r);
                        v[j].g = COLOR_NORM(*pState->m_wave[i].var_pp_g);
                        v[j].b = COLOR_NORM(*pState->m_wave[i].var_pp_b);
                    }

                    // Save changes to t1-t8 this frame.
                    /*
                    pState->m_wave[i].t_values_after_init_code[0] = *pState->m_wave[i].var_pp_t1;
                    pState->m_wave[i].t_values_after_init_code[1] = *pState->m_wave[i].var_pp_t2;
                    pState->m_wave[i].t_values_after_init_code[2] = *pState->m_wave[i].var_pp_t3;
                    pState->m_wave[i].t_values_after_init_code[3] = *pState->m_wave[i].var_pp_t4;
                    pState->m_wave[i].t_values_after_init_code[4] = *pState->m_wave[i].var_pp_t5;
                    pState->m_wave[i].t_values_after_init_code[5] = *pState->m_wave[i].var_pp_t6;
                    pState->m_wave[i].t_values_after_init_code[6] = *pState->m_wave[i].var_pp_t7;
                    pState->m_wave[i].t_values_after_init_code[7] = *pState->m_wave[i].var_pp_t8;
                    */

                    // 3. Smooth it.
                    WFVERTEX* pVerts = v;
                    if (!pState->m_wave[i].bUseDots)
                    {
                        WFVERTEX v2[2048];
                        nSamples = SmoothWave(v, nSamples, v2);
                        pVerts = v2;
                    }

                    // 4. Draw it.
                    lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, pState->m_wave[i].bAdditive ? D3D11_BLEND_ONE : D3D11_BLEND_INV_SRC_ALPHA);

                    float ptsize = (float)((m_nTexSizeX >= 1024) ? 2 : 1) + (pState->m_wave[i].bDrawThick ? 1 : 0);
                    // DirectX 11 has not point size, so render quads.
                    //if (pState->m_wave[i].bUseDots)
                    //    lpDevice->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&ptsize));
                    if (pState->m_wave[i].bUseDots && ptsize > 1.0)
                    {
                        float dx = ptsize / (float)m_nTexSizeX;
                        float dy = ptsize / (float)m_nTexSizeY;
                        WFVERTEX* v2 = new WFVERTEX[nSamples * 6];
                        int j = 0;
                        for (int k = 0; k < nSamples * 6; k += 6)
                        {
                            v2[k] = v[j]; v2[k].x -= dx; v2[k].y -= dy;
                            v2[k + 1] = v[j]; v2[k + 1].x += dx; v2[k + 1].y -= dy;
                            v2[k + 2] = v[j]; v2[k + 2].x += dx; v2[k + 2].y += dy;
                            v2[k + 3] = v2[k];
                            v2[k + 4] = v2[k + 2];
                            v2[k + 5] = v[j]; v2[k + 5].x -= dx; v2[k + 5].y += dy;
                            ++j;
                        }
                        lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, nSamples * 2, (LPVOID)v2, sizeof(WFVERTEX));
                        delete[] v2;
                    }
                    else
                    {
                        int its = (pState->m_wave[i].bDrawThick && !pState->m_wave[i].bUseDots) ? 4 : 1;
                        float x_inc = 2.0f / (float)m_nTexSizeX;
                        float y_inc = 2.0f / (float)m_nTexSizeY;
                        for (int it = 0; it < its; it++)
                        {
                            switch (it)
                            {
                                case 0: break;
                                case 1: for (int j = 0; j < nSamples; j++) pVerts[j].x += x_inc; break; // draw fat dots
                                case 2: for (int j = 0; j < nSamples; j++) pVerts[j].y += y_inc; break; // draw fat dots
                                case 3: for (int j = 0; j < nSamples; j++) pVerts[j].x -= x_inc; break; // draw fat dots
                            }
                            lpDevice->DrawPrimitive(pState->m_wave[i].bUseDots ? D3D_PRIMITIVE_TOPOLOGY_POINTLIST : D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, nSamples - (pState->m_wave[i].bUseDots ? 0 : 1), (LPVOID)pVerts, sizeof(WFVERTEX));
                        }
                    }
                    ptsize = 1.0f;
                    //if (pState->m_wave[i].bUseDots)
                    //    lpDevice->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&ptsize));
                }
            }
        }
    }

    lpDevice->SetBlendState(false, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
}

void CPlugin::DrawWave(float* fL, float* fR)
{
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, NULL);
    lpDevice->SetVertexShader(NULL, NULL);
    lpDevice->SetVertexColor(true);
    //lpDevice->SetFVF(WFVERTEX_FORMAT);

    WFVERTEX v1[576 + 1], v2[576 + 1];

    /*
    m_lpD3DDev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD); //D3DSHADE_FLAT
    m_lpD3DDev->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
    m_lpD3DDev->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    if (m_D3DDevDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_DITHER)
        m_lpD3DDev->SetRenderState(D3DRENDERSTATE_DITHERENABLE, TRUE);
    m_lpD3DDev->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    m_lpD3DDev->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
    m_lpD3DDev->SetRenderState(D3DRENDERSTATE_COLORVERTEX, TRUE);
    m_lpD3DDev->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME); // vs. SOLID
    m_lpD3DDev->SetRenderState(D3DRENDERSTATE_AMBIENT, D3DCOLOR_RGBA_01(1, 1, 1, 1));

    hr = m_lpD3DDev->SetTexture(0, NULL);
    if (hr != D3D_OK)
    {
        //DumpDebugMessage("Draw(): ERROR: SetTexture");
        //IdentifyD3DError(hr);
    }
    */

    lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, (*m_pState->var_pf_wave_additive) ? D3D11_BLEND_ONE : D3D11_BLEND_INV_SRC_ALPHA);

    //float cr = m_pState->m_waveR.eval(GetTime());
    //float cg = m_pState->m_waveG.eval(GetTime());
    //float cb = m_pState->m_waveB.eval(GetTime());
    float cr = (float)(*m_pState->var_pf_wave_r);
    float cg = (float)(*m_pState->var_pf_wave_g);
    float cb = (float)(*m_pState->var_pf_wave_b);
    float cx = (float)(*m_pState->var_pf_wave_x);
    float cy = (float)(*m_pState->var_pf_wave_y); // note: it was backwards (top==1) in the original milkdrop, so we keep it that way!
    float fWaveParam = (float)(*m_pState->var_pf_wave_mystery);

    /*if (m_pState->m_bBlending)
    {
        cr = cr*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress) * ((float)(*m_pOldState->var_pf_wave_r));
        cg = cg*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress) * ((float)(*m_pOldState->var_pf_wave_g));
        cb = cb*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress) * ((float)(*m_pOldState->var_pf_wave_b));
        cx = cx*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress) * ((float)(*m_pOldState->var_pf_wave_x));
        cy = cy*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress) * ((float)(*m_pOldState->var_pf_wave_y));
        fWaveParam = fWaveParam*(m_pState->m_fBlendProgress) + (1.0f-m_pState->m_fBlendProgress) * ((float)(*m_pOldState->var_pf_wave_mystery));
    }*/

    if (cr < 0) cr = 0;
    if (cg < 0) cg = 0;
    if (cb < 0) cb = 0;
    if (cr > 1) cr = 1;
    if (cg > 1) cg = 1;
    if (cb > 1) cb = 1;

    // Maximize color.
    if (*m_pState->var_pf_wave_brighten)
    {
        float fMaximizeWaveColorAmount = 1.0f;
        float max = cr;
        if (max < cg) max = cg;
        if (max < cb) max = cb;
        if (max > 0.01f)
        {
            cr = cr / max * fMaximizeWaveColorAmount + cr * (1.0f - fMaximizeWaveColorAmount);
            cg = cg / max * fMaximizeWaveColorAmount + cg * (1.0f - fMaximizeWaveColorAmount);
            cb = cb / max * fMaximizeWaveColorAmount + cb * (1.0f - fMaximizeWaveColorAmount);
        }
    }

    float fWavePosX = cx * 2.0f - 1.0f; // go from 0..1 user-range to -1..1 D3D range
    float fWavePosY = cy * 2.0f - 1.0f;

    //float bass_rel = mdsound.imm[0];
    //float mid_rel = mdsound.imm[1];
    float treble_rel = mdsound.imm[2];

    int sample_offset = 0;
    int new_wavemode = (int)(*m_pState->var_pf_wave_mode) % NUM_WAVES; // since it can be changed from per-frame code!

    int its = (m_pState->m_bBlending && (new_wavemode != m_pState->m_nOldWaveMode)) ? 2 : 1;
    int nVerts1 = 0;
    int nVerts2 = 0;
    int nBreak1 = -1;
    int nBreak2 = -1;
    float alpha1 = 0.0f;
    float alpha2 = 0.0f;

    for (int it = 0; it < its; it++)
    {
        int wave = (it == 0) ? new_wavemode : m_pState->m_nOldWaveMode;
        int nVerts = NUM_WAVEFORM_SAMPLES; // allowed to peek ahead 64 (i.e. left is [i], right is [i+64])
        int nBreak = -1;

        float fWaveParam2 = fWaveParam;
        //std::string fWaveParam; // kill its scope
        if ((wave == 0 || wave == 1 || wave == 4) && (fWaveParam2 < -1 || fWaveParam2 > 1))
        {
            //fWaveParam2 = max(fWaveParam2, -1.0f);
            //fWaveParam2 = min(fWaveParam2,  1.0f);
            fWaveParam2 = fWaveParam2 * 0.5f + 0.5f;
            fWaveParam2 -= floorf(fWaveParam2);
            fWaveParam2 = fabsf(fWaveParam2);
            fWaveParam2 = fWaveParam2 * 2 - 1;
        }

        WFVERTEX* v = (it == 0) ? v1 : v2;
        ZeroMemory(v, sizeof(WFVERTEX) * nVerts);

        float alpha = (float)(*m_pState->var_pf_wave_a); //m_pState->m_fWaveAlpha.eval(GetTime());

        switch (wave)
        {
            case 0:
                // Circular wave.
                nVerts /= 2;
                sample_offset = (NUM_WAVEFORM_SAMPLES - nVerts) / 2; //mdsound.GoGoAlignatron(nVerts * 12/10); // only call this once nVerts is final!

                if (m_pState->m_bModWaveAlphaByVolume)
                    alpha *= ((mdsound.imm_rel[0] + mdsound.imm_rel[1] + mdsound.imm_rel[2]) * 0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime())) / (m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
                if (alpha < 0.0f) alpha = 0.0f;
                if (alpha > 1.0f) alpha = 1.0f;
                //color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

                {
                    float inv_nverts_minus_one = 1.0f / (float)(nVerts - 1);

                    for (int i = 0; i < nVerts; i++)
                    {
                        float rad = 0.5f + 0.4f * fR[i + sample_offset] + fWaveParam2;
                        float ang = (i)*inv_nverts_minus_one * 6.28f + GetTime() * 0.2f;
                        if (i < nVerts / 10)
                        {
                            float mix = i / (nVerts * 0.1f);
                            mix = 0.5f - 0.5f * cosf(mix * 3.1416f);
                            float rad_2 = 0.5f + 0.4f * fR[i + nVerts + sample_offset] + fWaveParam2;
                            rad = rad_2 * (1.0f - mix) + rad * (mix);
                        }
                        v[i].x = rad * cosf(ang) * m_fAspectY + fWavePosX; // 0.75 = adj. for aspect ratio
                        v[i].y = rad * sinf(ang) * m_fAspectX + fWavePosY;
                        //v[i].Diffuse = color;
                    }
                }

                // Duplicate last vertex to connect the lines; skip if blending.
                if (!m_pState->m_bBlending)
                {
                    nVerts++;
                    memcpy_s(&v[nVerts - 1], sizeof(v[nVerts - 1]), &v[0], sizeof(WFVERTEX));
                }
                break;

            case 1:
                // x-y osc. that goes around in a spiral, in time
                alpha *= 1.25f;
                if (m_pState->m_bModWaveAlphaByVolume)
                    alpha *= ((mdsound.imm_rel[0] + mdsound.imm_rel[1] + mdsound.imm_rel[2]) * 0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime())) / (m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
                if (alpha < 0.0f) alpha = 0.0f;
                if (alpha > 1.0f) alpha = 1.0f;
                //color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

                nVerts /= 2;

                for (int i = 0; i < nVerts; i++)
                {
                    float rad = 0.53f + 0.43f * fR[i] + fWaveParam2;
                    float ang = fL[i + 32] * 1.57f + GetTime() * 2.3f;
                    v[i].x = rad * cosf(ang) * m_fAspectY + fWavePosX; // 0.75 = adj. for aspect ratio
                    v[i].y = rad * sinf(ang) * m_fAspectX + fWavePosY;
                    //v[i].Diffuse = color;//(D3DCOLOR_RGBA_01(cr, cg, cb, alpha*min(1, max(0, fL[i])));
                }
                break;

            case 2:
                // centered spiro (alpha constant)
                //    aimed at not being so sound-responsive, but being very "nebula-like"
                //   difference is that alpha is constant (and faint), and waves a scaled way up
                switch (m_nTexSizeX)
                {
                    case 256:  alpha *= 0.07f; break;
                    case 512:  alpha *= 0.09f; break;
                    case 1024: alpha *= 0.11f; break;
                    case 2048: alpha *= 0.13f; break;
                }

                if (m_pState->m_bModWaveAlphaByVolume)
                    alpha *= ((mdsound.imm_rel[0] + mdsound.imm_rel[1] + mdsound.imm_rel[2]) * 0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime())) / (m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
                if (alpha < 0.0f) alpha = 0.0f;
                if (alpha > 1.0f) alpha = 1.0f;
                //color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

                for (int i = 0; i < nVerts; i++)
                {
                    v[i].x = fR[i] * m_fAspectY + fWavePosX; //((pR[i] ^ 128) - 128) / 90.0f * ASPECT; // 0.75 = adj. for aspect ratio
                    v[i].y = fL[i + 32] * m_fAspectX + fWavePosY; //((pL[i + 32] ^ 128) - 128) / 90.0f;
                    //v[i].Diffuse = color;
                }
                break;

            case 3:
                // centered spiro (alpha tied to volume)
                //     aimed at having a strong audio-visual tie-in
                //   colors are always bright (no darks)
                switch (m_nTexSizeX)
                {
                    case 256:  alpha = 0.075f; break;
                    case 512:  alpha = 0.150f; break;
                    case 1024: alpha = 0.220f; break;
                    case 2048: alpha = 0.330f; break;
                }

                alpha *= 1.3f;
                alpha *= powf(treble_rel, 2.0f);
                if (m_pState->m_bModWaveAlphaByVolume)
                    alpha *= ((mdsound.imm_rel[0] + mdsound.imm_rel[1] + mdsound.imm_rel[2]) * 0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime())) / (m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
                if (alpha < 0.0f) alpha = 0.0f;
                if (alpha > 1.0f) alpha = 1.0f;
                //color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

                for (int i = 0; i < nVerts; i++)
                {
                    v[i].x = fR[i] * m_fAspectY + fWavePosX; //((pR[i] ^ 128) - 128) / 90.0f * ASPECT; // 0.75 = adj. for aspect ratio
                    v[i].y = fL[i + 32] * m_fAspectX + fWavePosY; //((pL[i + 32] ^ 128) - 128) / 90.0f;
                    //v[i].Diffuse = color;
                }
                break;

            case 4:
                // Horizontal "script", left channel
                if (nVerts > m_nTexSizeX / 3)
                    nVerts = m_nTexSizeX / 3;

                sample_offset = (NUM_WAVEFORM_SAMPLES - nVerts) / 2; //mdsound.GoGoAlignatron(nVerts + 25); // only call this once nVerts is final!

                /*
                if (treble_rel > treb_thresh_for_wave6)
                {
                    //alpha = 1.0f;
                    treb_thresh_for_wave6 = treble_rel * 1.025f;
                }
                else
                {
                    alpha *= 0.2f;
                    treb_thresh_for_wave6 *= 0.996f; // FIXME: make this fps-independent
                }
                */

                if (m_pState->m_bModWaveAlphaByVolume)
                    alpha *= ((mdsound.imm_rel[0] + mdsound.imm_rel[1] + mdsound.imm_rel[2]) * 0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime())) / (m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
                if (alpha < 0.0f) alpha = 0.0f;
                if (alpha > 1.0f) alpha = 1.0f;
                //color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

                {
                    float w1 = 0.45f + 0.5f * (fWaveParam2 * 0.5f + 0.5f); // 0.1 - 0.9
                    float w2 = 1.0f - w1;

                    float inv_nverts = 1.0f / (float)(nVerts);

                    for (int i = 0; i < nVerts; i++)
                    {
                        v[i].x = -1.0f + 2.0f * (i * inv_nverts) + fWavePosX;
                        v[i].y = fL[i + sample_offset] * 0.47f + fWavePosY; //((pL[i] ^ 128) - 128) / 270.0f;
                        v[i].x += fR[i + 25 + sample_offset] * 0.44f; //((pR[i + 25] ^ 128) - 128) / 290.0f;
                        //v[i].Diffuse = color;

                        // Momentum.
                        if (i > 1)
                        {
                            v[i].x = v[i].x * w2 + w1 * (v[i - 1].x * 2.0f - v[i - 2].x);
                            v[i].y = v[i].y * w2 + w1 * (v[i - 1].y * 2.0f - v[i - 2].y);
                        }
                    }

                    /*
                    // center on Y
                    float avg_y = 0;
                    for (int i = 0; i < nVerts; i++)
                        avg_y += v[i].y;
                    avg_y /= (float)nVerts;
                    avg_y *= 0.5f; // damp the movement
                    for (int i = 0; i < nVerts; i++)
                        v[i].y -= avg_y;
                    */
                }
                break;

            case 5:
                // Weird explosive complex number thingy.
                switch (m_nTexSizeX)
                {
                    case 256:  alpha *= 0.07f; break;
                    case 512:  alpha *= 0.09f; break;
                    case 1024: alpha *= 0.11f; break;
                    case 2048: alpha *= 0.13f; break;
                }

                if (m_pState->m_bModWaveAlphaByVolume)
                    alpha *= ((mdsound.imm_rel[0] + mdsound.imm_rel[1] + mdsound.imm_rel[2]) * 0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime())) / (m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
                if (alpha < 0.0f) alpha = 0.0f;
                if (alpha > 1.0f) alpha = 1.0f;
                //color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

                {
                    float cos_rot = cosf(GetTime() * 0.3f);
                    float sin_rot = sinf(GetTime() * 0.3f);

                    for (int i = 0; i < nVerts; i++)
                    {
                        float x0 = (fR[i] * fL[i + 32] + fL[i] * fR[i + 32]);
                        float y0 = (fR[i] * fR[i] - fL[i + 32] * fL[i + 32]);
                        v[i].x = (x0 * cos_rot - y0 * sin_rot) * m_fAspectY + fWavePosX;
                        v[i].y = (x0 * sin_rot + y0 * cos_rot) * m_fAspectX + fWavePosY;
                        //v[i].Diffuse = color;
                    }
                }
                break;

            case 6:
            case 7:
            case 8:
                // 6: angle-adjustable left channel, with temporal wave alignment;
                //   fWaveParam2 controls the angle at which it's drawn
                //   fWavePosX slides the wave away from the center, transversely.
                //   fWavePosY does nothing
                //
                // 7: same, except there are two channels shown, and
                //   fWavePosY determines the separation distance.
                //
                // 8: same as 6, except using the spectrum analyzer (UNFINISHED)
                nVerts /= 2;

                if (nVerts > m_nTexSizeX / 3)
                    nVerts = m_nTexSizeX / 3;

                if (wave == 8)
                    nVerts = 256;
                else
                    sample_offset = (NUM_WAVEFORM_SAMPLES - nVerts) / 2; //mdsound.GoGoAlignatron(nVerts); // only call this once nVerts is final!

                if (m_pState->m_bModWaveAlphaByVolume)
                    alpha *= ((mdsound.imm_rel[0] + mdsound.imm_rel[1] + mdsound.imm_rel[2]) * 0.333f - m_pState->m_fModWaveAlphaStart.eval(GetTime())) / (m_pState->m_fModWaveAlphaEnd.eval(GetTime()) - m_pState->m_fModWaveAlphaStart.eval(GetTime()));
                if (alpha < 0.0f) alpha = 0.0f;
                if (alpha > 1.0f) alpha = 1.0f;
                //color = D3DCOLOR_RGBA_01(cr, cg, cb, alpha);

                {
                    float ang = 1.57f * fWaveParam2; // from -PI/2 to PI/2
                    float dx = cosf(ang);
                    float dy = sinf(ang);

                    float edge_x[2], edge_y[2];

                    //edge_x[0] = fWavePosX - dx * 3.0f;
                    //edge_y[0] = fWavePosY - dy * 3.0f;
                    //edge_x[1] = fWavePosX + dx * 3.0f;
                    //edge_y[1] = fWavePosY + dy * 3.0f;
                    edge_x[0] = fWavePosX * cosf(ang + 1.57f) - dx * 3.0f;
                    edge_y[0] = fWavePosX * sinf(ang + 1.57f) - dy * 3.0f;
                    edge_x[1] = fWavePosX * cosf(ang + 1.57f) + dx * 3.0f;
                    edge_y[1] = fWavePosX * sinf(ang + 1.57f) + dy * 3.0f;

                    for (int i = 0; i < 2; i++) // for each point defining the line
                    {
                        // clip the point against 4 edges of screen
                        // be a bit lenient (use +/-1.1 instead of +/-1.0)
                        //     so the dual-wave doesn't end too soon, after the channels are moved apart
                        for (int j = 0; j < 4; j++)
                        {
                            float t = 0.0f;
                            bool bClip = false;

                            switch (j)
                            {
                                case 0:
                                    if (edge_x[i] > 1.1f)
                                    {
                                        t = (1.1f - edge_x[1 - i]) / (edge_x[i] - edge_x[1 - i]);
                                        bClip = true;
                                    }
                                    break;
                                case 1:
                                    if (edge_x[i] < -1.1f)
                                    {
                                        t = (-1.1f - edge_x[1 - i]) / (edge_x[i] - edge_x[1 - i]);
                                        bClip = true;
                                    }
                                    break;
                                case 2:
                                    if (edge_y[i] > 1.1f)
                                    {
                                        t = (1.1f - edge_y[1 - i]) / (edge_y[i] - edge_y[1 - i]);
                                        bClip = true;
                                    }
                                    break;
                                case 3:
                                    if (edge_y[i] < -1.1f)
                                    {
                                        t = (-1.1f - edge_y[1 - i]) / (edge_y[i] - edge_y[1 - i]);
                                        bClip = true;
                                    }
                                    break;
                            }

                            if (bClip)
                            {
                                dx = edge_x[i] - edge_x[1 - i];
                                dy = edge_y[i] - edge_y[1 - i];
                                edge_x[i] = edge_x[1 - i] + dx * t;
                                edge_y[i] = edge_y[1 - i] + dy * t;
                            }
                        }
                    }

                    dx = (edge_x[1] - edge_x[0]) / (float)nVerts;
                    dy = (edge_y[1] - edge_y[0]) / (float)nVerts;
                    float ang2 = atan2f(dy, dx);
                    float perp_dx = cosf(ang2 + 1.57f);
                    float perp_dy = sinf(ang2 + 1.57f);

                    if (wave == 6)
                        for (int i = 0; i < nVerts; i++)
                        {
                            v[i].x = edge_x[0] + dx * i + perp_dx * 0.25f * fL[i + sample_offset];
                            v[i].y = edge_y[0] + dy * i + perp_dy * 0.25f * fL[i + sample_offset];
                            //v[i].Diffuse = color;
                        }
                    else if (wave == 8)
                        // 256 verts.
                        for (int i = 0; i < nVerts; i++)
                        {
                            float f = 0.1f * logf(mdsound.fSpecLeft[i * 2] + mdsound.fSpecLeft[i * 2 + 1]);
                            v[i].x = edge_x[0] + dx * i + perp_dx * f;
                            v[i].y = edge_y[0] + dy * i + perp_dy * f;
                            //v[i].Diffuse = color;
                        }
                    else
                    {
                        float sep = powf(fWavePosY * 0.5f + 0.5f, 2.0f);
                        for (int i = 0; i < nVerts; i++)
                        {
                            v[i].x = edge_x[0] + dx * i + perp_dx * (0.25f * fL[i + sample_offset] + sep);
                            v[i].y = edge_y[0] + dy * i + perp_dy * (0.25f * fL[i + sample_offset] + sep);
                            //v[i].Diffuse = color;
                        }

                        //D3DPRIMITIVETYPE primtype = (*m_pState->var_pf_wave_usedots) ? D3DPT_POINTLIST : D3DPT_LINESTRIP;
                        //m_lpD3DDev->DrawPrimitive(primtype, D3DFVF_LVERTEX, (LPVOID)v, nVerts, NULL);

                        for (int i = 0; i < nVerts; i++)
                        {
                            v[i + nVerts].x = edge_x[0] + dx * i + perp_dx * (0.25f * fR[i + sample_offset] - sep);
                            v[i + nVerts].y = edge_y[0] + dy * i + perp_dy * (0.25f * fR[i + sample_offset] - sep);
                            //v[i+nVerts].Diffuse = color;
                        }

                        nBreak = nVerts;
                        nVerts *= 2;
                    }
                }
                break;
        }

        if (it == 0)
        {
            nVerts1 = nVerts;
            nBreak1 = nBreak;
            alpha1 = alpha;
        }
        else
        {
            nVerts2 = nVerts;
            nBreak2 = nBreak;
            alpha2 = alpha;
        }
    }

    // v1[] is for the current waveform
    // v2[] is for the old waveform (from prev. preset - only used if blending)
    // nVerts1 is the # of vertices in v1
    // nVerts2 is the # of vertices in v2
    // nBreak1 is the index of the point at which to break the solid line in v1[] (-1 if no break)
    // nBreak2 is the index of the point at which to break the solid line in v2[] (-1 if no break)

    float mix = CosineInterp(m_pState->m_fBlendProgress);
    float mix2 = 1.0f - mix;

    // blend 2 waveforms
    if (nVerts2 > 0)
    {
        // note: this won't yet handle the case where (nBreak1 > 0 && nBreak2 > 0)
        //       in this case, code must break wave into THREE segments
        float m = (nVerts2 - 1) / (float)nVerts1;
        float x, y;
        for (int i = 0; i < nVerts1; i++)
        {
            float fIdx = i * m;
            int nIdx = (int)fIdx;
            float t = fIdx - nIdx;
            if (nIdx == nBreak2 - 1)
            {
                x = v2[nIdx].x;
                y = v2[nIdx].y;
                nBreak1 = i + 1;
            }
            else
            {
                x = v2[nIdx].x * (1 - t) + v2[nIdx + 1].x * (t);
                y = v2[nIdx].y * (1 - t) + v2[nIdx + 1].y * (t);
            }
            v1[i].x = v1[i].x * (mix) + x * (mix2);
            v1[i].y = v1[i].y * (mix) + y * (mix2);
        }
    }

    // Determine alpha.
    if (nVerts2 > 0)
    {
        alpha1 = alpha1 * (mix) + alpha2 * (1.0f - mix);
    }

    // Apply color and alpha.
    // ALSO reverse all y values, to stay consistent with the pre-VMS MilkDrop,
    // which DIDN'T:
    v1[0].a = COLOR_NORM(alpha1);
    v1[0].r = COLOR_NORM(cr);
    v1[0].g = COLOR_NORM(cb);
    v1[0].b = COLOR_NORM(cg);
    for (int i = 0; i < nVerts1; i++)
    {
        COPY_COLOR(v1[i], v1[0]);
        v1[i].y = -v1[i].y;
    }

    WFVERTEX* pVerts = v1;

    // Do not draw wave if (possibly blended) alpha is less than zero.
    if (alpha1 < 0.004f)
        goto SKIP_DRAW_WAVE;

    // TESSELLATE - smooth the wave, one time.
    WFVERTEX vTess[(576 + 3) * 2];
    if (1)
    {
        if (nBreak1 == -1)
        {
            nVerts1 = SmoothWave(v1, nVerts1, vTess);
        }
        else
        {
            int oldBreak = nBreak1;
            nBreak1 = SmoothWave(v1, nBreak1, vTess);
            nVerts1 = SmoothWave(&v1[oldBreak], nVerts1 - oldBreak, &vTess[nBreak1]) + nBreak1;
        }
        pVerts = vTess;
    }

    // Draw primitives.
    {
        //D3DPRIMITIVETYPE primtype = (*m_pState->var_pf_wave_usedots) ? D3DPT_POINTLIST : D3DPT_LINESTRIP;
        float x_inc = 2.0f / (float)m_nTexSizeX;
        float y_inc = 2.0f / (float)m_nTexSizeY;
        int drawing_its = ((*m_pState->var_pf_wave_thick || *m_pState->var_pf_wave_usedots) && (m_nTexSizeX >= 512)) ? 4 : 1;

        for (int it = 0; it < drawing_its; it++)
        {
            int j;

            switch (it)
            {
                case 0: break;
                case 1: for (j = 0; j < nVerts1; j++) pVerts[j].x += x_inc; break; // draw fat dots
                case 2: for (j = 0; j < nVerts1; j++) pVerts[j].y += y_inc; break; // draw fat dots
                case 3: for (j = 0; j < nVerts1; j++) pVerts[j].x -= x_inc; break; // draw fat dots
            }

            if (nBreak1 == -1)
            {
                if (*m_pState->var_pf_wave_usedots)
                    lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, nVerts1, (LPVOID)pVerts, sizeof(WFVERTEX));
                else
                    lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, nVerts1 - 1, (LPVOID)pVerts, sizeof(WFVERTEX));
            }
            else
            {
                if (*m_pState->var_pf_wave_usedots)
                {
                    lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, nBreak1, (LPVOID)pVerts, sizeof(WFVERTEX));
                    lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, nVerts1 - nBreak1, (LPVOID)&pVerts[nBreak1], sizeof(WFVERTEX));
                }
                else
                {
                    lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, nBreak1 - 1, (LPVOID)pVerts, sizeof(WFVERTEX));
                    lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, nVerts1 - nBreak1 - 1, (LPVOID)&pVerts[nBreak1], sizeof(WFVERTEX));
                }
            }
        }
    }

SKIP_DRAW_WAVE:
    lpDevice->SetBlendState(false);
}

void CPlugin::DrawSprites()
{
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, NULL);
    lpDevice->SetVertexShader(NULL, NULL);
    lpDevice->SetVertexColor(true);
    //lpDevice->SetFVF(WFVERTEX_FORMAT);

    if (*m_pState->var_pf_darken_center)
    {
        lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);

        WFVERTEX v3[6];
        ZeroMemory(v3, sizeof(WFVERTEX) * 6);

        // Colors.
        v3[0].r = v3[0].g = v3[0].b = 0.0f; v3[0].a = 3.0f / 32.0f;
        v3[1].r = v3[1].g = v3[1].b = 0.0f; v3[1].a = 0.0f;
        COPY_COLOR(v3[2], v3[1]);
        COPY_COLOR(v3[3], v3[1]);
        COPY_COLOR(v3[4], v3[1]);
        COPY_COLOR(v3[5], v3[1]);

        // Positioning.
        float fHalfSize = 0.05f;
        v3[0].x = 0.0f;
        v3[1].x = 0.0f - fHalfSize * m_fAspectY;
        v3[2].x = 0.0f;
        v3[3].x = 0.0f + fHalfSize * m_fAspectY;
        v3[4].x = 0.0f;
        v3[5].x = v3[1].x;
        v3[0].y = 0.0f;
        v3[1].y = 0.0f;
        v3[2].y = 0.0f - fHalfSize;
        v3[3].y = 0.0f;
        v3[4].y = 0.0f + fHalfSize;
        v3[5].y = v3[1].y;
        //v3[0].tu = 0; v3[1].tu = 1; v3[2].tu = 0; v3[3].tu = 1;
        //v3[0].tv = 1; v3[1].tv = 1; v3[2].tv = 0; v3[3].tv = 0;

        lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP + 1, 4, (LPVOID)v3, sizeof(WFVERTEX));

        lpDevice->SetBlendState(false);
    }

    // Do borders.
    {
        float fOuterBorderSize = (float)*m_pState->var_pf_ob_size;
        float fInnerBorderSize = (float)*m_pState->var_pf_ib_size;

        lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);

        for (int it = 0; it < 2; it++)
        {
            WFVERTEX v3[4];
            ZeroMemory(v3, sizeof(WFVERTEX) * 4);

            // Colors.
            float r = (it == 0) ? (float)*m_pState->var_pf_ob_r : (float)*m_pState->var_pf_ib_r;
            float g = (it == 0) ? (float)*m_pState->var_pf_ob_g : (float)*m_pState->var_pf_ib_g;
            float b = (it == 0) ? (float)*m_pState->var_pf_ob_b : (float)*m_pState->var_pf_ib_b;
            float a = (it == 0) ? (float)*m_pState->var_pf_ob_a : (float)*m_pState->var_pf_ib_a;
            if (a > 0.001f)
            {
                v3[0].a = COLOR_NORM(a);
                v3[0].r = COLOR_NORM(r);
                v3[0].g = COLOR_NORM(g);
                v3[0].b = COLOR_NORM(b);
                COPY_COLOR(v3[1], v3[0]);
                COPY_COLOR(v3[2], v3[0]);
                COPY_COLOR(v3[3], v3[0]);

                // Positioning.
                float fInnerRad = (it == 0) ? 1.0f - fOuterBorderSize : 1.0f - fOuterBorderSize - fInnerBorderSize;
                float fOuterRad = (it == 0) ? 1.0f : 1.0f - fOuterBorderSize;
                v3[0].x =  fInnerRad;
                v3[1].x =  fOuterRad;
                v3[2].x =  fOuterRad;
                v3[3].x =  fInnerRad;
                v3[0].y =  fInnerRad;
                v3[1].y =  fOuterRad;
                v3[2].y = -fOuterRad;
                v3[3].y = -fInnerRad;

                for (int rot = 0; rot < 4; rot++)
                {
                    lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP + 1, 2, (LPVOID)v3, sizeof(WFVERTEX));

                    // Rotate by 90 degrees.
                    for (int v = 0; v < 4; v++)
                    {
                        float t = 1.570796327f;
                        float x = v3[v].x;
                        float y = v3[v].y;
                        v3[v].x = x * cosf(t) - y * sinf(t);
                        v3[v].y = x * sinf(t) + y * cosf(t);
                    }
                }
            }
        }
        lpDevice->SetBlendState(false);
    }
}

// Draws sprites from system memory to back buffer.
void CPlugin::DrawUserSprites()
{
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, NULL);
    lpDevice->SetVertexShader(NULL, NULL);
    //lpDevice->SetFVF(SPRITEVERTEX_FORMAT);

    //lpDevice->SetRenderState(D3DRS_WRAP0, 0);
    //lpDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    //lpDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    //lpDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);

    // Reset these to the standard safe mode.
    lpDevice->SetShader(0);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
    //lpDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
    //lpDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    //lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    //lpDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    //lpDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    //if (m_D3DDevDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_DITHER)
    //    lpDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE);
    //lpDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    //lpDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    //lpDevice->SetRenderState(D3DRS_COLORVERTEX, TRUE);
    //lpDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID); // vs. wireframe
    //lpDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_RGBA_01(1,1,1,1));
    //lpDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTFG_LINEAR);
    //lpDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTFN_LINEAR);
    //lpDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTFP_LINEAR);

    for (int iSlot = 0; iSlot < NUM_TEX; iSlot++)
    {
        if (m_texmgr.m_tex[iSlot].pSurface)
        {
            // Set values of input variables.
            *(m_texmgr.m_tex[iSlot].var_time) = (double)(GetTime() - m_texmgr.m_tex[iSlot].fStartTime);
            *(m_texmgr.m_tex[iSlot].var_frame) = (double)(GetFrame() - m_texmgr.m_tex[iSlot].nStartFrame);
            *(m_texmgr.m_tex[iSlot].var_fps) = (double)GetFps();
            *(m_texmgr.m_tex[iSlot].var_progress) = (double)m_pState->m_fBlendProgress;
            *(m_texmgr.m_tex[iSlot].var_bass) = (double)mdsound.imm_rel[0];
            *(m_texmgr.m_tex[iSlot].var_mid) = (double)mdsound.imm_rel[1];
            *(m_texmgr.m_tex[iSlot].var_treb) = (double)mdsound.imm_rel[2];
            *(m_texmgr.m_tex[iSlot].var_bass_att) = (double)mdsound.avg_rel[0];
            *(m_texmgr.m_tex[iSlot].var_mid_att) = (double)mdsound.avg_rel[1];
            *(m_texmgr.m_tex[iSlot].var_treb_att) = (double)mdsound.avg_rel[2];

#ifndef _NO_EXPR_
            // Evaluate expressions.
            if (m_texmgr.m_tex[iSlot].m_codehandle)
            {
                NSEEL_code_execute(m_texmgr.m_tex[iSlot].m_codehandle);
            }
#endif

            bool bKillSprite = (*m_texmgr.m_tex[iSlot].var_done != 0.0);
            bool bBurnIn = (*m_texmgr.m_tex[iSlot].var_burn != 0.0);

            // Remember the original backbuffer and Z-buffer.
            ID3D11Texture2D* pBackBuffer = NULL; //, *pZBuffer = NULL;
            lpDevice->GetRenderTarget(&pBackBuffer);
            //lpDevice->GetDepthStencilSurface(&pZBuffer);

            if (/*bKillSprite &&*/ bBurnIn)
            {
                // Set up to render [from NULL] to VS1 (for burn-in).
                lpDevice->SetTexture(0, NULL);

                lpDevice->SetRenderTarget(m_lpVS[1]);

                lpDevice->SetTexture(0, NULL);
            }

            // Finally, use the results to draw the sprite.
            lpDevice->SetTexture(0, m_texmgr.m_tex[iSlot].pSurface);
            //if (lpDevice->SetTexture(0, m_texmgr.m_tex[iSlot].pSurface) != D3D_OK)
            //  return;

            SPRITEVERTEX v3[4];
            ZeroMemory(v3, sizeof(SPRITEVERTEX) * 4);

            /*int dest_w, dest_h;
            {
                LPDIRECT3DSURFACE9 pRT;
                lpDevice->GetRenderTarget( 0, &pRT );

                D3DSURFACE_DESC desc;
                pRT->GetDesc(&desc);
                dest_w = desc.Width;
                dest_h = desc.Height;
                pRT->Release();
            }*/

            float x = std::min(1000.0f, std::max(-1000.0f, (float)(*m_texmgr.m_tex[iSlot].var_x) * 2.0f - 1.0f));
            float y = std::min(1000.0f, std::max(-1000.0f, (float)(*m_texmgr.m_tex[iSlot].var_y) * 2.0f - 1.0f));
            float sx = std::min(1000.0f, std::max(-1000.0f, (float)(*m_texmgr.m_tex[iSlot].var_sx)));
            float sy = std::min(1000.0f, std::max(-1000.0f, (float)(*m_texmgr.m_tex[iSlot].var_sy)));
            float rot = (float)(*m_texmgr.m_tex[iSlot].var_rot);
            int flipx = (*m_texmgr.m_tex[iSlot].var_flipx == 0.0) ? 0 : 1;
            int flipy = (*m_texmgr.m_tex[iSlot].var_flipy == 0.0) ? 0 : 1;
            float repeatx = std::min(100.0f, std::max(0.01f, (float)(*m_texmgr.m_tex[iSlot].var_repeatx)));
            float repeaty = std::min(100.0f, std::max(0.01f, (float)(*m_texmgr.m_tex[iSlot].var_repeaty)));

            int blendmode = std::min(4, std::max(0, ((int)(*m_texmgr.m_tex[iSlot].var_blendmode))));
            float r = std::min(1.0f, std::max(0.0f, ((float)(*m_texmgr.m_tex[iSlot].var_r))));
            float g = std::min(1.0f, std::max(0.0f, ((float)(*m_texmgr.m_tex[iSlot].var_g))));
            float b = std::min(1.0f, std::max(0.0f, ((float)(*m_texmgr.m_tex[iSlot].var_b))));
            float a = std::min(1.0f, std::max(0.0f, ((float)(*m_texmgr.m_tex[iSlot].var_a))));

            // Set x,y coordinates.
            v3[0 + flipx].x = -sx;
            v3[1 - flipx].x = sx;
            v3[2 + flipx].x = -sx;
            v3[3 - flipx].x = sx;
            v3[0 + flipy * 2].y = -sy;
            v3[1 + flipy * 2].y = -sy;
            v3[2 - flipy * 2].y = sy;
            v3[3 - flipy * 2].y = sy;

            // First aspect ratio: adjust for non-1:1 images.
            {
                float aspect = m_texmgr.m_tex[iSlot].img_h / (float)m_texmgr.m_tex[iSlot].img_w;

                if (aspect < 1)
                    for (int k = 0; k < 4; k++)
                        v3[k].y *= aspect; // wide image
                else
                    for (int k = 0; k < 4; k++)
                        v3[k].x /= aspect; // tall image
            }

            // 2D rotation.
            {
                float cos_rot = cosf(rot);
                float sin_rot = sinf(rot);
                for (int k = 0; k < 4; k++)
                {
                    float x2 = v3[k].x * cos_rot - v3[k].y * sin_rot;
                    float y2 = v3[k].x * sin_rot + v3[k].y * cos_rot;
                    v3[k].x = x2;
                    v3[k].y = y2;
                }
            }

            // Translation.
            for (int k = 0; k < 4; k++)
            {
                v3[k].x += x;
                v3[k].y += y;
            }

            // Second aspect ratio: normalize to width of screen.
            {
                float aspect = GetWidth() / (float)(GetHeight());
                if (aspect > 1)
                    for (int k = 0; k < 4; k++)
                        v3[k].y *= aspect;
                else
                    for (int k = 0; k < 4; k++)
                        v3[k].x /= aspect;
            }

            // Third aspect ratio: adjust for burn-in.
            if (bKillSprite && bBurnIn) // final render-to-VS1
            {
                float aspect = GetWidth() / (float)(GetHeight() * 4.0f / 3.0f);
                if (aspect < 1.0f)
                    for (int k = 0; k < 4; k++)
                        v3[k].x *= aspect;
                else
                    for (int k = 0; k < 4; k++)
                        v3[k].y /= aspect;
            }

            // Finally, flip Y for annoying DirectX.
            //for (int k = 0; k < 4; k++)
            //    v3[k].y *= -1.0f;

            // Set U,V coordinates.
            {
                float dtu = 0.5f; // / (float)m_texmgr.m_tex[iSlot].tex_w;
                float dtv = 0.5f; // / (float)m_texmgr.m_tex[iSlot].tex_h;
                v3[0].tu = -dtu;
                v3[1].tu =  dtu; //m_texmgr.m_tex[iSlot].img_w / (float)m_texmgr.m_tex[iSlot].tex_w
                v3[2].tu = -dtu;
                v3[3].tu =  dtu; //m_texmgr.m_tex[iSlot].img_w / (float)m_texmgr.m_tex[iSlot].tex_w
                v3[0].tv = -dtv;
                v3[1].tv = -dtv;
                v3[2].tv =  dtv; //m_texmgr.m_tex[iSlot].img_h / (float)m_texmgr.m_tex[iSlot].tex_h
                v3[3].tv =  dtv; //m_texmgr.m_tex[iSlot].img_h / (float)m_texmgr.m_tex[iSlot].tex_h

                // Repeat on X,Y.
                for (int k = 0; k < 4; k++)
                {
                    v3[k].tu = (v3[k].tu - 0.0f) * repeatx + 0.5f;
                    v3[k].tv = (v3[k].tv - 0.0f) * repeaty + 0.5f;
                }
            }

            // Blend modes                                     src alpha         dest alpha
            // -------------------------------------------------------------------------------
            // 0   blend      r,g,b=modulate     a=opacity     SRCALPHA          INVSRCALPHA
            // 1   decal      r,g,b=modulate     a=modulate    D3DBLEND_ONE      D3DBLEND_ZERO
            // 2   additive   r,g,b=modulate     a=modulate    D3DBLEND_ONE      D3DBLEND_ONE
            // 3   srccolor   r,g,b=no effect    a=no effect   SRCCOLOR          INVSRCCOLOR
            // 4   colorkey   r,g,b=modulate     a=no effect
            switch (blendmode)
            {
                case 0:
                default:
                    // Alpha blend.

                    /*
                    Q. I am rendering with alpha blending and setting the alpha
                    of the diffuse vertex component to determine the opacity.
                    It works when there is no texture set, but as soon as I set
                    a texture the alpha that I set is no longer applied.  Why?

                    The problem originates in the texture blending stages, rather
                    than in the subsequent alpha blending.  Alpha can come from
                    several possible sources.  If this has not been specified,
                    then the alpha will be taken from the texture, if one is selected.
                    If no texture is selected, then the default will use the alpha
                    channel of the diffuse vertex component.

                    Explicitly specifying the diffuse vertex component as the source
                    for alpha will insure that the alpha is drawn from the alpha value
                    you set, whether a texture is selected or not:

                    pDevice->SetSamplerState(D3DSAMP_ALPHAOP, D3DTOP_SELECTARG1);
                    pDevice->SetSamplerState(D3DSAMP_ALPHAARG1, D3DTA_DIFFUSE);

                    If you later need to use the texture alpha as the source, set
                    D3DSAMP_ALPHAARG1 to D3DTA_TEXTURE.
                    */

                    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
                    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                    lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
                    for (int k = 0; k < 4; k++)
                    {
                        v3[k].a = COLOR_NORM(a);
                        v3[k].r = COLOR_NORM(r);
                        v3[k].g = COLOR_NORM(g);
                        v3[k].b = COLOR_NORM(b);
                    }
                    break;
                case 1:
                    // Decal.
                    lpDevice->SetBlendState(false);
                    for (int k = 0; k < 4; k++)
                    {
                        v3[k].a = 1.0f;
                        v3[k].r = COLOR_NORM(r * a);
                        v3[k].g = COLOR_NORM(g * a);
                        v3[k].b = COLOR_NORM(b * a);
                    }
                    break;
                case 2:
                    // Additive.
                    lpDevice->SetBlendState(true, D3D11_BLEND_ONE, D3D11_BLEND_ONE);
                    for (int k = 0; k < 4; k++)
                    {
                        v3[k].a = 1.0f;
                        v3[k].r = COLOR_NORM(r * a);
                        v3[k].g = COLOR_NORM(g * a);
                        v3[k].b = COLOR_NORM(b * a);
                    }
                    break;
                case 3:
                    // Source color.
                    lpDevice->SetBlendState(true, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_INV_SRC_COLOR);
                    for (int k = 0; k < 4; k++)
                    {
                        v3[k].a = 1.0f;
                        v3[k].r = 1.0f;
                        v3[k].g = 1.0f;
                        v3[k].b = 1.0f;
                    }
                    break;
                case 4:
                    // Color-keyed texture: use the alpha value in the texture to
                    // determine which texels get drawn.
                    /*
                    lpDevice->SetRenderState(D3DRS_ALPHAREF, 0);
                    lpDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
                    lpDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
                    */
                    lpDevice->SetShader(3);
                    //lpDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
                    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
                    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
                    //lpDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
                    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
                    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
                    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
                    //lpDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

                    // Also, smoothly blend this in-between texels.
                    lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
                    for (int k = 0; k < 4; k++)
                    {
                        v3[k].a = COLOR_NORM(a);
                        v3[k].r = COLOR_NORM(r);
                        v3[k].g = COLOR_NORM(g);
                        v3[k].b = COLOR_NORM(b);
                    }
                    break;
            }

            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));

            if (/*bKillSprite &&*/ bBurnIn) // final render-to-VS1
            {
                // Change the render target back to the original setup.
                lpDevice->SetTexture(0, NULL);
                lpDevice->SetRenderTarget(pBackBuffer);
                //lpDevice->SetDepthStencilSurface(pZBuffer);
                lpDevice->SetTexture(0, m_texmgr.m_tex[iSlot].pSurface);

                // Undo aspect ratio changes (that were used to fit it to VS1).
                {
                    float aspect = GetWidth() / static_cast<float>(GetHeight() * 4.0f / 3.0f);
                    if (aspect < 1.0f)
                        for (int k = 0; k < 4; k++)
                            v3[k].x /= aspect;
                    else
                        for (int k = 0; k < 4; k++)
                            v3[k].y *= aspect;
                }

                lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));
            }

            SafeRelease(pBackBuffer);
            //SafeRelease(pZBuffer);

            if (bKillSprite)
            {
                //KillSprite(iSlot);
            }

            lpDevice->SetShader(0);
            //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
            //lpDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        }
    }

    lpDevice->SetBlendState(false);

    // Reset these to the standard safe mode.
    lpDevice->SetShader(0);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
    //lpDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    //lpDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

// (screen space = -1..1 on both axes; corresponds to UV space)
// uv space = [0..1] on both axes
// "math" space = what the preset authors are used to:
//      upper left = [0,0]
//      bottom right = [1,1]
//      rad == 1 at corners of screen
//      ang == 0 at three o'clock, and increases counter-clockwise (to 6.28).
void CPlugin::UvToMathSpace(float u, float v, float* rad, float* ang)
{
    float px = (u * 2 - 1) * m_fAspectX; // probably 1.0
    float py = (v * 2 - 1) * m_fAspectY; // probably <1

    *rad = sqrtf(px * px + py * py) / sqrtf(m_fAspectX * m_fAspectX + m_fAspectY * m_fAspectY);
    *ang = atan2f(py, px);
    if (*ang < 0)
        *ang += 6.2831853071796f;
}

void CPlugin::RestoreShaderParams()
{
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    for (int i = 0; i < 2; i++)
        lpDevice->SetSamplerState(i, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);

    for (int i = 0; i < 16; i++)
        lpDevice->SetTexture(i, NULL);

    lpDevice->SetVertexShader(NULL, NULL);
    //lpDevice->SetVertexDeclaration(NULL); // DirectX debug runtime complains heavily about this
    lpDevice->SetPixelShader(NULL, NULL);
}

void CPlugin::ApplyShaderParams(CShaderParams* p, CConstantTable* pCT, CState* pState)
{
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    //if (p->texbind_vs >= 0) lpDevice->SetTexture(p->texbind_vs, m_lpVS[0]);
    //if (p->texbind_noise >= 0) lpDevice->SetTexture(p->texbind_noise, m_pTexNoise);

    // Bind textures.
    for (int i = 0; i < sizeof(p->m_texture_bindings) / sizeof(p->m_texture_bindings[0]); i++)
    {
        if (p->m_texcode[i] == TEX_VS)
            lpDevice->SetTexture(p->m_texture_bindings[i].bindPoint, m_lpVS[0]);
        else
            lpDevice->SetTexture(p->m_texture_bindings[i].texptr ? p->m_texture_bindings[i].bindPoint : i, p->m_texture_bindings[i].texptr);

        // Also set up sampler stage, if anything is bound here...
        if (p->m_texcode[i] == TEX_VS || p->m_texture_bindings[i].texptr)
        {
            bool bAniso = false; // TODO: DirectX 11 anisotropy
            D3D11_FILTER HQFilter = bAniso ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            D3D11_TEXTURE_ADDRESS_MODE wrap = p->m_texture_bindings[i].bWrap ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
            D3D11_FILTER filter = p->m_texture_bindings[i].bBilinear ? HQFilter : D3D11_FILTER_MIN_MAG_MIP_POINT;
            lpDevice->SetSamplerState(i, filter, wrap);
        }

        // Finally, if it was a blur texture, note that.
        if (p->m_texcode[i] >= TEX_BLUR1 && p->m_texcode[i] <= TEX_BLUR_LAST)
            m_nHighestBlurTexUsedThisFrame = std::max(m_nHighestBlurTexUsedThisFrame, ((int)p->m_texcode[i] - (int)TEX_BLUR1) + 1);
    }

    // Bind "texsize_XYZ" parameters.
    size_t N = p->texsize_params.size();
    for (unsigned int i = 0; i < N; i++)
    {
        TexSizeParamInfo* q = &(p->texsize_params[i]);
        XMFLOAT4 v4((float)q->w, (float)q->h, 1.0f / q->w, 1.0f / q->h);
        pCT->SetVector(q->texsize_param, &v4);
    }

    float time_since_preset_start = GetTime() - pState->GetPresetStartTime();
    float time_since_preset_start_wrapped = time_since_preset_start - (int)(time_since_preset_start / 10000) * 10000;
    float time = GetTime() - m_fStartTime;
    float progress = (GetTime() - m_fPresetStartTime) / (m_fNextPresetTime - m_fPresetStartTime);
    float mip_x = logf((float)GetWidth()) / logf(2.0f);
    float mip_y = logf((float)GetWidth()) / logf(2.0f);
    float mip_avg = 0.5f * (mip_x + mip_y);
    float aspect_x = 1;
    float aspect_y = 1;
    if (GetWidth() > GetHeight())
        aspect_y = GetHeight() / (float)GetWidth();
    else
        aspect_x = GetWidth() / (float)GetHeight();

    float blur_min[3], blur_max[3];
    GetSafeBlurMinMax(pState, blur_min, blur_max);

    // Bind `float4`s.
    // clang-format off
    if (p->rand_frame) pCT->SetVector(p->rand_frame, &m_rand_frame);
    if (p->rand_preset) pCT->SetVector(p->rand_preset, &pState->m_rand_preset);
    LPCSTR* h = p->const_handles;
    if (h[0]) { XMFLOAT4 v4(aspect_x, aspect_y, 1.0f / aspect_x, 1.0f / aspect_y); pCT->SetVector(h[0], &v4); }
    if (h[1]) { XMFLOAT4 v4(0, 0, 0, 0); pCT->SetVector(h[1], &v4); }
    if (h[2]) { XMFLOAT4 v4(time_since_preset_start_wrapped, GetFps(), (float)GetFrame(), progress); pCT->SetVector(h[2], &v4); }
    if (h[3]) { XMFLOAT4 v4(mdsound.imm_rel[0], mdsound.imm_rel[1], mdsound.imm_rel[2], 0.3333f * (mdsound.imm_rel[0], mdsound.imm_rel[1], mdsound.imm_rel[2])); pCT->SetVector(h[3], &v4); }
    if (h[4]) { XMFLOAT4 v4(mdsound.avg_rel[0], mdsound.avg_rel[1], mdsound.avg_rel[2], 0.3333f * (mdsound.avg_rel[0], mdsound.avg_rel[1], mdsound.avg_rel[2])); pCT->SetVector(h[4], &v4); }
    if (h[5]) { XMFLOAT4 v4(blur_max[0] - blur_min[0], blur_min[0], blur_max[1] - blur_min[1], blur_min[1]); pCT->SetVector(h[5], &v4); }
    if (h[6]) { XMFLOAT4 v4(blur_max[2] - blur_min[2], blur_min[2], blur_min[0], blur_max[0]); pCT->SetVector(h[6], &v4); }
    if (h[7]) { XMFLOAT4 v4((float)m_nTexSizeX, (float)m_nTexSizeY, 1.0f / (float)m_nTexSizeX, 1.0f / (float)m_nTexSizeY); pCT->SetVector(h[7], &v4); }
    if (h[8]) { XMFLOAT4 v4(0.5f + 0.5f * cosf(time * 0.329f + 1.2f), 0.5f + 0.5f * cosf(time * 1.293f + 3.9f), 0.5f + 0.5f * cosf(time * 5.070f + 2.5f), 0.5f + 0.5f * cosf(time * 20.051f + 5.4f)); pCT->SetVector(h[8], &v4); }
    if (h[9]) { XMFLOAT4 v4(0.5f + 0.5f * sinf(time * 0.329f + 1.2f), 0.5f + 0.5f * sinf(time * 1.293f + 3.9f), 0.5f + 0.5f * sinf(time * 5.070f + 2.5f), 0.5f + 0.5f * sinf(time * 20.051f + 5.4f)); pCT->SetVector(h[9], &v4); }
    if (h[10]) { XMFLOAT4 v4(0.5f + 0.5f * cosf(time * 0.0050f + 2.7f), 0.5f + 0.5f * cosf(time * 0.0085f + 5.3f), 0.5f + 0.5f * cosf(time * 0.0133f + 4.5f), 0.5f + 0.5f * cosf(time * 0.0217f + 3.8f)); pCT->SetVector(h[10], &v4); }
    if (h[11]) { XMFLOAT4 v4(0.5f + 0.5f * sinf(time * 0.0050f + 2.7f), 0.5f + 0.5f * sinf(time * 0.0085f + 5.3f), 0.5f + 0.5f * sinf(time * 0.0133f + 4.5f), 0.5f + 0.5f * sinf(time * 0.0217f + 3.8f)); pCT->SetVector(h[11], &v4); }
    if (h[12]) { XMFLOAT4 v4(mip_x, mip_y, mip_avg, 0); pCT->SetVector(h[12], &v4); }
    if (h[13]) { XMFLOAT4 v4(blur_min[1], blur_max[1], blur_min[2], blur_max[2]); pCT->SetVector(h[13], &v4); }
    // clang-format on

    // Write q variables.
    int num_q_float4s = sizeof(p->q_const_handles) / sizeof(p->q_const_handles[0]);
    for (int i = 0; i < num_q_float4s; i++)
    {
        if (p->q_const_handles[i])
        {
            XMFLOAT4 v4((float)*pState->var_pf_q[i * 4 + 0], (float)*pState->var_pf_q[i * 4 + 1], (float)*pState->var_pf_q[i * 4 + 2], (float)*pState->var_pf_q[i * 4 + 3]);
            pCT->SetVector(p->q_const_handles[i], &v4);
        }
    }

    // Write matrices.
    for (int i = 0; i < 20; i++)
    {
        if (p->rot_mat[i])
        {
            XMMATRIX mx, my, mz, mxlate, temp;

            mx = XMMatrixRotationX(pState->m_rot_base[i].x + pState->m_rot_speed[i].x * time);
            my = XMMatrixRotationY(pState->m_rot_base[i].y + pState->m_rot_speed[i].y * time);
            mz = XMMatrixRotationZ(pState->m_rot_base[i].z + pState->m_rot_speed[i].z * time);
            mxlate = XMMatrixTranslation(pState->m_xlate[i].x, pState->m_xlate[i].y, pState->m_xlate[i].z);

            temp = XMMatrixMultiply(mx, mxlate);
            temp = XMMatrixMultiply(temp, mz);
            temp = XMMatrixMultiply(temp, my);

            pCT->SetMatrix(p->rot_mat[i], &temp);
        }
    }
    // The last 4 are totally random, each frame.
    for (int i = 20; i < 24; i++)
    {
        if (p->rot_mat[i])
        {
            XMMATRIX mx, my, mz, mxlate, temp;

            mx = XMMatrixRotationX(FRAND * 6.28f);
            my = XMMatrixRotationY(FRAND * 6.28f);
            mz = XMMatrixRotationZ(FRAND * 6.28f);
            mxlate = XMMatrixTranslation(FRAND, FRAND, FRAND);

            temp = XMMatrixMultiply(mx, mxlate);
            temp = XMMatrixMultiply(temp, mz);
            temp = XMMatrixMultiply(temp, my);

            pCT->SetMatrix(p->rot_mat[i], &temp);
        }
    }
}

// Note: Draws the whole screen! (one big quad)
void CPlugin::ShowToUser_NoShaders() //int bRedraw, int nPassOverride)
{
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, m_lpVS[1]);
    lpDevice->SetVertexShader(NULL, NULL);
    lpDevice->SetPixelShader(NULL, NULL);
    //lpDevice->SetFVF(SPRITEVERTEX_FORMAT);

    // Stages 0 and 1 always just use bilinear filtering.
    lpDevice->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
    //lpDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    //lpDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    //lpDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

    // Note: This texture stage state setup works for 0 or 1 texture.
    //       If a texture is set, it will be modulated with the current diffuse color.
    //       If a texture is not set, it will just use the current diffuse color.
    lpDevice->SetShader(0);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    //lpDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    //lpDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    //lpDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    //lpDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    float fZoom = 1.0f;
    SPRITEVERTEX v3[4];
    ZeroMemory(v3, sizeof(SPRITEVERTEX) * 4);

    // extend the poly we draw by 1 pixel around the viewable image area,
    // in case the video card wraps u/v coords with a +0.5-texel offset
    // (otherwise, a 1-pixel-wide line of the image would wrap at the top and left edges).
    float fOnePlusInvWidth = 1.0f + 1.0f / static_cast<float>(std::max(1, GetWidth()));
    float fOnePlusInvHeight = 1.0f + 1.0f / static_cast<float>(std::max(1, GetHeight()));
    v3[0].x = -fOnePlusInvWidth;
    v3[1].x = fOnePlusInvWidth;
    v3[2].x = -fOnePlusInvWidth;
    v3[3].x = fOnePlusInvWidth;
    v3[0].y = fOnePlusInvHeight;
    v3[1].y = fOnePlusInvHeight;
    v3[2].y = -fOnePlusInvHeight;
    v3[3].y = -fOnePlusInvHeight;

    float aspect = GetWidth() / static_cast<float>(GetHeight() * m_fInvAspectY /* * 4.0f / 3.0f */);
    float x_aspect_mult = 1.0f;
    float y_aspect_mult = 1.0f;

    if (aspect > 1)
        y_aspect_mult = aspect;
    else
        x_aspect_mult = 1.0f / aspect;

    for (int n = 0; n < 4; n++)
    {
        v3[n].x *= x_aspect_mult;
        v3[n].y *= y_aspect_mult;
    }

    {
        float shade[4][3] = {
            {1.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, 1.0f}
        }; // for each vertex, then each comp.

        float fShaderAmount = m_pState->m_fShader.eval(GetTime());

        if (fShaderAmount > 0.001f)
        {
            for (int i = 0; i < 4; i++)
            {
                shade[i][0] = 0.6f + 0.3f * sinf(GetTime() * 30.0f * 0.0143f + 3 + i * 21 + m_fRandStart[3]);
                shade[i][1] = 0.6f + 0.3f * sinf(GetTime() * 30.0f * 0.0107f + 1 + i * 13 + m_fRandStart[1]);
                shade[i][2] = 0.6f + 0.3f * sinf(GetTime() * 30.0f * 0.0129f + 6 + i * 9 + m_fRandStart[2]);
                float max = ((shade[i][0] > shade[i][1]) ? shade[i][0] : shade[i][1]);
                if (shade[i][2] > max)
                    max = shade[i][2];
                for (int k = 0; k < 3; k++)
                {
                    shade[i][k] /= max;
                    shade[i][k] = 0.5f + 0.5f * shade[i][k];
                }
                for (int k = 0; k < 3; k++)
                {
                    shade[i][k] = shade[i][k] * (fShaderAmount) + 1.0f * (1.0f - fShaderAmount);
                }
                v3[i].r = COLOR_NORM(shade[i][0]);
                v3[i].g = COLOR_NORM(shade[i][1]);
                v3[i].b = COLOR_NORM(shade[i][2]);
                v3[i].a = 1.0f;
            }
        }

        float fVideoEchoZoom = (float)(*m_pState->var_pf_echo_zoom); //m_pState->m_fVideoEchoZoom.eval(GetTime());
        float fVideoEchoAlpha = (float)(*m_pState->var_pf_echo_alpha); //m_pState->m_fVideoEchoAlpha.eval(GetTime());
        int nVideoEchoOrientation = (int)(*m_pState->var_pf_echo_orient) % 4; //m_pState->m_nVideoEchoOrientation;
        float fGammaAdj = (float)(*m_pState->var_pf_gamma); //m_pState->m_fGammaAdj.eval(GetTime());

        if (m_pState->m_bBlending &&
            m_pState->m_fVideoEchoAlpha.eval(GetTime()) > 0.01f &&
            m_pState->m_fVideoEchoAlphaOld > 0.01f &&
            m_pState->m_nVideoEchoOrientation != m_pState->m_nVideoEchoOrientationOld)
        {
            if (m_pState->m_fBlendProgress < m_fSnapPoint)
            {
                nVideoEchoOrientation = m_pState->m_nVideoEchoOrientationOld;
                fVideoEchoAlpha *= 1.0f - 2.0f * CosineInterp(m_pState->m_fBlendProgress);
            }
            else
            {
                fVideoEchoAlpha *= 2.0f * CosineInterp(m_pState->m_fBlendProgress) - 1.0f;
            }
        }

        if (fVideoEchoAlpha > 0.001f)
        {
            // Video echo.
            lpDevice->SetBlendState(true, D3D11_BLEND_ONE, D3D11_BLEND_ZERO);

            for (int i = 0; i < 2; i++)
            {
                fZoom = (i == 0) ? 1.0f : fVideoEchoZoom;

                float temp_lo = 0.5f - 0.5f / fZoom;
                float temp_hi = 0.5f + 0.5f / fZoom;
                v3[0].tu = temp_lo;
                v3[0].tv = temp_hi;
                v3[1].tu = temp_hi;
                v3[1].tv = temp_hi;
                v3[2].tu = temp_lo;
                v3[2].tv = temp_lo;
                v3[3].tu = temp_hi;
                v3[3].tv = temp_lo;

                // Flipping.
                if (i == 1)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        if (nVideoEchoOrientation % 2)
                            v3[j].tu = 1.0f - v3[j].tu;
                        if (nVideoEchoOrientation >= 2)
                            v3[j].tv = 1.0f - v3[j].tv;
                    }
                }

                float mix = (i == 1) ? fVideoEchoAlpha : 1.0f - fVideoEchoAlpha;
                for (int k = 0; k < 4; k++)
                {
                    v3[k].r = COLOR_NORM(mix * shade[k][0]);
                    v3[k].g = COLOR_NORM(mix * shade[k][1]);
                    v3[k].b = COLOR_NORM(mix * shade[k][2]);
                    v3[k].a = 1.0f;
                }

                lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));

                if (i == 0)
                {
                    lpDevice->SetBlendState(true, D3D11_BLEND_ONE, D3D11_BLEND_ONE);
                }

                if (fGammaAdj > 0.001f)
                {
                    // Draw layer 'i' a 2nd (or 3rd, or 4th...) time, additively.
                    int nRedraws = static_cast<int>(fGammaAdj - 0.0001f);
                    float gamma;

                    for (int nRedraw = 0; nRedraw < nRedraws; nRedraw++)
                    {
                        if (nRedraw == nRedraws - 1)
                            gamma = fGammaAdj - static_cast<int>(fGammaAdj - 0.0001f);
                        else
                            gamma = 1.0f;

                        for (int k = 0; k < 4; k++)
                        {
                            v3[k].r = COLOR_NORM(gamma * mix * shade[k][0]);
                            v3[k].g = COLOR_NORM(gamma * mix * shade[k][1]);
                            v3[k].b = COLOR_NORM(gamma * mix * shade[k][2]);
                            v3[k].a = 1.0f;
                        }
                        lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));
                    }
                }
            }
        }
        else
        {
            // No video echo.
            v3[0].tu = 0.0f;
            v3[1].tu = 1.0f;
            v3[2].tu = 0.0f;
            v3[3].tu = 1.0f;
            v3[0].tv = 1.0f;
            v3[1].tv = 1.0f;
            v3[2].tv = 0.0f;
            v3[3].tv = 0.0f;

            lpDevice->SetBlendState(false, D3D11_BLEND_ONE, D3D11_BLEND_ZERO);

            // Draw it iteratively, solid the first time, and additively after that.
            int nPasses = static_cast<int>(fGammaAdj - 0.001f) + 1;
            float gamma;

            for (int nPass = 0; nPass < nPasses; nPass++)
            {
                if (nPass == nPasses - 1)
                    gamma = fGammaAdj - (float)nPass;
                else
                    gamma = 1.0f;

                for (int k = 0; k < 4; k++)
                {
                    v3[k].r = COLOR_NORM(gamma * shade[k][0]);
                    v3[k].g = COLOR_NORM(gamma * shade[k][1]);
                    v3[k].b = COLOR_NORM(gamma * shade[k][2]);
                    v3[k].a = 1.0f;
                }
                lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));

                if (nPass == 0)
                {
                    lpDevice->SetBlendState(true, D3D11_BLEND_ONE, D3D11_BLEND_ONE);
                }
            }
        }

        ZeroMemory(v3, sizeof(SPRITEVERTEX) * 4);
        v3[0].x = -fOnePlusInvWidth;
        v3[1].x = fOnePlusInvWidth;
        v3[2].x = -fOnePlusInvWidth;
        v3[3].x = fOnePlusInvWidth;
        v3[0].y = fOnePlusInvHeight;
        v3[1].y = fOnePlusInvHeight;
        v3[2].y = -fOnePlusInvHeight;
        v3[3].y = -fOnePlusInvHeight;
        for (int i = 0; i < 4; i++)
        {
            v3[i].r = v3[i].g = v3[i].b = v3[i].a = 1.0f;
        }

        if (*m_pState->var_pf_brighten /*&& (GetCaps()->SrcBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR) && (GetCaps()->DestBlendCaps & D3DPBLENDCAPS_DESTCOLOR)*/)
        {
            // Square root filter.
            //lpDevice->SetRenderState(D3DRS_COLORVERTEX, FALSE);
            //lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);

            lpDevice->SetTexture(0, NULL);
            lpDevice->SetVertexColor(true);

            // First, a perfect invert.
            lpDevice->SetBlendState(true, D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND_ZERO);
            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));

            // Then, modulate by self (square it).
            lpDevice->SetBlendState(true, D3D11_BLEND_ZERO, D3D11_BLEND_DEST_COLOR);
            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));

            // Then, another perfect invert.
            lpDevice->SetBlendState(true, D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND_ZERO);
            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));
        }

        if (*m_pState->var_pf_darken /*&& (GetCaps()->DestBlendCaps & D3DPBLENDCAPS_DESTCOLOR)*/)
        {
            // Squaring filter.
            //lpDevice->SetRenderState(D3DRS_COLORVERTEX, FALSE);
            //lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);

            lpDevice->SetTexture(0, NULL);
            lpDevice->SetVertexColor(true);

            lpDevice->SetBlendState(true, D3D11_BLEND_ZERO, D3D11_BLEND_DEST_COLOR);
            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));

            //lpDevice->SetBlendState(true, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ONE);
            //lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));
        }

        if (*m_pState->var_pf_solarize /*&& (GetCaps()->SrcBlendCaps & D3DPBLENDCAPS_DESTCOLOR) && (GetCaps()->DestBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR)*/)
        {
            //lpDevice->SetRenderState(D3DRS_COLORVERTEX, FALSE);
            //lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);

            lpDevice->SetTexture(0, NULL);
            lpDevice->SetVertexColor(true);

            lpDevice->SetBlendState(true, D3D11_BLEND_ZERO, D3D11_BLEND_INV_DEST_COLOR);
            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));

            lpDevice->SetBlendState(true, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ONE);
            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));
        }

        if (*m_pState->var_pf_invert /*&& (GetCaps()->SrcBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR)*/)
        {
            //lpDevice->SetRenderState(D3DRS_COLORVERTEX, FALSE);
            //lpDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);

            lpDevice->SetTexture(0, NULL);
            lpDevice->SetVertexColor(true);

            lpDevice->SetBlendState(true, D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND_ZERO);
            lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, (LPVOID)v3, sizeof(SPRITEVERTEX));
        }

        lpDevice->SetBlendState(false);
    }
}

void CPlugin::ShowToUser_Shaders(int nPass, bool bAlphaBlend, bool bFlipAlpha, bool bCullTiles, bool bFlipCulling) //int bRedraw, int nPassOverride)
{
    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    //lpDevice->SetTexture(0, m_lpVS[1]);
    lpDevice->SetVertexShader(NULL, NULL);
    //lpDevice->SetFVF(MDVERTEX_FORMAT);
    lpDevice->SetBlendState(false);

    //float fZoom = 1.0f;

    float aspect = GetWidth() / static_cast<float>(GetHeight() * m_fInvAspectY /* * 4.0f / 3.0f*/);
    float x_aspect_mult = 1.0f;
    float y_aspect_mult = 1.0f;

    if (aspect > 1)
        y_aspect_mult = aspect;
    else
        x_aspect_mult = 1.0f / aspect;

    // Hue shader.
    float shade[4][3] = {
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f}
    }; // for each vertex, then each comp.

    float fShaderAmount = 1.0f; // since do not know if shader uses it or not! m_pState->m_fShader.eval(GetTime());

    if (fShaderAmount > 0.001f || m_pState->m_bBlending)
    {
        // Pick 4 colors for the 4 corners.
        for (int i = 0; i < 4; i++)
        {
            shade[i][0] = 0.6f + 0.3f * std::sin(GetTime() * 30.0f * 0.0143f + 3 + i * 21 + m_fRandStart[3]);
            shade[i][1] = 0.6f + 0.3f * std::sin(GetTime() * 30.0f * 0.0107f + 1 + i * 13 + m_fRandStart[1]);
            shade[i][2] = 0.6f + 0.3f * std::sin(GetTime() * 30.0f * 0.0129f + 6 + i * 9 + m_fRandStart[2]);
            float max = ((shade[i][0] > shade[i][1]) ? shade[i][0] : shade[i][1]);
            if (shade[i][2] > max) max = shade[i][2];
            for (int k = 0; k < 3; k++)
            {
                shade[i][k] /= max;
                shade[i][k] = 0.5f + 0.5f * shade[i][k];
            }
            // Note: Now pass the raw hue shader colors down; the shader can
            // only use a certain percentage if it wants.
            //for (int k = 0; k < 3; k++)
            //    shade[i][k] = shade[i][k] * (fShaderAmount) + 1.0f * (1.0f - fShaderAmount);
            //m_comp_verts[i].Diffuse = D3DCOLOR_RGBA_01(shade[i][0], shade[i][1], shade[i][2], 1);
        }

        // Interpolate the 4 colors and apply to all the vertices.
        for (int j = 0; j < FCGSY; j++)
        {
            for (int i = 0; i < FCGSX; i++)
            {
                MDVERTEX* p = &m_comp_verts[i + j * FCGSX];
                float x = p->x * 0.5f + 0.5f;
                float y = p->y * 0.5f + 0.5f;

                float col[3] = {1, 1, 1};
                if (fShaderAmount > 0.001f)
                {
                    // clang-format off
                    for (int c = 0; c < 3; c++)
                        col[c] = shade[0][c] * (x) * (y) +
                                 shade[1][c] * (1 - x) * (y) +
                                 shade[2][c] * (x) * (1 - y) +
                                 shade[3][c] * (1 - x) * (1 - y);
                    // clang-format on
                }

                // TODO: Improve interpolation here?
                // TODO: During blend, only send the triangles needed.

                // If blending, also set up the alpha values; pull them from the alphas used for the warped blit.
                double alpha = 1.0;
                if (m_pState->m_bBlending)
                {
                    x *= (m_nGridX + 1);
                    y *= (m_nGridY + 1);
                    x = std::max(std::min(x, static_cast<float>(m_nGridX) - 1.0f), 0.0f);
                    y = std::max(std::min(y, static_cast<float>(m_nGridY) - 1.0f), 0.0f);
                    int nx = static_cast<int>(x);
                    int ny = static_cast<int>(y);
                    double dx = x - nx;
                    double dy = y - ny;
                    // clang-format off
                    double alpha00 = (m_verts[(ny) * (m_nGridX + 1) + (nx)].a * 255);
                    double alpha01 = (m_verts[(ny) * (m_nGridX + 1) + (nx + 1)].a * 255);
                    double alpha10 = (m_verts[(ny + 1) * (m_nGridX + 1) + (nx)].a * 255);
                    double alpha11 = (m_verts[(ny + 1) * (m_nGridX + 1) + (nx + 1)].a * 255);
                    alpha = alpha00 * (1 - dx) * (1 - dy) +
                            alpha01 * (dx) * (1 - dy) +
                            alpha10 * (1 - dx) * (dy) +
                            alpha11 * (dx) * (dy);
                    // clang-format on
                    alpha /= 255.0f;
                    //if (bFlipAlpha)
                    //    alpha = 1 - alpha;
                    //alpha = (m_verts[y * (m_nGridX + 1) + x].Diffuse >> 24) / 255.0f;
                }
                p->r = col[0];
                p->g = col[1];
                p->b = col[2];
                p->a = static_cast<float>(alpha);
            }
        }
    }

    int nAlphaTestValue = 0;
    if (bFlipCulling)
        nAlphaTestValue = 1 - nAlphaTestValue;

    if (bAlphaBlend)
    {
        if (bFlipAlpha)
        {
            lpDevice->SetBlendState(true, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_SRC_ALPHA);
        }
        else
        {
            lpDevice->SetBlendState(true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA);
        }
    }
    else
        lpDevice->SetBlendState(false);

    // Now do the final composite blit, fullscreen;
    // or do it twice, alpha-blending, if blending between two sets of shaders.

    int pass = nPass;
    {
        // PASS 0: draw using *blended per-vertex motion vectors*, but with the OLD comp shader.
        // PASS 1: draw using *blended per-vertex motion vectors*, but with the NEW comp shader.
        PShaderInfo* si = (pass == 0) ? &m_OldShaders.comp : &m_shaders.comp;
        CState* state = (pass == 0) ? m_pOldState : m_pState;

        //lpDevice->SetVertexDeclaration(m_pMyVertDecl);
        lpDevice->SetVertexShader(m_fallbackShaders_vs.comp.ptr, m_fallbackShaders_vs.comp.CT);

        ApplyShaderParams(&(si->params), si->CT, state);
        lpDevice->SetPixelShader(si->ptr, si->CT);

        // Hurl the triangles at the video card.
        // Going to un-index it, to not stress non-performant drivers.
        // Not a big deal - only ~800 polys / 24kb of data.
        // If blending, skip any polygon that is all alpha-blended out.
        // This also respects the `MaxPrimCount` limit of the video card.
        MDVERTEX tempv[1024 * 3];
        int primCount = (FCGSX - 2) * (FCGSY - 2) * 2; // although, some might not be drawn!
        int max_prims_per_batch = std::min(lpDevice->GetMaxPrimitiveCount(), static_cast<UINT>(ARRAYSIZE(tempv) / 3)) - 4;
        int src_idx = 0;
        while (src_idx < primCount * 3)
        {
            int prims_queued = 0;
            int i = 0;
            while (prims_queued < max_prims_per_batch && src_idx < primCount * 3)
            {
                // copy 3 verts
                for (int j = 0; j < 3; j++)
                    tempv[i++] = m_comp_verts[m_comp_indices[src_idx++]];
                if (bCullTiles)
                {
                    DWORD d1 = static_cast<DWORD>(tempv[i - 3].a * 255);
                    DWORD d2 = static_cast<DWORD>(tempv[i - 2].a * 255);
                    DWORD d3 = static_cast<DWORD>(tempv[i - 1].a * 255);
                    bool bIsNeeded;
                    if (nAlphaTestValue)
                        bIsNeeded = ((d1 & d2 & d3) < 255); //(d1 < 255) || (d2 < 255) || (d3 < 255);
                    else
                        bIsNeeded = ((d1 | d2 | d3) > 0); //(d1 > 0) || (d2 > 0) || (d3 > 0);
                    if (!bIsNeeded)
                        i -= 3;
                    else
                        prims_queued++;
                }
                else
                    prims_queued++;
            }
            if (prims_queued > 0)
                lpDevice->DrawPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, prims_queued, tempv, sizeof(MDVERTEX));
        }
    }

    lpDevice->SetBlendState(false);

    RestoreShaderParams();
}

void CPlugin::ShowSongTitleAnim(int w, int h, float fProgress)
{
    if (!m_lpDDSTitle)
        return;

    D3D11Shim* lpDevice = GetDevice();
    if (!lpDevice)
        return;

    lpDevice->SetTexture(0, m_lpDDSTitle);
    lpDevice->SetVertexShader(NULL, NULL);
    //lpDevice->SetFVF(SPRITEVERTEX_FORMAT);

    lpDevice->SetBlendState(true, D3D11_BLEND_ONE, D3D11_BLEND_ONE);

    SPRITEVERTEX v3[128];
    ZeroMemory(v3, sizeof(SPRITEVERTEX) * 128);

    if (m_supertext.bIsSongTitle)
    {
        m_superTitle->CreateWindowSizeDependentResources(w, h);
        m_superTitle->SetTextFont(m_supertext.szText, m_supertext.nFontFace, static_cast<float>(m_supertext.nFontSizeUsed));
        m_superTitle->OnRender();
#if 0
        // Positioning.
        float fSizeX = 50.0f / static_cast<float>(m_supertext.nFontSizeUsed) * std::pow(1.5f, m_supertext.fFontSize - 2.0f);
        float fSizeY = fSizeX * m_nTitleTexSizeY / static_cast<float>(m_nTitleTexSizeX); //* m_nWidth/(float)m_nHeight;

        if (fSizeX > 0.88f)
        {
            fSizeY *= 0.88f / fSizeX;
            fSizeX = 0.88f;
        }

        // FIXME
        if (fProgress < 1.0f) //(w != h) // regular render-to-backbuffer
        {
            //float aspect = w / static_cast<float>(h * 4.0f / 3.0f);
            //fSizeY *= aspect;
        }
        else // final render-to-VS0
        {
            //float aspect = GetWidth()/(float)(GetHeight()*4.0f/3.0f);
            //if (aspect < 1.0f)
            //{
            //    fSizeX *= aspect;
            //    fSizeY *= aspect;
            //}

            //fSizeY *= -1;
        }

        //if (fSizeX > 0.92f) fSizeX = 0.92f;
        //if (fSizeY > 0.92f) fSizeY = 0.92f;
        int i = 0;
        float vert_clip = VERT_CLIP; //1.0f;//0.45f; // warning: visible clipping has been observed at 0.4!
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                v3[i].tu = x / 15.0f;
                v3[i].tv = (y / 7.0f - 0.5f) * vert_clip + 0.5f;
                v3[i].x = (v3[i].tu * 2.0f - 1.0f) * fSizeX;
                v3[i].y = (v3[i].tv * 2.0f - 1.0f) * fSizeY;
                if (fProgress >= 1.0f)
                    v3[i].y += 1.0f / (float)m_nTexSizeY; // this is a pretty hacky guess at getting it to align...
                i++;
            }
        }

        // Warping.
        float ramped_progress = std::max(0.0f, 1 - fProgress * 1.5f);
        float t2 = std::pow(ramped_progress, 1.8f) * 1.3f;
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                i = y * 16 + x;
                v3[i].x += t2 * 0.070f * std::sin(GetTime() * 0.31f + v3[i].x * 0.39f - v3[i].y * 1.94f);
                v3[i].x += t2 * 0.044f * std::sin(GetTime() * 0.81f - v3[i].x * 1.91f + v3[i].y * 0.27f);
                v3[i].x += t2 * 0.061f * std::sin(GetTime() * 1.31f + v3[i].x * 0.61f + v3[i].y * 0.74f);
                v3[i].y += t2 * 0.061f * std::sin(GetTime() * 0.37f + v3[i].x * 1.83f + v3[i].y * 0.69f);
                v3[i].y += t2 * 0.070f * std::sin(GetTime() * 0.67f + v3[i].x * 0.42f - v3[i].y * 1.39f);
                v3[i].y += t2 * 0.087f * std::sin(GetTime() * 1.07f + v3[i].x * 3.55f + v3[i].y * 0.89f);
            }
        }

        // Scale down over time.
        float scale = 1.01f / (std::pow(fProgress, 0.21f) + 0.01f);
        for (i = 0; i < 128; i++)
        {
            v3[i].x *= scale;
            v3[i].y *= scale;
        }
    }
    else
    {
        // Positioning.
        float fSizeX = static_cast<float>(m_nTexSizeX) / 1024.0f * 100.0f / static_cast<float>(m_supertext.nFontSizeUsed) * std::pow(1.033f, m_supertext.fFontSize - 50.0f);
        float fSizeY = fSizeX * m_nTitleTexSizeY / static_cast<float>(m_nTitleTexSizeX);

        // FIXME
        if (fProgress < 1.0f) //(w != h) // regular render-to-backbuffer
        {
            //float aspect = w / static_cast<float>(h * 4.0f / 3.0f);
            //fSizeY *= aspect;
        }
        else // final render-to-VS0
        {
            //float aspect = GetWidth() / static_cast<float>(GetHeight() * 4.0f / 3.0f);
            //if (aspect < 1.0f)
            //{
            //    fSizeX *= aspect;
            //    fSizeY *= aspect;
            //}
            //fSizeY *= -1.0f;
        }

        //if (fSize > 0.92f) fSize = 0.92f;
        int i = 0;
        float vert_clip = VERT_CLIP; //0.67f; // warning: visible clipping has been observed at 0.5 (for very short strings) and even 0.6 (for wingdings)!
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                v3[i].tu = x / 15.0f;
                v3[i].tv = (y / 7.0f - 0.5f) * vert_clip + 0.5f;
                v3[i].x = (v3[i].tu * 2.0f - 1.0f) * fSizeX;
                v3[i].y = (v3[i].tv * 2.0f - 1.0f) * fSizeY;
                if (fProgress >= 1.0f)
                    v3[i].y += 1.0f / static_cast<float>(m_nTexSizeY); // this is a pretty hacky guess at getting it to align...
                i++;
            }
        }

        // Apply "growth" factor and move to user-specified (x,y).
        //if (std::abs(m_supertext.fGrowth - 1.0f) > 0.001f)
        {
            float t = 1.0f * (1.0f - fProgress) + (fProgress * m_supertext.fGrowth);
            float dx = m_supertext.fX * 2.0f - 1.0f;
            float dy = m_supertext.fY * 2.0f - 1.0f;
            if (w != h) // regular render-to-backbuffer
            {
                float aspect = w / static_cast<float>(h * 4.0f / 3.0f);
                if (aspect < 1)
                    dx /= aspect;
                else
                    dy *= aspect;
            }

            for (i = 0; i < 128; i++)
            {
                // Note: (x,y) are in (-1,1) range, but `m_supertext.f{X|Y}` are in (0..1) range.
                v3[i].x = (v3[i].x) * t + dx;
                v3[i].y = (v3[i].y) * t + dy;
            }
        }
    }

    WORD indices[7 * 15 * 6];
    int i = 0;
    for (int y = 0; y < 7; y++)
    {
        for (int x = 0; x < 15; x++)
        {
            indices[i++] = static_cast<WORD>(y * 16 + x);
            indices[i++] = static_cast<WORD>(y * 16 + x + 1);
            indices[i++] = static_cast<WORD>(y * 16 + x + 16);
            indices[i++] = static_cast<WORD>(y * 16 + x + 1);
            indices[i++] = static_cast<WORD>(y * 16 + x + 16);
            indices[i++] = static_cast<WORD>(y * 16 + x + 17);
        }
    }

    // Final flip on Y.
    //for (i = 0; i < 128; i++)
    //    v3[i].y *= -1.0f;
    for (i = 0; i < 128; i++)
        //v3[i].y /= ASPECT_Y;
        v3[i].y *= m_fInvAspectY;

    for (int it = 0; it < 2; it++)
    {
        // Colors.
        {
            float t;

            if (m_supertext.bIsSongTitle)
                t = powf(fProgress, 0.3f) * 1.0f;
            else
                t = CosineInterp(std::min(1.0f, (fProgress / m_supertext.fFadeTime)));

            if (it == 0)
                v3[0].r = v3[0].g = v3[0].b = v3[0].a = t;
            else
            {
                v3[0].r = COLOR_NORM(t * m_supertext.nColorR / 255.0f);
                v3[0].g = COLOR_NORM(t * m_supertext.nColorG / 255.0f);
                v3[0].b = COLOR_NORM(t * m_supertext.nColorB / 255.0f);
                v3[0].a = t;
            }

            for (i = 1; i < 128; i++)
                COPY_COLOR(v3[i], v3[0]);
        }

        // Nudge down and right for shadow, up and left for solid text.
        float offset_x = 0, offset_y = 0;
        switch (it)
        {
            case 0:
                offset_x = 2.0f / static_cast<float>(m_nTitleTexSizeX);
                offset_y = 2.0f / static_cast<float>(m_nTitleTexSizeY);
                break;
            case 1:
                offset_x = -4.0f / static_cast<float>(m_nTitleTexSizeX);
                offset_y = -4.0f / static_cast<float>(m_nTitleTexSizeY);
                break;
        }

        for (i = 0; i < 128; i++)
        {
            v3[i].x += offset_x;
            v3[i].y += offset_y;
        }

        if (it == 0)
        {
            lpDevice->SetBlendState(true, D3D11_BLEND_ZERO, D3D11_BLEND_INV_SRC_COLOR);
        }
        else
        {
            lpDevice->SetBlendState(true, D3D11_BLEND_ONE, D3D11_BLEND_ONE);
        }

        lpDevice->DrawIndexedPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 0, 128, 15 * 7 * 6 / 3, indices, v3, sizeof(SPRITEVERTEX));
#else
    }

    lpDevice->SetBlendState(false);
}