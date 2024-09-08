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

#ifndef __MILKDROP_MENU_H__
#define __MILKDROP_MENU_H__

#include "textmgr.h"

typedef enum
{
    MENUITEMTYPE_BUNK,
    MENUITEMTYPE_BOOL,
    MENUITEMTYPE_INT,
    MENUITEMTYPE_FLOAT,
    MENUITEMTYPE_LOGFLOAT,
    MENUITEMTYPE_BLENDABLE,
    MENUITEMTYPE_LOGBLENDABLE,
    MENUITEMTYPE_STRING,
    MENUITEMTYPE_UIMODE,
    //MENUITEMTYPE_OSC,
} MENUITEMTYPE;

#define MAX_CHILD_MENUS 16

typedef void (*MilkMenuCallbackFnPtr)(LPARAM param1, LPARAM param2); // `MilkMenuCallbackFnPtr` is synonym for "pointer to function returning void and taking 2 lparams"

#pragma region Menu Item
class CMilkMenuItem
{
  public:
    CMilkMenuItem();
    ~CMilkMenuItem();

    wchar_t m_szName[64];
    wchar_t m_szToolTip[1024];
    MENUITEMTYPE m_type;
    TextElement m_element[3];
    float m_fMin;                        // note: has different meanings based on the MENUITEMTYPE
    float m_fMax;                        // note: has different meanings based on the MENUITEMTYPE
    WPARAM m_wParam;
    LPARAM m_lParam;
    MilkMenuCallbackFnPtr m_pCallbackFn; // callback function pointer; if non-NULL, this function will be called whenever the menu item is modified by the user
    ptrdiff_t m_var_offset;              // distance of variable's memory location, in bytes, from pg->m_pState
    LPARAM m_original_value;             // can hold a float or int
    size_t m_nLastCursorPos;             // for strings; remembers most recent position of the cursor
    bool m_bEnabled;

    // Special data used for MENUITEMTYPE_OSCILLATOR.
    //int m_nSubSel;
    //bool m_bEditingSubSel;

    CMilkMenuItem* m_pNext;
};
#pragma endregion

#pragma region Menu
class CMilkMenu
{
  public:
    CMilkMenu();
    ~CMilkMenu();

    void Init(wchar_t* szName);
    void Finish();
    void AddChildMenu(CMilkMenu* pChildMenu);
    void AddItem(wchar_t* szName, void* var, MENUITEMTYPE type, wchar_t* szToolTip, float min = 0, float max = 0, MilkMenuCallbackFnPtr pCallback = NULL, unsigned int wParam = 0, unsigned int lParam = 0);
    void SetParentPointer(CMilkMenu* pParentMenu) { m_pParentMenu = pParentMenu; }
    LRESULT HandleKeydown(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void DrawMenu(D2D1_RECT_F rect, int xR, int yB, int bCalcRect = 0, D2D1_RECT_F* pCalcRect = nullptr);
    void UndrawMenus();
    void OnWaitStringAccept(wchar_t* szNewString);
    void EnableItem(wchar_t* szName, bool bEnable);
    CMilkMenuItem* GetCurItem() // NOTE: only works if an "item" is highlighted, not a child menu
    {
        CMilkMenuItem* pItem = m_pFirstChildItem;
        for (int i = m_nChildMenus; i < m_nCurSel; i++)
            pItem = pItem->m_pNext;
        return pItem;
    }
    const wchar_t* GetName() { return m_szMenuName; }
    void Enable(bool bEnabled) { m_bEnabled = bEnabled; }
    bool IsEnabled() { return m_bEnabled; }
    bool ItemIsEnabled(int i);

  protected:
    void Reset();
    CMilkMenu* m_pParentMenu;
    CMilkMenu* m_ppChildMenu[MAX_CHILD_MENUS]; // pointers are kept here, but these should be cleaned up by the app.
    CMilkMenuItem* m_pFirstChildItem;          // linked list; these are dynamically allocated, and automatically cleaned up in destructor
    wchar_t m_szMenuName[64];
    TextElement m_element;
    int m_nChildMenus;
    int m_nChildItems;
    int m_nCurSel;
    bool m_bEditingCurSel;
    bool m_bEnabled;
};
#pragma endregion

#endif