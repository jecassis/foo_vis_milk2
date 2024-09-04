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
#include "menu.h"

#include "plugin.h"
#include "utility.h"
#define WASABI_API_ORIG_HINST g_plugin.GetInstance()
#include "api.h"
//#include "resource.h"

extern CPlugin g_plugin;

CMilkMenuItem::CMilkMenuItem()
{
    WASABI_API_LNGSTRINGW_BUF(IDS_UNTITLED_MENU_ITEM, m_szName, 64);
    m_szToolTip[0] = 0;
    m_type = MENUITEMTYPE_BUNK;
    m_fMin = 0.0f;
    m_fMax = 0.0f;
    m_var_offset = NULL;
    m_pCallbackFn = NULL;
    m_pNext = NULL;
    m_original_value = NULL;
    m_nLastCursorPos = 0;
    m_bEnabled = true;
    m_wParam = 0;
    m_lParam = 0;
}

CMilkMenuItem::~CMilkMenuItem()
{
    if (m_pNext)
    {
        delete m_pNext;
        m_pNext = NULL;
    }
}

CMilkMenu::CMilkMenu()
{
    //Reset();
}

CMilkMenu::~CMilkMenu()
{
    /*
    if (m_pFirstChildItem)
    {
        delete m_pFirstChildItem;
        m_pFirstChildItem = NULL;
    }
    */
}

bool CMilkMenu::ItemIsEnabled(int j)
{
    if (j < m_nChildMenus)
        return m_ppChildMenu[j]->IsEnabled();

    int i = m_nChildMenus;
    CMilkMenuItem* pChild = m_pFirstChildItem;
    while (pChild && i < j)
    {
        pChild = pChild->m_pNext;
        i++;
    }
    if (pChild)
        return pChild->m_bEnabled;

    return false;
}

void CMilkMenu::EnableItem(wchar_t* szName, bool bEnable)
{
    int i = 0;

    // Search submenus.
    for (; i < m_nChildMenus; i++)
    {
        if (!wcscmp(m_ppChildMenu[i]->GetName(), szName))
        {
            m_ppChildMenu[i]->Enable(bEnable);
            if (!bEnable)
            {
                while (m_nCurSel > 0 && !ItemIsEnabled(m_nCurSel))
                    m_nCurSel--;
                if (m_nCurSel == 0 && !ItemIsEnabled(m_nCurSel))
                    while (m_nCurSel < m_nChildMenus + m_nChildItems - 1 && !ItemIsEnabled(m_nCurSel))
                        m_nCurSel++;
            }
            return;
        }
    }

    // Search child items.
    CMilkMenuItem* pChild = m_pFirstChildItem;
    while (pChild)
    {
        if (!wcscmp(pChild->m_szName, szName))
        {
            pChild->m_bEnabled = bEnable;
            if (!bEnable)
            {
                while (m_nCurSel > 0 && !ItemIsEnabled(m_nCurSel))
                    m_nCurSel--;
                if (m_nCurSel == 0 && !ItemIsEnabled(m_nCurSel))
                    while (m_nCurSel < m_nChildMenus + m_nChildItems - 1 && !ItemIsEnabled(m_nCurSel))
                        m_nCurSel++;
            }
            return;
        }
        pChild = pChild->m_pNext;
        i++;
    }
}

void CMilkMenu::Reset()
{
    m_pParentMenu = NULL;
    for (int i = 0; i < MAX_CHILD_MENUS; i++)
        m_ppChildMenu[i] = NULL;
    m_pFirstChildItem = NULL;
    WASABI_API_LNGSTRINGW_BUF(IDS_UNTITLED_MENU, m_szMenuName, 64);
    m_nChildMenus = 0;
    m_nChildItems = 0;
    m_nCurSel = 0;
    m_bEditingCurSel = false;
    m_bEnabled = true;
}

void CMilkMenu::Init(wchar_t* szName)
{
    Reset();
    if (szName && szName[0])
        wcsncpy_s(m_szMenuName, szName, 64);
}

void CMilkMenu::Finish()
{
    if (m_pFirstChildItem)
    {
        delete m_pFirstChildItem;
        m_pFirstChildItem = NULL;
    }
}

