/*
 * preferences.cpp - Configuration settings for the MilkDrop 2
 *                   visualization component accessible through a
 *                   preferences page and advanced preferences.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#include "pch.h"
#include "config.h"

extern HWND g_hWindow;

namespace
{
// clang-format off
static cfg_bool cfg_bPresetLockOnAtStartup(guid_cfg_bPresetLockOnAtStartup, default_bPresetLockOnAtStartup);
static cfg_bool cfg_bPreventScollLockHandling(guid_cfg_bPreventScollLockHandling, default_bPreventScollLockHandling);
static cfg_bool cfg_bWarningsDisabled(guid_cfg_bWarningsDisabled, default_bWarningsDisabled2);
static cfg_bool cfg_bEnableRating(guid_cfg_bEnableRating, !default_bEnableRating);
static cfg_bool cfg_bShowPressF1ForHelp(guid_cfg_bShowPressF1ForHelp, default_bShowPressF1ForHelp);
static cfg_bool cfg_bSkipCompShader(guid_cfg_bSkipCompShader, default_bSkipCompShader);
static cfg_bool cfg_allow_page_tearing_fs(guid_cfg_allow_page_tearing_fs, default_allow_page_tearing_fs);
static cfg_bool cfg_bSongTitleAnims(guid_cfg_bSongTitleAnims, default_bSongTitleAnims);
static cfg_bool cfg_bAutoGamma(guid_cfg_bAutoGamma, default_bAutoGamma);
static cfg_bool cfg_bHardCutsDisabled(guid_cfg_bHardCutsDisabled, default_bHardCutsDisabled);
static cfg_bool cfg_bShowFPS(guid_cfg_bShowFPS, default_bShowFPS);
static cfg_bool cfg_bShowRating(guid_cfg_bShowRating, default_bShowRating);
static cfg_bool cfg_bShowPresetInfo(guid_cfg_bShowPresetInfo, default_bShowPresetInfo);
//static cfg_bool cfg_bShowDebugInfo(guid_cfg_bShowDebugInfo, default_bShowDebugInfo);
static cfg_bool cfg_bShowSongTitle(guid_cfg_bShowSongTitle, default_bShowSongTitle);
static cfg_bool cfg_bShowSongTime(guid_cfg_bShowSongTime, default_bShowSongTime);
static cfg_bool cfg_bShowSongLen(guid_cfg_bShowSongLen, default_bShowSongLen);
static cfg_bool cfg_bShowShaderHelp(guid_cfg_bShowShaderHelp, default_bShowShaderHelp);
//static cfg_bool cfg_bTexSizeWasAutoPow2(guid_cfg_bTexSizeWasAutoPow2, default_bTexSizeWasAutoPow2);
//static cfg_bool cfg_bTexSizeWasAutoExact(guid_cfg_bTexSizeWasAutoExact, default_bTexSizeWasAutoExact);
static cfg_bool cfg_bPresetLockedByCode(guid_cfg_bPresetLockedByCode, default_bPresetLockedByCode);
static cfg_bool cfg_bEnableDownmix(guid_cfg_bEnableDownmix, default_bEnableDownmix);
static cfg_bool cfg_bShowAlbum(guid_cfg_bShowAlbum, default_bShowAlbum);
static cfg_bool cfg_bEnableHDR(guid_cfg_bEnableHDR, default_bEnableHDR);
static cfg_int cfg_max_fps_fs(guid_cfg_max_fps_fs, static_cast<int64_t>(default_max_fps_fs));
static cfg_int cfg_n16BitGamma(guid_cfg_n16BitGamma, static_cast<int64_t>(default_n16BitGamma));
static cfg_int cfg_nMaxBytes(guid_cfg_nMaxBytes, static_cast<int64_t>(default_nMaxBytes));
static cfg_int cfg_nMaxImages(guid_cfg_nMaxImages, static_cast<int64_t>(default_nMaxImages));
static cfg_int cfg_nCanvasStretch(guid_cfg_nCanvasStretch, static_cast<int64_t>(default_nCanvasStretch));
static cfg_int cfg_nGridX(guid_cfg_nGridX, static_cast<int64_t>(default_nGridX));
static cfg_int cfg_nTexSizeX(guid_cfg_nTexSizeX, static_cast<int64_t>(default_nTexSizeX));
static cfg_int cfg_nTexBitsPerCh(guid_cfg_nTexBitsPerCh, static_cast<int64_t>(default_nTexBitsPerCh));
static cfg_int cfg_nMaxPSVersion(guid_cfg_nMaxPSVersion, static_cast<int64_t>(default_nMaxPSVersion));
//static cfg_int cfg_nTexSizeY(guid_cfg_nTexSizeY, static_cast<int64_t>(default_nTexSizeY));
//static cfg_int cfg_nGridY(guid_cfg_nGridY, static_cast<int64_t>(default_nGridY));
static cfg_int cfg_nBackBufferFormat(guid_cfg_nBackBufferFormat, static_cast<int64_t>(default_nBackBufferFormat));
static cfg_int cfg_nDepthBufferFormat(guid_cfg_nDepthBufferFormat, static_cast<int64_t>(default_nDepthBufferFormat));
static cfg_int cfg_nBackBufferCount(guid_cfg_nBackBufferCount, static_cast<int64_t>(default_nBackBufferCount));
static cfg_int cfg_nMinFeatureLevel(guid_cfg_nMinFeatureLevel, static_cast<int64_t>(default_nMinFeatureLevel));
static cfg_float cfg_fTimeBetweenPresets(guid_cfg_fTimeBetweenPresets, static_cast<double>(default_fTimeBetweenPresets));
static cfg_float cfg_fTimeBetweenPresetsRand(guid_cfg_fTimeBetweenPresetsRand, static_cast<double>(default_fTimeBetweenPresetsRand));
static cfg_float cfg_fBlendTimeAuto(guid_cfg_fBlendTimeAuto, static_cast<double>(default_fBlendTimeAuto));
static cfg_float cfg_fBlendTimeUser(guid_cfg_fBlendTimeUser, static_cast<double>(default_fBlendTimeUser));
static cfg_float cfg_fHardCutHalflife(guid_cfg_fHardCutHalflife, static_cast<double>(default_fHardCutHalflife));
static cfg_float cfg_fHardCutLoudnessThresh(guid_cfg_fHardCutLoudnessThresh, static_cast<double>(default_fHardCutLoudnessThresh));
static cfg_float cfg_fSongTitleAnimDuration(guid_cfg_fSongTitleAnimDuration, static_cast<double>(default_fSongTitleAnimDuration));
static cfg_float cfg_fTimeBetweenRandomSongTitles(guid_cfg_fTimeBetweenRandomSongTitles, static_cast<double>(default_fTimeBetweenRandomSongTitles));
static cfg_float cfg_fTimeBetweenRandomCustomMsgs(guid_cfg_fTimeBetweenRandomCustomMsgs, static_cast<double>(default_fTimeBetweenRandomCustomMsgs));
static cfg_string cfg_szTitleFormat(guid_cfg_szTitleFormat, default_szTitleFormat);
static cfg_string cfg_szArtworkFormat(guid_cfg_szArtworkFormat, default_szArtworkFormat);
static cfg_blob cfg_stFontInfo(guid_cfg_stFontInfo, default_stFontInfo, sizeof(td_fontinfo) * (NUM_BASIC_FONTS + NUM_EXTRA_FONTS));
static advconfig_branch_factory g_advconfigBranch("MilkDrop", guid_advconfig_branch, advconfig_branch::guid_branch_vis, 0);
static advconfig_checkbox_factory cfg_bDebugOutput("Debug output", "milk2.bDebugOutput", guid_cfg_bDebugOutput, guid_advconfig_branch, order_bDebugOutput, default_bDebugOutput, 0);
static advconfig_string_factory cfg_szPresetDir("Preset directory", "milk2.szPresetDir", guid_cfg_szPresetDir, guid_advconfig_branch, order_szPresetDir, "", advconfig_entry_string::flag_is_folder_path);
// clang-format on
} // namespace

#pragma region Preferences Page
BOOL milk2_preferences_page::OnInitDialog(CWindow, LPARAM)
{
    PREFS_CONSOLE_LOG("OnInitDialog")
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
    CheckDlgButton(IDC_CB_NOCOMPSHADER, static_cast<UINT>(cfg_bSkipCompShader));

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

    n = static_cast<int>((static_cast<float>(cfg_fHardCutLoudnessThresh) - 1.25f) * 20.0f);
    if (n < 0)
        n = 0;
    if (n > 40)
        n = 40;
    ctrl = GetDlgItem(IDC_HARDCUT_LOUDNESS);
    SendMessage(ctrl, TBM_SETRANGEMIN, (WPARAM)FALSE, (LPARAM)0);
    SendMessage(ctrl, TBM_SETRANGEMAX, (WPARAM)FALSE, (LPARAM)40);
    SendMessage(ctrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)n);

    CheckDlgButton(IDC_CB_HARDCUTS, static_cast<UINT>(cfg_bHardCutsDisabled));

    // Pixel shaders.
    ctrl = GetDlgItem(IDC_SHADERS);
    AddItem(ctrl, buf, IDS_PS_AUTO_RECOMMENDED, unsigned(-1));
    AddItem(ctrl, buf, IDS_PS_DISABLED, MD2_PS_NONE);
    AddItem(ctrl, buf, IDS_PS_SHADER_MODEL_2, MD2_PS_2_0);
    AddItem(ctrl, buf, IDS_PS_SHADER_MODEL_4, MD2_PS_4_0);
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
    AutoHideGamma16();

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

    // Title and artwork format.
    swprintf_s(buf, L"%hs", cfg_szTitleFormat.get().c_str());
    SetDlgItemText(IDC_TITLE_FORMAT, buf);
    swprintf_s(buf, L"%hs", cfg_szArtworkFormat.get().c_str());
    SetDlgItemText(IDC_ARTWORK_FORMAT, buf);

    // Push buttons.
    milk2_config::initialize_paths();
    ::EnableWindow(GetDlgItem(ID_SPRITE), static_cast<BOOL>(std::filesystem::exists(default_szImgIniFile)));
    ::EnableWindow(GetDlgItem(ID_MSG), static_cast<BOOL>(std::filesystem::exists(default_szMsgIniFile)));

    // clang-format off
    const std::map<UINT16, UINT16> tips = {
        {(UINT16)IDC_CB_SCROLLON3, (UINT16)IDS_START_WITH_PRESET_LOCK_ON_HELP},
        {(UINT16)IDC_CB_NORATING2, (UINT16)IDS_DISABLE_PRESET_RATING_HELP},
        {(UINT16)IDC_CB_NOWARN3, (UINT16)IDS_NOWARN_HELP},
        {(UINT16)IDC_CB_PRESS_F1_MSG, (UINT16)IDS_HELP_ON_F1_HELP},
        {(UINT16)IDC_CB_SCROLLON4, (UINT16)IDS_SCROLL_CTRL_HELP},
        {(UINT16)IDC_CB_NOCOMPSHADER, (UINT16)IDS_NOCOMPSHADER_HELP},
        {(UINT16)IDC_FS_MAXFPS2, (UINT16)IDS_MAX_FRAMERATE_HELP},
        {(UINT16)IDC_CB_FSPT, (UINT16)IDS_PAGE_TEARING_HELP},
        {(UINT16)IDC_TITLE_FORMAT, (UINT16)IDS_TITLE_FORMAT_HELP},
        {(UINT16)IDC_ARTWORK_FORMAT, (UINT16)IDS_ARTWORK_FORMAT_HELP},
        {(UINT16)IDC_BETWEEN_TIME, (UINT16)IDS_BETWEEN_TIME_HELP},
        {(UINT16)IDC_BETWEEN_TIME_RANDOM, (UINT16)IDS_BETWEEN_TIME_RANDOM_HELP},
        {(UINT16)IDC_BLEND_AUTO, (UINT16)IDS_BLEND_AUTO_HELP},
        {(UINT16)IDC_BLEND_USER, (UINT16)IDS_BLEND_USER_HELP},
        {(UINT16)IDC_HARDCUT_BETWEEN_TIME, (UINT16)IDS_HARDCUT_BETWEEN_TIME_HELP},
        {(UINT16)IDC_HARDCUT_LOUDNESS, (UINT16)IDS_HARDCUT_LOUDNESS_HELP},
        {(UINT16)IDC_CB_HARDCUTS, (UINT16)IDS_HARDCUTS_HELP},
        {(UINT16)IDC_BRIGHT_SLIDER2, (UINT16)IDS_BRIGHT_SLIDER_HELP},
        {(UINT16)IDC_CB_AUTOGAMMA2, (UINT16)IDS_CB_AUTOGAMMA_HELP},
        {(UINT16)IDC_SONGTITLEANIM_DURATION, (UINT16)IDS_SONGTITLEANIM_DURATION_HELP},
        {(UINT16)IDC_RAND_TITLE, (UINT16)IDS_RAND_TITLE_HELP},
        {(UINT16)IDC_RAND_MSG, (UINT16)IDS_RAND_MSG_HELP},
        {(UINT16)IDC_CB_TITLE_ANIMS, (UINT16)IDS_TITLE_ANIMS_HELP},
        {(UINT16)IDC_MAX_IMAGES2, (UINT16)IDS_MAX_IMAGES_BYTES_HELP},
        {(UINT16)IDC_MAX_BYTES2, (UINT16)IDS_MAX_IMAGES_BYTES_HELP},
        {(UINT16)IDC_STRETCH2, (UINT16)IDS_CANVAS_STRETCH_HELP},
        {(UINT16)IDC_MESHSIZECOMBO, (UINT16)IDS_MESH_SIZE_HELP},
        {(UINT16)IDC_SHADERS, (UINT16)IDS_PIXEL_SHADERS_HELP},
        {(UINT16)IDC_TEXSIZECOMBO, (UINT16)IDS_CANVAS_SIZE_HELP},
        {(UINT16)ID_SPRITE, (UINT16)IDS_SPRITE},
        {(UINT16)ID_MSG, (UINT16)IDS_MSG},
        {(UINT16)ID_FONTS, (UINT16)IDS_FONTS_HELP},
    };
    // clang-format on
    m_tooltips.Create(get_wnd(), nullptr, nullptr, TTS_ALWAYSTIP | TTS_NOANIMATE);
    for (const auto& tip : tips)
    {
        LoadString(core_api::get_my_instance(), tip.second, buf, 256);
        m_tooltips.AddTool(CToolInfo(TTF_IDISHWND | TTF_SUBCLASS, m_hWnd, (UINT_PTR)GetDlgItem(tip.first).m_hWnd, nullptr, (LPWSTR)buf));
    }
    m_tooltips.SetMaxTipWidth(200);
    SetWindowTheme(m_tooltips, m_dark ? L"DarkMode_Explorer" : nullptr, nullptr);

    return FALSE;
}

LRESULT milk2_preferences_page::OnNotify(int idCtrl, LPNMHDR pnmh)
{
#ifndef _DEBUG
    UNREFERENCED_PARAMETER(idCtrl);
#endif
    UNREFERENCED_PARAMETER(pnmh);

    PREFS_CONSOLE_LOG("OnNotify ", idCtrl) //, static_cast<int>(pnmh))
    return static_cast<LRESULT>(0);
}

void milk2_preferences_page::OnClose()
{
    PREFS_CONSOLE_LOG("OnClose")
}

void milk2_preferences_page::OnDestroy()
{
    PREFS_CONSOLE_LOG("OnDestroy")
    //if (m_tooltips.IsWindow())
    //    m_tooltips.DestroyWindow();
}

void milk2_preferences_page::OnButtonPushed(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    UNREFERENCED_PARAMETER(uNotifyCode);
    UNREFERENCED_PARAMETER(wndCtl);

    switch (nID)
    {
        case ID_SPRITE:
            OpenToEdit(default_szImgIniFile, IMG_INIFILE);
            break;
        case ID_MSG:
            OpenToEdit(default_szMsgIniFile, MSG_INIFILE);
            break;
        case ID_FONTS:
            {
                FontDlg dlg; INT_PTR ret = dlg.DoModal(get_wnd(), reinterpret_cast<LPARAM>(this));
                if (ret == IDOK/* && HasChanged()*/) {}
            }
            break;
    }
}

