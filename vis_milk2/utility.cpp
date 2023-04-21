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


#include "pch.h"
#include "utility.h"

float PowCosineInterp(float x, float pow)
{
    // input (x) & output should be in range 0..1.
    // pow > 0: tends to push things toward 0 and 1.
    // pow < 0: tends to push things toward 0.5.

    if (x < 0)
        return 0;
    if (x > 1)
        return 1;

    int bneg = (pow < 0) ? 1 : 0;
    if (bneg)
        pow = -pow;

    if (pow > 1000)
        pow = 1000;

    int its = (int)pow;
    for (int i = 0; i < its; i++)
    {
        if (bneg)
            x = InvCosineInterp(x);
        else
            x = CosineInterp(x);
    }
    float x2 = (bneg) ? InvCosineInterp(x) : CosineInterp(x);
    float dx = pow - its;
    return ((1 - dx) * x + (dx) * x2);
}

float AdjustRateToFPS(float per_frame_decay_rate_at_fps1, float fps1, float actual_fps)
{
    // Returns the equivalent per-frame decay rate at actual_fps

    // Basically, do all your testing at fps1 and get a good decay rate;
    // then, in the real application, adjust that rate by the actual fps each time you use it.

    float per_second_decay_rate_at_fps1 = powf(per_frame_decay_rate_at_fps1, fps1);
    float per_frame_decay_rate_at_fps2 = powf(per_second_decay_rate_at_fps1, 1.0f / actual_fps);

    return per_frame_decay_rate_at_fps2;
}

float GetPrivateProfileFloatW(const wchar_t* szSectionName, const wchar_t* szKeyName, const float fDefault, const wchar_t* szIniFile)
{
    wchar_t string[64];
    wchar_t szDefault[64];
    float ret = fDefault;

    _swprintf_s_l(szDefault, ARRAYSIZE(szDefault), L"%f", g_use_C_locale, fDefault);

    if (GetPrivateProfileString(szSectionName, szKeyName, szDefault, string, 64, szIniFile) > 0)
    {
        _swscanf_s_l(string, L"%f", g_use_C_locale, &ret);
    }
    return ret;
}

bool WritePrivateProfileFloatW(float f, const wchar_t* szKeyName, const wchar_t* szIniFile, const wchar_t* szSectionName)
{
    wchar_t szValue[32];
    _swprintf_s_l(szValue, ARRAYSIZE(szValue), L"%f", g_use_C_locale, f);
    return (WritePrivateProfileString(szSectionName, szKeyName, szValue, szIniFile) != 0);
}

bool WritePrivateProfileIntW(int d, const wchar_t* szKeyName, const wchar_t* szIniFile, const wchar_t* szSectionName)
{
    wchar_t szValue[32];
    swprintf_s(szValue, L"%d", d);
    return (WritePrivateProfileString(szSectionName, szKeyName, szValue, szIniFile) != 0);
}

void SetScrollLock(int bNewState, bool bPreventHandling)
{
#if 0
    if(bPreventHandling) return;

    if (bNewState != (GetKeyState(VK_SCROLL) & 1))
    {
        // Simulate a key press
        keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);

        // Simulate a key release
        keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }
#endif
}

void RemoveExtension(wchar_t* str)
{
    wchar_t* p = wcsrchr(str, L'.');
    if (p)
        *p = 0;
}

static void ShiftDown(wchar_t* str)
{
    while (*str)
    {
        str[0] = str[1];
        str++;
    }
}

void TextToGuidA(char* str, GUID* pGUID)
{
    if (!str || !pGUID)
        return;

    DWORD d[11];

    sscanf_s(str, "%X %X %X %X %X %X %X %X %X %X %X", &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7], &d[8], &d[9], &d[10]);

    pGUID->Data1 = (DWORD)d[0];
    pGUID->Data2 = (WORD)d[1];
    pGUID->Data3 = (WORD)d[2];
    pGUID->Data4[0] = (BYTE)d[3];
    pGUID->Data4[1] = (BYTE)d[4];
    pGUID->Data4[2] = (BYTE)d[5];
    pGUID->Data4[3] = (BYTE)d[6];
    pGUID->Data4[4] = (BYTE)d[7];
    pGUID->Data4[5] = (BYTE)d[8];
    pGUID->Data4[6] = (BYTE)d[9];
    pGUID->Data4[7] = (BYTE)d[10];
}