void CMilkMenu::AddChildMenu(CMilkMenu* pMenu)
{
    if (m_nChildMenus < MAX_CHILD_MENUS)
    {
        m_ppChildMenu[m_nChildMenus++] = pMenu;
        pMenu->SetParentPointer(this);
    }
}

void CMilkMenu::AddItem(wchar_t* szName, void* var, MENUITEMTYPE type, wchar_t* szToolTip, float min, float max, MilkMenuCallbackFnPtr pCallback, unsigned int wParam, unsigned int lParam)
{
    CMilkMenuItem* pLastItem = NULL;

    // Find last item in linked list.
    if (!m_pFirstChildItem)
    {
        // First item.
        m_pFirstChildItem = new CMilkMenuItem;
        pLastItem = m_pFirstChildItem;
    }
    else
    {
        pLastItem = m_pFirstChildItem;
        while (pLastItem->m_pNext)
            pLastItem = pLastItem->m_pNext;

        // Allocate a new `CMilkMenuItem`.
        pLastItem->m_pNext = new CMilkMenuItem;
        pLastItem = pLastItem->m_pNext;
    }

    // Set its attributes.
    wcsncpy_s(pLastItem->m_szName, szName, 64);
    wcsncpy_s(pLastItem->m_szToolTip, szToolTip, 1024);
    pLastItem->m_var_offset = reinterpret_cast<size_t>(var) - reinterpret_cast<size_t>(g_plugin.m_pState);
    pLastItem->m_type = type;
    pLastItem->m_fMin = min;
    pLastItem->m_fMax = max;
    pLastItem->m_wParam = wParam;
    pLastItem->m_lParam = lParam;
    if ((type == MENUITEMTYPE_LOGBLENDABLE || type == MENUITEMTYPE_LOGFLOAT) && min == max)
    {
        // Special defaults.
        pLastItem->m_fMin = 0.01f;
        pLastItem->m_fMax = 100.0f;
    }
    pLastItem->m_pCallbackFn = pCallback;

    m_nChildItems++;
}

void MDMenuTextOut(TextElement* element, eFontIndex font_index, wchar_t* str, DWORD color, D2D1_RECT_F* pRect, int bCalcRect, D2D1_RECT_F* pCalcRect)
{
    D2D1_COLOR_F fColor = D2D1::ColorF(color, CPlugin::GetAlpha(color));
    element->Initialize(g_plugin.m_lpDX->GetD2DDeviceContext());
    element->SetAlignment(AlignNear, AlignNear);
    element->SetTextColor(fColor);
    element->SetTextOpacity(fColor.a);
    element->SetVisible(true);
    element->SetText(str);
    element->SetTextStyle(g_plugin.GetFont(font_index));
    element->SetTextShadow(false);

    if (bCalcRect)
    {
        D2D1_RECT_F t = *pRect;
        element->SetContainer(t);
        pRect->top += g_plugin.m_text.DrawD2DText(g_plugin.GetFont(font_index), element, str, &t, DT_SINGLELINE | DT_END_ELLIPSIS | DT_CALCRECT, 0xFFFFFFFF, false);
        pCalcRect->bottom += t.bottom - t.top;
        //if (pCalcRect->bottom > pRect->bottom)
        //    pCalcRect->bottom = pRect->bottom;
        pCalcRect->right = std::max(pCalcRect->right, pCalcRect->left + t.right - t.left);
    }
    else
    {
        pRect->top += g_plugin.m_text.DrawD2DText(g_plugin.GetFont(font_index), element, str, pRect, DT_SINGLELINE | DT_END_ELLIPSIS, color, true, 0xFF000000);
        element->SetContainer(*pRect);
        g_plugin.m_text.RegisterElement(element);
    }
}