void milk2_preferences_page::OnEditNotification(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    UNREFERENCED_PARAMETER(uNotifyCode);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(wndCtl);

    OnChanged();
}

void milk2_preferences_page::OnButtonClick(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    UNREFERENCED_PARAMETER(uNotifyCode);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(wndCtl);

    OnChanged();
    AutoHideGamma16();
}

void milk2_preferences_page::OnComboChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    UNREFERENCED_PARAMETER(uNotifyCode);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(wndCtl);

    OnChanged();
}

void milk2_preferences_page::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    UNREFERENCED_PARAMETER(nPos);

    PREFS_CONSOLE_LOG("OnHScroll")
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

void milk2_preferences_page::AutoHideGamma16()
{
    bool bAutoGamma = static_cast<bool>(IsDlgButtonChecked(IDC_CB_AUTOGAMMA2));
    ::ShowWindow(GetDlgItem(IDC_BRIGHT_SLIDER_BOX), static_cast<UINT>(!bAutoGamma));
    ::ShowWindow(GetDlgItem(IDC_BRIGHT_SLIDER2), static_cast<UINT>(!bAutoGamma));
    ::ShowWindow(GetDlgItem(IDC_GAMMA16_0), static_cast<UINT>(!bAutoGamma));
    ::ShowWindow(GetDlgItem(IDC_GAMMA16_1), static_cast<UINT>(!bAutoGamma));
    ::ShowWindow(GetDlgItem(IDC_GAMMA16_2), static_cast<UINT>(!bAutoGamma));
    ::ShowWindow(GetDlgItem(IDC_GAMMA16_3), static_cast<UINT>(!bAutoGamma));
    ::ShowWindow(GetDlgItem(IDC_GAMMA16_4), static_cast<UINT>(!bAutoGamma));
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

    CheckDlgButton(IDC_CB_SCROLLON3, static_cast<UINT>(default_bPresetLockOnAtStartup));
    CheckDlgButton(IDC_CB_SCROLLON4, static_cast<UINT>(default_bPreventScollLockHandling));
    CheckDlgButton(IDC_CB_NOWARN3, static_cast<UINT>(default_bWarningsDisabled2));
    CheckDlgButton(IDC_CB_NORATING2, static_cast<UINT>(!default_bEnableRating));
    CheckDlgButton(IDC_CB_PRESS_F1_MSG, static_cast<UINT>(default_bShowPressF1ForHelp));
    CheckDlgButton(IDC_CB_NOCOMPSHADER, static_cast<UINT>(default_bSkipCompShader));

    CheckDlgButton(IDC_CB_FSPT, static_cast<UINT>(default_allow_page_tearing_fs));
    UpdateMaxFps(FULLSCREEN);

    swprintf_s(buf, L"%2.1f", static_cast<float>(default_fTimeBetweenPresets));
    SetDlgItemText(IDC_BETWEEN_TIME, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(default_fTimeBetweenPresetsRand));
    SetDlgItemText(IDC_BETWEEN_TIME_RANDOM, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(default_fBlendTimeAuto));
    SetDlgItemText(IDC_BLEND_AUTO, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(default_fBlendTimeUser));
    SetDlgItemText(IDC_BLEND_USER, buf);

    swprintf_s(buf, L" %2.1f", static_cast<float>(default_fHardCutHalflife));
    SetDlgItemText(IDC_HARDCUT_BETWEEN_TIME, buf);
    n = static_cast<int>((static_cast<float>(default_fHardCutLoudnessThresh) - 1.25f) * 20.0f);
    if (n < 0)
        n = 0;
    if (n > 40)
        n = 40;
    SendMessage(GetDlgItem(IDC_HARDCUT_LOUDNESS), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)n);
    CheckDlgButton(IDC_CB_HARDCUTS, static_cast<UINT>(default_bHardCutsDisabled));

    SelectItemByValue(GetDlgItem(IDC_SHADERS), static_cast<DWORD>(default_nMaxPSVersion));

    //SelectItemByValue(GetDlgItem(IDC_TEXFORMAT), static_cast<DWORD>(default_nTexBitsPerCh));

    SelectItemByValue(GetDlgItem(IDC_MESHSIZECOMBO), static_cast<DWORD>(default_nGridX));

    SelectItemByValue(GetDlgItem(IDC_STRETCH2), static_cast<DWORD>(default_nCanvasStretch));

    SelectItemByValue(GetDlgItem(IDC_TEXSIZECOMBO), static_cast<LRESULT>(default_nTexSizeX));

    SendMessage(GetDlgItem(IDC_BRIGHT_SLIDER2), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)default_n16BitGamma);
    CheckDlgButton(IDC_CB_AUTOGAMMA2, static_cast<UINT>(default_bAutoGamma));
    AutoHideGamma16();

    SelectItemByValue(GetDlgItem(IDC_MAX_BYTES2), static_cast<DWORD>(default_nMaxBytes));

    SelectItemByValue(GetDlgItem(IDC_MAX_IMAGES2), static_cast<DWORD>(default_nMaxImages));

    swprintf_s(buf, L"%2.1f", static_cast<float>(default_fSongTitleAnimDuration));
    SetDlgItemText(IDC_SONGTITLEANIM_DURATION, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(default_fTimeBetweenRandomSongTitles));
    SetDlgItemText(IDC_RAND_TITLE, buf);
    swprintf_s(buf, L"%2.1f", static_cast<float>(default_fTimeBetweenRandomCustomMsgs));
    SetDlgItemText(IDC_RAND_MSG, buf);
    CheckDlgButton(IDC_CB_TITLE_ANIMS, static_cast<UINT>(default_bSongTitleAnims));

    swprintf_s(buf, L"%hs", default_szTitleFormat);
    SetDlgItemText(IDC_TITLE_FORMAT, buf);
    swprintf_s(buf, L"%hs", default_szArtworkFormat);
    SetDlgItemText(IDC_ARTWORK_FORMAT, buf);

    m_resetpage = true; // fonts

    OnChanged();
}

