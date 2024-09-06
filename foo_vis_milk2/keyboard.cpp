/*
 * keyboard.cpp - Implements the visualization component's keyboard behaviors.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

// Keyboard Commands
// -----------------
// Note: Keyboard messages from foobar2000 only propagate to the component
// during fullscreen or when the visualization window is not embedded in the
// player's layout.
//
// GENERAL
//  ESC: toggle interactive mode or exit fullscreen
//  ALT + K: launch preferences page
//
// PRESET LOADING
//  BACKSPACE: return to previous preset
//  SPACE: transition to next preset
//  H: instant hard cut (to next preset)
//  R: toggle random (vs. sequential) preset traversal
//  L: load a specific preset (invokes the 'Load' menu)
//  + / -: rate current preset (better / worse)
//  scroll lock: lock/unlock current preset
//      (keyboard light on means preset is locked which
//       prevents random switch to new preset)
//  A: aggregate preset - loads a random preset,
//     steals the warp shader from a different random preset,
//     and steals the composite shader from a third random preset.
//  D: cycle between various lock-states for the warp and
//     composite shaders. When one of these shaders is locked,
//     loading a new preset will load everything *except* the
//     locked shaders, creating a mix between the two presets.
//
// PRESET EDITING AND SAVING
//  M: show/hide the preset-editing menu
//  S: save new preset (asks for the new filename)
//  N: show per-frame variable monitor (see milkdrop_preset_authoring.html)
//
// MUSIC PLAYBACK
//  Z / X / C / V / B: navigate playlist (prev / play / pause / stop / next)
//  u / U: toggle shuffle mode forward / back
//  P: show playlist
//  up / down arrows: volume up / down
//  left / right arrows: rewind / forward 5 seconds
//  SHIFT + left / right arrows: rewind / forward 30 seconds
//
// STATUS INFORMATION
//  F1: show help screen
//  F2: show song title
//  F3: show song length
//  F4: show preset name
//  F5: show frames per second (FPS)
//  F6: show rating of current preset
//  Y, F7: re-read custom message file (milk_msg.ini) from disk
//  F8: jump to new presets directory
//  F9: show shader help (in shader edit mode)
//
// MENU / PLAYLIST / PRESETS
//  ESC: exit menu
//  BACKSPACE / left arrow: return to previous menu or exit menu if top
//  ENTER / SPACE / right arrow: select or go to sub-menu
//  up / down arrows: change selection up / down
//
// SPRITES AND CUSTOM MESSAGES
//  T: launch song title animation
//  Y: enter custom message mode
//      ##: load message ## (where ## is a 2-digit numeric code [00-99]
//                           of a message defined in milk_msg.ini)
//      *: clear any digits entered
//      DELETE: clear message (if visible)
//      F7: re-read milk_msg.ini from disk
//  K: enter sprite mode
//      ##: load sprite ## (where ## is a 2-digit numeric code [00-99]
//                          of a sprite defined in milk_img.ini)
//      *: clear any digits entered
//      DELETE: clear newest sprite 
//      SHIFT + DELETE: clear oldest sprite
//      CTRL + SHIFT + DELETE: clear all sprites
//      F7: no effect (milk_img.ini is never cached)
//  SHIFT + K: enter sprite kill mode
//      ##: clear all sprites with code ##
//      *: clear any digits entered
//  CTRL + T: kill song title
//  CTRL + Y: kill custom messages
//  CTRL + K: kill all sprites
//
// The following are hotkeys for changing certain common parameters
// on the current preset.
//
// MOTION
//  i / I: zoom in / out
//  [ / ]: push motion (`dx`) left / right
//  { / }: push motion (`dy`) up / down
//  < / >: rotate (`rot`) left / right
//  o / O: shrink / grow the amplitude of the warp effect
//
// WAVEFORM
//   w / W: cycle through waveforms forward / back
//   j / J: scale waveform down / up
//   e / E: make the waveform more transparent / solid
//
// BRIGHTNESS
//   g / G: decrease / increase gamma (brightness) **
//
// VIDEO ECHO EFFECT **
//   q / Q: scale second graphics layer down / up **
//   F: flip second graphics layer (cycles through 4 fixed orientations) **
//
// SHADERS
//   !: randomize warp shader
//   @: randomize composite shader
//
// ** These keys only have an effect on MilkDrop 1-era presets.
//    In MilkDrop 2-era presets, these values are embedded in
//    the shader, so you need to go into the composite shader
//    and modify the code.

// KEY HANDLING
//  - In all cases, handle or capture:
//    - ZXCVBRS, zxcvbrs; case-insensitive (lowercase come through only as WM_CHAR; uppercase come in as both)
//    - ALT+ENTER
//    - F1, ESC, UP, DOWN, LEFT, RIGHT, SHIFT+LEFT/RIGHT
//    - P for playlist
//      - When playlist showing, steal J, HOME, END, PGUP, PGDN, UP, DOWN, ESC
//    - Block J, L
//  - When integrated with Winamp (using `embedwnd`), also handle:
//    - j, l, L, CTRL+L [windowed mode only!]
//    - CTRL+P, CTRL+D
//    - CTRL+TAB
//    - ALT+E
//    - ALT+F (main menu)
//    - ALT+3 (id3)

// IMPORTANT: For the WM_KEYDOWN, WM_KEYUP, and WM_CHAR messages,
//            return 0 if the message (key) is processed, and 1 if not.
//            DO NOT call `DefWindowProc()` for these particular messages!

#pragma region Keyboard Controls
#define waitstring g_plugin.m_waitstring
#define UI_mode g_plugin.m_UI_mode
#define RemoveText g_plugin.ClearText

void milk2_ui_element::OnChar(TCHAR chChar, UINT nRepCnt, UINT nFlags)
{
    MILK2_CONSOLE_LOG("OnChar ", GetWnd())
    wchar_t buf[256] = {0};
    USHORT mask = 1 << (sizeof(SHORT) * 8 - 1); // get the highest-order bit
    bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0;

    if (g_plugin.m_show_playlist) // in playlist mode
    {
        auto api = playlist_manager::get();
        switch (chChar)
        {
            case 'j':
            case 'J':
                {
                    size_t pos = api->activeplaylist_get_focus_item();
                    if (pos == pfc::infinite_size)
                        pos = -1;
                    g_plugin.m_playlist_pos = static_cast<LRESULT>(pos);
                }
                return;
            /*
            case 'p':
            case 'P':
                TogglePlaylist();
                return;
            */
            default:
                {
                    int nSongs = static_cast<int>(api->activeplaylist_get_item_count());
                    bool found = false;
                    LRESULT orig_pos = g_plugin.m_playlist_pos;
                    int inc = (chChar >= 'A' && chChar <= 'Z') ? -1 : 1;
                    while (true)
                    {
                        if (inc == 1 && g_plugin.m_playlist_pos >= LRESULT{nSongs} - 1)
                            break;
                        if (inc == -1 && g_plugin.m_playlist_pos <= 0)
                            break;
                        g_plugin.m_playlist_pos += inc;

                        if (m_search.is_empty())
                        {
                            pfc::string8 pattern = "%title%";
                            static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_search, pattern);
                        }
                        pfc::string_formatter state;
                        metadb_handle_list list;
                        api->activeplaylist_get_all_items(list);
                        if (!(list.get_item(static_cast<size_t>(g_plugin.m_playlist_pos)))->format_title(NULL, state, m_search, NULL))
                            state = "";
                        m_szBuffer = pfc::wideFromUTF8(state);

                        TCHAR buf[32];
                        wcsncpy_s(buf, m_szBuffer.c_str(), 31);
                        buf[31] = L'\0';

                        // Remove song number and period from beginning.
                        PTCHAR p = buf;
                        while (*p >= '0' && *p <= '9')
                            ++p;
                        if (*p == '.' && *(p + 1) == ' ')
                        {
                            p += 2;
                            int pos = 0;
                            while (*p != L'\0')
                            {
                                buf[pos++] = *p;
                                p++;
                            }
                            buf[pos++] = L'\0';
                        }

                        TCHAR chChar2 = (chChar >= 'A' && chChar <= 'Z') ? (chChar + 'a' - 'A') : (chChar + 'A' - 'a');
                        if (unsigned(buf[0]) == chChar || unsigned(buf[0]) == chChar2)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                        g_plugin.m_playlist_pos = orig_pos;
                }
                return;
        }
    }
    else if (waitstring.bActive) // in the middle of editing a string
    {
        if ((chChar >= ' ' && chChar <= 'z') || chChar == '{' || chChar == '}')
        {
            size_t len = 0;
            if (waitstring.bDisplayAsCode)
                len = strnlen_s(reinterpret_cast<char*>(waitstring.szText), ARRAYSIZE(waitstring.szText) * sizeof(wchar_t) / sizeof(char));
            else
                len = wcsnlen_s(waitstring.szText, ARRAYSIZE(waitstring.szText));

            // NOTE: '&' is legal in filenames, but we try to avoid it since during GDI display it acts as a control code (it will not show up, but instead, underline the character following it).
            if (waitstring.bFilterBadChars && (chChar == '\"' || chChar == '\\' || chChar == '/' || chChar == ':' || chChar == '*' ||
                                               chChar == '?' || chChar == '|' || chChar == '<' || chChar == '>' || chChar == '&'))
            {
                // Illegal character.
                g_plugin.AddError(WASABI_API_LNGSTRINGW(IDS_ILLEGAL_CHARACTER), 2.5f, ERR_MISC, true);
            }
            else if (size_t{len} + size_t{nRepCnt} >= waitstring.nMaxLen)
            {
                // `waitstring.szText` has reached its limit.
                g_plugin.AddError(WASABI_API_LNGSTRINGW(IDS_STRING_TOO_LONG), 2.5f, ERR_MISC, true);
            }
            else
            {
                //m_fShowUserMessageUntilThisTime = GetTime(); // if there was an error message already, clear it
                if (waitstring.bDisplayAsCode)
                {
                    char buf[16] = {0};
                    sprintf_s(buf, "%c", static_cast<char>(chChar));

                    if (waitstring.nSelAnchorPos != -1)
                        g_plugin.WaitString_NukeSelection();

                    if (waitstring.bOvertypeMode)
                    {
                        // Overtype mode.
                        for (UINT rep = 0; rep < nRepCnt; rep++)
                        {
                            if (waitstring.nCursorPos == len)
                            {
                                strcat_s(reinterpret_cast<char*>(waitstring.szText), sizeof(waitstring.szText), buf);
                                len++;
                            }
                            else
                            {
                                char* ptr = reinterpret_cast<char*>(waitstring.szText);
                                *(ptr + waitstring.nCursorPos) = buf[0];
                            }
                            waitstring.nCursorPos++;
                        }
                    }
                    else
                    {
                        // Insert mode.
                        char* ptr = reinterpret_cast<char*>(waitstring.szText);
                        for (UINT rep = 0; rep < nRepCnt; rep++)
                        {
                            for (size_t i = len + 1; i-- > waitstring.nCursorPos;)
                                *(ptr + i + 1) = *(ptr + i);
                            *(ptr + waitstring.nCursorPos) = buf[0];
                            waitstring.nCursorPos++;
                            len++;
                        }
                    }
                }
                else
                {
                    wchar_t buf[16] = {0};
                    swprintf_s(buf, L"%c", static_cast<wchar_t>(chChar));

                    if (waitstring.nSelAnchorPos != -1)
                        g_plugin.WaitString_NukeSelection();

                    if (waitstring.bOvertypeMode)
                    {
                        // Overtype mode.
                        for (UINT rep = 0; rep < nRepCnt; rep++)
                        {
                            if (waitstring.nCursorPos == len)
                            {
                                wcscat_s(waitstring.szText, buf);
                                len++;
                            }
                            else
                                waitstring.szText[waitstring.nCursorPos] = buf[0];
                            waitstring.nCursorPos++;
                        }
                    }
                    else
                    {
                        // Insert mode.
                        for (UINT rep = 0; rep < nRepCnt; rep++)
                        {
                            for (size_t i = len + 1; i-- > waitstring.nCursorPos;)
                                waitstring.szText[i + 1] = waitstring.szText[i];
                            waitstring.szText[waitstring.nCursorPos] = buf[0];
                            waitstring.nCursorPos++;
                            len++;
                        }
                    }
                }
            }
        }
        return;
    }
    else if (UI_mode == UI_LOAD_DEL) // waiting to confirm file delete
    {
        if (chChar == 'y' || chChar == 'Y')
        {
            // First add pathname to filename.
            wchar_t szDelFile[512] = {0};
            swprintf_s(szDelFile, L"%s%s", g_plugin.GetPresetDir(), g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str());

            g_plugin.DeletePresetFile(szDelFile);
            //m_nCurrentPreset = -1;
        }
        UI_mode = UI_LOAD;
        RemoveText();
        return;
    }
    else if (UI_mode == UI_UPGRADE_PIXEL_SHADER)
    {
        if (chChar == 'y' || chChar == 'Y')
        {
            if (g_plugin.m_pState->m_nMinPSVersion == g_plugin.m_pState->m_nMaxPSVersion)
            {
                switch (g_plugin.m_pState->m_nMinPSVersion)
                {
                    case MD2_PS_NONE:
                        g_plugin.m_pState->m_nWarpPSVersion = MD2_PS_2_0;
                        g_plugin.m_pState->m_nCompPSVersion = MD2_PS_2_0;
                        g_plugin.m_pState->GenDefaultWarpShader();
                        g_plugin.m_pState->GenDefaultCompShader();
                        break;
                    case MD2_PS_2_0:
                        g_plugin.m_pState->m_nWarpPSVersion = MD2_PS_2_X;
                        g_plugin.m_pState->m_nCompPSVersion = MD2_PS_2_X;
                        break;
                    case MD2_PS_2_X:
                        g_plugin.m_pState->m_nWarpPSVersion = MD2_PS_4_0;
                        g_plugin.m_pState->m_nCompPSVersion = MD2_PS_4_0;
                        break;
                    case MD2_PS_4_0:
                        g_plugin.m_pState->m_nWarpPSVersion = MD2_PS_5_0;
                        g_plugin.m_pState->m_nCompPSVersion = MD2_PS_5_0;
                        break;
                    default:
                        assert(0);
                        break;
                }
            }
            else
            {
                switch (g_plugin.m_pState->m_nMinPSVersion)
                {
                    case MD2_PS_NONE:
                        if (g_plugin.m_pState->m_nWarpPSVersion < MD2_PS_2_0)
                        {
                            g_plugin.m_pState->m_nWarpPSVersion = MD2_PS_2_0;
                            g_plugin.m_pState->GenDefaultWarpShader();
                        }
                        if (g_plugin.m_pState->m_nCompPSVersion < MD2_PS_2_0)
                        {
                            g_plugin.m_pState->m_nCompPSVersion = MD2_PS_2_0;
                            g_plugin.m_pState->GenDefaultCompShader();
                        }
                        break;
                    case MD2_PS_2_0:
                        g_plugin.m_pState->m_nWarpPSVersion = std::max(g_plugin.m_pState->m_nWarpPSVersion, (int)MD2_PS_2_X);
                        g_plugin.m_pState->m_nCompPSVersion = std::max(g_plugin.m_pState->m_nCompPSVersion, (int)MD2_PS_2_X);
                        break;
                    case MD2_PS_2_X:
                        g_plugin.m_pState->m_nWarpPSVersion = std::max(g_plugin.m_pState->m_nWarpPSVersion, (int)MD2_PS_4_0);
                        g_plugin.m_pState->m_nCompPSVersion = std::max(g_plugin.m_pState->m_nCompPSVersion, (int)MD2_PS_4_0);
                        break;
                    case MD2_PS_5_0:
                        g_plugin.m_pState->m_nWarpPSVersion = std::max(g_plugin.m_pState->m_nWarpPSVersion, (int)MD2_PS_5_0);
                        g_plugin.m_pState->m_nCompPSVersion = std::max(g_plugin.m_pState->m_nCompPSVersion, (int)MD2_PS_5_0);
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
            g_plugin.m_pState->m_nMinPSVersion = std::min(g_plugin.m_pState->m_nWarpPSVersion, g_plugin.m_pState->m_nCompPSVersion);
            g_plugin.m_pState->m_nMaxPSVersion = std::max(g_plugin.m_pState->m_nWarpPSVersion, g_plugin.m_pState->m_nCompPSVersion);

            g_plugin.LoadShaders(&g_plugin.m_shaders, g_plugin.m_pState, false);
            g_plugin.SetMenusForPresetVersion(g_plugin.m_pState->m_nWarpPSVersion, g_plugin.m_pState->m_nCompPSVersion);
        }
        if (chChar != '\r')
        {
            UI_mode = UI_MENU;
            RemoveText();
        }
        return;
    }
    else if (UI_mode == UI_SAVE_OVERWRITE) // waiting to confirm overwrite file on save
    {
        if (chChar == 'y' || chChar == 'Y')
        {
            // First add pathname + extension to filename.
            wchar_t szNewFile[512] = {0};
            swprintf_s(szNewFile, L"%s%s.milk", g_plugin.GetPresetDir(), waitstring.szText);

            g_plugin.SavePresetAs(szNewFile);

            // Exit "WaitString" mode.
            UI_mode = UI_REGULAR;
            waitstring.bActive = false;
            //m_bPresetLockedByCode = false;
            RemoveText();
        }
        else if ((chChar >= ' ' && chChar <= 'z') || chChar == '\x1B') // '\x1B' is the ESCAPE key
        {
            // Go back to SAVE AS mode.
            UI_mode = UI_SAVEAS;
            waitstring.bActive = true;
            RemoveText();
        }
        return;
    }
    else if (UI_mode == UI_LOAD && ((chChar >= 'A' && chChar <= 'Z') || (chChar >= 'a' && chChar <= 'z')))
    {
        g_plugin.SeekToPreset(chChar);
        return;
    }
    else if (UI_mode == UI_MASHUP && chChar >= '1' && chChar <= ('0' + MASH_SLOTS))
    {
        g_plugin.m_nMashSlot = chChar - '1';
    }
    else // UI_REGULAR (mostly): normal handling of a simple keys (non-virtual keys)
    {
        switch (chChar)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                {
                    int digit = chChar - '0';
                    g_plugin.m_nNumericInputNum = (g_plugin.m_nNumericInputNum * 10) + digit;
                    g_plugin.m_nNumericInputDigits++;

                    if (g_plugin.m_nNumericInputDigits >= 2)
                    {
                        if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_CUST_MSG)
                            g_plugin.LaunchCustomMessage(g_plugin.m_nNumericInputNum);
                        else if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_SPRITE)
                            g_plugin.LaunchSprite(g_plugin.m_nNumericInputNum, -1);
                        else if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_SPRITE_KILL)
                        {
                            for (int x = 0; x < NUM_TEX; x++)
                                if (g_plugin.m_texmgr.m_tex[x].nUserData == g_plugin.m_nNumericInputNum)
                                    g_plugin.m_texmgr.KillTex(x);
                        }

                        g_plugin.m_nNumericInputDigits = 0;
                        g_plugin.m_nNumericInputNum = 0;
                    }
                }
                return;
            case 'q':
                g_plugin.m_pState->m_fVideoEchoZoom /= 1.05f;
                return;
            case 'Q':
                g_plugin.m_pState->m_fVideoEchoZoom *= 1.05f;
                return;
            case 'w':
                g_plugin.m_pState->m_nWaveMode++;
                if (g_plugin.m_pState->m_nWaveMode >= NUM_WAVES)
                    g_plugin.m_pState->m_nWaveMode = 0;
                return;
            case 'W':
                g_plugin.m_pState->m_nWaveMode--;
                if (g_plugin.m_pState->m_nWaveMode < 0)
                    g_plugin.m_pState->m_nWaveMode = NUM_WAVES - 1;
                return;
            case 'e':
                g_plugin.m_pState->m_fWaveAlpha -= 0.1f;
                if (g_plugin.m_pState->m_fWaveAlpha.eval(-1) < 0.0f)
                    g_plugin.m_pState->m_fWaveAlpha = 0.0f;
                return;
            case 'E':
                g_plugin.m_pState->m_fWaveAlpha += 0.1f;
                //if (g_plugin.m_pState->m_fWaveAlpha.eval(-1) > 1.0f)
                //    g_plugin.m_pState->m_fWaveAlpha = 1.0f;
                return;
            case 'i':
                g_plugin.m_pState->m_fZoom += 0.01f; //g_plugin.m_pState->m_fWarpAnimSpeed /= 1.1f;
                return;
            case 'I':
                g_plugin.m_pState->m_fZoom -= 0.01f; //g_plugin.m_pState->m_fWarpAnimSpeed *= 1.1f;
                return;
            case 'n':
            case 'N':
                g_plugin.m_bShowDebugInfo = !g_plugin.m_bShowDebugInfo;
                return;
            case 'r':
            case 'R':
                g_plugin.m_bSequentialPresetOrder = !g_plugin.m_bSequentialPresetOrder;
                {
                    LoadString(core_api::get_my_instance(), IDS_PRESET_ORDER_IS_NOW_X, &buf[64], 64);
                    LoadString(core_api::get_my_instance(), g_plugin.m_bSequentialPresetOrder ? IDS_SEQUENTIAL : IDS_RANDOM, &buf[128], 64);
                    swprintf_s(buf, 64, &buf[64], &buf[128]);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                // Erase all history, too.
                g_plugin.m_presetHistory[0] = g_plugin.m_szCurrentPresetFile;
                g_plugin.m_presetHistoryPos = 0;
                g_plugin.m_presetHistoryFwdFence = 1;
                g_plugin.m_presetHistoryBackFence = 0;
                return;
            case 'u': //g_plugin.m_pState->m_fWarpScale /= 1.1f; return;
            case 'U': //g_plugin.m_pState->m_fWarpScale *= 1.1f; return;
                {
                    const char* szMode = ToggleShuffle(chChar == 'u');
                    swprintf_s(buf, TEXT("Playback Order: %hs"), szMode);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                return;
            case 't':
            case 'T':
                g_plugin.LaunchSongTitleAnim();
                return;
            case 'o':
                g_plugin.m_pState->m_fWarpAmount /= 1.1f;
                return;
            case 'O':
                g_plugin.m_pState->m_fWarpAmount *= 1.1f;
                return;
            case '!':
                // Randomize warp shader.
                {
                    bool bWarpLock = g_plugin.m_bWarpShaderLock;
                    wchar_t szOldPreset[MAX_PATH];
                    wcscpy_s(szOldPreset, g_plugin.m_szCurrentPresetFile);
                    g_plugin.m_bWarpShaderLock = false;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bWarpShaderLock = true;
                    g_plugin.LoadPreset(szOldPreset, 0.0f);
                    g_plugin.m_bWarpShaderLock = bWarpLock;
                }
                return;
            case '@':
                // Randomize comp shader.
                {
                    bool bCompLock = g_plugin.m_bCompShaderLock;
                    wchar_t szOldPreset[MAX_PATH];
                    wcscpy_s(szOldPreset, g_plugin.m_szCurrentPresetFile);
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bCompShaderLock = true;
                    g_plugin.LoadPreset(szOldPreset, 0.0f);
                    g_plugin.m_bCompShaderLock = bCompLock;
                }
                return;
            case 'a':
            case 'A':
                // Load a random preset, a random warp shader and a random comp shader.
                // Not quite as extreme as a mash-up.
                {
                    bool bCompLock = g_plugin.m_bCompShaderLock;
                    bool bWarpLock = g_plugin.m_bWarpShaderLock;
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.m_bWarpShaderLock = false;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bCompShaderLock = true;
                    g_plugin.m_bWarpShaderLock = false;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.m_bWarpShaderLock = true;
                    g_plugin.LoadRandomPreset(0.0f);
                    g_plugin.m_bCompShaderLock = bCompLock;
                    g_plugin.m_bWarpShaderLock = bWarpLock;
                }
                return;
            case 'd':
            case 'D':
                if (!g_plugin.m_bCompShaderLock && !g_plugin.m_bWarpShaderLock)
                {
                    g_plugin.m_bCompShaderLock = true;
                    g_plugin.m_bWarpShaderLock = false;
                    LoadString(core_api::get_my_instance(), IDS_COMPSHADER_LOCKED, buf, 256);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                else if (g_plugin.m_bCompShaderLock && !g_plugin.m_bWarpShaderLock)
                {
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.m_bWarpShaderLock = true;
                    LoadString(core_api::get_my_instance(), IDS_WARPSHADER_LOCKED, buf, 256);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                else if (!g_plugin.m_bCompShaderLock && g_plugin.m_bWarpShaderLock)
                {
                    g_plugin.m_bCompShaderLock = true;
                    g_plugin.m_bWarpShaderLock = true;
                    LoadString(core_api::get_my_instance(), IDS_ALLSHADERS_LOCKED, buf, 256);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                else
                {
                    g_plugin.m_bCompShaderLock = false;
                    g_plugin.m_bWarpShaderLock = false;
                    LoadString(core_api::get_my_instance(), IDS_ALLSHADERS_UNLOCKED, buf, 256);
                    g_plugin.AddError(buf, 3.0f, ERR_NOTIFY, false);
                }
                return;
            /*
            case 'a':
                g_plugin.m_pState->m_fVideoEchoAlpha -= 0.1f;
                if (g_plugin.m_pState->m_fVideoEchoAlpha.eval(-1) < 0)
                    m_pState->m_fVideoEchoAlpha = 0.0f;
                return;
            case 'A':
                g_plugin.m_pState->m_fVideoEchoAlpha += 0.1f;
                if (g_plugin.m_pState->m_fVideoEchoAlpha.eval(-1) > 1.0f)
                    g_plugin.m_pState->m_fVideoEchoAlpha = 1.0f;
                return;
            case 'd':
                g_plugin.m_pState->m_fDecay += 0.01f;
                if (g_plugin.m_pState->m_fDecay.eval(-1) > 1.0f)
                    g_plugin.m_pState->m_fDecay = 1.0f;
                return;
            case 'D':
                g_plugin.m_pState->m_fDecay -= 0.01f;
                if (g_plugin.m_pState->m_fDecay.eval(-1) < 0.9f)
                    g_plugin.m_pState->m_fDecay = 0.9f;
                return;
            */
            case 'f':
            case 'F':
                g_plugin.m_pState->m_nVideoEchoOrientation = (g_plugin.m_pState->m_nVideoEchoOrientation + 1) % 4;
                return;
            case 'g':
                g_plugin.m_pState->m_fGammaAdj -= 0.1f;
                if (g_plugin.m_pState->m_fGammaAdj.eval(-1) < 0.0f)
                    g_plugin.m_pState->m_fGammaAdj = 0.0f;
                return;
            case 'G':
                g_plugin.m_pState->m_fGammaAdj += 0.1f;
                //if (g_plugin.m_pState->m_fGammaAdj > 1.0f)
                //    m_pState->m_fGammaAdj = 1.0f;
                return;
            case 'j':
                g_plugin.m_pState->m_fWaveScale *= 0.9f;
                return;
            case 'J':
                g_plugin.m_pState->m_fWaveScale /= 0.9f;
                return;
            case 'k':
            case 'K':
                {
                    if (bShiftHeldDown)
                        g_plugin.m_nNumericInputMode = NUMERIC_INPUT_MODE_SPRITE_KILL;
                    else
                        g_plugin.m_nNumericInputMode = NUMERIC_INPUT_MODE_SPRITE;
                    g_plugin.m_nNumericInputNum = 0;
                    g_plugin.m_nNumericInputDigits = 0;
                }
                return;
            case '[':
                g_plugin.m_pState->m_fXPush -= 0.005f;
                return;
            case ']':
                g_plugin.m_pState->m_fXPush += 0.005f;
                return;
            case '{':
                g_plugin.m_pState->m_fYPush -= 0.005f;
                return;
            case '}':
                g_plugin.m_pState->m_fYPush += 0.005f;
                return;
            case '<':
                g_plugin.m_pState->m_fRot += 0.02f;
                return;
            case '>':
                g_plugin.m_pState->m_fRot -= 0.02f;
                return;
            case 's':
            case 'S':
                // Save preset.
                if (UI_mode == UI_REGULAR)
                {
                    //g_plugin.m_bPresetLockedByCode = true;
                    UI_mode = UI_SAVEAS;

                    // Enter WaitString mode.
                    waitstring.bActive = true;
                    waitstring.bFilterBadChars = true;
                    waitstring.bDisplayAsCode = false;
                    waitstring.nSelAnchorPos = -1;
                    waitstring.nMaxLen = std::min(sizeof(waitstring.szText) - 1, static_cast<size_t>(MAX_PATH - wcsnlen_s(g_plugin.GetPresetDir(), MAX_PATH) - 6)); // 6 for the extension + null character because Win32 barfs if the path+filename+ext are greater than MAX_PATH characters
                    wcscpy_s(waitstring.szText, g_plugin.m_pState->m_szDesc); // initial string is the filename, minus the extension
                    LoadString(core_api::get_my_instance(), IDS_SAVE_AS, waitstring.szPrompt, 512);
                    waitstring.szToolTip[0] = L'\0';
                    waitstring.nCursorPos = wcsnlen_s(waitstring.szText, ARRAYSIZE(waitstring.szText)); // set the starting edit position

                    RemoveText();
                }
                return;
            case 'l':
            case 'L':
                // Load preset.
                if (UI_mode == UI_LOAD)
                {
                    UI_mode = UI_REGULAR;
                }
                else if (UI_mode == UI_REGULAR || UI_mode == UI_MENU)
                {
                    g_plugin.UpdatePresetList(); // make sure list is completely ready
                    UI_mode = UI_LOAD;
                    g_plugin.m_bUserPagedUp = false;
                    g_plugin.m_bUserPagedDown = false;
                }
                RemoveText();
                return;
            case 'm':
            case 'M':
                if (UI_mode == UI_MENU)
                    UI_mode = UI_REGULAR;
                else if (UI_mode == UI_REGULAR || UI_mode == UI_LOAD)
                    UI_mode = UI_MENU;
                RemoveText();
                return;
            case '-':
                SetPresetRating(-1.0f);
                return;
            case '+':
                SetPresetRating(1.0f);
                return;
            case '*':
                g_plugin.m_nNumericInputDigits = 0;
                g_plugin.m_nNumericInputNum = 0;
                return;
            case 'y':
            case 'Y':
                g_plugin.m_nNumericInputMode = NUMERIC_INPUT_MODE_CUST_MSG;
                g_plugin.m_nNumericInputNum = 0;
                g_plugin.m_nNumericInputDigits = 0;
                return;
            case 'z':
            case 'Z':
                m_playback_control->previous(); // Previous track button 40044
                return;
            case 'x':
            case 'X':
                m_playback_control->start();
                return;
            case 'c':
            case 'C':
                m_playback_control->toggle_pause();
                return;
            case 'v':
            case 'V':
                m_playback_control->stop();
                return;
            case 'b':
            case 'B':
                m_playback_control->next();
                return;
            case 'p':
            case 'P':
                TogglePlaylist();
                return;
            /*
            case 'l':
                PostMessage(m_hWndWinamp, WM_COMMAND, 40029, 0); // Open file dialog 40029
                return;
            case 'L':
                PostMessage(m_hWndWinamp, WM_COMMAND, 40187, 0); // ?
                return;
            case 'j':
                PostMessage(m_hWndWinamp, WM_COMMAND, 40194, 0); // Open jump to file dialog 40194
                return;
            */
            case 'h':
            case 'H':
                if (UI_mode == UI_MASHUP)
                {
                    if (chChar == 'h')
                    {
                        g_plugin.m_nMashPreset[g_plugin.m_nMashSlot] =
                            g_plugin.m_nDirs + (warand() % (g_plugin.m_nPresets - g_plugin.m_nDirs));
                        g_plugin.m_nLastMashChangeFrame[g_plugin.m_nMashSlot] =
                            g_plugin.GetFrame() + MASH_APPLY_DELAY_FRAMES; // causes instant apply
                    }
                    else
                    {
                        for (int mash = 0; mash < MASH_SLOTS; mash++)
                        {
                            g_plugin.m_nMashPreset[mash] = g_plugin.m_nDirs + (warand() % (g_plugin.m_nPresets - g_plugin.m_nDirs));
                            g_plugin.m_nLastMashChangeFrame[mash] = g_plugin.GetFrame() + MASH_APPLY_DELAY_FRAMES; // causes instant apply
                        }
                    }
                }
                else
                {
                    // Instant hard cut.
                    NextPreset(0.0f);
                    g_plugin.m_fHardCutThresh *= 2.0f; // make it a little less likely that a random hard cut follows soon
                }
                return;
        }
    }
}

void milk2_ui_element::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    MILK2_CONSOLE_LOG("OnKeyDown ", GetWnd())
    USHORT mask = 1 << (sizeof(SHORT) * 8 - 1); // get the highest-order bit
    bool bShiftHeldDown = (GetKeyState(VK_SHIFT) & mask) != 0; // or "< 0" without masking
    bool bCtrlHeldDown = (GetKeyState(VK_CONTROL) & mask) != 0; // or "< 0" without masking
    //bool bAltHeldDown: most keys come in under WM_SYSKEYDOWN when ALT is depressed.

    if (g_plugin.m_show_playlist) // in playlist mode
    {
        auto api = playlist_manager::get();
        switch (nChar)
        {
            case VK_ESCAPE:
                if (g_plugin.m_show_playlist)
                    TogglePlaylist();
                return;
            case VK_UP:
                if (GetKeyState(VK_SHIFT) & mask)
                    g_plugin.m_playlist_pos -= 10 * static_cast<LRESULT>(nRepCnt);
                else
                    g_plugin.m_playlist_pos -= nRepCnt;
                SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                return;
            case VK_DOWN:
                if (GetKeyState(VK_SHIFT) & mask)
                    g_plugin.m_playlist_pos += 10 * static_cast<LRESULT>(nRepCnt);
                else
                    g_plugin.m_playlist_pos += nRepCnt;
                SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                return;
            case VK_HOME:
                g_plugin.m_playlist_pos = 0;
                SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                return;
            case VK_END:
                {
                    const size_t count = api->activeplaylist_get_item_count();
                    g_plugin.m_playlist_pos = count - 1;
                    SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                }
                return;
            case VK_PRIOR:
                if (GetKeyState(VK_SHIFT) & mask)
                    g_plugin.m_playlist_pageups += 10;
                else
                    ++g_plugin.m_playlist_pageups;
                return;
            case VK_NEXT:
                if (GetKeyState(VK_SHIFT) & mask)
                    g_plugin.m_playlist_pageups -= 10;
                else
                    --g_plugin.m_playlist_pageups;
                return;
            case VK_RETURN:
                {
                    size_t active = api->get_active_playlist();
                    if (active == pfc::infinite_size)
                        return;
                    SetSelectionSingle(static_cast<size_t>(g_plugin.m_playlist_pos));
                    api->set_playing_playlist(active);
                    m_playback_control->start(playback_control::track_command_settrack);
                }
                return;
        }
    }
    else
    {
        switch (nChar)
        {
            case VK_F2:
                ToggleSongTitle();
                return;
            case VK_F3:
                ToggleSongLength();
                return;
            case VK_F4:
                TogglePresetInfo();
                return;
            case VK_F5:
                ToggleFps();
                return;
            case VK_F6:
                ToggleRating();
                return;
            case VK_F7:
                if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_CUST_MSG)
                    g_plugin.ReadCustomMessages(); // re-read custom messages
                return;
            case VK_F8:
                {
                    UI_mode = UI_CHANGEDIR;

                    // Enter WaitString mode.
                    waitstring.bActive = true;
                    waitstring.bFilterBadChars = false;
                    waitstring.bDisplayAsCode = false;
                    waitstring.nSelAnchorPos = -1;
                    waitstring.nMaxLen = std::min(sizeof(waitstring.szText) - 1, static_cast<size_t>(MAX_PATH - 1));
                    wcscpy_s(waitstring.szText, g_plugin.GetPresetDir());
                    {
                        // For subtle beauty, remove the trailing '\' from the directory name (if it's not just "x:\").
                        size_t len = wcsnlen_s(waitstring.szText, ARRAYSIZE(waitstring.szText));
                        if (len > 3 && waitstring.szText[len - 1] == '\\')
                            waitstring.szText[len - 1] = L'\0';
                    }
                    WASABI_API_LNGSTRINGW_BUF(IDS_DIRECTORY_TO_JUMP_TO, waitstring.szPrompt, 512);
                    waitstring.szToolTip[0] = L'\0';
                    waitstring.nCursorPos = wcsnlen_s(waitstring.szText, ARRAYSIZE(waitstring.szText)); // set the starting edit position

                    RemoveText();
                }
                return;
            case VK_F9:
                ToggleShaderHelp();
                return;
            case VK_SCROLL:
                {
                    SHORT lock = GetKeyState(VK_SCROLL) & 0x0001;
                    LockPreset(static_cast<bool>(lock));
                }
                return;
        }
    }

    if (waitstring.bActive) // Case 1: "WaitString" mode (for string-editing).
    {
        // Handle arrow keys, home, end, etc.
        if (nChar == VK_LEFT || nChar == VK_RIGHT || nChar == VK_HOME || nChar == VK_END || nChar == VK_UP || nChar == VK_DOWN)
        {
            if (bShiftHeldDown)
            {
                if (waitstring.nSelAnchorPos == -1)
                    waitstring.nSelAnchorPos = static_cast<int>(waitstring.nCursorPos);
            }
            else
            {
                waitstring.nSelAnchorPos = -1;
            }
        }

        if (bCtrlHeldDown) // copy/cut/paste
        {
            switch (nChar)
            {
                case 'c':
                case 'C':
                case VK_INSERT:
                    g_plugin.WaitString_Copy();
                    return;
                case 'x':
                case 'X':
                    g_plugin.WaitString_Cut();
                    return;
                case 'v':
                case 'V':
                    g_plugin.WaitString_Paste();
                    return;
                case VK_LEFT:
                    g_plugin.WaitString_SeekLeftWord();
                    return;
                case VK_RIGHT:
                    g_plugin.WaitString_SeekRightWord();
                    return;
                case VK_HOME:
                    waitstring.nCursorPos = 0;
                    return;
                case VK_END:
                    if (waitstring.bDisplayAsCode)
                    {
                        waitstring.nCursorPos = strnlen_s(reinterpret_cast<char*>(waitstring.szText), ARRAYSIZE(waitstring.szText) * sizeof(wchar_t) / sizeof(char));
                    }
                    else
                    {
                        waitstring.nCursorPos = wcsnlen_s(waitstring.szText, ARRAYSIZE(waitstring.szText));
                    }
                    return;
                case VK_RETURN:
                    if (waitstring.bDisplayAsCode)
                    {
                        //assert(g_plugin.m_pCurMenu);

                        // CTRL+ENTER accepts the string -> finished editing.
                        // `OnWaitStringAccept()` calls the callback function.
                        // See the calls to `CMenu::AddItem` from "milkdrop.cpp"
                        // to find the callback functions for different "WaitStrings".
                        g_plugin.m_pCurMenu->OnWaitStringAccept(waitstring.szText);
                        UI_mode = UI_MENU;
                        waitstring.bActive = false;
                        RemoveText();
                    }
                    return;
            }
        }
        else // "WaitString" mode key pressed and ctrl NOT held down
        {
            switch (nChar)
            {
                case VK_INSERT:
                    waitstring.bOvertypeMode = ~waitstring.bOvertypeMode;
                    return;
                case VK_LEFT:
                    for (UINT rep = 0; rep < nRepCnt; rep++)
                        if (waitstring.nCursorPos > 0)
                            waitstring.nCursorPos--;
                    return;
                case VK_RIGHT:
                    for (UINT rep = 0; rep < nRepCnt; rep++)
                    {
                        if (waitstring.bDisplayAsCode)
                        {
                            if (waitstring.nCursorPos < strnlen_s(reinterpret_cast<char*>(waitstring.szText), ARRAYSIZE(waitstring.szText) * sizeof(wchar_t) / sizeof(char)))
                                waitstring.nCursorPos++;
                        }
                        else
                        {
                            if (waitstring.nCursorPos < wcsnlen_s(waitstring.szText, ARRAYSIZE(waitstring.szText)))
                                waitstring.nCursorPos++;
                        }
                    }
                    return;
                case VK_HOME:
                    waitstring.nCursorPos -= g_plugin.WaitString_GetCursorColumn();
                    return;
                case VK_END:
                    waitstring.nCursorPos += g_plugin.WaitString_GetLineLength() - g_plugin.WaitString_GetCursorColumn();
                    return;
                case VK_UP:
                    for (UINT rep = 0; rep < nRepCnt; rep++)
                        g_plugin.WaitString_SeekUpOneLine();
                    return;
                case VK_DOWN:
                    for (UINT rep = 0; rep < nRepCnt; rep++)
                        g_plugin.WaitString_SeekDownOneLine();
                    return;
                case VK_BACK:
                    if (waitstring.nSelAnchorPos != -1)
                    {
                        g_plugin.WaitString_NukeSelection();
                    }
                    else if (waitstring.nCursorPos > 0)
                    {
                        size_t len;
                        if (waitstring.bDisplayAsCode)
                        {
                            len = strnlen_s(reinterpret_cast<char*>(waitstring.szText), ARRAYSIZE(waitstring.szText) * sizeof(wchar_t) / sizeof(char));
                        }
                        else
                        {
                            len = wcsnlen_s(waitstring.szText, ARRAYSIZE(waitstring.szText));
                        }
                        size_t src_pos = waitstring.nCursorPos;
                        int dst_pos = static_cast<int>(waitstring.nCursorPos - nRepCnt);
                        int gap = nRepCnt;
                        size_t copy_chars = len - waitstring.nCursorPos + 1; // includes NULL at end
                        if (dst_pos < 0)
                        {
                            gap += dst_pos;
                            //copy_chars += dst_pos;
                            dst_pos = 0;
                        }

                        if (waitstring.bDisplayAsCode)
                        {
                            char* ptr = reinterpret_cast<char*>(waitstring.szText);
                            for (unsigned int i = 0; i < copy_chars; i++)
                                *(ptr + dst_pos + i) = *(ptr + src_pos + i);
                        }
                        else
                        {
                            for (unsigned int i = 0; i < copy_chars; i++)
                                waitstring.szText[dst_pos + i] = waitstring.szText[src_pos + i];
                        }
                        waitstring.nCursorPos -= gap;
                    }
                    return;
                case VK_DELETE:
                    if (waitstring.nSelAnchorPos != -1)
                    {
                        g_plugin.WaitString_NukeSelection();
                    }
                    else
                    {
                        if (waitstring.bDisplayAsCode)
                        {
                            int len = static_cast<int>(strnlen_s(reinterpret_cast<char*>(waitstring.szText), ARRAYSIZE(waitstring.szText) * sizeof(wchar_t) / sizeof(char)));
                            char* ptr = reinterpret_cast<char*>(waitstring.szText);
                            for (int i = static_cast<int>(waitstring.nCursorPos); i <= std::abs(static_cast<int>(len - nRepCnt)); i++)
                                *(ptr + i) = *(ptr + i + nRepCnt);
                        }
                        else
                        {
                            int len = static_cast<int>(wcsnlen_s(waitstring.szText, ARRAYSIZE(waitstring.szText)));
                            for (int i = static_cast<int>(waitstring.nCursorPos); i <= std::abs(static_cast<int>(len - nRepCnt)); i++)
                                waitstring.szText[i] = waitstring.szText[i + nRepCnt];
                        }
                    }
                    return;
                case VK_RETURN:
                    if (UI_mode == UI_LOAD_RENAME) // rename (move) the file
                    {
                        // First add pathnames to filenames.
                        wchar_t szOldFile[512];
                        wchar_t szNewFile[512];
                        wcscpy_s(szOldFile, g_plugin.GetPresetDir());
                        wcscpy_s(szNewFile, g_plugin.GetPresetDir());
                        wcscat_s(szOldFile, g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str());
                        wcscat_s(szNewFile, waitstring.szText);
                        wcscat_s(szNewFile, L".milk");

                        g_plugin.RenamePresetFile(szOldFile, szNewFile);
                    }
                    else if (UI_mode == UI_IMPORT_WAVE || UI_mode == UI_EXPORT_WAVE ||
                             UI_mode == UI_IMPORT_SHAPE || UI_mode == UI_EXPORT_SHAPE)
                    {
                        //int bWave = (UI_mode == UI_IMPORT_WAVE || UI_mode == UI_EXPORT_WAVE);
                        int bImport = (UI_mode == UI_IMPORT_WAVE || UI_mode == UI_IMPORT_SHAPE);

                        LPARAM i = g_plugin.m_pCurMenu->GetCurItem()->m_lParam;
                        int ret = 0;
                        switch (UI_mode)
                        {
                            case UI_IMPORT_WAVE:
                                ret = g_plugin.m_pState->m_wave[i].Import(NULL, waitstring.szText, 0);
                                break;
                            case UI_EXPORT_WAVE:
                                ret = g_plugin.m_pState->m_wave[i].Export(NULL, waitstring.szText, 0);
                                break;
                            case UI_IMPORT_SHAPE:
                                ret = g_plugin.m_pState->m_shape[i].Import(NULL, waitstring.szText, 0);
                                break;
                            case UI_EXPORT_SHAPE:
                                ret = g_plugin.m_pState->m_shape[i].Export(NULL, waitstring.szText, 0);
                                break;
                        }

                        if (bImport)
                            g_plugin.m_pState->RecompileExpressions(1);

                        //m_fShowUserMessageUntilThisTime = GetTime() - 1.0f; // if there was an error message already, clear it
                        if (!ret)
                        {
                            wchar_t buf[1024];
                            if (UI_mode == UI_IMPORT_WAVE || UI_mode == UI_IMPORT_SHAPE)
                                WASABI_API_LNGSTRINGW_BUF(IDS_ERROR_IMPORTING_BAD_FILENAME, buf, 1024);
                            else
                                WASABI_API_LNGSTRINGW_BUF(IDS_ERROR_IMPORTING_BAD_FILENAME_OR_NOT_OVERWRITEABLE, buf, 1024);
                            /*g_plugin.AddError(WASABI_API_LNGSTRINGW(IDS_STRING_TOO_LONG), 2.5f, ERR_MISC, true);*/
                        }

                        UI_mode = UI_MENU;
                        waitstring.bActive = false;
                        RemoveText();
                        //m_bPresetLockedByCode = false;
                    }
                    else if (UI_mode == UI_SAVEAS)
                    {
                        // First add pathname + extension to filename.
                        wchar_t szNewFile[512];
                        swprintf_s(szNewFile, L"%s%s.milk", g_plugin.GetPresetDir(), waitstring.szText);

                        if (GetFileAttributes(szNewFile) != INVALID_FILE_ATTRIBUTES) // check if file already exists
                        {
                            // File already exists -> overwrite it?
                            UI_mode = UI_SAVE_OVERWRITE;
                            waitstring.bActive = false;
                            RemoveText();
                        }
                        else
                        {
                            g_plugin.SavePresetAs(szNewFile);

                            // Exit "WaitString" mode.
                            UI_mode = UI_REGULAR;
                            waitstring.bActive = false;
                            //m_bPresetLockedByCode = false;
                            RemoveText();
                        }
                    }
                    else if (UI_mode == UI_EDIT_MENU_STRING)
                    {
                        if (waitstring.bDisplayAsCode)
                        {
                            if (waitstring.nSelAnchorPos != -1)
                                g_plugin.WaitString_NukeSelection();

                            size_t len = strnlen_s(reinterpret_cast<char*>(waitstring.szText), ARRAYSIZE(waitstring.szText) * sizeof(wchar_t) / sizeof(char));
                            char* ptr = reinterpret_cast<char*>(waitstring.szText);
                            if (len + 1 < waitstring.nMaxLen)
                            {
                                // Insert a line feed. Use CTRL+RETURN to accept changes in this case.
                                for (size_t pos = len + 1; pos > waitstring.nCursorPos; pos--)
                                    *(ptr + pos) = *(ptr + pos - 1);
                                *(ptr + waitstring.nCursorPos++) = LINEFEED_CONTROL_CHAR;

                                //m_fShowUserMessageUntilThisTime = GetTime() - 1.0f; // if there was an error message already, clear it
                            }
                            else
                            {
                                // `m_waitstring.szText` has reached its limit.
                                /*g_plugin.AddError(WASABI_API_LNGSTRINGW(IDS_STRING_TOO_LONG), 2.5f, ERR_MISC, true);*/
                            }
                        }
                        else
                        {
                            // Finished editing.
                            //assert(m_pCurMenu);
                            g_plugin.m_pCurMenu->OnWaitStringAccept(waitstring.szText);
                            // OnWaitStringAccept calls the callback function.  See the
                            // calls to `CMenu::AddItem()` from "milkdrop.cpp" to find the
                            // callback functions for different "WaitStrings".
                            UI_mode = UI_MENU;
                            waitstring.bActive = false;
                            RemoveText();
                        }
                    }
                    else if (UI_mode == UI_CHANGEDIR)
                    {
                        //m_fShowUserMessageUntilThisTime = GetTime(); // if there was an error message already, clear it

                        // Change directory.
                        wchar_t szOldDir[512] = {0};
                        wchar_t szNewDir[512] = {0};
                        wcscpy_s(szOldDir, g_plugin.m_szPresetDir);
                        wcscpy_s(szNewDir, waitstring.szText);

                        size_t len = wcsnlen_s(szNewDir, 512);
                        if (len > 0 && szNewDir[len - 1] != L'\\')
                            wcscat_s(szNewDir, L"\\");

                        wcscpy_s(g_plugin.m_szPresetDir, szNewDir);

                        bool bSuccess = true;
                        if (GetFileAttributes(g_plugin.m_szPresetDir) == INVALID_FILE_ATTRIBUTES)
                            bSuccess = false;
                        if (bSuccess)
                        {
                            g_plugin.UpdatePresetList(false, true, false);
                            bSuccess = (g_plugin.m_nPresets > 0);
                        }

                        if (!bSuccess)
                        {
                            // New directory was invalid. Allow another try.
                            wcscpy_s(g_plugin.m_szPresetDir, szOldDir);

                            // Present a warning.
                            /*AddError(WASABI_API_LNGSTRINGW(IDS_INVALID_PATH), 3.5f, ERR_MISC, true);*/
                        }
                        else
                        {
                            // Success.
                            wcscpy_s(g_plugin.m_szPresetDir, szNewDir);

                            // Save new path to registry.
                            /*WritePrivateProfileString(L"settings", L"szPresetDir", g_plugin.m_szPresetDir, g_plugin.GetConfigIniFile());*/

                            // Set current preset index to -1 because current preset is no longer in the list.
                            g_plugin.m_nCurrentPreset = -1;

                            // Go to file load menu.
                            UI_mode = UI_LOAD;
                            waitstring.bActive = false;
                            RemoveText();

                            g_plugin.ClearErrors(ERR_MISC);
                        }
                    }
                    return;
                case VK_ESCAPE:
                    if (UI_mode == UI_LOAD_RENAME)
                    {
                        UI_mode = UI_LOAD;
                        waitstring.bActive = false;
                    }
                    else if (UI_mode == UI_SAVEAS || UI_mode == UI_SAVE_OVERWRITE ||
                             UI_mode == UI_EXPORT_SHAPE || UI_mode == UI_IMPORT_SHAPE ||
                             UI_mode == UI_EXPORT_WAVE || UI_mode == UI_IMPORT_WAVE)
                    {
                        UI_mode = UI_REGULAR;
                        waitstring.bActive = false;
                        //g_plugin.m_bPresetLockedByCode = false;
                    }
                    else if (UI_mode == UI_EDIT_MENU_STRING)
                    {
                        if (waitstring.bDisplayAsCode) // if were editing code...
                            UI_mode = UI_MENU; // return to menu
                        else
                            UI_mode = UI_REGULAR; // otherwise don't (we might have been editing a filename, for example)
                        waitstring.bActive = false;
                    }
                    else //if (UI_mode == UI_EDIT_MENU_STRING || UI_mode == UI_CHANGEDIR || 1)
                    {
                        UI_mode = UI_REGULAR;
                        waitstring.bActive = false;
                    }
                    RemoveText();
                    return;
            }
        }
    }
    else if (UI_mode == UI_MENU) // Case 2: menu is up and gets the keyboard input (menu navigation).
    {
        //assert(g_plugin.m_pCurMenu);
        if (g_plugin.m_pCurMenu->HandleKeydown(get_wnd(), WM_KEYDOWN, nChar, nRepCnt) == 0)
            return;
    }
    else // Case 3: Handle non-character keys (virtual keys) (normal case).
    {
        switch (nChar)
        {
            case VK_LEFT:
            case VK_RIGHT:
                if (UI_mode == UI_LOAD)
                {
                    // It is annoying when the music skips if left arrow is pressed
                    // from the Load menu, so instead, exit the menu.
                    if (nChar == VK_LEFT)
                    {
                        UI_mode = UI_REGULAR;
                        RemoveText();
                    }
                }
                else if (UI_mode == UI_UPGRADE_PIXEL_SHADER)
                {
                    UI_mode = UI_MENU;
                    RemoveText();
                }
                else if (UI_mode == UI_MASHUP)
                {
                    if (nChar == VK_LEFT)
                        g_plugin.m_nMashSlot = std::max(static_cast<WPARAM>(0), g_plugin.m_nMashSlot - 1);
                    else
                        g_plugin.m_nMashSlot = std::min(static_cast<WPARAM>(MASH_SLOTS - 1), g_plugin.m_nMashSlot + 1);
                }
                else
                {
                    Seek(nRepCnt, bShiftHeldDown, nChar == VK_LEFT ? -5.0 : 5.0);
                }
                return;
            case VK_ESCAPE:
                if (UI_mode == UI_LOAD || UI_mode == UI_MENU || UI_mode == UI_MASHUP)
                {
                    UI_mode = UI_REGULAR;
                    RemoveText();
                }
                else if (UI_mode == UI_LOAD_DEL)
                {
                    UI_mode = UI_LOAD;
                    RemoveText();
                }
                else if (UI_mode == UI_UPGRADE_PIXEL_SHADER)
                {
                    UI_mode = UI_MENU;
                    RemoveText();
                }
                else if (UI_mode == UI_SAVE_OVERWRITE)
                {
                    UI_mode = UI_SAVEAS;
                    // Return to "WaitString" mode, leaving all the parameters as they were before.
                    waitstring.bActive = true;
                    RemoveText();
                }
                /*
                else if (hwnd == g_plugin.GetPluginWindow()) // (don't close on ESC for text window)
                {
                    dumpmsg("User pressed ESCAPE");
                    //m_bExiting = true;
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                }
                */
                else if (g_plugin.m_show_help)
                {
                    ToggleHelp();
                }
                else if (s_fullscreen)
                {
                    ToggleFullScreen();
                }
                return;
            case VK_UP:
                if (UI_mode == UI_MASHUP)
                {
                    for (UINT rep = 0; rep < nRepCnt; rep++)
                        g_plugin.m_nMashPreset[g_plugin.m_nMashSlot] = std::max(g_plugin.m_nMashPreset[g_plugin.m_nMashSlot] - 1, g_plugin.m_nDirs);
                    g_plugin.m_nLastMashChangeFrame[g_plugin.m_nMashSlot] = g_plugin.GetFrame(); // causes delayed apply
                }
                else if (UI_mode == UI_LOAD)
                {
                    for (UINT rep = 0; rep < nRepCnt; rep++)
                        if (g_plugin.m_nPresetListCurPos > 0)
                            g_plugin.m_nPresetListCurPos--;

                    // Remember this preset's name so the next time they hit 'L' it jumps straight to it.
                    //wcscpy_s(g_plugin.m_szLastPresetSelected, g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str());
                }
                else
                {
                    m_playback_control->volume_up();
                }
                return;
            case VK_DOWN:
                if (UI_mode == UI_MASHUP)
                {
                    for (UINT rep = 0; rep < nRepCnt; rep++)
                        g_plugin.m_nMashPreset[g_plugin.m_nMashSlot] = std::min(g_plugin.m_nMashPreset[g_plugin.m_nMashSlot] + 1, g_plugin.m_nPresets - 1);
                    g_plugin.m_nLastMashChangeFrame[g_plugin.m_nMashSlot] = g_plugin.GetFrame(); // causes delayed apply
                }
                else if (UI_mode == UI_LOAD)
                {
                    for (UINT rep = 0; rep < nRepCnt; rep++)
                        if (g_plugin.m_nPresetListCurPos < g_plugin.m_nPresets - 1)
                            g_plugin.m_nPresetListCurPos++;

                    // Remember this preset's name so the next time they hit 'L' it jumps straight to it.
                    //wcscpy_s(g_plugin.m_szLastPresetSelected, g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str());
                }
                else
                {
                    m_playback_control->volume_down();
                }
                return;
            case VK_SPACE:
                if (UI_mode == UI_LOAD)
                    goto HitEnterFromLoadMenu;
                if (!IsPresetLock())
                    RandomPreset(s_config.settings.m_fBlendTimeUser); // BUG!: F1 help says this should be soft `NextPreset()`.
                return;
            case VK_PRIOR:
                if (UI_mode == UI_LOAD || UI_mode == UI_MASHUP)
                {
                    g_plugin.m_bUserPagedUp = true;
                    if (UI_mode == UI_MASHUP)
                        g_plugin.m_nLastMashChangeFrame[g_plugin.m_nMashSlot] = g_plugin.GetFrame(); // causes delayed apply
                }
                return;
            case VK_NEXT:
                if (UI_mode == UI_LOAD || UI_mode == UI_MASHUP)
                {
                    g_plugin.m_bUserPagedDown = true;
                    if (UI_mode == UI_MASHUP)
                        g_plugin.m_nLastMashChangeFrame[g_plugin.m_nMashSlot] = g_plugin.GetFrame(); // causes delayed apply
                }
                return;
            case VK_HOME:
                if (UI_mode == UI_LOAD)
                {
                    g_plugin.m_nPresetListCurPos = 0;
                }
                else if (UI_mode == UI_MASHUP)
                {
                    g_plugin.m_nMashPreset[g_plugin.m_nMashSlot] = g_plugin.m_nDirs;
                    g_plugin.m_nLastMashChangeFrame[g_plugin.m_nMashSlot] = g_plugin.GetFrame(); // causes delayed apply
                }
                return;
            case VK_END:
                if (UI_mode == UI_LOAD)
                {
                    g_plugin.m_nPresetListCurPos = g_plugin.m_nPresets - 1;
                }
                else if (UI_mode == UI_MASHUP)
                {
                    g_plugin.m_nMashPreset[g_plugin.m_nMashSlot] = g_plugin.m_nPresets - 1;
                    g_plugin.m_nLastMashChangeFrame[g_plugin.m_nMashSlot] = g_plugin.GetFrame(); // causes delayed apply
                }
                return;
            case VK_DELETE:
                if (UI_mode == UI_LOAD)
                {
                    if (g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str()[0] != '*') // can't delete directories
                    {
                        UI_mode = UI_LOAD_DEL;
                        RemoveText();
                    }
                }
                else //if (m_nNumericInputDigits == 0)
                {
                    if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_CUST_MSG)
                    {
                        g_plugin.m_nNumericInputDigits = 0;
                        g_plugin.m_nNumericInputNum = 0;

                        // Stop display of text message.
                        g_plugin.m_supertext.fStartTime = -1.0f;
                    }
                    else if (g_plugin.m_nNumericInputMode == NUMERIC_INPUT_MODE_SPRITE)
                    {
                        // Kill newest sprite (regular DELETE key)
                        // oldest sprite (SHIFT + DELETE),
                        // or all sprites (CTRL + SHIFT + DELETE).
                        g_plugin.m_nNumericInputDigits = 0;
                        g_plugin.m_nNumericInputNum = 0;

                        if (bShiftHeldDown && bCtrlHeldDown)
                        {
                            for (int x = 0; x < NUM_TEX; x++)
                                g_plugin.m_texmgr.KillTex(x);
                        }
                        else
                        {
                            int newest = -1;
                            int frame = 0;
                            for (int x = 0; x < NUM_TEX; x++)
                            {
                                if (g_plugin.m_texmgr.m_tex[x].pSurface)
                                {
                                    if ((newest == -1) || (!bShiftHeldDown && g_plugin.m_texmgr.m_tex[x].nStartFrame > frame) ||
                                        (bShiftHeldDown && g_plugin.m_texmgr.m_tex[x].nStartFrame < frame))
                                    {
                                        newest = x;
                                        frame = g_plugin.m_texmgr.m_tex[x].nStartFrame;
                                    }
                                }
                            }

                            if (newest != -1)
                                g_plugin.m_texmgr.KillTex(newest);
                        }
                    }
                }
                return;
            /*
            case VK_INSERT: // Rename.
                if (UI_mode == UI_LOAD)
                {
                    if (g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str()[0] != '*') // can't rename directories
                    {
                        // Go into rename mode.
                        UI_mode = UI_LOAD_RENAME;
                        waitstring.bActive = true;
                        waitstring.bFilterBadChars = true;
                        waitstring.bDisplayAsCode = false;
                        waitstring.nSelAnchorPos = -1;
                        waitstring.nMaxLen = std::min(sizeof(waitstring.szText) - 1, MAX_PATH - wcsnlen_s(g_plugin.GetPresetDir(), MAX_PATH) - 6); // 6 for the extension + null char.  We set this because Win32 LoadFile, MoveFile, etc. barf if the path+filename+ext are > MAX_PATH chars.

                        // Initial string is the filename, minus the extension.
                        wcscpy_s(waitstring.szText, g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str());
                        RemoveExtension(waitstring.szText);

                        // Set the prompt and tooltip.
                        swprintf_s(waitstring.szPrompt, WASABI_API_LNGSTRINGW(IDS_ENTER_THE_NEW_NAME_FOR_X), waitstring.szText);
                        waitstring.szToolTip[0] = L'\0';

                        // Set the starting edit position.
                        waitstring.nCursorPos = wcsnlen_s(waitstring.szText, ARRAYSIZE(waitstring.szText));
                        RemoveText();
                    }
                }
                return;
            */
            case VK_RETURN:
                if (UI_mode == UI_MASHUP)
                {
                    g_plugin.m_nLastMashChangeFrame[g_plugin.m_nMashSlot] = g_plugin.GetFrame() + MASH_APPLY_DELAY_FRAMES; // causes instant apply
                }
                else if (UI_mode == UI_LOAD)
                {
                HitEnterFromLoadMenu:
                    if (g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str()[0] == '*') // Change directory.
                    {
                        wchar_t* p = g_plugin.GetPresetDir();
                        if (wcscmp(g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str(), L"*..") == 0)
                        {
                            // Prevent full filesystem navigation.
                            if (wcscmp(s_config.settings.m_szPresetDir, p) == 0)
                            {
                                return;
                            }

                            // Back up one directory.
                            wchar_t* p2 = wcsrchr(p, L'\\');
                            if (p2)
                            {
                                *p2 = L'\0';
                                p2 = wcsrchr(p, L'\\');
                                if (p2)
                                    *(++p2) = L'\0';
                            }
                        }
                        else
                        {
                            // Open subdirectory.
                            wcscat_s(p, MAX_PATH, &g_plugin.m_presets[g_plugin.m_nPresetListCurPos].szFilename.c_str()[1]);
                            wcscat_s(p, MAX_PATH, L"\\");
                        }

                        //wcscpy_s(s_config.settings.m_szPresetDir, g_plugin.GetPresetDir());

                        g_plugin.UpdatePresetList(false, true, false);

                        // Set current preset index to -1 because current preset is no longer in the list.
                        g_plugin.m_nCurrentPreset = -1;
                    }
                    else // Load new preset.
                    {
                        g_plugin.m_nCurrentPreset = g_plugin.m_nPresetListCurPos;

                        // First take the filename and prepend the path (already has extension).
                        wchar_t s[MAX_PATH];
                        wcscpy_s(s, g_plugin.GetPresetDir()); // note: m_szPresetDir always ends with '\'
                        wcscat_s(s, g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename.c_str());

                        // Now load (and blend to) the new preset.
                        g_plugin.m_presetHistoryPos = (g_plugin.m_presetHistoryPos + 1) % PRESET_HIST_LEN;
                        g_plugin.LoadPreset(s, (nChar == VK_SPACE) ? g_plugin.m_fBlendTimeUser : 0.0f);
                    }
                }
                return;
            case VK_BACK:
                PrevPreset(0.0f);
                g_plugin.m_fHardCutThresh *= 2.0f; // make it a little less likely that a random hard cut follows soon.
                return;
            case 'T':
            case 'Y':
                if (bCtrlHeldDown)
                {
                    // Stop display of custom message or song title.
                    g_plugin.m_supertext.fStartTime = -1.0f;
                }
                return;
            case 'K':
                if (bCtrlHeldDown)
                {
                    // Kill all sprites.
                    for (int x = 0; x < NUM_TEX; x++)
                        if (g_plugin.m_texmgr.m_tex[x].pSurface)
                            g_plugin.m_texmgr.KillTex(x);
                }
                return;
            case VK_F1:
                //g_plugin.m_show_press_f1_msg = 0u;
                ToggleHelp();
                return;
            case VK_SUBTRACT:
                SetPresetRating(-1.0f);
                return;
            case VK_ADD:
                SetPresetRating(1.0f);
                return;
            default:
                // Pass CTRL+A thru CTRL+Z, and also CTRL+TAB, to Winamp, *if we're in windowed mode* and using an embedded window.
                // be careful though; uppercase chars come both here AND to WM_CHAR handler,
                // so we have to eat some of them here, to avoid them from acting twice.
                if (g_plugin.GetScreenMode() == WINDOWED && g_plugin.m_lpDX && g_plugin.m_lpDX->m_current_mode.m_skin)
                {
                    if (bCtrlHeldDown && ((nChar >= 'A' && nChar <= 'Z') || nChar == VK_TAB))
                    {
                        //OnKeyDown((UINT)wParam, (UINT)lParam & 0xFFFF, (UINT)((lParam & 0xFFFF0000) >> 16));
                        //PostMessage(WM_KEYDOWN, nChar, lParam);
                        return;
                    }
                }
        }
    }
}

