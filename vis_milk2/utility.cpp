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
    if (bPreventHandling)
        return;

    if (bNewState != (GetKeyState(VK_SCROLL) & 1))
    {
        // Simulate a key press
        keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);

        // Simulate a key release
        keybd_event(VK_SCROLL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }
#else
	UNREFERENCED_PARAMETER(bNewState);
	UNREFERENCED_PARAMETER(bPreventHandling);
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

void RemoveSingleAmpersands(wchar_t* str)
{
    while (*str)
    {
        if (str[0] == L'&')
        {
            if (str[1] == L'&') // two in a row: replace with single ampersand, move on
                str++;

            ShiftDown(str);
        }
        else
            str = CharNext(str);
    }
}

void TextToGuidA(char* str, GUID* pGUID)
{
    if (!str || !pGUID)
        return;

    DWORD d[11]{};

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

void TextToGuidW(wchar_t* str, GUID* pGUID)
{
    if (!str || !pGUID)
        return;

    DWORD d[11]{};

    swscanf_s(str, L"%X %X %X %X %X %X %X %X %X %X %X", &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6], &d[7], &d[8], &d[9], &d[10]);

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

void TextToLuidA(char* str, LUID* pLUID)
{
    if (!str || !pLUID)
        return;

    DWORD l;
    LONG h;

    sscanf_s(str, "%08X%08X", &h, &l);

    pLUID->LowPart = static_cast<DWORD>(l);
    pLUID->HighPart = static_cast<LONG>(h);
}

void GuidToTextA(GUID* pGUID, char* str, int nStrLen)
{
    // Note: `nStrLen` should be set to sizeof(str).
    if (!str || !nStrLen || !pGUID)
        return;
    str[0] = '\0';

    DWORD d[11]{};
    d[0] = (DWORD)pGUID->Data1;
    d[1] = (DWORD)pGUID->Data2;
    d[2] = (DWORD)pGUID->Data3;
    d[3] = (DWORD)pGUID->Data4[0];
    d[4] = (DWORD)pGUID->Data4[1];
    d[5] = (DWORD)pGUID->Data4[2];
    d[6] = (DWORD)pGUID->Data4[3];
    d[7] = (DWORD)pGUID->Data4[4];
    d[8] = (DWORD)pGUID->Data4[5];
    d[9] = (DWORD)pGUID->Data4[6];
    d[10] = (DWORD)pGUID->Data4[7];

    sprintf_s(str, nStrLen, "%08X %04X %04X %02X %02X %02X %02X %02X %02X %02X %02X", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10]);
}

void GuidToTextW(GUID* pGUID, wchar_t* str, int nStrLen)
{
    // Note: `nStrLen` should be set to sizeof(str).
    if (!str || !nStrLen || !pGUID)
        return;
    str[0] = L'\0';

    DWORD d[11]{};
    d[0] = (DWORD)pGUID->Data1;
    d[1] = (DWORD)pGUID->Data2;
    d[2] = (DWORD)pGUID->Data3;
    d[3] = (DWORD)pGUID->Data4[0];
    d[4] = (DWORD)pGUID->Data4[1];
    d[5] = (DWORD)pGUID->Data4[2];
    d[6] = (DWORD)pGUID->Data4[3];
    d[7] = (DWORD)pGUID->Data4[4];
    d[8] = (DWORD)pGUID->Data4[5];
    d[9] = (DWORD)pGUID->Data4[6];
    d[10] = (DWORD)pGUID->Data4[7];

    swprintf_s(str, nStrLen, L"%08X %04X %04X %02X %02X %02X %02X %02X %02X %02X %02X", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10]);
}

void LuidToTextA(LUID* pLUID, char* str, int nStrLen)
{
    // Note: `nStrLen` should be set to sizeof(str).
    if (!str || !nStrLen || !pLUID)
        return;
    str[0] = '\0';

    DWORD l = pLUID->LowPart;
    LONG h = pLUID->HighPart;

    sprintf_s(str, nStrLen, "%08X%08X", h, l);
}