void milk2_preferences_page::apply()
{
    WCHAR buf[256], *stop;
    LRESULT t = 0;

    cfg_bPresetLockOnAtStartup = static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON3));
    cfg_bPreventScollLockHandling = static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON4));
    cfg_bWarningsDisabled = static_cast<bool>(IsDlgButtonChecked(IDC_CB_NOWARN3));
    cfg_bEnableRating = !static_cast<bool>(IsDlgButtonChecked(IDC_CB_NORATING2));
    cfg_bShowPressF1ForHelp = static_cast<bool>(IsDlgButtonChecked(IDC_CB_PRESS_F1_MSG));
    cfg_bSkipCompShader = static_cast<bool>(IsDlgButtonChecked(IDC_CB_NOCOMPSHADER));

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
        cfg_fHardCutLoudnessThresh = static_cast<double>(1.25f + t / 20.0f);
    cfg_bHardCutsDisabled = static_cast<bool>(IsDlgButtonChecked(IDC_CB_HARDCUTS));

    cfg_nMaxPSVersion = ReadCBValue(IDC_SHADERS);

    //cfg_nTexBitsPerCh = ReadCBValue(IDC_TEXFORMAT);

    cfg_nGridX = ReadCBValue(IDC_MESHSIZECOMBO);

    cfg_nCanvasStretch = ReadCBValue(IDC_STRETCH2);

    cfg_nTexSizeX = ReadCBValue(IDC_TEXSIZECOMBO);

    t = SendMessage(GetDlgItem(IDC_BRIGHT_SLIDER2), TBM_GETPOS, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        cfg_n16BitGamma = static_cast<int64_t>(t);
    cfg_bAutoGamma = static_cast<bool>(IsDlgButtonChecked(IDC_CB_AUTOGAMMA2));

    cfg_nMaxBytes = ReadCBValue(IDC_MAX_BYTES2);

    cfg_nMaxImages = ReadCBValue(IDC_MAX_IMAGES2);

    GetDlgItemText(IDC_SONGTITLEANIM_DURATION, buf, 256);
    cfg_fSongTitleAnimDuration = wcstof(buf, &stop);
    GetDlgItemText(IDC_RAND_TITLE, buf, 256);
    cfg_fTimeBetweenRandomSongTitles = wcstof(buf, &stop);
    GetDlgItemText(IDC_RAND_MSG, buf, 256);
    cfg_fTimeBetweenRandomCustomMsgs = wcstof(buf, &stop);
    cfg_bSongTitleAnims = static_cast<bool>(IsDlgButtonChecked(IDC_CB_TITLE_ANIMS));

    titleformat_object::ptr script;
    pfc::string8 pattern;
    GetDlgItemText(IDC_TITLE_FORMAT, buf, 256);
    pattern = pfc::utf8FromWide(buf);
    if (static_api_ptr_t<titleformat_compiler>()->compile(script, pattern))
    {
        cfg_szTitleFormat = pfc::utf8FromWide(buf);
    }
    else
    {
        SetDlgItemText(IDC_TITLE_FORMAT, L"<ERROR>");
    }
    GetDlgItemText(IDC_ARTWORK_FORMAT, buf, 256);
    pattern = pfc::utf8FromWide(buf);
    if (static_api_ptr_t<titleformat_compiler>()->compile(script, pattern))
    {
        cfg_szArtworkFormat = pfc::utf8FromWide(buf);
    }
    else
    {
        SetDlgItemText(IDC_ARTWORK_FORMAT, L"<ERROR>");
    }

    if (m_resetpage)
    {
        cfg_stFontInfo.set(default_stFontInfo, sizeof(td_fontinfo) * (NUM_BASIC_FONTS + NUM_EXTRA_FONTS));
        m_resetpage = false;
    }

    OnChanged(); // The dialog content has not changed but the flags have;
                 // the currently shown values now match the settings so the
                 // apply button can be disabled.
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
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_NOCOMPSHADER)) != cfg_bSkipCompShader) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_FSPT)) != cfg_allow_page_tearing_fs) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_HARDCUTS)) != cfg_bHardCutsDisabled) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_AUTOGAMMA2)) != cfg_bAutoGamma) ||
                            (static_cast<bool>(IsDlgButtonChecked(IDC_CB_TITLE_ANIMS)) != cfg_bSongTitleAnims);

    bool combobox_changes = false;
    LRESULT n = SendMessage(GetDlgItem(IDC_FS_MAXFPS2), CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        if (n > 0)
            n = MAX_MAX_FPS + 1 - n;
        combobox_changes = combobox_changes || (static_cast<UINT>(n) != cfg_max_fps_fs);
    }
    combobox_changes = combobox_changes ||
                       IsComboDiff(IDC_SHADERS, cfg_nMaxPSVersion) ||
                       //IsComboDiff(IDC_TEXFORMAT, cfg_nTexBitsPerCh) ||
                       IsComboDiff(IDC_MESHSIZECOMBO, cfg_nGridX) ||
                       IsComboDiff(IDC_STRETCH2, cfg_nCanvasStretch) ||
                       IsComboDiff(IDC_TEXSIZECOMBO, cfg_nTexSizeX) ||
                       IsComboDiff(IDC_MAX_BYTES2, cfg_nMaxBytes) ||
                       IsComboDiff(IDC_MAX_IMAGES2, cfg_nMaxImages);

    WCHAR buf[256] = {0}, *stop;
    pfc::string8 current;
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
    GetDlgItemText(IDC_TITLE_FORMAT, buf, 256);
    current = pfc::utf8FromWide(buf);
    editcontrol_changes = editcontrol_changes || !current.equals(cfg_szTitleFormat);
    GetDlgItemText(IDC_ARTWORK_FORMAT, buf, 256);
    current = pfc::utf8FromWide(buf);
    editcontrol_changes = editcontrol_changes || !current.equals(cfg_szArtworkFormat);

    LRESULT t;
    bool slider_changes = false;
    t = SendMessage(GetDlgItem(IDC_HARDCUT_LOUDNESS), TBM_GETPOS, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
    {
        if (t < 0)
            t = 0;
        if (t > 40)
            t = 40;
        int prev_slider_position = static_cast<int>((static_cast<float>(cfg_fHardCutLoudnessThresh) - 1.25f) * 20.0f);
        slider_changes = slider_changes || (static_cast<int>(t) != prev_slider_position);
    }
    t = SendMessage(GetDlgItem(IDC_BRIGHT_SLIDER2), TBM_GETPOS, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        slider_changes = slider_changes || (static_cast<UINT>(t) != static_cast<UINT>(cfg_n16BitGamma));

    return checkbox_changes || combobox_changes || editcontrol_changes || slider_changes;
}

// Tells the host that state has changed to enable or disable the "Apply"
// button appropriately.
void milk2_preferences_page::OnChanged()
{
    m_callback->on_state_changed();
}

inline void milk2_preferences_page::AddItem(HWND ctrl, LPWSTR buffer, UINT id, DWORD itemdata)
{
    LoadString(core_api::get_my_instance(), id, buffer, 256);
    AddItem(ctrl, buffer, itemdata);
}

inline void milk2_preferences_page::AddItem(HWND ctrl, LPCWSTR text, DWORD itemdata)
{
    LRESULT nPos = SendMessage(ctrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)text);
    SendMessage(ctrl, CB_SETITEMDATA, nPos, itemdata);
}