void milk2_ui_element::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    MILK2_CONSOLE_LOG("OnSysKeyDown ", GetWnd())
    // Bit 29: The context code. The value is 1 if the ALT key is down while the
    //         key is pressed; it is 0 if the WM_SYSKEYDOWN message is posted to
    //         the active window because no window has the keyboard focus.
    // Bit 30: The previous key state. The value is 1 if the key is down before
    //         the message is sent, or it is 0 if the key is up.
    if ((nChar == VK_RETURN && (nFlags & 0x6000) == 0x2000) && g_plugin.GetFrame() > 0)
    {
        ToggleFullScreen();
        return;
    }
}

void milk2_ui_element::OnSysChar(TCHAR chChar, UINT nRepCnt, UINT nFlags)
{
    MILK2_CONSOLE_LOG("OnSysChar ", GetWnd())
    if (chChar == 'k' || chChar == 'K')
    {
        ShowPreferencesPage();
        g_plugin.OnAltK(); // Leave in as easter egg
        return;
    }
#if 0
    if ((chChar == 'd' || chChar == 'D') && g_plugin.GetFrame() > 0)
    {
        g_plugin.ToggleDesktop();
        return;
    }
#endif
}

void milk2_ui_element::OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    MILK2_CONSOLE_LOG("OnCommand ", GetWnd())
    if (g_plugin.GetScreenMode() == WINDOWED)
    {
        switch (nID)
        {
            case ID_QUIT:
                g_plugin.m_exiting = 1;
                PostMessage(WM_CLOSE, (WPARAM)0, (LPARAM)0);
                return;
            //case ID_DESKTOP_MODE:
            //    if (g_plugin.GetFrame() > 0)
            //        ToggleDesktop();
            //    return;
            case ID_SHOWHELP:
                ToggleHelp();
                return;
            case ID_SHOWPLAYLIST:
                TogglePlaylist();
                return;
            case ID_VIS_NEXT:
                NextPreset(g_plugin.m_fBlendTimeUser);
                return;
            case ID_VIS_PREV:
                PrevPreset(g_plugin.m_fBlendTimeUser);
                return;
            case ID_VIS_RANDOM:
                {   /*
                    // Note: When the visualization is launched using a fancy modern skin
                    //       (with a Random button), it will send one of these...
                    //       If it's NOT a fancy skin, will never get this message (confirmed).
                    USHORT v = uNotifyCode; // here, v is 0 (locked) or 1 (random) or 0xFFFF (don't know / startup!)
                    if (v == 0xFFFF)
                    {
                        // Plugin just launched or changed modes -
                        // Winamp wants to know what our saved Random state is...
                        SendMessage(g_plugin.GetWinampWindow(), WM_WA_IPC, (g_plugin.m_bPresetLockOnAtStartup ? 0 : 1) << 16, IPC_CB_VISRANDOM);

                        return;
                    }

                    // otherwise it's 0 or 1 - user clicked the button, respond.
                    v = v ? 1 : 0; // same here

                    //see also - IPC_CB_VISRANDOM
                    g_plugin.m_bPresetLockedByUser = (v == 0);
                    SetScrollLock(g_plugin.m_bPresetLockedByUser, g_plugin.m_bPreventScollLockHandling);
                    */
                    return;
                }
            case ID_VIS_FS:
                if (g_plugin.GetFrame() > 0)
                    ToggleFullScreen(); //PostMessage(WM_USER + 1667, 0, 0);
                return;
            case ID_VIS_CFG:
                ShowPreferencesPage(); //ToggleHelp();
                return;
            case ID_VIS_MENU:
                POINT pt;
                GetCursorPos(&pt);
                SendMessage(WM_CONTEXTMENU, (WPARAM)get_wnd(), (pt.y << 16) | pt.x);
                return;
        }
    }
}