LPVOID GetTextResource(UINT id, int no_fallback)
{
    LPVOID data = nullptr;
    HINSTANCE hinst = NULL; //WASABI_API_LNG_HINST;
    HRSRC hrsrc = FindResource(hinst, MAKEINTRESOURCE(id), L"TEXT");
    if (!hrsrc && !no_fallback)
        hrsrc = FindResource((hinst = HINST_THISCOMPONENT /*WASABI_API_ORIG_HINST*/), MAKEINTRESOURCE(id), L"TEXT");
    if (hrsrc)
    {
        HGLOBAL hglob = LoadResource(hinst, hrsrc);
        if (hglob)
        {
            data = LockResource(hglob);
            if (data)
                UnlockResource(data);
            FreeResource(hglob);
        }
    }
    return data;
}

LPWSTR GetStringW(HINSTANCE hinst, HINSTANCE owner, UINT uID, LPWSTR str, int maxlen)
{
    static WCHAR buffer[512];
    UNREFERENCED_PARAMETER(hinst);

    if (!str)
    {
        str = buffer;
        maxlen = 512;
    }
    if (str)
    {
        LoadStringW(owner, uID, str, maxlen);
        return str;
    }
    return const_cast<LPWSTR>(L"notfound");
}

// clang-format off
#ifdef _DEBUG
void OutputDebugMessage(const char* szStartText, const HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    // Note: These identifiers were gathered from "WinUser.h" and https://blog.airesoft.co.uk/2009/11/wm_messages/
    // Note: This function does NOT log WM_MOUSEMOVE, WM_NCHITTEST, or WM_SETCURSOR
    //       messages, since they are so frequent.
    if (msg == WM_MOUSEMOVE || msg == WM_NCHITTEST || msg == WM_SETCURSOR)
        return;

    wchar_t buf[64];

    swprintf_s(buf, L"WM_");

    switch (msg)
    {
        case 0x0000: wcscat_s(buf, L"NULL"); break;
        case 0x0001: wcscat_s(buf, L"CREATE"); break;
        case 0x0002: wcscat_s(buf, L"DESTROY"); break;
        case 0x0003: wcscat_s(buf, L"MOVE"); break;
        case 0x0005: wcscat_s(buf, L"SIZE"); break;
        case 0x0006: wcscat_s(buf, L"ACTIVATE"); break;
        case 0x0007: wcscat_s(buf, L"SETFOCUS"); break;
        case 0x0008: wcscat_s(buf, L"KILLFOCUS"); break;
        case 0x000A: wcscat_s(buf, L"ENABLE"); break;
        case 0x000B: wcscat_s(buf, L"SETREDRAW"); break;
        case 0x000C: wcscat_s(buf, L"SETTEXT"); break;
        case 0x000D: wcscat_s(buf, L"GETTEXT"); break;
        case 0x000E: wcscat_s(buf, L"GETTEXTLENGTH"); break;
        case 0x000F: wcscat_s(buf, L"PAINT"); break;
        case 0x0010: wcscat_s(buf, L"CLOSE"); break;
        case 0x0011: wcscat_s(buf, L"QUERYENDSESSION"); break;
        case 0x0012: wcscat_s(buf, L"QUIT"); break;
        case 0x0013: wcscat_s(buf, L"QUERYOPEN"); break;
        case 0x0014: wcscat_s(buf, L"ERASEBKGND"); break;
        case 0x0015: wcscat_s(buf, L"SYSCOLORCHANGE"); break;
        case 0x0016: wcscat_s(buf, L"ENDSESSION"); break;
        case 0x0018: wcscat_s(buf, L"SHOWWINDOW"); break;
        case 0x001A: wcscat_s(buf, L"WININICHANGE"); break; // also SETTINGCHANGE
        case 0x001B: wcscat_s(buf, L"DEVMODECHANGE"); break;
        case 0x001C: wcscat_s(buf, L"ACTIVATEAPP"); break;
        case 0x001D: wcscat_s(buf, L"FONTCHANGE"); break;
        case 0x001E: wcscat_s(buf, L"TIMECHANGE"); break;
        case 0x001F: wcscat_s(buf, L"CANCELMODE"); break;
        case 0x0020: wcscat_s(buf, L"SETCURSOR"); break;
        case 0x0021: wcscat_s(buf, L"MOUSEACTIVATE"); break;
        case 0x0022: wcscat_s(buf, L"CHILDACTIVATE"); break;
        case 0x0023: wcscat_s(buf, L"QUEUESYNC"); break;
        case 0x0024: wcscat_s(buf, L"GETMINMAXINFO"); break;
        case 0x0026: wcscat_s(buf, L"PAINTICON"); break;
        case 0x0027: wcscat_s(buf, L"ICONERASEBKGND"); break;
        case 0x0028: wcscat_s(buf, L"NEXTDLGCTL"); break;
        case 0x002A: wcscat_s(buf, L"SPOOLERSTATUS"); break;
        case 0x002B: wcscat_s(buf, L"DRAWITEM"); break;
        case 0x002C: wcscat_s(buf, L"MEASUREITEM"); break;
        case 0x002D: wcscat_s(buf, L"DELETEITEM"); break;
        case 0x002E: wcscat_s(buf, L"VKEYTOITEM"); break;
        case 0x002F: wcscat_s(buf, L"CHARTOITEM"); break;
        case 0x0030: wcscat_s(buf, L"SETFONT"); break;
        case 0x0031: wcscat_s(buf, L"GETFONT"); break;
        case 0x0032: wcscat_s(buf, L"SETHOTKEY"); break;
        case 0x0033: wcscat_s(buf, L"GETHOTKEY"); break;
        case 0x0037: wcscat_s(buf, L"QUERYDRAGICON"); break;
        case 0x0039: wcscat_s(buf, L"COMPAREITEM"); break;
#if (WINVER >= 0x0500)
        case 0x003D: wcscat_s(buf, L"GETOBJECT"); break;
#endif
        case 0x0041: wcscat_s(buf, L"COMPACTING"); break;
        case 0x0044: wcscat_s(buf, L"COMMNOTIFY"); break; // no longer suported
        case 0x0046: wcscat_s(buf, L"WINDOWPOSCHANGING"); break;
        case 0x0047: wcscat_s(buf, L"WINDOWPOSCHANGED"); break;
        case 0x0048: wcscat_s(buf, L"POWER"); break;
        case 0x004A: wcscat_s(buf, L"COPYDATA"); break;
        case 0x004B: wcscat_s(buf, L"CANCELJOURNAL"); break;
#if (WINVER >= 0x0400)
        case 0x004E: wcscat_s(buf, L"NOTIFY"); break;
        case 0x0050: wcscat_s(buf, L"INPUTLANGCHANGEREQUEST"); break;
        case 0x0051: wcscat_s(buf, L"INPUTLANGCHANGE"); break;
        case 0x0052: wcscat_s(buf, L"TCARD"); break;
        case 0x0053: wcscat_s(buf, L"HELP"); break;
        case 0x0054: wcscat_s(buf, L"USERCHANGED"); break;
        case 0x0055: wcscat_s(buf, L"NOTIFYFORMAT"); break;
        case 0x007B: wcscat_s(buf, L"CONTEXTMENU"); break;
        case 0x007C: wcscat_s(buf, L"STYLECHANGING"); break;
        case 0x007D: wcscat_s(buf, L"STYLECHANGED"); break;
        case 0x007E: wcscat_s(buf, L"DISPLAYCHANGE"); break;
        case 0x007F: wcscat_s(buf, L"GETICON"); break;
        case 0x0080: wcscat_s(buf, L"SETICON"); break;
#endif
        case 0x0081: wcscat_s(buf, L"NCCREATE"); break;
        case 0x0082: wcscat_s(buf, L"NCDESTROY"); break;
        case 0x0083: wcscat_s(buf, L"NCCALCSIZE"); break;
        case 0x0084: wcscat_s(buf, L"NCHITTEST"); break;
        case 0x0085: wcscat_s(buf, L"NCPAINT"); break;
        case 0x0086: wcscat_s(buf, L"NCACTIVATE"); break;
        case 0x0087: wcscat_s(buf, L"GETDLGCODE"); break;
        case 0x0088: wcscat_s(buf, L"SYNCPAINT"); break;
        case 0x0090: wcscat_s(buf, L"UAHDESTROYWINDOW"); break;
        case 0x0091: wcscat_s(buf, L"UAHDRAWMENU"); break;
        case 0x0092: wcscat_s(buf, L"UAHDRAWMENUITEM"); break;
        case 0x0093: wcscat_s(buf, L"UAHINITMENU"); break;
        case 0x0094: wcscat_s(buf, L"UAHMEASUREMENUITEM"); break;
        case 0x0095: wcscat_s(buf, L"UAHNCPAINTMENUPOPUP"); break;
        case 0x0096: wcscat_s(buf, L"UAHUPDATE"); break;
        case 0x00A0: wcscat_s(buf, L"NCMOUSEMOVE"); break;
        case 0x00A1: wcscat_s(buf, L"NCLBUTTONDOWN"); break;
        case 0x00A2: wcscat_s(buf, L"NCLBUTTONUP"); break;
        case 0x00A3: wcscat_s(buf, L"NCLBUTTONDBLCLK"); break;
        case 0x00A4: wcscat_s(buf, L"NCRBUTTONDOWN"); break;
        case 0x00A5: wcscat_s(buf, L"NCRBUTTONUP"); break;
        case 0x00A6: wcscat_s(buf, L"NCRBUTTONDBLCLK"); break;
        case 0x00A7: wcscat_s(buf, L"NCMBUTTONDOWN"); break;
        case 0x00A8: wcscat_s(buf, L"NCMBUTTONUP"); break;
        case 0x00A9: wcscat_s(buf, L"NCMBUTTONDBLCLK"); break;
#if (_WIN32_WINNT >= 0x0500)
        case 0x00AB: wcscat_s(buf, L"NCXBUTTONDOWN"); break;
        case 0x00AC: wcscat_s(buf, L"NCXBUTTONUP"); break;
        case 0x00AD: wcscat_s(buf, L"NCXBUTTONDBLCLK"); break;
#endif
#if (_WIN32_WINNT >= 0x0501)
        case 0x00FE: wcscat_s(buf, L"INPUT_DEVICE_CHANGE"); break;
        case 0x00FF: wcscat_s(buf, L"INPUT"); break;
#endif
        case 0x0100: wcscat_s(buf, L"KEYDOWN"); break; // also KEYFIRST
        case 0x0101: wcscat_s(buf, L"KEYUP"); break;
        case 0x0102: wcscat_s(buf, L"CHAR"); break;
        case 0x0103: wcscat_s(buf, L"DEADCHAR"); break;
        case 0x0104: wcscat_s(buf, L"SYSKEYDOWN"); break;
        case 0x0105: wcscat_s(buf, L"SYSKEYUP"); break;
        case 0x0106: wcscat_s(buf, L"SYSCHAR"); break;
        case 0x0107: wcscat_s(buf, L"SYSDEADCHAR"); break;
#if (_WIN32_WINNT >= 0x0501)
        case 0x0109: wcscat_s(buf, L"KEYLAST"); break; // also UNICHAR
#else
        case 0x0108: wcscat_s(buf, L"KEYLAST"); break;
#endif
#if (WINVER >= 0x0400)
        case 0x010D: wcscat_s(buf, L"IME_STARTCOMPOSITION"); break;
        case 0x010E: wcscat_s(buf, L"IME_ENDCOMPOSITION"); break;
        case 0x010F: wcscat_s(buf, L"IME_COMPOSITION"); break; // also IME_KEYLAST
#endif
        case 0x0110: wcscat_s(buf, L"INITDIALOG"); break;
        case 0x0111: wcscat_s(buf, L"COMMAND"); break;
        case 0x0112: wcscat_s(buf, L"SYSCOMMAND"); break;
        case 0x0113: wcscat_s(buf, L"TIMER"); break;
        case 0x0114: wcscat_s(buf, L"HSCROLL"); break;
        case 0x0115: wcscat_s(buf, L"VSCROLL"); break;
        case 0x0116: wcscat_s(buf, L"INITMENU"); break;
        case 0x0117: wcscat_s(buf, L"INITMENUPOPUP"); break;
#if(WINVER >= 0x0601)
        case 0x0119: wcscat_s(buf, L"GESTURE"); break;
        case 0x011A: wcscat_s(buf, L"GESTURENOTIFY"); break;
#endif
        case 0x011F: wcscat_s(buf, L"MENUSELECT"); break;
        case 0x0120: wcscat_s(buf, L"MENUCHAR"); break;
        case 0x0121: wcscat_s(buf, L"ENTERIDLE"); break;
#if (WINVER >= 0x0500)
        case 0x0122: wcscat_s(buf, L"MENURBUTTONUP"); break;
        case 0x0123: wcscat_s(buf, L"MENUDRAG"); break;
        case 0x0124: wcscat_s(buf, L"MENUGETOBJECT"); break;
        case 0x0125: wcscat_s(buf, L"UNINITMENUPOPUP"); break;
        case 0x0126: wcscat_s(buf, L"MENUCOMMAND"); break;
#if (_WIN32_WINNT >= 0x0500)
        case 0x0127: wcscat_s(buf, L"CHANGEUISTATE"); break;
        case 0x0128: wcscat_s(buf, L"UPDATEUISTATE"); break;
        case 0x0129: wcscat_s(buf, L"QUERYUISTATE"); break;
#endif
#endif
        case 0x0132: wcscat_s(buf, L"CTLCOLORMSGBOX"); break;
        case 0x0133: wcscat_s(buf, L"CTLCOLOREDIT"); break;
        case 0x0134: wcscat_s(buf, L"CTLCOLORLISTBOX"); break;
        case 0x0135: wcscat_s(buf, L"CTLCOLORBTN"); break;
        case 0x0136: wcscat_s(buf, L"CTLCOLORDLG"); break;
        case 0x0137: wcscat_s(buf, L"CTLCOLORSCROLLBAR"); break;
        case 0x0138: wcscat_s(buf, L"CTLCOLORSTATIC"); break;
        //case 0x0200: wcscat_s(buf, L"MOUSEFIRST"); break;
        case 0x0200: wcscat_s(buf, L"MOUSEMOVE"); break;
        case 0x0201: wcscat_s(buf, L"LBUTTONDOWN"); break;
        case 0x0202: wcscat_s(buf, L"LBUTTONUP"); break;
        case 0x0203: wcscat_s(buf, L"LBUTTONDBLCLK"); break;
        case 0x0204: wcscat_s(buf, L"RBUTTONDOWN"); break;
        case 0x0205: wcscat_s(buf, L"RBUTTONUP"); break;
        case 0x0206: wcscat_s(buf, L"RBUTTONDBLCLK"); break;
        case 0x0207: wcscat_s(buf, L"MBUTTONDOWN"); break;
        case 0x0208: wcscat_s(buf, L"MBUTTONUP"); break;
        case 0x0209: wcscat_s(buf, L"MBUTTONDBLCLK"); break;
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
        case 0x020A: wcscat_s(buf, L"MOUSEWHEEL"); break;
#endif
#if (_WIN32_WINNT >= 0x0500)
        case 0x020B: wcscat_s(buf, L"XBUTTONDOWN"); break;
        case 0x020C: wcscat_s(buf, L"XBUTTONUP"); break;
        case 0x020D: wcscat_s(buf, L"XBUTTONDBLCLK"); break;
#endif
#if (_WIN32_WINNT >= 0x0600)
        case 0x020E: wcscat_s(buf, L"MOUSELAST"); break; // also MOUSEHWHEEL
#elif (_WIN32_WINNT >= 0x0500)
        case 0x020D: wcscat_s(buf, L"MOUSELAST"); break;
#elif (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
        case 0x020A: wcscat_s(buf, L"MOUSELAST"); break;
#else
        case 0x0209: wcscat_s(buf, L"MOUSELAST"); break;
#endif
        case 0x0210: wcscat_s(buf, L"PARENTNOTIFY"); break;
        case 0x0211: wcscat_s(buf, L"ENTERMENULOOP"); break;
        case 0x0212: wcscat_s(buf, L"EXITMENULOOP"); break;
#if (WINVER >= 0x0400)
        case 0x0213: wcscat_s(buf, L"NEXTMENU"); break;
        case 0x0214: wcscat_s(buf, L"SIZING"); break;
        case 0x0215: wcscat_s(buf, L"CAPTURECHANGED"); break;
        case 0x0216: wcscat_s(buf, L"MOVING"); break;
        case 0x0218: wcscat_s(buf, L"POWERBROADCAST"); break;
        case 0x0219: wcscat_s(buf, L"DEVICECHANGE"); break;
#endif
        /*
        case 0x0220: wcscat_s(buf, L"MDICREATE"); break;
        case 0x0221: wcscat_s(buf, L"MDIDESTROY"); break;
        case 0x0222: wcscat_s(buf, L"MDIACTIVATE"); break;
        case 0x0223: wcscat_s(buf, L"MDIRESTORE"); break;
        case 0x0224: wcscat_s(buf, L"MDINEXT"); break;
        case 0x0225: wcscat_s(buf, L"MDIMAXIMIZE"); break;
        case 0x0226: wcscat_s(buf, L"MDITILE"); break;
        case 0x0227: wcscat_s(buf, L"MDICASCADE"); break;
        case 0x0228: wcscat_s(buf, L"MDIICONARRANGE"); break;
        case 0x0229: wcscat_s(buf, L"MDIGETACTIVE"); break;
        */
        case 0x0230: wcscat_s(buf, L"MDISETMENU"); break;
        case 0x0231: wcscat_s(buf, L"ENTERSIZEMOVE"); break;
        case 0x0232: wcscat_s(buf, L"EXITSIZEMOVE"); break;
        case 0x0233: wcscat_s(buf, L"DROPFILES"); break;
        case 0x0234: wcscat_s(buf, L"MDIREFRESHMENU"); break;
#if(WINVER >= 0x0602)
        case 0x0238: wcscat_s(buf, L"POINTERDEVICECHANGE"); break;
        case 0x0239: wcscat_s(buf, L"POINTERDEVICEINRANGE"); break;
        case 0x023A: wcscat_s(buf, L"POINTERDEVICEOUTOFRANGE"); break;
#endif
#if(WINVER >= 0x0601)
        case 0x0240: wcscat_s(buf, L"TOUCH"); break;
#endif
#if(WINVER >= 0x0602)
        case 0x0241: wcscat_s(buf, L"NCPOINTERUPDATE"); break;
        case 0x0242: wcscat_s(buf, L"NCPOINTERDOWN"); break;
        case 0x0243: wcscat_s(buf, L"NCPOINTERUP"); break;
        case 0x0245: wcscat_s(buf, L"POINTERUPDATE"); break;
        case 0x0246: wcscat_s(buf, L"POINTERDOWN"); break;
        case 0x0247: wcscat_s(buf, L"POINTERUP"); break;
        case 0x0249: wcscat_s(buf, L"POINTERENTER"); break;
        case 0x024A: wcscat_s(buf, L"POINTERLEAVE"); break;
        case 0x024B: wcscat_s(buf, L"POINTERACTIVATE"); break;
        case 0x024C: wcscat_s(buf, L"POINTERCAPTURECHANGED"); break;
        case 0x024D: wcscat_s(buf, L"TOUCHHITTESTING"); break;
        case 0x024E: wcscat_s(buf, L"POINTERWHEEL"); break;
        case 0x024F: wcscat_s(buf, L"POINTERHWHEEL"); break;
        case 0x0250: wcscat_s(buf, L"POINTERHITTEST"); break; // really DM_POINTERHITTEST
        case 0x0251: wcscat_s(buf, L"POINTERROUTEDTO"); break;
        case 0x0252: wcscat_s(buf, L"POINTERROUTEDAWAY"); break;
        case 0x0253: wcscat_s(buf, L"POINTERROUTEDRELEASED"); break;
#endif
#if(WINVER >= 0x0400)
        case 0x0281: wcscat_s(buf, L"IME_SETCONTEXT"); break;
        case 0x0282: wcscat_s(buf, L"IME_NOTIFY"); break;
        case 0x0283: wcscat_s(buf, L"IME_CONTROL"); break;
        case 0x0284: wcscat_s(buf, L"IME_COMPOSITIONFULL"); break;
        case 0x0285: wcscat_s(buf, L"IME_SELECT"); break;
        case 0x0286: wcscat_s(buf, L"IME_CHAR"); break;
#endif
#if(WINVER >= 0x0500)
        case 0x0288: wcscat_s(buf, L"IME_REQUEST"); break;
#endif
#if(WINVER >= 0x0400)
        case 0x0290: wcscat_s(buf, L"IME_KEYDOWN"); break;
        case 0x0291: wcscat_s(buf, L"IME_KEYUP"); break;
#endif
#if ((_WIN32_WINNT >= 0x0400) || (WINVER >= 0x0500))
        case 0x02A1: wcscat_s(buf, L"MOUSEHOVER"); break;
        case 0x02A3: wcscat_s(buf, L"MOUSELEAVE"); break;
#endif
#if(WINVER >= 0x0500)
        case 0x02A0: wcscat_s(buf, L"NCMOUSEHOVER"); break;
        case 0x02A2: wcscat_s(buf, L"NCMOUSELEAVE"); break;
#endif
#if(_WIN32_WINNT >= 0x0501)
        case 0x02B1: wcscat_s(buf, L"WTSSESSION_CHANGE"); break;
        case 0x02C0: wcscat_s(buf, L"TABLET_FIRST"); break;
        case 0x02DF: wcscat_s(buf, L"TABLET_LAST"); break;
#endif
#if(WINVER >= 0x0601)
        case 0x02E0: wcscat_s(buf, L"DPICHANGED"); break;
#endif
#if(WINVER >= 0x0605)
        case 0x02E2: wcscat_s(buf, L"DPICHANGED_BEFOREPARENT"); break;
        case 0x02E3: wcscat_s(buf, L"DPICHANGED_AFTERPARENT"); break;
        case 0x02E4: wcscat_s(buf, L"GETDPISCALEDSIZE"); break;
#endif
        case 0x0300: wcscat_s(buf, L"CUT"); break;
        case 0x0301: wcscat_s(buf, L"COPY"); break;
        case 0x0302: wcscat_s(buf, L"PASTE"); break;
        case 0x0303: wcscat_s(buf, L"CLEAR"); break;
        case 0x0304: wcscat_s(buf, L"UNDO"); break;
        case 0x0305: wcscat_s(buf, L"RENDERFORMAT"); break;
        case 0x0306: wcscat_s(buf, L"RENDERALLFORMATS"); break;
        case 0x0307: wcscat_s(buf, L"DESTROYCLIPBOARD"); break;
        case 0x0308: wcscat_s(buf, L"DRAWCLIPBOARD"); break;
        case 0x0309: wcscat_s(buf, L"PAINTCLIPBOARD"); break;
        case 0x030A: wcscat_s(buf, L"VSCROLLCLIPBOARD"); break;
        case 0x030B: wcscat_s(buf, L"SIZECLIPBOARD"); break;
        case 0x030C: wcscat_s(buf, L"ASKCBFORMATNAME"); break;
        case 0x030D: wcscat_s(buf, L"CHANGECBCHAIN"); break;
        case 0x030E: wcscat_s(buf, L"HSCROLLCLIPBOARD"); break;
        case 0x030F: wcscat_s(buf, L"QUERYNEWPALETTE"); break;
        case 0x0310: wcscat_s(buf, L"PALETTEISCHANGING"); break;
        case 0x0311: wcscat_s(buf, L"PALETTECHANGED"); break;
        case 0x0312: wcscat_s(buf, L"HOTKEY"); break;
#if (WINVER >= 0x0400)
        case 0x0317: wcscat_s(buf, L"PRINT"); break;
        case 0x0318: wcscat_s(buf, L"PRINTCLIENT"); break;
#endif
#if(_WIN32_WINNT >= 0x0500)
        case 0x0319: wcscat_s(buf, L"APPCOMMAND"); break;
#endif
#if(_WIN32_WINNT >= 0x0501)
        case 0x031A: wcscat_s(buf, L"THEMECHANGED"); break;
#endif
#if(_WIN32_WINNT >= 0x0501)
        case 0x031D: wcscat_s(buf, L"CLIPBOARDUPDATE"); break;
#endif
#if(_WIN32_WINNT >= 0x0600)
        case 0x031E: wcscat_s(buf, L"DWMCOMPOSITIONCHANGED"); break;
        case 0x031F: wcscat_s(buf, L"DWMNCRENDERINGCHANGED"); break;
        case 0x0320: wcscat_s(buf, L"DWMCOLORIZATIONCOLORCHANGED"); break;
        case 0x0321: wcscat_s(buf, L"DWMWINDOWMAXIMIZEDCHANGE"); break;
#endif
#if(_WIN32_WINNT >= 0x0601)
        case 0x0323: wcscat_s(buf, L"DWMSENDICONICTHUMBNAIL"); break;
        case 0x0326: wcscat_s(buf, L"DWMSENDICONICLIVEPREVIEWBITMAP"); break;
#endif
#if(WINVER >= 0x0600)
        case 0x033F: wcscat_s(buf, L"GETTITLEBARINFOEX"); break;
#endif
#if (WINVER >= 0x0400)
        case 0x0358: wcscat_s(buf, L"HANDHELDFIRST"); break;
        case 0x035F: wcscat_s(buf, L"HANDHELDLAST"); break;
        case 0x0360: wcscat_s(buf, L"AFXFIRST"); break;
        case 0x037F: wcscat_s(buf, L"AFXLAST"); break;
#endif
        case 0x0380: wcscat_s(buf, L"PENWINFIRST"); break;
        case 0x038F: wcscat_s(buf, L"PENWINLAST"); break;
        case 0x0400: wcscat_s(buf, L"USER/WA_IPC/MILK2"); break;
        default: swprintf_s(buf, L"unknown/0x%08x", msg); break;
    }

    char buf2[256];
    size_t n = wcslen(buf);
    buf[n] = ',';
    int desired_len = 24;
    size_t spaces_to_append = desired_len - n;
    if (spaces_to_append > 0)
    {
        for (unsigned int i = 1; i < spaces_to_append; i++)
            buf[n + i] = ' ';
        buf[desired_len] = L'\0';
    }

    sprintf_s(buf2, "%shwnd=%08p, msg=%ls w=%08x, l=%08x\n", szStartText, hwnd, buf, static_cast<unsigned int>(wParam), static_cast<int>(lParam));
    OutputDebugStringA(buf2);
}
#endif
// clang-format on