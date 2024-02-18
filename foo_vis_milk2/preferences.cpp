/*
 *  preferences.cpp - Configuration settings for the MilkDrop 2
 *                    visualization component accessible through a
 *                    preferences page and advanced preferences.
 *
 *  Copyright (c) 2023-2024 Jimmy Cassis
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "pch.h"
#include "config.h"

extern HWND g_hWindow;

// clang-format off
static cfg_bool cfg_bPresetLockOnAtStartup(guid_cfg_bPresetLockOnAtStartup, default_bPresetLockOnAtStartup);
static cfg_bool cfg_bPreventScollLockHandling(guid_cfg_bPreventScollLockHandling, default_bPreventScollLockHandling);
static cfg_bool cfg_bWarningsDisabled(guid_cfg_bWarningsDisabled, default_bWarningsDisabled2);
static cfg_bool cfg_bEnableRating(guid_cfg_bEnableRating, !default_bEnableRating);
static cfg_bool cfg_bShowPressF1ForHelp(guid_cfg_bShowPressF1ForHelp, default_bShowPressF1ForHelp);
static cfg_bool cfg_allow_page_tearing_fs(guid_cfg_allow_page_tearing_fs, default_allow_page_tearing_fs);
static cfg_bool cfg_bSongTitleAnims(guid_cfg_bSongTitleAnims, default_bSongTitleAnims);
static cfg_bool cfg_bAutoGamma(guid_cfg_bAutoGamma, default_bAutoGamma);
static cfg_bool cfg_bHardCutsDisabled(guid_cfg_bHardCutsDisabled, default_bHardCutsDisabled);
static cfg_int cfg_max_fps_fs(guid_cfg_max_fps_fs, static_cast<int64_t>(default_max_fps_fs));
static cfg_int cfg_n16BitGamma(guid_cfg_n16BitGamma, static_cast<int64_t>(default_n16BitGamma));
static cfg_int cfg_nMaxBytes(guid_cfg_nMaxBytes, static_cast<int64_t>(default_nMaxBytes));
static cfg_int cfg_nMaxImages(guid_cfg_nMaxImages, static_cast<int64_t>(default_nMaxImages));
static cfg_int cfg_nCanvasStretch(guid_cfg_nCanvasStretch, static_cast<int64_t>(default_nCanvasStretch));
static cfg_int cfg_nGridX(guid_cfg_nGridX, static_cast<int64_t>(default_nGridX));
static cfg_int cfg_nTexSizeX(guid_cfg_nTexSizeX, static_cast<int64_t>(default_nTexSizeX));
static cfg_int cfg_nTexBitsPerCh(guid_cfg_nTexBitsPerCh, static_cast<int64_t>(default_nTexBitsPerCh));
static cfg_int cfg_nMaxPSVersion(guid_cfg_nMaxPSVersion, static_cast<int64_t>(default_nMaxPSVersion));
static cfg_float cfg_fTimeBetweenPresets(guid_cfg_fTimeBetweenPresets, static_cast<double>(default_fTimeBetweenPresets));
static cfg_float cfg_fTimeBetweenPresetsRand(guid_cfg_fTimeBetweenPresetsRand, static_cast<double>(default_fTimeBetweenPresetsRand));
static cfg_float cfg_fBlendTimeAuto(guid_cfg_fBlendTimeAuto, static_cast<double>(default_fBlendTimeAuto));
static cfg_float cfg_fBlendTimeUser(guid_cfg_fBlendTimeUser, static_cast<double>(default_fBlendTimeUser));
static cfg_float cfg_fHardCutHalflife(guid_cfg_fHardCutHalflife, static_cast<double>(default_fHardCutHalflife));
static cfg_float cfg_fHardCutLoudnessThresh(guid_cfg_fHardCutLoudnessThresh, static_cast<double>(default_fHardCutLoudnessThresh));
static cfg_float cfg_fSongTitleAnimDuration(guid_cfg_fSongTitleAnimDuration, static_cast<double>(default_fSongTitleAnimDuration));
static cfg_float cfg_fTimeBetweenRandomSongTitles(guid_cfg_fTimeBetweenRandomSongTitles, static_cast<double>(default_fTimeBetweenRandomSongTitles));
static cfg_float cfg_fTimeBetweenRandomCustomMsgs(guid_cfg_fTimeBetweenRandomCustomMsgs, static_cast<double>(default_fTimeBetweenRandomCustomMsgs));
static advconfig_branch_factory g_advconfigBranch("MilkDrop", guid_advconfig_branch, advconfig_branch::guid_branch_vis, 0);
static advconfig_checkbox_factory cfg_bDebugOutput("Debug output", "milk2.bDebugOutput", guid_cfg_bDebugOutput, guid_advconfig_branch, order_bDebugOutput, default_bDebugOutput);
static advconfig_string_factory cfg_szPresetDir("Preset directory", "milk2.szPresetDir", guid_cfg_szPresetDir, guid_advconfig_branch, order_szPresetDir, "");
// clang-format on

#pragma region Preferences Page
BOOL milk2_preferences_page::OnInitDialog(CWindow, LPARAM)
{
    WCHAR buf[256] = {0};
    HWND ctrl = nullptr;
    int n = 0;

    m_dark.AddDialogWithControls(*this);

    // Loose checkboxes.
    CheckDlgButton(IDC_CB_SCROLLON3, static_cast<UINT>(cfg_bPresetLockOnAtStartup));
    CheckDlgButton(IDC_CB_SCROLLON4, static_cast<UINT>(cfg_bPreventScollLockHandling));
    CheckDlgButton(IDC_CB_NOWARN3, static_cast<UINT>(cfg_bWarningsDisabled));
    CheckDlgButton(IDC_CB_NORATING2, static_cast<UINT>(!cfg_bEnableRating));
    CheckDlgButton(IDC_CB_PRESS_F1_MSG, static_cast<UINT>(cfg_bShowPressF1ForHelp));

    // Maximum FPS.
    CheckDlgButton(IDC_CB_FSPT, static_cast<UINT>(cfg_allow_page_tearing_fs));
    UpdateMaxFps(FULLSCREEN);

    // Soft cuts.
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fTimeBetweenPresets));
    SetDlgItemText(IDC_BETWEEN_TIME, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fTimeBetweenPresetsRand));
    SetDlgItemText(IDC_BETWEEN_TIME_RANDOM, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fBlendTimeAuto));
    SetDlgItemText(IDC_BLEND_AUTO, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fBlendTimeUser));
    SetDlgItemText(IDC_BLEND_USER, buf);

    // Hard cuts.
    swprintf_s(buf, L" %2.1f", static_cast<float>(cfg_fHardCutHalflife));
    SetDlgItemText(IDC_HARDCUT_BETWEEN_TIME, buf);

    n = static_cast<int>((static_cast<float>(cfg_fHardCutLoudnessThresh) - 1.25f) * 10.0f);
    if (n < 0)
        n = 0;
    if (n > 20)
        n = 20;
    ctrl = GetDlgItem(IDC_HARDCUT_LOUDNESS);
    SendMessage(ctrl, TBM_SETRANGEMIN, (WPARAM)FALSE, (LPARAM)0);
    SendMessage(ctrl, TBM_SETRANGEMAX, (WPARAM)FALSE, (LPARAM)20);
    SendMessage(ctrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)n);

    CheckDlgButton(IDC_CB_HARDCUTS, static_cast<UINT>(cfg_bHardCutsDisabled));

    // Pixel shaders.
    ctrl = GetDlgItem(IDC_SHADERS);
    AddItem(ctrl, buf, IDS_PS_AUTO_RECOMMENDED, unsigned(-1));
    AddItem(ctrl, buf, IDS_PS_DISABLED, MD2_PS_NONE);
    AddItem(ctrl, buf, IDS_PS_SHADER_MODEL_2, MD2_PS_2_0);
    AddItem(ctrl, buf, IDS_PS_SHADER_MODEL_3, MD2_PS_3_0);
    SelectItemByPos(ctrl, 0); // as a safe default
    SelectItemByValue(ctrl, static_cast<DWORD>(cfg_nMaxPSVersion));

    // Texture format.
    //ctrl = GetDlgItem(IDC_TEXFORMAT);
    //AddItem(ctrl, buf, IDS_TX_8_BITS_PER_CHANNEL, 8);
    ////AddItem(ctrl, " 10 bits per channel", 10);
    //AddItem(ctrl, buf, IDS_TX_16_BITS_PER_CHANNEL, 16);
    //AddItem(ctrl, buf, IDS_TX_32_BITS_PER_CHANNEL, 32);
    //SelectItemByPos(ctrl, 0); // as a safe default
    //SelectItemByValue(ctrl, static_cast<DWORD>(cfg_nTexBitsPerCh));

    // Mesh size.
    ctrl = GetDlgItem(IDC_MESHSIZECOMBO);
    AddItem(ctrl, buf, IDS_8X6_FAST, 8);
    AddItem(ctrl, buf, IDS_16X12_FAST, 16);
    AddItem(ctrl, buf, IDS_24X18, 24);
    AddItem(ctrl, buf, IDS_32X24, 32);
    AddItem(ctrl, buf, IDS_40X30, 40);
    AddItem(ctrl, buf, IDS_48X36_DEFAULT, 48);
    AddItem(ctrl, buf, IDS_64X48_SLOW, 64);
    AddItem(ctrl, buf, IDS_80X60_SLOW, 80);
    AddItem(ctrl, buf, IDS_96X72_SLOW, 96);
    AddItem(ctrl, buf, IDS_128X96_SLOW, 128);
    AddItem(ctrl, buf, IDS_160X120_SLOW, 160);
    AddItem(ctrl, buf, IDS_192X144_SLOW, 192);
    SelectItemByPos(ctrl, 0); // as a safe default
    SelectItemByValue(ctrl, static_cast<DWORD>(cfg_nGridX));

    // Canvas stretch.
    ctrl = GetDlgItem(IDC_STRETCH2);
    AddItem(ctrl, buf, IDS_AUTO, 0);
    AddItem(ctrl, buf, IDS_NONE_BEST_IMAGE_QUALITY, 100);
    AddItem(ctrl, buf, IDS_1_25_X, 125);
    AddItem(ctrl, buf, IDS_1_33_X, 133);
    AddItem(ctrl, buf, IDS_1_5_X, 150);
    AddItem(ctrl, buf, IDS_1_67_X, 167);
    AddItem(ctrl, buf, IDS_2_X, 200);
    AddItem(ctrl, buf, IDS_3_X, 300);
    AddItem(ctrl, buf, IDS_4_X, 400);
    SelectItemByPos(ctrl, 0); // as a safe default
    SelectItemByValue(ctrl, static_cast<DWORD>(cfg_nCanvasStretch));

    // Texture size.
    ctrl = GetDlgItem(IDC_TEXSIZECOMBO);
    LRESULT nPos = 0;
    for (int i = 0; i < 5; ++i)
    {
        int size = static_cast<int>(pow(2.0, i + 8));
        swprintf_s(buf, L" %4d x %4d ", size, size);
        nPos = SendMessage(ctrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)buf);
        SendMessage(ctrl, CB_SETITEMDATA, (WPARAM)nPos, (LPARAM)size);
    }
    LoadString(core_api::get_my_instance(), IDS_NEAREST_POWER_OF_2, buf, 256);
    nPos = SendMessage(ctrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)buf);
    LoadString(core_api::get_my_instance(), IDS_EXACT_RECOMMENDED, buf, 256);
    SendMessage(ctrl, CB_SETITEMDATA, (WPARAM)nPos, (LPARAM)(-2));
    nPos = SendMessage(ctrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)buf);
    SendMessage(ctrl, CB_SETITEMDATA, (WPARAM)nPos, (LPARAM)(-1));
    for (int i = 0; i < 5 + 2; ++i)
    {
        LRESULT size = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)i, (LPARAM)0);
        if (size == static_cast<LRESULT>(cfg_nTexSizeX))
            SendMessage(ctrl, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
    }

    // 16-bit brightness.
    ctrl = GetDlgItem(IDC_BRIGHT_SLIDER2);
    SendMessage(ctrl, TBM_SETRANGEMIN, (WPARAM)FALSE, (LPARAM)0);
    SendMessage(ctrl, TBM_SETRANGEMAX, (WPARAM)FALSE, (LPARAM)4);
    SendMessage(ctrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)cfg_n16BitGamma);
    for (int i = 0; i < 5; ++i)
        SendMessage(ctrl, TBM_SETTIC, (WPARAM)0, (LPARAM)i);

    CheckDlgButton(IDC_CB_AUTOGAMMA2, static_cast<UINT>(cfg_bAutoGamma));

    // Image cache maximum bytes.
    ctrl = GetDlgItem(IDC_MAX_BYTES2);
    LoadString(core_api::get_my_instance(), IDS_AUTO, buf, 256);
    AddItem(ctrl, buf, unsigned(-1));
    AddItem(ctrl, FormImageCacheSizeString(L"   0", IDS_MB), 0);
    AddItem(ctrl, FormImageCacheSizeString(L"   1", IDS_MB), 1000000);
    AddItem(ctrl, FormImageCacheSizeString(L"   2", IDS_MB), 2000000);
    AddItem(ctrl, FormImageCacheSizeString(L"   3", IDS_MB), 3000000);
    AddItem(ctrl, FormImageCacheSizeString(L"   4", IDS_MB), 4000000);
    AddItem(ctrl, FormImageCacheSizeString(L"   6", IDS_MB), 6000000);
    AddItem(ctrl, FormImageCacheSizeString(L"   9", IDS_MB), 8000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  10", IDS_MB), 10000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  12", IDS_MB), 12000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  14", IDS_MB), 14000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  16", IDS_MB), 16000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  20", IDS_MB), 20000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  24", IDS_MB), 24000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  28", IDS_MB), 28000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  32", IDS_MB), 32000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  40", IDS_MB), 40000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  48", IDS_MB), 48000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  56", IDS_MB), 56000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  64", IDS_MB), 64000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  80", IDS_MB), 80000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  96", IDS_MB), 96000000);
    AddItem(ctrl, FormImageCacheSizeString(L" 128", IDS_MB), 128000000);
    AddItem(ctrl, FormImageCacheSizeString(L" 160", IDS_MB), 160000000);
    AddItem(ctrl, FormImageCacheSizeString(L" 192", IDS_MB), 192000000);
    AddItem(ctrl, FormImageCacheSizeString(L" 224", IDS_MB), 224000000);
    AddItem(ctrl, FormImageCacheSizeString(L" 256", IDS_MB), 256000000);
    AddItem(ctrl, FormImageCacheSizeString(L" 384", IDS_MB), 384000000);
    AddItem(ctrl, FormImageCacheSizeString(L" 512", IDS_MB), 512000000);
    AddItem(ctrl, FormImageCacheSizeString(L" 768", IDS_MB), 768000000);
    AddItem(ctrl, FormImageCacheSizeString(L"     1", IDS_GB), 1000000000);
    AddItem(ctrl, FormImageCacheSizeString(L"1.25", IDS_GB), 1250000000);
    AddItem(ctrl, FormImageCacheSizeString(L"  1.5", IDS_GB), 1500000000);
    AddItem(ctrl, FormImageCacheSizeString(L"1.75", IDS_GB), 1750000000);
    AddItem(ctrl, FormImageCacheSizeString(L"      2", IDS_GB), 2000000000);
    SelectItemByPos(ctrl, 0); // as a safe default
    SelectItemByValue(ctrl, static_cast<DWORD>(cfg_nMaxBytes));

    // Image cache maximum number of images.
    ctrl = GetDlgItem(IDC_MAX_IMAGES2);
    LoadString(core_api::get_my_instance(), IDS_AUTO, buf, 256);
    AddItem(ctrl, buf, unsigned(-1));
    AddItem(ctrl, L"    0 ", 0);
    AddItem(ctrl, L"    1 ", 1);
    AddItem(ctrl, L"    2 ", 2);
    AddItem(ctrl, L"    3 ", 3);
    AddItem(ctrl, L"    4 ", 4);
    AddItem(ctrl, L"    6 ", 6);
    AddItem(ctrl, L"    8 ", 8);
    AddItem(ctrl, L"   10 ", 10);
    AddItem(ctrl, L"   12 ", 12);
    AddItem(ctrl, L"   14 ", 14);
    AddItem(ctrl, L"   16 ", 16);
    AddItem(ctrl, L"   20 ", 20);
    AddItem(ctrl, L"   24 ", 24);
    AddItem(ctrl, L"   28 ", 28);
    AddItem(ctrl, L"   32 ", 32);
    AddItem(ctrl, L"   40 ", 40);
    AddItem(ctrl, L"   48 ", 48);
    AddItem(ctrl, L"   56 ", 56);
    AddItem(ctrl, L"   64 ", 64);
    AddItem(ctrl, L"   80 ", 80);
    AddItem(ctrl, L"   96 ", 96);
    AddItem(ctrl, L"  128 ", 128);
    AddItem(ctrl, L"  160 ", 160);
    AddItem(ctrl, L"  192 ", 192);
    AddItem(ctrl, L"  224 ", 224);
    AddItem(ctrl, L"  256 ", 256);
    AddItem(ctrl, L"  384 ", 384);
    AddItem(ctrl, L"  512 ", 512);
    AddItem(ctrl, L"  768 ", 768);
    AddItem(ctrl, L" 1024 ", 1024);
    AddItem(ctrl, L" 1536 ", 1536);
    AddItem(ctrl, L" 2048 ", 2048);
    SelectItemByPos(ctrl, 0); // as a safe default
    SelectItemByValue(ctrl, static_cast<DWORD>(cfg_nMaxImages));

    // Song title and custom animations.
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fSongTitleAnimDuration));
    SetDlgItemText(IDC_SONGTITLEANIM_DURATION, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fTimeBetweenRandomSongTitles));
    SetDlgItemText(IDC_RAND_TITLE, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fTimeBetweenRandomCustomMsgs));
    SetDlgItemText(IDC_RAND_MSG, buf);
    CheckDlgButton(IDC_CB_TITLE_ANIMS, static_cast<UINT>(cfg_bSongTitleAnims));

    return FALSE;
}

void milk2_preferences_page::OnEditNotification(UINT, int, CWindow)
{
    OnChanged();
}

void milk2_preferences_page::OnButtonClick(UINT, int, CWindow)
{
    OnChanged();
}

void milk2_preferences_page::OnComboChange(UINT, int, CWindow)
{
    OnChanged();
}

void milk2_preferences_page::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    switch (pScrollBar.GetDlgCtrlID())
    {
        case IDC_HARDCUT_LOUDNESS:
        case IDC_BRIGHT_SLIDER2:
            switch (nSBCode)
            {
                //case TB_BOTTOM:
                //case TB_LINEDOWN:
                //case TB_LINEUP:
                //case TB_PAGEDOWN:
                //case TB_PAGEUP:
                //case TB_THUMBPOSITION:
                //case TB_THUMBTRACK:
                //case TB_TOP:
                case TB_ENDTRACK:
                    OnChanged();
            }
    }
}

uint32_t milk2_preferences_page::get_state()
{
    // IMPORTANT: Always return "dark_mode_supported" to tell foobar2000 that this preferences page is dark mode compliant.
    uint32_t state = preferences_state::resettable | preferences_state::dark_mode_supported;
    if (HasChanged())
        state |= preferences_state::changed;
    return state;
}

void milk2_preferences_page::reset()
{
    WCHAR buf[256] = {0};
    int n = 0;

    CheckDlgButton(IDC_CB_SCROLLON3, static_cast<UINT>(cfg_bPresetLockOnAtStartup));
    CheckDlgButton(IDC_CB_SCROLLON4, static_cast<UINT>(cfg_bPreventScollLockHandling));
    CheckDlgButton(IDC_CB_NOWARN3, static_cast<UINT>(cfg_bWarningsDisabled));
    CheckDlgButton(IDC_CB_NORATING2, static_cast<UINT>(!cfg_bEnableRating));
    CheckDlgButton(IDC_CB_PRESS_F1_MSG, static_cast<UINT>(cfg_bShowPressF1ForHelp));

    CheckDlgButton(IDC_CB_FSPT, static_cast<UINT>(cfg_allow_page_tearing_fs));
    UpdateMaxFps(FULLSCREEN);

    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fTimeBetweenPresets));
    SetDlgItemText(IDC_BETWEEN_TIME, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fTimeBetweenPresetsRand));
    SetDlgItemText(IDC_BETWEEN_TIME_RANDOM, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fBlendTimeAuto));
    SetDlgItemText(IDC_BLEND_AUTO, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fBlendTimeUser));
    SetDlgItemText(IDC_BLEND_USER, buf);

    swprintf_s(buf, L" %2.1f", static_cast<float>(cfg_fHardCutHalflife));
    SetDlgItemText(IDC_HARDCUT_BETWEEN_TIME, buf);
    n = static_cast<int>((static_cast<float>(cfg_fHardCutLoudnessThresh) - 1.25f) * 10.0f);
    if (n < 0)
        n = 0;
    if (n > 20)
        n = 20;
    SendMessage(GetDlgItem(IDC_HARDCUT_LOUDNESS), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)n);
    CheckDlgButton(IDC_CB_HARDCUTS, static_cast<UINT>(cfg_bHardCutsDisabled));

    SelectItemByValue(GetDlgItem(IDC_SHADERS), static_cast<DWORD>(cfg_nMaxPSVersion));

    //SelectItemByValue(GetDlgItem(IDC_TEXFORMAT), static_cast<DWORD>(cfg_nTexBitsPerCh));

    SelectItemByValue(GetDlgItem(IDC_MESHSIZECOMBO), static_cast<DWORD>(cfg_nGridX));

    SelectItemByValue(GetDlgItem(IDC_STRETCH2), static_cast<DWORD>(cfg_nCanvasStretch));

    SelectItemByValue(GetDlgItem(IDC_TEXSIZECOMBO), static_cast<DWORD>(cfg_nTexSizeX));

    SendMessage(GetDlgItem(IDC_BRIGHT_SLIDER2), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)cfg_n16BitGamma);
    CheckDlgButton(IDC_CB_AUTOGAMMA2, static_cast<UINT>(cfg_bAutoGamma));

    SelectItemByValue(GetDlgItem(IDC_MAX_BYTES2), static_cast<DWORD>(cfg_nMaxBytes));

    SelectItemByValue(GetDlgItem(IDC_MAX_IMAGES2), static_cast<DWORD>(cfg_nMaxImages));

    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fSongTitleAnimDuration));
    SetDlgItemText(IDC_SONGTITLEANIM_DURATION, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fTimeBetweenRandomSongTitles));
    SetDlgItemText(IDC_RAND_TITLE, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(cfg_fTimeBetweenRandomCustomMsgs));
    SetDlgItemText(IDC_RAND_MSG, buf);
    CheckDlgButton(IDC_CB_TITLE_ANIMS, static_cast<UINT>(cfg_bSongTitleAnims));

    OnChanged();
}

void milk2_preferences_page::apply()
{
    WCHAR buf[256], *stop;
    HWND ctrl = nullptr;
    LRESULT t = 0;

    cfg_bPresetLockOnAtStartup = static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON3));
    cfg_bPreventScollLockHandling = static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON4));
    cfg_bWarningsDisabled = static_cast<bool>(IsDlgButtonChecked(IDC_CB_NOWARN3));
    cfg_bEnableRating = !static_cast<bool>(IsDlgButtonChecked(IDC_CB_NORATING2));
    cfg_bShowPressF1ForHelp = static_cast<bool>(IsDlgButtonChecked(IDC_CB_PRESS_F1_MSG));

    cfg_allow_page_tearing_fs = static_cast<bool>(IsDlgButtonChecked(IDC_CB_FSPT));
    SaveMaxFps(FULLSCREEN);

    GetDlgItemText(IDC_BETWEEN_TIME, buf, 256);
    cfg_fTimeBetweenPresets = wcstof(buf, &stop);
    if (cfg_fTimeBetweenPresets < 0.1f)
        cfg_fTimeBetweenPresets = 0.1f;
    GetDlgItemText(IDC_BETWEEN_TIME_RANDOM, buf, 256);
    cfg_fTimeBetweenPresetsRand = wcstof(buf, &stop);
    if (cfg_fTimeBetweenPresetsRand < 0.0f)
        cfg_fTimeBetweenPresetsRand = 0.0f;
    GetDlgItemText(IDC_BLEND_AUTO, buf, 256);
    cfg_fBlendTimeAuto = wcstof(buf, &stop);
    GetDlgItemText(IDC_BLEND_USER, buf, 256);
    cfg_fBlendTimeUser = wcstof(buf, &stop);

    GetDlgItemText(IDC_HARDCUT_BETWEEN_TIME, buf, 256);
    cfg_fHardCutHalflife = wcstof(buf, &stop);
    t = SendMessage(GetDlgItem(IDC_HARDCUT_LOUDNESS), TBM_GETPOS, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        cfg_fHardCutLoudnessThresh = static_cast<double>(1.25f + t / 10.0f);
    cfg_bHardCutsDisabled = static_cast<bool>(IsDlgButtonChecked(IDC_CB_HARDCUTS));

    ctrl = GetDlgItem(IDC_SHADERS);
    t = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        cfg_nMaxPSVersion = static_cast<int64_t>(SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)t, (LPARAM)0));

    //ctrl = GetDlgItem(IDC_TEXFORMAT);
    //t = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    //if (t != CB_ERR)
    //    cfg_nTexBitsPerCh = static_cast<int64_t>(SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)t, (LPARAM)0));

    ctrl = GetDlgItem(IDC_MESHSIZECOMBO);
    t = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        cfg_nGridX = static_cast<int64_t>(SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)t, (LPARAM)0));

    ctrl = GetDlgItem(IDC_STRETCH2);
    t = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        cfg_nCanvasStretch = static_cast<int64_t>(SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)t, (LPARAM)0));

    ctrl = GetDlgItem(IDC_TEXSIZECOMBO);
    t = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        cfg_nTexSizeX = static_cast<int64_t>(SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)t, (LPARAM)0));

    SendMessage(GetDlgItem(IDC_BRIGHT_SLIDER2), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)cfg_n16BitGamma);

    t = SendMessage(GetDlgItem(IDC_BRIGHT_SLIDER2), TBM_GETPOS, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        cfg_n16BitGamma = static_cast<int64_t>(t);
    cfg_bAutoGamma = static_cast<bool>(IsDlgButtonChecked(IDC_CB_AUTOGAMMA2));

    ctrl = GetDlgItem(IDC_MAX_BYTES2);
    t = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        cfg_nMaxBytes = static_cast<int64_t>(SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)t, (LPARAM)0));

    ctrl = GetDlgItem(IDC_MAX_IMAGES2);
    t = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        cfg_nMaxImages = static_cast<int64_t>(SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)t, (LPARAM)0));

    GetDlgItemText(IDC_SONGTITLEANIM_DURATION, buf, 256);
    cfg_fSongTitleAnimDuration = wcstof(buf, &stop);
    GetDlgItemText(IDC_RAND_TITLE, buf, 256);
    cfg_fTimeBetweenRandomSongTitles = wcstof(buf, &stop);
    GetDlgItemText(IDC_RAND_MSG, buf, 256);
    cfg_fTimeBetweenRandomCustomMsgs = wcstof(buf, &stop);
    cfg_bSongTitleAnims = static_cast<bool>(IsDlgButtonChecked(IDC_CB_TITLE_ANIMS));

    OnChanged(); // The dialog content has not changed but the flags have; the currently shown values now match the settings so the apply button can be disabled.
    ::SendMessage(g_hWindow, WM_CONFIG_CHANGE, (WPARAM)0, (LPARAM)0);
}

// Returns whether the dialog content is different from the current configuration;
// i.e., whether the "Apply" button should be enabled or not.
bool milk2_preferences_page::HasChanged() const
{
    bool checkbox_changes = (static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON3)) != cfg_bPresetLockOnAtStartup) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON4)) != cfg_bPreventScollLockHandling) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_NOWARN3)) != cfg_bWarningsDisabled) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_NORATING2)) == cfg_bEnableRating) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_PRESS_F1_MSG)) != cfg_bShowPressF1ForHelp) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_FSPT)) != cfg_allow_page_tearing_fs) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_HARDCUTS)) != cfg_bHardCutsDisabled) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_AUTOGAMMA2)) != cfg_bAutoGamma) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_TITLE_ANIMS)) != cfg_bSongTitleAnims);

    HWND ctrl = nullptr;
    LRESULT n = 0;
    bool combobox_changes = false;
    n = SendMessage(GetDlgItem(IDC_FS_MAXFPS2), CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        if (n > 0)
            n = MAX_MAX_FPS + 1 - n;
        combobox_changes = combobox_changes || (static_cast<UINT>(n) != cfg_max_fps_fs);
    }
    ctrl = GetDlgItem(IDC_SHADERS);
    n = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        n = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)n, (LPARAM)0);
        combobox_changes = combobox_changes || (static_cast<UINT>(n) != static_cast<UINT>(cfg_nMaxPSVersion));
    }
    //ctrl = GetDlgItem(IDC_TEXFORMAT);
    //n = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    //if (n != CB_ERR)
    //{
    //    n = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)n, (LPARAM)0);
    //    combobox_changes = combobox_changes || static_cast<UINT>(n) != static_cast<UINT>(cfg_nTexBitsPerCh);
    //}
    ctrl = GetDlgItem(IDC_MESHSIZECOMBO);
    n = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        n = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)n, (LPARAM)0);
        combobox_changes = combobox_changes || (static_cast<UINT>(n) != static_cast<UINT>(cfg_nGridX));
    }
    ctrl = GetDlgItem(IDC_STRETCH2);
    n = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        n = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)n, (LPARAM)0);
        combobox_changes = combobox_changes || (static_cast<UINT>(n) != static_cast<UINT>(cfg_nCanvasStretch));
    }
    ctrl = GetDlgItem(IDC_TEXSIZECOMBO);
    n = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        n = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)n, (LPARAM)0);
        combobox_changes = combobox_changes || (static_cast<UINT>(n) != static_cast<UINT>(cfg_nTexSizeX));
    }
    ctrl = GetDlgItem(IDC_MAX_BYTES2);
    n = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        n = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)n, (LPARAM)0);
        combobox_changes = combobox_changes || (static_cast<UINT>(n) != static_cast<UINT>(cfg_nMaxBytes));
    }
    ctrl = GetDlgItem(IDC_MAX_IMAGES2);
    n = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        n = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)n, (LPARAM)0);
        combobox_changes = combobox_changes || (static_cast<UINT>(n) != static_cast<UINT>(cfg_nMaxImages));
    }

    WCHAR buf[256], *stop;
    bool editcontrol_changes = false;
    GetDlgItemText(IDC_BETWEEN_TIME, buf, 256);
    editcontrol_changes = editcontrol_changes || (wcstof(buf, &stop) != cfg_fTimeBetweenPresets);
    GetDlgItemText(IDC_BETWEEN_TIME_RANDOM, buf, 256);
    editcontrol_changes = editcontrol_changes || (wcstof(buf, &stop) != cfg_fTimeBetweenPresetsRand);
    GetDlgItemText(IDC_BLEND_AUTO, buf, 256);
    editcontrol_changes = editcontrol_changes || (wcstof(buf, &stop) != cfg_fBlendTimeAuto);
    GetDlgItemText(IDC_BLEND_USER, buf, 256);
    editcontrol_changes = editcontrol_changes || (wcstof(buf, &stop) != cfg_fBlendTimeUser);
    GetDlgItemText(IDC_HARDCUT_BETWEEN_TIME, buf, 256);
    editcontrol_changes = editcontrol_changes || (wcstof(buf, &stop) != cfg_fHardCutHalflife);
    GetDlgItemText(IDC_BETWEEN_TIME, buf, 256);
    editcontrol_changes = editcontrol_changes || (wcstof(buf, &stop) != cfg_fTimeBetweenPresets);
    GetDlgItemText(IDC_SONGTITLEANIM_DURATION, buf, 256);
    editcontrol_changes = editcontrol_changes || (wcstof(buf, &stop) != cfg_fSongTitleAnimDuration);
    GetDlgItemText(IDC_RAND_TITLE, buf, 256);
    editcontrol_changes = editcontrol_changes || (wcstof(buf, &stop) != cfg_fTimeBetweenRandomSongTitles);
    GetDlgItemText(IDC_RAND_MSG, buf, 256);
    editcontrol_changes = editcontrol_changes || (wcstof(buf, &stop) != cfg_fTimeBetweenRandomCustomMsgs);

    LRESULT t;
    bool slider_changes = false;
    t = SendMessage(GetDlgItem(IDC_HARDCUT_LOUDNESS), TBM_GETPOS, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
    {
        if (t < 0)
            t = 0;
        if (t > 20)
            t = 20;
        int prev_slider_position = static_cast<int>((static_cast<float>(cfg_fHardCutLoudnessThresh) - 1.25f) * 10.0f);
        slider_changes = slider_changes || (static_cast<int>(t) != prev_slider_position);
    }
    t = SendMessage(GetDlgItem(IDC_BRIGHT_SLIDER2), TBM_GETPOS, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        slider_changes = slider_changes || (static_cast<UINT>(t) != static_cast<UINT>(cfg_n16BitGamma));

    return checkbox_changes || combobox_changes || editcontrol_changes || slider_changes;
}

// Tells the host that state has changed to enable or disable the "Apply" button appropriately.
void milk2_preferences_page::OnChanged()
{
    m_callback->on_state_changed();
}

class preferences_page_milk2 : public preferences_page_impl<milk2_preferences_page>
{
  public:
    const char* get_name() { return "MilkDrop"; }
    GUID get_guid()
    {
        static const GUID guid = {
            0x5feadec6, 0x37f3, 0x4ebf, {0xb3, 0xfd, 0x57, 0x11, 0xfa, 0xe3, 0xa3, 0xe8}
        }; // {5FEADEC6-37F3-4EBF-B3FD-5711FAE3A3E8}
        return guid;
    }
    GUID get_parent_guid() { return guid_visualisations; }
    bool get_help_url(pfc::string_base& p_out)
    {
        p_out.reset();
        p_out << "https://www.geisswerks.com/milkdrop/milkdrop.html";
        //"http://help.foobar2000.org/" << core_version_info::g_get_version_string() << "/" << "preferences" << "/" << pfc::print_guid(get_guid()) << "/" << get_name();
        return true;
    }
};

inline void milk2_preferences_page::AddItem(HWND ctrl, wchar_t* buffer, UINT id, DWORD itemdata)
{
    LoadString(core_api::get_my_instance(), id, buffer, 256);
    AddItem(ctrl, buffer, itemdata);
}

inline void milk2_preferences_page::AddItem(HWND ctrl, const wchar_t* text, DWORD itemdata)
{
    LRESULT nPos = SendMessage(ctrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)text);
    SendMessage(ctrl, CB_SETITEMDATA, nPos, itemdata);
}

inline void milk2_preferences_page::SelectItemByPos(HWND ctrl, int pos)
{
    SendMessage(ctrl, CB_SETCURSEL, (WPARAM)pos, (LPARAM)0);
}

int milk2_preferences_page::SelectItemByValue(HWND ctrl, DWORD value)
{
    LRESULT count = SendMessage(ctrl, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    for (int i = 0; i < count; ++i)
    {
        LRESULT value_i = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)i, (LPARAM)0);
        if (value_i == static_cast<LRESULT>(value))
        {
            SendMessage(ctrl, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
            return i;
        }
    }
    return -1;
}

wchar_t* milk2_preferences_page::FormImageCacheSizeString(const wchar_t* itemStr, const UINT sizeID)
{
    static wchar_t cacheBuf[128] = {0};
    wchar_t buf[128] = {0};
    LoadString(core_api::get_my_instance(), sizeID, buf, 128);
    swprintf_s(cacheBuf, L"%s %s", itemStr, buf);
    return cacheBuf;
}

void milk2_preferences_page::UpdateMaxFps(int screenmode) const
{
    HWND ctrl = nullptr;
    switch (screenmode)
    {
        case WINDOWED:
            ctrl = GetDlgItem(IDC_W_MAXFPS2);
            break;
        case FULLSCREEN:
            ctrl = GetDlgItem(IDC_FS_MAXFPS2);
            break;
    }

    if (!ctrl)
        return;

    SendMessage(ctrl, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
    for (int j = 0; j <= MAX_MAX_FPS; ++j)
    {
        WCHAR buf[256];
        if (j == 0)
            LoadString(core_api::get_my_instance(), IDS_UNLIMITED, buf, 256);
        else
        {
            LoadString(core_api::get_my_instance(), IDS_X_FPS, buf, 256);
            swprintf_s(buf, buf, MAX_MAX_FPS + 1 - j);
        }

        SendMessage(ctrl, CB_ADDSTRING, (WPARAM)j, (LPARAM)buf);
    }

    // Set previous selection.
    UINT max_fps = 0;
    switch (screenmode)
    {
        case FULLSCREEN:
            max_fps = static_cast<UINT>(cfg_max_fps_fs);
            break;
    }
    if (max_fps == 0)
        SendMessage(ctrl, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    else
        SendMessage(ctrl, CB_SETCURSEL, (WPARAM)(MAX_MAX_FPS - max_fps + 1), (LPARAM)0);
}

void milk2_preferences_page::SaveMaxFps(int screenmode) const
{
    HWND ctrl = nullptr;
    switch (screenmode)
    {
        case FULLSCREEN:
            ctrl = GetDlgItem(IDC_FS_MAXFPS2);
            break;
    }

    // Read maximum FPS settings.
    if (ctrl)
    {
        LRESULT n = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
        if (n != CB_ERR)
        {
            if (n > 0)
                n = MAX_MAX_FPS + 1 - n;

            switch (screenmode)
            {
                case FULLSCREEN:
                    cfg_max_fps_fs = static_cast<UINT>(n);
                    break;
            }
        }
    }
}

static preferences_page_factory_t<preferences_page_milk2> g_preferences_page_milk2_factory;
#pragma endregion

#pragma region Configuration Settings
milk2_config::milk2_config()
{
}

uint32_t milk2_config::get_version() const
{
    return m_version;
}

uint32_t milk2_config::get_sentinel() const
{
    return m_sentinel;
}

void milk2_config::init()
{
    initialize_paths();
    reset();
}

void milk2_config::reset()
{
    //--- CPluginShell::ReadConfig()
    settings.m_multisample_fs = {1u, 0u};

    //settings.m_start_fullscreen;
    settings.m_max_fps_fs = static_cast<uint32_t>(cfg_max_fps_fs);
    settings.m_show_press_f1_msg = static_cast<uint32_t>(cfg_bShowPressF1ForHelp);
    settings.m_allow_page_tearing_fs = static_cast<uint32_t>(cfg_allow_page_tearing_fs);
    //settings.m_save_cpu;
    //settings.m_skin;
    //settings.m_fix_slow_text;
    //settings.m_disp_mode_fs;

    //--- CPlugin::MilkDropReadConfig()
    settings.m_bEnableRating = cfg_bEnableRating;
    settings.m_bHardCutsDisabled = cfg_bHardCutsDisabled;
    settings.g_bDebugOutput = cfg_bDebugOutput;
    //settings.m_bShowSongInfo;
    //settings.m_bShowPressF1ForHelp;
    //settings.m_bShowMenuToolTips;
    settings.m_bSongTitleAnims = cfg_bSongTitleAnims;

    settings.m_bShowFPS = default_bShowFPS;
    settings.m_bShowRating = default_bShowRating;
    settings.m_bShowPresetInfo = default_bShowPresetInfo;
    //settings.m_bShowDebugInfo = default_bShowDebugInfo;
    settings.m_bShowSongTitle = default_bShowSongTitle;
    settings.m_bShowSongTime = default_bShowSongTime;
    settings.m_bShowSongLen = default_bShowSongLen;
    settings.m_bShowShaderHelp = default_bShowShaderHelp;

    //settings.m_bFixPinkBug = default_bFixPinkBug;
    settings.m_n16BitGamma = static_cast<uint32_t>(cfg_n16BitGamma);
    settings.m_bAutoGamma = cfg_bAutoGamma;
    //settings.m_bAlways3D = default_bAlways3D;
    //settings.m_fStereoSep = default_fStereoSep;
    //settings.m_bFixSlowText = default_bFixSlowText;
    //settings.m_bAlwaysOnTop = default_bAlwaysOnTop;
    //settings.m_bWarningsDisabled = default_bWarningsDisabled;
    settings.m_bWarningsDisabled2 = cfg_bWarningsDisabled;
    //settings.m_bAnisotropicFiltering = default_bAnisotropicFiltering;
    settings.m_bPresetLockOnAtStartup = cfg_bPresetLockOnAtStartup;
    settings.m_bPreventScollLockHandling = cfg_bPreventScollLockHandling;

    settings.m_nCanvasStretch = static_cast<uint32_t>(cfg_nCanvasStretch);
    settings.m_nTexSizeX = static_cast<uint32_t>(cfg_nTexSizeX);
    settings.m_nTexSizeY = settings.m_nTexSizeX;
    settings.m_bTexSizeWasAutoPow2 = (settings.m_nTexSizeX == -2);
    settings.m_bTexSizeWasAutoExact = (settings.m_nTexSizeX == -1);
    settings.m_nTexBitsPerCh = static_cast<uint32_t>(cfg_nTexBitsPerCh);
    settings.m_nGridX = static_cast<uint32_t>(cfg_nGridX);
    if (settings.m_nGridX > MAX_GRID_X)
        settings.m_nGridX = MAX_GRID_X;
    settings.m_nGridY = settings.m_nGridX * 3 / 4;
    if (settings.m_nGridY > MAX_GRID_Y)
        settings.m_nGridY = MAX_GRID_Y;
    settings.m_nMaxPSVersion = static_cast<uint32_t>(cfg_nMaxPSVersion);
    settings.m_nMaxImages = static_cast<uint32_t>(cfg_nMaxImages);
    settings.m_nMaxBytes = static_cast<uint32_t>(cfg_nMaxBytes);

    settings.m_fBlendTimeUser = static_cast<float>(cfg_fBlendTimeUser);
    settings.m_fBlendTimeAuto = static_cast<float>(cfg_fBlendTimeAuto);
    settings.m_fTimeBetweenPresets = static_cast<float>(cfg_fTimeBetweenPresets);
    settings.m_fTimeBetweenPresetsRand = static_cast<float>(cfg_fTimeBetweenPresetsRand);
    settings.m_fHardCutLoudnessThresh = static_cast<float>(cfg_fHardCutLoudnessThresh);
    settings.m_fHardCutHalflife = static_cast<float>(cfg_fHardCutHalflife);
    settings.m_fSongTitleAnimDuration = static_cast<float>(cfg_fSongTitleAnimDuration);
    settings.m_fTimeBetweenRandomSongTitles = static_cast<float>(cfg_fTimeBetweenRandomSongTitles);
    settings.m_fTimeBetweenRandomCustomMsgs = static_cast<float>(cfg_fTimeBetweenRandomCustomMsgs);

    settings.m_bPresetLockedByCode = default_bPresetLockedByCode;
    settings.m_bPresetLockedByUser = settings.m_bPresetLockOnAtStartup;
    //settings.m_bMilkdropScrollLockState = settings.m_bPresetLockOnAtStartup;

    //--- Extras
    settings.m_bEnableDownmix = default_bEnableDownmix;
    settings.m_bEnableHDR = default_bEnableHDR;
    settings.m_nBackBufferFormat = default_nBackBufferFormat;
    settings.m_nDepthBufferFormat = default_nDepthBufferFormat;
    settings.m_nBackBufferCount = default_nBackBufferCount;
    settings.m_nMinFeatureLevel = default_nMinFeatureLevel;

    //settings.m_nFpsLimit = default_nFpsLimit;

    //--- Paths
    update_paths();
}

void milk2_config::initialize_paths()
{
    //std::string component_path = core_api::get_my_full_path();
    //std::string::size_type t = component_path.rfind('\\');
    //if (t != std::string::npos)
    //    component_path.erase(t + 1);
    std::string profile_path = core_api::get_profile_path();
    assert(strncmp(profile_path.substr(0, 7).c_str(), "file://", 7) == 0);
    profile_path = profile_path.substr(7);
    profile_path.append("\\");

    swprintf_s(default_szPluginsDirPath, L"%hs", const_cast<char*>(profile_path.c_str()));
    swprintf_s(default_szMilkdrop2Path, L"%ls%ls", default_szPluginsDirPath, SUBDIR);
    swprintf_s(default_szConfigIniFile, L"%ls%ls", default_szMilkdrop2Path, INIFILE);
    swprintf_s(default_szPresetDir, L"%lspresets\\", default_szMilkdrop2Path);
    wchar_t szConfigDir[MAX_PATH] = {0};
    wcscpy_s(szConfigDir, default_szConfigIniFile);
    wchar_t* p = wcsrchr(szConfigDir, L'\\');
    if (p)
        *(p + 1) = L'\0';
    swprintf_s(default_szMsgIniFile, L"%ls%ls", szConfigDir, MSG_INIFILE);
    swprintf_s(default_szImgIniFile, L"%ls%ls", szConfigDir, IMG_INIFILE);
}

void milk2_config::update_paths()
{
    if (m_version > 2 && cfg_szPresetDir.get().empty())
    {
        CHAR szPresetDirA[MAX_PATH];
        wcscpy_s(settings.m_szPresetDir, default_szPresetDir);
        wcstombs_s(nullptr, szPresetDirA, default_szPresetDir, MAX_PATH);
        cfg_szPresetDir.set(szPresetDirA);
    }
    else
    {
        size_t convertedChars;
        mbstowcs_s(&convertedChars, settings.m_szPresetDir, cfg_szPresetDir.get().c_str(), MAX_PATH);
    }
    wcscpy_s(settings.m_szPluginsDirPath, default_szPluginsDirPath);
    wcscpy_s(settings.m_szConfigIniFile, default_szConfigIniFile);
}

// Reads the configuration.
void milk2_config::parse(ui_element_config_parser& parser)
{
    try
    {
        uint32_t sentinel, version;
        parser >> sentinel;
        parser >> version;
        switch (version)
        {
            case 2:
                parser >> settings.m_bShowFPS;
                parser >> settings.m_bShowRating;
                parser >> settings.m_bShowPresetInfo;
                //parser >> settings.m_bShowDebugInfo;
                parser >> settings.m_bShowSongTitle;
                parser >> settings.m_bShowSongTime;
                parser >> settings.m_bShowSongLen;
                parser >> settings.m_bShowShaderHelp;

                parser >> settings.m_nTexSizeY;
                parser >> settings.m_bTexSizeWasAutoPow2;
                parser >> settings.m_bTexSizeWasAutoExact;
                parser >> settings.m_nGridY;

                parser >> settings.m_bPresetLockedByCode;

                parser >> settings.m_bEnableDownmix;
                parser >> settings.m_bEnableHDR;
                parser >> settings.m_nBackBufferFormat;
                parser >> settings.m_nDepthBufferFormat;
                parser >> settings.m_nBackBufferCount;
                parser >> settings.m_nMinFeatureLevel;
                break;
            default:
                FB2K_console_formatter() << core_api::get_my_file_name() << ": Unknown configuration format version: " << version;
        }
    }
    catch (exception_io& exc)
    {
        FB2K_console_formatter() << core_api::get_my_file_name() << ": Exception while reading configuration data: " << exc;
    }
}

// Writes the configuration.
void milk2_config::build(ui_element_config_builder& builder)
{
    builder << m_sentinel;
    builder << m_version;

    //builder << m_multisample_fs;
    //builder << m_multisample_w;

    //builder << settings.m_start_fullscreen;
    cfg_max_fps_fs = settings.m_max_fps_fs;
    //builder << settings.m_show_press_f1_msg;
    cfg_allow_page_tearing_fs = settings.m_allow_page_tearing_fs;
    //builder << settings.m_minimize_winamp;
    //builder << settings.m_dualhead_horz;
    //builder << settings.m_dualhead_vert;
    //builder << settings.m_save_cpu;
    //builder << settings.m_skin;
    //builder << settings.m_fix_slow_text;

    //parser >> m_disp_mode_fs;

    cfg_bEnableRating = settings.m_bEnableRating;
    cfg_bHardCutsDisabled = settings.m_bHardCutsDisabled;
    //cfg_bDebugOutput = settings.g_bDebugOutput;
    //builder << settings.m_bShowSongInfo;
    cfg_bShowPressF1ForHelp = settings.m_show_press_f1_msg;
    //builder << settings.m_bShowMenuToolTips;
    cfg_bSongTitleAnims = settings.m_bSongTitleAnims;

    builder << settings.m_bShowFPS;
    builder << settings.m_bShowRating;
    builder << settings.m_bShowPresetInfo;
    //builder << settings.m_bShowDebugInfo;
    builder << settings.m_bShowSongTitle;
    builder << settings.m_bShowSongTime;
    builder << settings.m_bShowSongLen;
    builder << settings.m_bShowShaderHelp;

    //builder << settings.m_bFixPinkBug;
    cfg_n16BitGamma = settings.m_n16BitGamma;
    cfg_bAutoGamma = settings.m_bAutoGamma;
    //builder << settings.m_bAlways3D;
    //builder << settings.m_fStereoSep;
    //builder << settings.m_bFixSlowText;
    //builder << settings.m_bAlwaysOnTop;
    //builder << settings.m_bWarningsDisabled;
    cfg_bWarningsDisabled = settings.m_bWarningsDisabled2;
    //builder << settings.m_bAnisotropicFiltering;
    cfg_bPresetLockOnAtStartup = settings.m_bPresetLockOnAtStartup;
    cfg_bPreventScollLockHandling = settings.m_bPreventScollLockHandling;

    cfg_nCanvasStretch = settings.m_nCanvasStretch;
    cfg_nTexSizeX = settings.m_nTexSizeX;
    builder << settings.m_nTexSizeY;
    builder << settings.m_bTexSizeWasAutoPow2;
    builder << settings.m_bTexSizeWasAutoExact;
    cfg_nTexBitsPerCh = settings.m_nTexBitsPerCh;
    cfg_nGridX = settings.m_nGridX;
    builder << settings.m_nGridY;
    cfg_nMaxPSVersion = settings.m_nMaxPSVersion;
    cfg_nMaxImages = settings.m_nMaxImages;
    cfg_nMaxBytes = settings.m_nMaxBytes;

    cfg_fBlendTimeUser = settings.m_fBlendTimeUser;
    cfg_fBlendTimeAuto = settings.m_fBlendTimeAuto;
    cfg_fTimeBetweenPresets = settings.m_fTimeBetweenPresets;
    cfg_fTimeBetweenPresetsRand = settings.m_fTimeBetweenPresetsRand;
    cfg_fHardCutLoudnessThresh = settings.m_fHardCutLoudnessThresh;
    cfg_fHardCutHalflife = settings.m_fHardCutHalflife;
    cfg_fSongTitleAnimDuration = settings.m_fSongTitleAnimDuration;
    cfg_fTimeBetweenRandomSongTitles = settings.m_fTimeBetweenRandomSongTitles;
    cfg_fTimeBetweenRandomCustomMsgs = settings.m_fTimeBetweenRandomCustomMsgs;

    builder << settings.m_bPresetLockedByCode;
    //builder << settings.m_bPresetLockedByUser;
    //builder << settings.m_bMilkdropScrollLockState;
    //builder << settings.m_nFpsLimit;

    builder << settings.m_bEnableDownmix;
    builder << settings.m_bEnableHDR;
    builder << settings.m_nBackBufferFormat;
    builder << settings.m_nDepthBufferFormat;
    builder << settings.m_nBackBufferCount;
    builder << settings.m_nMinFeatureLevel;
}
#pragma endregion