inline void milk2_preferences_page::SelectItemByPos(HWND ctrl, int pos)
{
    SendMessage(ctrl, CB_SETCURSEL, (WPARAM)pos, (LPARAM)0);
}

int milk2_preferences_page::SelectItemByValue(HWND ctrl, LRESULT value)
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

int64_t milk2_preferences_page::ReadCBValue(DWORD ctrl_id) const
{
    HWND ctrl = GetDlgItem(ctrl_id);
    LRESULT t = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (t != CB_ERR)
        return static_cast<int64_t>(SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)t, (LPARAM)0));
    else
        return -1;
}

bool milk2_preferences_page::IsComboDiff(DWORD ctrl_id, int64_t previous) const
{
    HWND ctrl = GetDlgItem(ctrl_id);
    LRESULT n = SendMessage(ctrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        n = SendMessage(ctrl, CB_GETITEMDATA, (WPARAM)n, (LPARAM)0);
        return static_cast<UINT>(n) != static_cast<UINT>(previous);
    }
    else
    {
        return false;
    }
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

void milk2_preferences_page::OpenToEdit(LPWSTR szDefault, LPCWSTR szFilename)
{

    if (szDefault[0] == L'\0')
        milk2_config::initialize_paths();
    wchar_t szPath[MAX_PATH]{}, szFile[MAX_PATH]{};
    wcscpy_s(szPath, szDefault);
    wchar_t* p = wcsrchr(szPath, L'\\');
    if (p != NULL)
    {
        *(p + 1) = L'\0';
        wcscpy_s(szFile, szPath);
        wcscat_s(szFile, szFilename);
        INT_PTR ret = reinterpret_cast<INT_PTR>(ShellExecute(NULL, L"open", szFile, NULL, szPath, SW_SHOWNORMAL));
        if (ret <= 32)
        {
            swprintf_s(szPath, L"Error Opening \"%ls\"", szFilename);
            wchar_t* str = WASABI_API_LNGSTRINGW(IDS_ERROR_IN_SHELLEXECUTE);
            MessageBox(str, szPath, MB_OK | MB_SETFOREGROUND | MB_TOPMOST | MB_TASKMODAL);
        }
    }
}

static preferences_page_factory_t<preferences_page_milk2> g_preferences_page_milk2_factory;
#pragma endregion

#pragma region Font Dialog
int CALLBACK EnumFontsProc(
    CONST LOGFONT* lplf, // logical font data
    CONST TEXTMETRIC* lptm, // physical font data
    DWORD dwType, // font type
    LPARAM lpData // application-defined data
)
{
    UNREFERENCED_PARAMETER(lptm);
    UNREFERENCED_PARAMETER(dwType);

    // Skip enumerating duplicate fonts.
    if (lplf->lfFaceName[0] == L'@' || lplf->lfFaceName[0] == L'8')
        return 1;

    SendMessage(GetDlgItem((HWND)lpData, IDC_FONT1), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));
    SendMessage(GetDlgItem((HWND)lpData, IDC_FONT2), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));
    SendMessage(GetDlgItem((HWND)lpData, IDC_FONT3), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));
    SendMessage(GetDlgItem((HWND)lpData, IDC_FONT4), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));
    SendMessage(GetDlgItem((HWND)lpData, IDC_FONT5), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));
    SendMessage(GetDlgItem((HWND)lpData, IDC_FONT6), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));
    SendMessage(GetDlgItem((HWND)lpData, IDC_FONT7), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));
    SendMessage(GetDlgItem((HWND)lpData, IDC_FONT8), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));
    SendMessage(GetDlgItem((HWND)lpData, IDC_FONT9), CB_ADDSTRING, 0, (LPARAM)(lplf->lfFaceName));

    return 1;
}