// `rect` is the bounding rectangle in which it is allowed to draw the menu;
// its `.top` member is incremented as it is drawn downward.
// If `bCalcRect == 1`, then return `pCalcRect` as the area that the menu actually
// occupies, excluding any tooltips.
void CMilkMenu::DrawMenu(D2D1_RECT_F rect, int xR, int yB, int bCalcRect, D2D1_RECT_F* pCalcRect)
{
    if (bCalcRect != 0 && pCalcRect == nullptr)
        return;

    if (bCalcRect)
    {
        pCalcRect->left = rect.left;
        pCalcRect->right = rect.left;
        pCalcRect->top = rect.top;
        pCalcRect->bottom = rect.top;
    }

    if (!m_bEditingCurSel)
    {
        int nLines = static_cast<int>(rect.bottom - rect.top - PLAYLIST_INNER_MARGIN * 2.0f) / g_plugin.GetFontHeight(SIMPLE_FONT) - 1; // save 1 line for the tooltip
        if (nLines < 1)
            return;
        int nStart = (m_nCurSel / nLines) * nLines;

        int nLinesDrawn = 0, i = 0;

        for (; i < m_nChildMenus; i++)
        {
            if (i >= nStart && i < nStart + nLines)
            {
                if (m_ppChildMenu[i]->IsEnabled())
                {
                    MDMenuTextOut(&(m_ppChildMenu[i]->m_element), SIMPLE_FONT, m_ppChildMenu[i]->m_szMenuName, (i == m_nCurSel) ? MENU_HILITE_COLOR : MENU_COLOR, &rect, bCalcRect, pCalcRect);
                    nLinesDrawn++;
                }

                if (g_plugin.m_bShowMenuToolTips && i == m_nCurSel && !bCalcRect)
                {
                    // Tooltip.
                    g_plugin.DrawTooltip(WASABI_API_LNGSTRINGW(IDS_SZ_MENU_NAV_TOOLTIP), xR, yB);
                }
            }
        }

        CMilkMenuItem* pItem = m_pFirstChildItem;

        while (pItem && nLinesDrawn < nStart + nLines)
        {
            if (!pItem->m_bEnabled)
            {
                pItem = pItem->m_pNext;
                i++;
                continue;
            }

            size_t addr = pItem->m_var_offset + reinterpret_cast<size_t>(g_plugin.m_pState);
            if (i >= nStart)
            {
                wchar_t szItemText[256];
                switch (pItem->m_type)
                {
                    case MENUITEMTYPE_STRING:
                        wcscpy_s(szItemText, pItem->m_szName);
                        break;
                    case MENUITEMTYPE_BOOL:
                        swprintf_s(szItemText, L"%s [%s]", pItem->m_szName, WASABI_API_LNGSTRINGW(*(reinterpret_cast<bool*>(addr)) ? IDS_ON : IDS_OFF));
                        break;
                    default:
                        wcscpy_s(szItemText, pItem->m_szName);
                        break;
                }

                if (i == m_nCurSel)
                {
                    MDMenuTextOut(&(pItem->m_element), SIMPLE_FONT, szItemText, MENU_HILITE_COLOR, &rect, bCalcRect, pCalcRect);

                    if (g_plugin.m_bShowMenuToolTips && !bCalcRect)
                    {
                        // Tooltip.
                        g_plugin.DrawTooltip(pItem->m_szToolTip, xR, yB);
                    }
                }
                else
                {
                    MDMenuTextOut(&(pItem->m_element), SIMPLE_FONT, szItemText, MENU_COLOR, &rect, bCalcRect, pCalcRect);
                }
                nLinesDrawn++;
            }

            pItem = pItem->m_pNext;
            i++;
        }
    }
    else // Editing current selection.
    {
        // Find the item.
        CMilkMenuItem* pItem = m_pFirstChildItem;
        for (int i = m_nChildMenus; i < m_nCurSel; i++)
            pItem = pItem->m_pNext;
        size_t addr = pItem->m_var_offset + reinterpret_cast<size_t>(g_plugin.m_pState);

        wchar_t buf[256] = {0};

        MDMenuTextOut(&(pItem->m_element), SIMPLE_FONT, WASABI_API_LNGSTRINGW(IDS_USE_UP_DOWN_ARROW_KEYS), MENU_COLOR, &rect, bCalcRect, pCalcRect);
        swprintf_s(buf, WASABI_API_LNGSTRINGW(IDS_CURRENT_VALUE_OF_X), pItem->m_szName);
        MDMenuTextOut(&(pItem->m_element), SIMPLE_FONT, buf, MENU_COLOR, &rect, bCalcRect, pCalcRect);

        switch (pItem->m_type)
        {
            case MENUITEMTYPE_INT:
                swprintf_s(buf, L" %d ", *(reinterpret_cast<int*>(addr)));
                break;
            case MENUITEMTYPE_FLOAT:
            case MENUITEMTYPE_LOGFLOAT:
                swprintf_s(buf, L" %5.3f ", *(reinterpret_cast<float*>(addr)));
                break;
            case MENUITEMTYPE_BLENDABLE:
            case MENUITEMTYPE_LOGBLENDABLE:
                swprintf_s(buf, L" %5.3f ", (reinterpret_cast<CBlendableFloat*>(addr))->eval(-1));
                break;
            default:
                wcscpy_s(buf, L" ? ");
                break;
        }

        MDMenuTextOut(&(pItem->m_element), SIMPLE_FONT, buf, MENU_HILITE_COLOR, &rect, bCalcRect, pCalcRect);

        // Tooltip.
        if (g_plugin.m_bShowMenuToolTips && !bCalcRect)
        {
            g_plugin.DrawTooltip(pItem->m_szToolTip, xR, yB);
        }
    }
}

