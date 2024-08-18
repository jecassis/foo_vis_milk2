/*
 * config.h - MilkDrop 2 visualization component's configuration header
 *            file.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "resource.h"
#include "settings.h"

static constexpr GUID guid_cfg_bPresetLockOnAtStartup = {
    0x8a6c8c08, 0xc298, 0x4485, {0xb9, 0xb5, 0xa3, 0x1d, 0xd4, 0xed, 0xfa, 0x4b}
}; // {8A6C8C08-C298-4485-B9B5-A31DD4EDFA4B}
static constexpr GUID guid_cfg_bPreventScollLockHandling = {
    0x96ba1b84, 0x1ff3, 0x47d3, {0x81, 0x9f, 0x35, 0xa7, 0xdb, 0xff, 0xde, 0x37}
}; // {96BA1B84-1FF3-47D3-819F-35A7DBFFDE37}
static constexpr GUID guid_cfg_bWarningsDisabled = {
    0x79f08d12, 0x115d, 0x4fd1, {0x94, 0xab, 0x8, 0x71, 0x93, 0xd3, 0xac, 0x7c}
}; // {79F08D12-115D-4FD1-94AB-087193D3AC7C}
static constexpr GUID guid_cfg_bEnableRating = {
    0x99902062, 0xe27a, 0x483c, {0x80, 0x83, 0xe2, 0x7b, 0x85, 0xec, 0x5e, 0x7a}
}; // {99902062-E27A-483C-8083-E27B85EC5E7A}
static constexpr GUID guid_cfg_max_fps_fs = {
    0x2c669890, 0x66fb, 0x4419, {0xa9, 0xb7, 0x93, 0xc8, 0x24, 0xf2, 0xfd, 0x19}
}; // {2C669890-66FB-4419-A9B7-93C824F2FD19}
static constexpr GUID guid_cfg_bShowPressF1ForHelp = {
    0x4c2e81ab, 0xfbde, 0x4c22, {0x81, 0x95, 0xe9, 0x1f, 0x74, 0x9, 0x59, 0x9c}
}; // {4C2E81AB-FBDE-4C22-8195-E91F7409599C}
static constexpr GUID guid_cfg_allow_page_tearing_fs = {
    0x1df637d, 0xb5ee, 0x455d, {0xa6, 0xe6, 0x15, 0x61, 0x94, 0x4f, 0xfe, 0x60}
}; // {01DF637D-B5EE-455D-A6E6-1561944FFE60}
static constexpr GUID guid_cfg_fTimeBetweenPresets = {
    0xd6b27268, 0x457, 0x411a, {0xbc, 0x48, 0x56, 0xdb, 0x17, 0x60, 0x37, 0x2c}
}; // {D6B27268-0457-411A-BC48-56DB1760372C}
static constexpr GUID guid_cfg_fTimeBetweenPresetsRand = {
    0x7a1df54c, 0x82bb, 0x4e71, {0x9c, 0x8e, 0x8f, 0xf1, 0xb, 0x86, 0xdd, 0x4e}
}; // {7A1DF54C-82BB-4E71-9C8E-8FF10B86DD4E}
static constexpr GUID guid_cfg_fBlendTimeAuto = {
    0x53f17431, 0xfbf0, 0x47a6, {0xb1, 0x6c, 0x63, 0xc8, 0xa7, 0xa8, 0x80, 0xbe}
}; // {53F17431-FBF0-47A6-B16C-63C8A7A880BE}
static constexpr GUID guid_cfg_fBlendTimeUser = {
    0x901d5b5c, 0x5038, 0x4236, {0x8b, 0xd3, 0x6a, 0x53, 0x42, 0xd2, 0x45, 0x23}
}; // {901D5B5C-5038-4236-8BD3-6A5342D24523}
static constexpr GUID guid_cfg_fHardCutHalflife = {
    0xee9f3690, 0x5383, 0x4358, {0x88, 0x30, 0x6d, 0x8, 0xe5, 0x4, 0x36, 0x34}
}; // {EE9F3690-5383-4358-8830-6D08E5043634}
static constexpr GUID guid_cfg_fHardCutLoudnessThresh = {
    0x5dc9f051, 0xf880, 0x43df, {0xb4, 0x8c, 0xdf, 0x78, 0x15, 0x66, 0xf3, 0x5e}
}; // {5DC9F051-F880-43DF-B48C-DF781566F35E}
static const GUID guid_cfg_bHardCutsDisabled = {
    0x5d1a0e92, 0x3f95, 0x410c, {0xad, 0x66, 0xb4, 0xa5, 0xfa, 0x4f, 0xdc, 0xf4}
}; // {5D1A0E92-3F95-410C-AD66-B4A5FA4FDCF4}
static const GUID guid_cfg_n16BitGamma = {
    0x350ba0d5, 0xee5b, 0x4886, {0x96, 0x3, 0x7f, 0xd4, 0xf5, 0xf8, 0x1b, 0x10}
}; // {350BA0D5-EE5B-4886-9603-7FD4F5F81B10}
static const GUID guid_cfg_bAutoGamma = {
    0xe40b6e03, 0xdeea, 0x4515, {0x96, 0xb2, 0xd0, 0xe0, 0xe6, 0x8b, 0x58, 0x86}
}; // {E40B6E03-DEEA-4515-96B2-D0E0E68B5886}
static constexpr GUID guid_cfg_nMaxImages = {
    0x779da878, 0xf890, 0x4dd5, {0x9b, 0xb1, 0xb8, 0xfc, 0x64, 0x3d, 0xa, 0xfb}
}; // {779DA878-F890-4DD5-9BB1-B8FC643D0AFB}
static constexpr GUID guid_cfg_nMaxBytes = {
    0x904342ae, 0x2844, 0x4970, {0x9c, 0x2, 0x8e, 0xc4, 0xcc, 0x75, 0x26, 0x9f}
}; // {904342AE-2844-4970-9C02-8EC4CC75269F}
static constexpr GUID guid_cfg_nMaxPSVersion = {
    0xcb39278b, 0x4c19, 0x4931, {0x92, 0x63, 0x63, 0x1f, 0x69, 0x24, 0xf1, 0xc4}
}; // {CB39278B-4C19-4931-9263-631F6924F1C4}
static const GUID guid_cfg_bSongTitleAnims = {
    0x7ff565aa, 0x8402, 0x4ae0, {0x99, 0xc7, 0x11, 0x18, 0x44, 0x1d, 0xee, 0xc2}
}; // {7FF565AA-8402-4AE0-99C7-1118441DEEC2}
static const GUID guid_cfg_fSongTitleAnimDuration = {
    0xe539e22c, 0x2c41, 0x4238, {0xae, 0xa9, 0x52, 0x25, 0xda, 0x29, 0xba, 0xcf}
}; // {E539E22C-2C41-4238-AEA9-5225DA29BACF}
static const GUID guid_cfg_fTimeBetweenRandomSongTitles = {
    0xbea5f8e5, 0x48d1, 0x4063, {0xa1, 0xf2, 0x9d, 0x2, 0xfd, 0x33, 0xce, 0x4d}
}; // {BEA5F8E5-48D1-4063-A1F2-9D02FD33CE4D}
static const GUID guid_cfg_fTimeBetweenRandomCustomMsgs = {
    0xd7778394, 0x8ed4, 0x4d0b, {0xb8, 0xec, 0x2e, 0x1d, 0xfb, 0x25, 0x49, 0x74}
}; // {D7778394-8ED4-4D0B-B8EC-2E1DFB254974}
static const GUID guid_cfg_nCanvasStretch = {
    0xce121917, 0xc83d, 0x4a0b, {0xb7, 0x4c, 0x56, 0xf8, 0x3c, 0x97, 0xbe, 0x6c}
}; // {CE121917-C83D-4A0B-B74C-56F83C97BE6C}
static const GUID guid_cfg_nGridX = {
    0xacf37191, 0xfbb4, 0x4ffa, {0x95, 0x24, 0xd1, 0x17, 0xba, 0x11, 0x4, 0x47}
}; // {ACF37191-FBB4-4FFA-9524-D117BA110447}
static const GUID guid_cfg_nTexSizeX = {
    0xa2cd1e44, 0x9056, 0x4a2c, {0x97, 0x9e, 0x5b, 0xa4, 0x52, 0x34, 0x80, 0x3e}
}; // {A2CD1E44-9056-4A2C-979E-5BA45234803E}
static const GUID guid_cfg_nTexBitsPerCh = {
    0xab5a6d53, 0xb4c9, 0x41c3, {0xa5, 0x66, 0x1d, 0x83, 0x90, 0xdb, 0x1e, 0xfa}
}; // {AB5A6D53-B4C9-41C3-A566-1D8390DB1EFA}
static constexpr GUID guid_advconfig_branch = {
    0xd7ab1cd7, 0x7956, 0x4497, {0x9b, 0x1d, 0x74, 0x78, 0x7e, 0xde, 0x1d, 0xbc}
}; // {D7AB1CD7-7956-4497-9B1D-74787EDE1DBC}
static constexpr GUID guid_cfg_bDebugOutput = {
    0x808a73c, 0x8857, 0x4472, {0xad, 0x49, 0xdb, 0x1a, 0x48, 0x4e, 0x3f, 0x5}
}; // {0808A73C-8857-4472-AD49-DB1A484E3F05}
static constexpr GUID guid_cfg_szPresetDir = {
    0xfa9e467b, 0xfe6d, 0x4d79, {0x83, 0x98, 0xcd, 0x3d, 0x8b, 0xf4, 0x7a, 0x63}
}; // {FA9E467B-FE6D-4D79-8398-CD3D8BF47A63}
static constexpr GUID guid_cfg_szTitleFormat = {
    0xca37e590, 0xb292, 0x43b5, {0xa2, 0x34, 0x3d, 0xba, 0x21, 0xa7, 0xa0, 0xdb}
}; // {CA37E590-B292-43B5-A234-3DBA21A7A0DB}
static const GUID guid_cfg_bSkip8Conversion = {
    0xa5fce654, 0xe371, 0x4d3b, {0xae, 0x4, 0x62, 0xc5, 0xba, 0x57, 0x68, 0x20}
}; // {A5FCE654-E371-4D3B-AE04-62C5BA576820}
static const GUID guid_cfg_bSkipCompShader = {
    0xa9220355, 0x1382, 0x41f3, {0xbd, 0x41, 0x7e, 0xb3, 0xe, 0x94, 0xa6, 0x42}
}; // {A9220355-1382-41F3-BD41-7EB30E94A642}

// Defaults
// `milk.ini` defaults
//constexpr int default_nFpsLimit = -1;
static constexpr bool default_bEnableRating = true;
static constexpr bool default_bHardCutsDisabled = true;
static constexpr bool default_bDebugOutput = false;
//static constexpr bool default_bShowSongInfo;
static constexpr bool default_bShowPressF1ForHelp = true;
//static constexpr bool default_bShowMenuToolTips;
static constexpr bool default_bSongTitleAnims = true;
static constexpr bool default_bShowFPS = false;
static constexpr bool default_bShowRating = false;
static constexpr bool default_bShowPresetInfo = false;
//static constexpr bool default_bShowDebugInfo = false;
static constexpr bool default_bShowSongTitle = false;
static constexpr bool default_bShowSongTime = false;
static constexpr bool default_bShowSongLen = false;
//static constexpr bool default_bFixPinkBug = -1;
static constexpr int default_n16BitGamma = 2;
static constexpr bool default_bAutoGamma = true;
//static constexpr bool default_bAlways3D = false;
static constexpr float default_fStereoSep = 1.0f;
//static constexpr bool default_bFixSlowText = true;
//static constexpr bool default_bAlwaysOnTop = false;
//static constexpr bool default_bWarningsDisabled = false;
static constexpr bool default_bWarningsDisabled2 = false;
//static constexpr bool default_bAnisotropicFiltering = true;
static constexpr bool default_bPresetLockOnAtStartup = false;
static constexpr bool default_bPreventScollLockHandling = true; //false
static constexpr int default_nCanvasStretch = 0;
static constexpr int default_nTexSizeX = -1; // -1 means "auto"
//static constexpr int default_nTexSizeY = -1; // -1 means "auto"
//static constexpr bool default_bTexSizeWasAutoPow2 = false;
//static constexpr bool default_bTexSizeWasAutoExact = false;
static constexpr int default_nTexBitsPerCh = 8;
static constexpr int default_nGridX = 48; //32;
//static constexpr int default_nGridY = 36; //24;
static constexpr int default_nMaxPSVersion = -1; // -1 = auto, 0 = disable shaders, 2 = ps_2_0, 3 = ps_3_0
static constexpr int default_nMaxImages = 32;
static constexpr int default_nMaxBytes = 16000000;
static constexpr bool default_bPresetLockedByCode = false;
static constexpr bool default_bShowShaderHelp = false;
static constexpr float default_fBlendTimeUser = 1.7f;
static constexpr float default_fBlendTimeAuto = 2.7f;
static constexpr float default_fTimeBetweenPresets = 16.0f;
static constexpr float default_fTimeBetweenPresetsRand = 10.0f;
static constexpr float default_fHardCutLoudnessThresh = 2.5f;
static constexpr float default_fHardCutHalflife = 60.0f;
static constexpr float default_fSongTitleAnimDuration = 1.7f;
static constexpr float default_fTimeBetweenRandomSongTitles = -1.0f;
static constexpr float default_fTimeBetweenRandomCustomMsgs = -1.0f;
static constexpr bool default_bEnableDownmix = false;
static constexpr bool default_bEnableHDR = false;
static constexpr int default_nBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
static constexpr int default_nDepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
static constexpr int default_nBackBufferCount = 2;
static constexpr int default_nMinFeatureLevel = D3D_FEATURE_LEVEL_9_1;
static constexpr UINT default_max_fps_fs = 30;
static constexpr bool default_allow_page_tearing_fs = false;
static constexpr const char* default_szTitleFormat = "%title%";
static constexpr bool default_bSkip8Conversion = false;
static constexpr bool default_bSkipCompShader = false;

static WCHAR default_szPluginsDirPath[MAX_PATH];
static WCHAR default_szConfigIniFile[MAX_PATH];
static WCHAR default_szMilkdrop2Path[MAX_PATH];
static WCHAR default_szPresetDir[MAX_PATH];
static CHAR default_szPresetDirA[MAX_PATH];
static WCHAR default_szMsgIniFile[MAX_PATH];
static WCHAR default_szImgIniFile[MAX_PATH];

// Advanced preferences order
enum
{
    order_bDebugOutput,
    order_szPresetDir,
};

class milk2_preferences_page : public preferences_page_instance, public CDialogImpl<milk2_preferences_page>
{
  public:
    milk2_preferences_page(preferences_page_callback::ptr callback) : m_callback(callback), m_bMsgHandled(TRUE) {}

    enum milk2_dialog_id
    {
        IDD = IDD_PREFS // Dialog resource ID
    };

    uint32_t get_state();
    HWND get_wnd() { return *this; }
    void apply();
    void reset();

    // clang-format off
    BEGIN_MSG_MAP_EX(milk2_preferences_page)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_HSCROLL(OnHScroll)
        COMMAND_HANDLER_EX(IDC_CB_SCROLLON3, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_SCROLLON4, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_NOWARN3, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_NORATING2, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_PRESS_F1_MSG, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_SKIP8, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_NOCOMPSHADER, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_W_MAXFPS2, CBN_SELCHANGE, OnComboChange)
        COMMAND_HANDLER_EX(IDC_FS_MAXFPS2, CBN_SELCHANGE, OnComboChange)
        COMMAND_HANDLER_EX(IDC_CB_WPT, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_FSPT, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_BETWEEN_TIME, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(IDC_BETWEEN_TIME_RANDOM, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(IDC_BLEND_AUTO, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(IDC_BLEND_USER, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(IDC_HARDCUT_BETWEEN_TIME, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(IDC_CB_HARDCUTS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_SHADERS, CBN_SELCHANGE, OnComboChange)
        //COMMAND_HANDLER_EX(IDC_TEXFORMAT, CBN_SELCHANGE, OnComboChange)
        COMMAND_HANDLER_EX(IDC_MESHSIZECOMBO, CBN_SELCHANGE, OnComboChange)
        COMMAND_HANDLER_EX(IDC_STRETCH2, CBN_SELCHANGE, OnComboChange)
        COMMAND_HANDLER_EX(IDC_TEXSIZECOMBO, CBN_SELCHANGE, OnComboChange)
        COMMAND_HANDLER_EX(IDC_CB_AUTOGAMMA2, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_MAX_BYTES2, CBN_SELCHANGE, OnComboChange)
        COMMAND_HANDLER_EX(IDC_MAX_IMAGES2, CBN_SELCHANGE, OnComboChange)
        COMMAND_HANDLER_EX(IDC_BETWEEN_TIME, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(IDC_SONGTITLEANIM_DURATION, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(IDC_RAND_TITLE, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(IDC_RAND_MSG, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(IDC_CB_TITLE_ANIMS, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_TITLE_FORMAT, EN_CHANGE, OnEditNotification)
    END_MSG_MAP()
    // clang-format on

  private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnEditNotification(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnButtonClick(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnComboChange(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
    bool HasChanged() const;
    void OnChanged();

    inline void AddItem(HWND ctrl, wchar_t* buffer, UINT id, DWORD itemdata);
    inline void AddItem(HWND ctrl, const wchar_t* text, DWORD itemdata);
    inline void SelectItemByPos(HWND ctrl, int pos);
    int SelectItemByValue(HWND ctrl, LRESULT value);
    int64_t ReadCBValue(DWORD ctrl_id) const;
    bool IsComboDiff(DWORD ctrl_id, int64_t previous) const;
    wchar_t* FormImageCacheSizeString(const wchar_t* itemStr, const UINT sizeID);
    void UpdateMaxFps(int screenmode) const;
    void SaveMaxFps(int screenmode) const;

    const preferences_page_callback::ptr m_callback;

    fb2k::CDarkModeHooks m_dark;
};

class milk2_config
{
  public:
    milk2_config();

    uint32_t get_version() const;
    uint32_t get_sentinel() const;
    void init();
    void reset();
    void parse(ui_element_config_parser& parser);
    void build(ui_element_config_builder& builder);

    // `milk2.ini`
    plugin_config settings{};

  private:
    uint32_t m_sentinel = ('M' << 24 | 'I' << 16 | 'L' << 8 | 'K');
    uint32_t m_version = 2;

    static void initialize_paths();
    void update_paths();
};