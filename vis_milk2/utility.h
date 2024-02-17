/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
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

#ifndef __NULLSOFT_DX_PLUGIN_SHELL_UTILITY_H__
#define __NULLSOFT_DX_PLUGIN_SHELL_UTILITY_H__

#define SafeRelease(x) { if (x) { x->Release(); x = nullptr; } }
#define SafeDelete(x) { if (x) { delete x; x = nullptr; } }
#define IsNullGuid(lpGUID) (((int*)lpGUID)[0] == 0 && ((int*)lpGUID)[1] == 0 && ((int*)lpGUID)[2] == 0 && ((int*)lpGUID)[3] == 0)
#define DlgItemIsChecked(hDlg, nIDDlgItem) ((SendDlgItemMessage(hDlg, nIDDlgItem, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED) ? true : false)
#define CosineInterp(x) (0.5f - 0.5f * cosf((x) * 3.1415926535898f))
#define InvCosineInterp(x) (acosf(1.0f - 2.0f * (x)) / 3.1415926535898f)
float PowCosineInterp(float x, float pow);
float AdjustRateToFPS(float per_frame_decay_rate_at_fps1, float fps1, float actual_fps);

// `GetPrivateProfileInt()` is part of the Win32 API.
#define GetPrivateProfileBoolW(w, x, y, z) (static_cast<bool>(GetPrivateProfileInt(w, x, y, z) != 0))
float GetPrivateProfileFloatW(const wchar_t* szSectionName, const wchar_t* szKeyName, const float fDefault, const wchar_t* szIniFile);
bool WritePrivateProfileIntW(int d, const wchar_t* szKeyName, const wchar_t* szIniFile, const wchar_t* szSectionName);
bool WritePrivateProfileFloatW(float f, const wchar_t* szKeyName, const wchar_t* szIniFile, const wchar_t* szSectionName);

extern _locale_t g_use_C_locale;

void SetScrollLock(int bNewState, bool bPreventHandling);
void RemoveExtension(wchar_t* str);
void RemoveSingleAmpersands(wchar_t* str);
void TextToGuidA(char* str, GUID* pGUID);
void TextToGuidW(wchar_t* str, GUID* pGUID);
void TextToLuidA(char* str, LUID* pLUID);
void GuidToTextA(GUID* pGUID, char* str, int nStrLen);
void GuidToTextW(GUID* pGUID, wchar_t* str, int nStrLen);
void LuidToTextA(LUID* pLUID, char* str, int nStrLen);

#ifdef NS_EEL2
void NSEEL_VM_resetvars(void* ctx);
#endif

LPWSTR GetStringW(HINSTANCE hinst, HINSTANCE owner, UINT uID, LPWSTR str = NULL, int maxlen = 0);

void* GetTextResource(UINT id, int no_fallback);

#ifdef _DEBUG
void OutputDebugMessage(const char* szStartText, const HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam);
#endif

#endif