void CMilkMenu::UndrawMenus()
{
    for (int i = 0; i < m_nChildMenus; i++)
    {
        if (m_ppChildMenu[i]->IsEnabled() && m_ppChildMenu[i]->m_element.IsVisible())
        {
            m_ppChildMenu[i]->m_element.SetVisible(false);
            g_plugin.m_text.UnregisterElement(&(m_ppChildMenu[i]->m_element));
        }
    }

    CMilkMenuItem* pItem = m_pFirstChildItem;
    while (pItem)
    {
        if (pItem->m_bEnabled && pItem->m_element.IsVisible())
        {
            pItem->m_element.SetVisible(false);
            g_plugin.m_text.UnregisterElement(&(pItem->m_element));
        }

        pItem = pItem->m_pNext;
    }
}

void CMilkMenu::OnWaitStringAccept(wchar_t* szNewString)
{
    m_bEditingCurSel = false;

    // Find the item.
    CMilkMenuItem* pItem = m_pFirstChildItem;
    for (int i = m_nChildMenus; i < m_nCurSel; i++)
        pItem = pItem->m_pNext;
    size_t addr = pItem->m_var_offset + reinterpret_cast<size_t>(g_plugin.m_pState);

    assert(pItem->m_type == MENUITEMTYPE_STRING);

    // Apply the edited string.
    wcscpy_s(reinterpret_cast<wchar_t*>(addr), wcslen(szNewString) + 1, szNewString); // BUG!!

    // If user provided a callback function pointer, call it now.
    if (pItem->m_pCallbackFn)
    {
        pItem->m_pCallbackFn(0, 0);
    }

    // Remember the last cursor position.
    pItem->m_nLastCursorPos = g_plugin.m_waitstring.nCursorPos;
}