UINT milk2_ui_element::OnGetDlgCode(LPMSG lpMsg)
{
    MILK2_CONSOLE_LOG("OnGetDlgCode ", GetWnd())
    //if (lpMsg && lpMsg->message == WM_KEYDOWN)
    //{
    //    switch (lpMsg->wParam)
    //    {
    //        case VK_ESCAPE:
    //            if (g_plugin.m_show_help)
    //                ToggleHelp();
    //            else if (s_fullscreen)
    //                ToggleFullScreen();
    //            break;
    //    }
    //}
    return WM_GETDLGCODE;
}

void milk2_ui_element::OnSetFocus(CWindow wndOld)
{
    MILK2_CONSOLE_LOG("OnSetFocus ", GetWnd())
    //g_plugin.m_bOrigScrollLockState = GetKeyState(VK_SCROLL) & 1;
    //SetScrollLock(g_plugin.m_bMilkdropScrollLockState);
}

void milk2_ui_element::OnKillFocus(CWindow wndFocus)
{
    MILK2_CONSOLE_LOG("OnKillFocus ", GetWnd())
    //g_plugin.m_bMilkdropScrollLockState = GetKeyState(VK_SCROLL) & 1;
    //SetScrollLock(g_plugin.m_bOrigScrollLockState);
}

#undef waitstring
#undef UI_mode
#undef RemoveText
#pragma endregion