#define InitFont(n, m) InitFontI(&fonts[n - 1], IDC_FONT##n, IDC_FONTSIZE##n, IDC_FONTBOLD##n, IDC_FONTITAL##n, IDC_FONTAA##n, hdlg, IDC_FONT_NAME_##n, m)
void milk2_preferences_page::InitFontI(td_fontinfo* fi, DWORD ctrl1, DWORD ctrl2, DWORD bold_id, DWORD ital_id, DWORD aa_id, HWND hdlg, DWORD ctrl4, wchar_t* szFontName)
{
    HWND namebox = ctrl4 ? ::GetDlgItem(hdlg, ctrl4) : NULL;
    HWND fontbox = ::GetDlgItem(hdlg, ctrl1);
    HWND sizebox = ::GetDlgItem(hdlg, ctrl2);
    ::ShowWindow(fontbox, SW_NORMAL);
    ::ShowWindow(sizebox, SW_NORMAL);
    ::ShowWindow(::GetDlgItem(hdlg, bold_id), SW_NORMAL);
    ::ShowWindow(::GetDlgItem(hdlg, ital_id), SW_NORMAL);
    ::ShowWindow(::GetDlgItem(hdlg, aa_id), SW_NORMAL);
    if (namebox && szFontName && szFontName[0])
    {
        ::ShowWindow(namebox, SW_NORMAL);
        wchar_t buf[256];
        swprintf_s(buf, L"%s:", szFontName);
        ::SetWindowText(::GetDlgItem(hdlg, ctrl4), buf);
    }

    // Set selection.
    LRESULT nPos = ::SendMessage(fontbox, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)fi->szFace);
    if (nPos == CB_ERR)
        nPos = 0;
    ::SendMessage(fontbox, CB_SETCURSEL, nPos, 0);

    // Font size box.
    int nSel = 0;
    int nMax = ARRAYSIZE(g_nFontSize);
    for (int i = 0; i < nMax; i++)
    {
        wchar_t buf[256];
        int s = g_nFontSize[i]; //g_nFontSize[nMax - 1 - i];
        swprintf_s(buf, L" %-2d", s);
        ::SendMessage(sizebox, CB_ADDSTRING, i, (LPARAM)buf);
        if (s == fi->nSize)
            nSel = i;
    }
    ::SendMessage(sizebox, CB_SETCURSEL, nSel, 0);

    // Font options box.
    ::CheckDlgButton(hdlg, bold_id, fi->bBold);
    ::CheckDlgButton(hdlg, ital_id, fi->bItalic);
    ::CheckDlgButton(hdlg, aa_id, fi->bAntiAliased);
}