// All `WM_KEYDOWNS` that the app gets when a menu is up should be handled here,
// by the menu that is currently active.
// Returns `FALSE` if it handled the key, `TRUE` if it didn't.
LRESULT CMilkMenu::HandleKeydown(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(message);
    int nRepeat = LOWORD(lParam);

    if (!m_bEditingCurSel)
    {
        switch (wParam)
        {
            case VK_UP:
                for (int rep = 0; rep < nRepeat; rep++)
                {
                    if (m_nCurSel == 0)
                        break;
                    do
                    {
                        m_nCurSel--;
                    } while (m_nCurSel > 0 && !ItemIsEnabled(m_nCurSel));
                }
                if (m_nCurSel < 0)
                    m_nCurSel = 0; //m_nChildMenus + m_nChildItems - 1;
                while (m_nCurSel < m_nChildMenus + m_nChildItems - 1 && !ItemIsEnabled(m_nCurSel))
                    m_nCurSel++;
                return FALSE;
            case VK_DOWN:
                for (int rep = 0; rep < nRepeat; rep++)
                {
                    if (m_nCurSel == m_nChildMenus + m_nChildItems - 1)
                        break;
                    do
                    {
                        m_nCurSel++;
                    } while (m_nCurSel < m_nChildMenus + m_nChildItems - 1 && !ItemIsEnabled(m_nCurSel));
                }
                if (m_nCurSel >= m_nChildMenus + m_nChildItems)
                    m_nCurSel = m_nChildMenus + m_nChildItems - 1; //0;
                while (m_nCurSel > 0 && !ItemIsEnabled(m_nCurSel))
                    m_nCurSel--;
                return FALSE;
            case VK_HOME:
                m_nCurSel = 0;
                return FALSE;
            case VK_END:
                m_nCurSel = m_nChildMenus + m_nChildItems - 1;
                return FALSE;
            case VK_ESCAPE:
                g_plugin.m_UI_mode = UI_REGULAR;
                g_plugin.ClearText();
                return FALSE;
            case VK_BACK:
            case VK_LEFT:
                if (m_pParentMenu)
                {
                    g_plugin.m_pCurMenu->UndrawMenus();
                    g_plugin.m_pCurMenu = m_pParentMenu;
                }
                else
                {
                    g_plugin.m_UI_mode = UI_REGULAR; // exit the menu
                    g_plugin.ClearText();
                }
                return FALSE;
            case VK_RETURN:
            case VK_RIGHT:
            case VK_SPACE:
                if (m_nCurSel < m_nChildMenus)
                {
                    g_plugin.m_pCurMenu->UndrawMenus();
                    g_plugin.m_pCurMenu = m_ppChildMenu[m_nCurSel]; // Go to sub-menu.
                }
                else
                {
                    // Find the item.
                    CMilkMenuItem* pItem = GetCurItem();
                    size_t addr = pItem->m_var_offset + reinterpret_cast<size_t>(g_plugin.m_pState);
                    float fTemp;
                    switch (pItem->m_type) // Begin editing the item.
                    {
                        case MENUITEMTYPE_UIMODE:
                            g_plugin.m_UI_mode = (ui_mode)pItem->m_wParam;
                            if (g_plugin.m_UI_mode == UI_IMPORT_WAVE ||
                                g_plugin.m_UI_mode == UI_EXPORT_WAVE ||
                                g_plugin.m_UI_mode == UI_IMPORT_SHAPE ||
                                g_plugin.m_UI_mode == UI_EXPORT_SHAPE)
                            {
                                g_plugin.m_bPresetLockedByCode = true;
                                g_plugin.m_waitstring.bActive = true; // enter "WaitString" mode
                                g_plugin.m_waitstring.bFilterBadChars = false;
                                g_plugin.m_waitstring.bDisplayAsCode = false;
                                g_plugin.m_waitstring.nSelAnchorPos = -1;
                                g_plugin.m_waitstring.nMaxLen = std::min(sizeof(g_plugin.m_waitstring.szText) - 1, MAX_PATH - wcslen(g_plugin.GetPresetDir()) - 6); // 6 for the extension and null character because Win32 `LoadFile()`, `MoveFile()`, etc. fail if "path+filename+ext" is greater than `MAX_PATH` characters.
                                swprintf_s(g_plugin.m_waitstring.szText, L"%sfile.dat", g_plugin.m_szPresetDir);
                                if (g_plugin.m_UI_mode == UI_IMPORT_WAVE || g_plugin.m_UI_mode == UI_IMPORT_SHAPE)
                                    WASABI_API_LNGSTRINGW_BUF(IDS_LOAD_FROM_FILE, g_plugin.m_waitstring.szPrompt, 512);
                                else
                                    WASABI_API_LNGSTRINGW_BUF(IDS_SAVE_TO_FILE, g_plugin.m_waitstring.szPrompt, 512);
                                g_plugin.m_waitstring.szToolTip[0] = 0;
                                g_plugin.m_waitstring.nCursorPos = wcslen(g_plugin.m_waitstring.szText); // set the starting edit position
                            }
                            g_plugin.ClearText();
                            break;
                        case MENUITEMTYPE_BOOL:
                            *((bool*)addr) = !(*((bool*)addr));
                            break;
                        case MENUITEMTYPE_INT:
                            m_bEditingCurSel = true;
                            pItem->m_original_value = (LPARAM)(*((int*)(addr)));
                            break;
                        case MENUITEMTYPE_FLOAT:
                        case MENUITEMTYPE_LOGFLOAT:
                            m_bEditingCurSel = true;
                            pItem->m_original_value = (LPARAM)(*((float*)(addr)) * 10000L);
                            break;
                        case MENUITEMTYPE_BLENDABLE:
                        case MENUITEMTYPE_LOGBLENDABLE:
                            m_bEditingCurSel = true;
                            {
                                //CBlendableFloat *p = (CBlendableFloat*)(pItem->m_pVariable);
                                //*p = 0.99f;
                                fTemp = ((CBlendableFloat*)addr)->eval(-1); //p->eval(-1);
                            }
                            pItem->m_original_value = (LPARAM)(fTemp * 10000L);
                            break;
                        case MENUITEMTYPE_STRING: // Enter "waitstring" mode.
                            // *** This function will cease to receive keyboard input while the string is being edited. ***
                            g_plugin.m_UI_mode = UI_EDIT_MENU_STRING;
                            g_plugin.m_waitstring.bActive = true;
                            g_plugin.m_waitstring.bFilterBadChars = false;
                            g_plugin.m_waitstring.bDisplayAsCode = true;
                            g_plugin.m_waitstring.nSelAnchorPos = -1;
                            g_plugin.m_waitstring.nMaxLen = pItem->m_wParam ? pItem->m_wParam : 8190;
                            g_plugin.m_waitstring.nMaxLen = std::min(g_plugin.m_waitstring.nMaxLen, sizeof(g_plugin.m_waitstring.szText) - 16);
                            strcpy_s(reinterpret_cast<char*>(g_plugin.m_waitstring.szText), sizeof(g_plugin.m_waitstring.szText), reinterpret_cast<char*>(addr));
                            swprintf_s(g_plugin.m_waitstring.szPrompt, WASABI_API_LNGSTRINGW(IDS_ENTER_THE_NEW_STRING), pItem->m_szName);
                            wcscpy_s(g_plugin.m_waitstring.szToolTip, pItem->m_szToolTip);
                            g_plugin.m_waitstring.nCursorPos = strnlen_s(reinterpret_cast<char*>(g_plugin.m_waitstring.szText), ARRAYSIZE(g_plugin.m_waitstring.szText));
                            if (pItem->m_nLastCursorPos < g_plugin.m_waitstring.nCursorPos)
                                g_plugin.m_waitstring.nCursorPos = pItem->m_nLastCursorPos;
                            g_plugin.ClearText();
                            break;
                        /*
                        case MENUITEMTYPE_OSC:
                            m_bEditingCurSel = true;
                            pItem->m_bEditingSubSel = false;
                            break;
                        */
                    }
                }
                return FALSE;

            default:
                // Key was not handled.
                return TRUE;
        }
    }
    else // `m_bEditingCurSel`
    {
        float fMult = 1.0f;
        bool bDec;

        // Find the item.
        CMilkMenuItem* pItem = m_pFirstChildItem;
        for (int i = m_nChildMenus; i < m_nCurSel; i++)
            pItem = pItem->m_pNext;
        size_t addr = pItem->m_var_offset + reinterpret_cast<size_t>(g_plugin.m_pState);

        switch (wParam)
        {
            case VK_ESCAPE: // exit edit mode and restore original value
                switch (pItem->m_type)
                {
                    case MENUITEMTYPE_INT:
                        m_bEditingCurSel = false;
                        *((int*)addr) = (int)pItem->m_original_value;
                        break;
                    case MENUITEMTYPE_FLOAT:
                        m_bEditingCurSel = false;
                        *((float*)addr) = ((float)pItem->m_original_value) * 0.0001f;
                        break;
                    case MENUITEMTYPE_LOGFLOAT:
                        m_bEditingCurSel = false;
                        *((float*)addr) = ((float)pItem->m_original_value) * 0.0001f;
                        break;
                    case MENUITEMTYPE_BLENDABLE:
                        m_bEditingCurSel = false;
                        *((CBlendableFloat*)(addr)) = ((float)(pItem->m_original_value)) * 0.0001f;
                        break;
                    case MENUITEMTYPE_LOGBLENDABLE:
                        m_bEditingCurSel = false;
                        *((CBlendableFloat*)(addr)) = ((float)(pItem->m_original_value)) * 0.0001f;
                        break;
                    //case MENUITEMTYPE_STRING:
                    //    ... will not ever happen - see `OnWaitStringCancel()`.
                }
                return FALSE;
            case VK_RETURN:
                //if (pItem->m_type == MENUITEMTYPE_STRING)
                //    ... will not ever happen - see `OnWaitStringAccept()`.
                m_bEditingCurSel = false;
                return FALSE;
            case VK_NEXT:
            case VK_PRIOR:
                fMult *= 10.0f;
                [[fallthrough]]; // `break` intentionally left out here...
            case VK_UP:
            case VK_DOWN:
                {
                    USHORT mask = 1 << (sizeof(USHORT) * 8 - 1); // we want the highest-order bit
                    bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0;
                    //bool bCtrlHeldDown = (GetKeyState(VK_CONTROL) & mask) != 0;
                    if (bShiftHeldDown && (wParam == VK_UP || wParam == VK_DOWN))
                        fMult *= 0.1f;
                }

                bDec = (wParam == VK_DOWN || wParam == VK_NEXT);

                switch (pItem->m_type)
                {
                    case MENUITEMTYPE_INT:
                        {
                            int* pInt = ((int*)addr);
                            if (fMult < 1)
                                fMult = 1;
                            (*pInt) += (int)((bDec) ? -fMult : fMult);
                            if (*pInt < pItem->m_fMin)
                                *pInt = (int)pItem->m_fMin;
                            if (*pInt > pItem->m_fMax)
                                *pInt = (int)pItem->m_fMax;
                        }
                        break;
                    case MENUITEMTYPE_FLOAT:
                        {
                            float* pFloat = ((float*)addr);
                            float fInc = (pItem->m_fMax - pItem->m_fMin) * 0.01f * fMult;
                            (*pFloat) += (bDec) ? -fInc : fInc;
                            if (*pFloat < pItem->m_fMin)
                                *pFloat = pItem->m_fMin;
                            if (*pFloat > pItem->m_fMax)
                                *pFloat = pItem->m_fMax;
                        }
                        break;
                    case MENUITEMTYPE_LOGFLOAT:
                        {
                            float* pFloat = ((float*)addr);
                            (*pFloat) *= (bDec) ? powf(1.0f / 1.01f, fMult) : powf(1.01f, fMult);
                            if (*pFloat < pItem->m_fMin)
                                *pFloat = pItem->m_fMin;
                            if (*pFloat > pItem->m_fMax)
                                *pFloat = pItem->m_fMax;
                        }
                        break;
                    case MENUITEMTYPE_BLENDABLE:
                        {
                            CBlendableFloat* pBlend = ((CBlendableFloat*)addr);
                            float fInc = (pItem->m_fMax - pItem->m_fMin) * 0.01f * fMult;
                            (*pBlend) += (bDec) ? -fInc : fInc;
                            if (pBlend->eval(-1) < pItem->m_fMin)
                                *pBlend = pItem->m_fMin;
                            if (pBlend->eval(-1) > pItem->m_fMax)
                                *pBlend = pItem->m_fMax;
                        }
                        break;
                    case MENUITEMTYPE_LOGBLENDABLE:
                        {
                            CBlendableFloat* pBlend = ((CBlendableFloat*)addr);
                            (*pBlend) *= (bDec) ? powf(1.0f / 1.01f, fMult) : powf(1.01f, fMult);
                            if (pBlend->eval(-1) < pItem->m_fMin)
                                *pBlend = pItem->m_fMin;
                            if (pBlend->eval(-1) > pItem->m_fMax)
                                *pBlend = pItem->m_fMax;
                        }
                        break;
                    /*
                    case MENUITEMTYPE_OSC:
                        if (pItem->m_bEditingSubSel)
                        {
                            if (wParam == VK_UP)
                            {
                                pItem->m_nSubSel--;
                                if (pItem->m_nSubSel < 0) pItem->m_nSubSel = 4;
                            }
                            else if (wParam == VK_DOWN)
                            {
                                pItem->m_nSubSel++;
                                if (pItem->m_nSubSel > 4) pItem->m_nSubSel = 0;
                            }
                        }
                        else
                        {
                            switch (pItem->m_nSubSel)
                            {
                                // also to do: make 'drawtext' draw it properly
                                case 0:
                                    FIXME - what are the bounds for each type? and are incs constant or log?
                                        break;
                                case 1:
                                    FIXME
                                        break;
                                case 2:
                                    FIXME
                                        break;
                                case 3:
                                    FIXME
                                        break;
                                case 4:
                                    FIXME
                                        break;
                            }
                        }
                        break;
                    */
                }
                return FALSE;
            default: // Key was not handled.
                return TRUE;
        }
    }
}