#define SaveFont(n) SaveFontI(&fonts[n - 1], IDC_FONT##n, IDC_FONTSIZE##n, IDC_FONTBOLD##n, IDC_FONTITAL##n, IDC_FONTAA##n, hdlg)
void milk2_preferences_page::SaveFontI(td_fontinfo* fi, DWORD ctrl1, DWORD ctrl2, DWORD bold_id, DWORD ital_id, DWORD aa_id, HWND hdlg)
{
    HWND fontbox = ::GetDlgItem(hdlg, ctrl1);
    HWND sizebox = ::GetDlgItem(hdlg, ctrl2);

    // Font face.
    LRESULT t = ::SendMessage(fontbox, CB_GETCURSEL, 0, 0);
    SendMessageW(fontbox, CB_GETLBTEXT, t, (LPARAM)fi->szFace);

    // Font size.
    t = ::SendMessage(sizebox, CB_GETCURSEL, 0, 0);
    if (t != CB_ERR)
    {
        //int nMax = ARRAYSIZE(g_nFontSize);
        fi->nSize = g_nFontSize[t]; //fi->nSize = g_nFontSize[nMax - 1 - t];
    }

    // Font options.
    fi->bBold = static_cast<bool>(::IsDlgButtonChecked(hdlg, bold_id)); //DlgItemIsChecked(hdlg, bold_id);
    fi->bItalic = static_cast<bool>(::IsDlgButtonChecked(hdlg, ital_id)); //DlgItemIsChecked(hdlg, ital_id);
    fi->bAntiAliased = static_cast<bool>(::IsDlgButtonChecked(hdlg, aa_id)); //DlgItemIsChecked(hdlg, aa_id);
}

void milk2_preferences_page::ScootControl(HWND hwnd, int ctrl_id, int dx, int dy)
{
    RECT r;
    ::GetWindowRect(::GetDlgItem(hwnd, ctrl_id), &r);
    ::ScreenToClient(hwnd, (LPPOINT)&r);
    ::SetWindowPos(::GetDlgItem(hwnd, ctrl_id), NULL, r.left + dx, r.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

BOOL milk2_preferences_page::PluginShellFontDialogProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
    OutputDebugMessage("FontDlgProc: ", hdlg, msg, wParam, lParam);
#else
    UNREFERENCED_PARAMETER(lParam);
#endif

    switch (msg)
    {
        case WM_INITDIALOG:
            {
                // Set the icon.
                HICON hIcon = ::LoadIcon(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDI_PLUGIN_ICON));
                ::SendMessage(hdlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                ::SendMessage(hdlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

                // Get the owner window and dialog box rectangles.
                HWND hwndOwner;
                RECT rc, rcDlg, rcOwner;
                if ((hwndOwner = ::GetParent(hdlg)) == NULL)
                {
                    hwndOwner = GetDesktopWindow();
                }

                ::GetWindowRect(hwndOwner, &rcOwner);
                ::GetWindowRect(hdlg, &rcDlg);
                CopyRect(&rc, &rcOwner);

                // Offset the owner and dialog box rectangles so that right and bottom
                // values represent the width and height, and then offset the owner again
                // to discard space taken up by the dialog box.
                OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
                OffsetRect(&rc, -rc.left, -rc.top);
                OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

                // The new position is the sum of half the remaining space and the owner's
                // original position. Ignores size arguments.
                ::SetWindowPos(hdlg, HWND_TOP, rcOwner.left + (rc.right / 2), rcOwner.top + (rc.bottom / 2), 0, 0, SWP_NOSIZE);

                // Finally, if not all extra fonts are in use, shrink the window size and
                // move up any controls that were at the bottom.
                RECT r;
                ::GetWindowRect(hdlg, &r);
                if constexpr (MAX_EXTRA_FONTS - NUM_EXTRA_FONTS > 0)
                {
                    int scoot_factor = static_cast<int>(176.0f * (MAX_EXTRA_FONTS - NUM_EXTRA_FONTS) / static_cast<float>(MAX_EXTRA_FONTS));
                    ::SetWindowPos(hdlg, NULL, 0, 0, r.right - r.left, r.bottom - r.top - scoot_factor, SWP_NOMOVE | SWP_NOZORDER);
                    ScootControl(hdlg, IDC_FONT_TEXT, 0, -scoot_factor);
                    ScootControl(hdlg, IDOK, 0, -scoot_factor);
                    ScootControl(hdlg, IDCANCEL, 0, -scoot_factor);
                }

                HDC hdc = ::GetDC(hdlg);
                if (hdc)
                {
                    EnumFonts(hdc, NULL, &EnumFontsProc, (LPARAM)hdlg);
                    ::ReleaseDC(hdlg, hdc);
                }

                td_fontinfo fonts[NUM_BASIC_FONTS + NUM_EXTRA_FONTS]{};
                auto v = cfg_stFontInfo.get();
                if (v.is_valid() && v->size() == sizeof(fonts))
                    memcpy_s(fonts, sizeof(fonts), v->get_ptr(), v->size());

                InitFont(1, 0);
                InitFont(2, 0);
                InitFont(3, 0);
                InitFont(4, 0);
#if (NUM_EXTRA_FONTS >= 1)
                InitFont(5, WASABI_API_LNGSTRINGW(IDS_EXTRA_FONT_1_NAME));
#endif
#if (NUM_EXTRA_FONTS >= 2)
                InitFont(6, WASABI_API_LNGSTRINGW(IDS_EXTRA_FONT_2_NAME));
#endif
#if (NUM_EXTRA_FONTS >= 3)
                InitFont(7, EXTRA_FONT_3_NAME);
#endif
#if (NUM_EXTRA_FONTS >= 4)
                InitFont(5, EXTRA_FONT_4_NAME);
#endif
#if (NUM_EXTRA_FONTS >= 5)
                InitFont(9, EXTRA_FONT_5_NAME);
#endif
            }
            break;
        case WM_COMMAND:
            {
                int id = LOWORD(wParam);
                switch (id)
                {
                    case IDOK:
                        {
                            td_fontinfo fonts[NUM_BASIC_FONTS + NUM_EXTRA_FONTS]{};
                            SaveFont(1);
                            SaveFont(2);
                            SaveFont(3);
                            SaveFont(4);
#if (NUM_EXTRA_FONTS >= 1)
                            SaveFont(5);
#endif
#if (NUM_EXTRA_FONTS >= 2)
                            SaveFont(6);
#endif
#if (NUM_EXTRA_FONTS >= 3)
                            SaveFont(7);
#endif
#if (NUM_EXTRA_FONTS >= 4)
                            SaveFont(5);
#endif
#if (NUM_EXTRA_FONTS >= 5)
                            SaveFont(9);
#endif
                            cfg_stFontInfo.set(fonts, sizeof(fonts));
                        }
                        ::EndDialog(hdlg, id);
                        break;
                    case IDCANCEL:
                        ::EndDialog(hdlg, id);
                        break;
                }
            }
            break;
        case WM_DESTROY:
            return 0;
    }

    return FALSE;
}
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
    settings.m_bShowAlbum = default_bShowAlbum;
    settings.m_bEnableHDR = default_bEnableHDR;
    settings.m_bSkipCompShader = static_cast<uint32_t>(cfg_bSkipCompShader);
    settings.m_nBackBufferFormat = default_nBackBufferFormat;
    settings.m_nDepthBufferFormat = default_nDepthBufferFormat;
    settings.m_nBackBufferCount = default_nBackBufferCount;
    settings.m_nMinFeatureLevel = default_nMinFeatureLevel;
    //settings.m_nFpsLimit = default_nFpsLimit;

    swprintf_s(settings.m_szTitleFormat, L"%ls", pfc::wideFromUTF8(cfg_szTitleFormat.get()).c_str());
    swprintf_s(settings.m_szArtworkFormat, L"%ls", pfc::wideFromUTF8(cfg_szArtworkFormat.get()).c_str());

    //--- Paths
    update_paths();

    //--- Fonts
    update_fonts();
}

// Initializes all font dialog variables.
void milk2_config::update_fonts() const
{
    auto v = cfg_stFontInfo.get();
    if (v.is_valid() && v->size() == sizeof(settings.m_fontinfo))
        memcpy_s((void*)settings.m_fontinfo, sizeof(settings.m_fontinfo), v->get_ptr(), v->size());
}

// Resolves profile directory, taking care of the case where the path contains
// non-ASCII characters.
void milk2_config::resolve_profile()
{
    // Get profile directory path through foobar2000 API.
#if 0
    std::string api_path = core_api::get_profile_path();
    assert(strncmp(api_path.substr(0, 7).c_str(), "file://", 7) == 0);
    api_path = api_path.substr(strlen("file://"));
    api_path.append("\\");
#else
    pfc::string8 native_path = filesystem::g_get_native_path(core_api::get_profile_path());
    native_path.end_with_slash();
    size_t path_length = native_path.get_length();
    std::wstring profile_path(path_length + 1, L'\0');
#if 0
    pfc::stringcvt::string_os_from_utf8 os_path(native_path.get_ptr());
    profile_path = os_path.get_ptr();
#else
    path_length = pfc::stringcvt::convert_utf8_to_wide(const_cast<wchar_t*>(profile_path.c_str()), profile_path.size(), native_path.get_ptr(), path_length);
    //profile_path = profile_path.c_str(); // or profile_dir_path.erase(profile_dir_path.find(L'\0'));
#endif
#endif

    swprintf_s(default_szPluginsDirPath, L"%ls", profile_path.c_str());

#if 0
    // Get profile directory path through Win32 API.
    wchar_t foobar_exe_path[MAX_PATH];
    GetModuleFileName(NULL, foobar_exe_path, MAX_PATH);
    std::wstring profile_dir_path = foobar_exe_path;
    size_t t = profile_dir_path.find_last_of(L"\\");
    if (t != std::wstring::npos)
        profile_dir_path.erase(t + 1);
    profile_dir_path.append(L"profile\\");

    // Use Win32 string if it mismatches with the foobar2000 string.
    if (wcscmp(default_szPluginsDirPath, profile_dir_path.c_str()))
    {
        profile_dir_path.copy(default_szPluginsDirPath, profile_dir_path.length());
        default_szPluginsDirPath[profile_dir_path.length()] = L'\0';
    }
#endif
}

void milk2_config::initialize_paths()
{
    //std::string component_path = core_api::get_my_full_path();
    //std::string::size_type t = component_path.rfind('\\');
    //if (t != std::string::npos)
    //    component_path.erase(t + 1);

    resolve_profile(); // sets `default_szPluginsDirPath`
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
    if (m_version < 2 || cfg_szPresetDir.get().empty())
    {
        CHAR szPresetDirA[MAX_PATH];
        wcscpy_s(settings.m_szPresetDir, default_szPresetDir);
        wcstombs_s(nullptr, szPresetDirA, default_szPresetDir, MAX_PATH);
        cfg_szPresetDir.set(szPresetDirA);
    }
    else
    {
        size_t convertedChars;
        cfg_szPresetDir.get().end_with_slash();
        mbstowcs_s(&convertedChars, settings.m_szPresetDir, cfg_szPresetDir.get().c_str(), MAX_PATH);
    }
    wcscpy_s(settings.m_szPluginsDirPath, default_szPluginsDirPath);
    wcscpy_s(settings.m_szConfigIniFile, default_szConfigIniFile);
    wcscpy_s(settings.m_szMsgIniFile, default_szMsgIniFile);
    wcscpy_s(settings.m_szImgIniFile, default_szImgIniFile);
}

// Reads the configuration from the foobar2000 configuration system.
// From foobar2000's perspective: sets the component's running configuration.
void milk2_config::parse(ui_element_config_parser& parser)
{
    try
    {
        uint32_t sentinel, version;
        parser >> sentinel;
        parser >> version;
        switch (version)
        {
            case 3:
                //parser.read_raw(settings.m_fontinfo, sizeof(settings.m_fontinfo));
                break;
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

// Writes the configuration into the foobar2000 configuration system.
// From foobar2000's perspective: gets/saves the component's running configuration.
void milk2_config::build(ui_element_config_builder& builder, const bool full_restore) const
{
    builder << m_sentinel;
    builder << m_version;

    if (full_restore) {
        //builder.write_raw(settings.m_fontinfo, sizeof(settings.m_fontinfo));

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
        cfg_bSkipCompShader = settings.m_bSkipCompShader;

        cfg_bShowFPS = settings.m_bShowFPS;
        cfg_bShowRating = settings.m_bShowRating;
        cfg_bShowPresetInfo = settings.m_bShowPresetInfo;
        //cfg_bShowDebugInfo = settings.m_bShowDebugInfo;
        cfg_bShowSongTitle = settings.m_bShowSongTitle;
        cfg_bShowSongTime = settings.m_bShowSongTime;
        cfg_bShowSongLen = settings.m_bShowSongLen;
        cfg_bShowShaderHelp = settings.m_bShowShaderHelp;

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
        //builder << settings.m_nTexSizeY;
        //builder << settings.m_bTexSizeWasAutoPow2;
        //builder << settings.m_bTexSizeWasAutoExact;
        cfg_nTexBitsPerCh = settings.m_nTexBitsPerCh;
        cfg_nGridX = settings.m_nGridX;
        //builder << settings.m_nGridY;
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

        cfg_bPresetLockedByCode = settings.m_bPresetLockedByCode;
        //builder << settings.m_bPresetLockedByUser;
        //builder << settings.m_bMilkdropScrollLockState;
        //builder << settings.m_nFpsLimit;

        cfg_nBackBufferFormat = settings.m_nBackBufferFormat;
        cfg_nDepthBufferFormat = settings.m_nDepthBufferFormat;
        cfg_nBackBufferCount = settings.m_nBackBufferCount;
        cfg_nMinFeatureLevel = settings.m_nMinFeatureLevel;
        cfg_bEnableDownmix = settings.m_bEnableDownmix;
        cfg_bShowAlbum = settings.m_bShowAlbum;
        cfg_bEnableHDR = settings.m_bEnableHDR;

        cfg_szTitleFormat = pfc::utf8FromWide(settings.m_szTitleFormat);
        cfg_szArtworkFormat = pfc::utf8FromWide(settings.m_szArtworkFormat);

        CHAR szPresetDirA[MAX_PATH];
        wcstombs_s(nullptr, szPresetDirA, settings.m_szPresetDir, MAX_PATH);
        cfg_szPresetDir.set(szPresetDirA);
    }
}
#pragma endregion