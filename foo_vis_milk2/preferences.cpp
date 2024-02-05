//
// preferences.cpp: Configuration settings accessible through a preferences page and advanced preferences.
//

#include "pch.h"
#include "resource.h"
#include "config.h"

extern HWND g_hWindow;

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

class milk2_preferences_page : public preferences_page_instance, public CDialogImpl<milk2_preferences_page>
{
  public:
    milk2_preferences_page(preferences_page_callback::ptr callback) : m_callback(callback) {}

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

    inline void AddItem(HWND ctrl, const wchar_t* text, DWORD itemdata);
    inline void SelectItemByPos(HWND ctrl, int pos);
    int SelectItemByValue(HWND ctrl, DWORD value);
    wchar_t* FormImageCacheSizeString(const wchar_t* itemStr, const UINT sizeID);
    void UpdateMaxFps(int screenmode) const;
    void SaveMaxFps(int screenmode) const;

    const preferences_page_callback::ptr m_callback;

    fb2k::CDarkModeHooks m_dark;
};

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
    LoadString(core_api::get_my_instance(), IDS_PS_AUTO_RECOMMENDED, buf, 256);
    AddItem(ctrl, buf, unsigned(-1));
    LoadString(core_api::get_my_instance(), IDS_PS_DISABLED, buf, 256);
    AddItem(ctrl, buf, MD2_PS_NONE);
    LoadString(core_api::get_my_instance(), IDS_PS_SHADER_MODEL_2, buf, 256);
    AddItem(ctrl, buf, MD2_PS_2_0);
    LoadString(core_api::get_my_instance(), IDS_PS_SHADER_MODEL_3, buf, 256);
    AddItem(ctrl, buf, MD2_PS_3_0);
    SelectItemByPos(ctrl, 0); // as a safe default
    SelectItemByValue(ctrl, static_cast<DWORD>(cfg_nMaxPSVersion));

    // Texture format.
    //ctrl = GetDlgItem(IDC_TEXFORMAT);
    //LoadString(core_api::get_my_instance(), IDS_TX_8_BITS_PER_CHANNEL, buf, 256);
    //AddItem(ctrl, buf, 8);
    ////AddItem(ctrl, " 10 bits per channel", 10);
    //LoadString(core_api::get_my_instance(), IDS_TX_16_BITS_PER_CHANNEL, buf, 256);
    //AddItem(ctrl, buf, 16);
    //LoadString(core_api::get_my_instance(), IDS_TX_32_BITS_PER_CHANNEL, buf, 256);
    //AddItem(ctrl, buf, 32);
    //SelectItemByPos(ctrl, 0); // as a safe default
    //SelectItemByValue(ctrl, static_cast<DWORD>(cfg_nTexBitsPerCh));

    // Mesh size.
    ctrl = GetDlgItem(IDC_MESHSIZECOMBO);
    LoadString(core_api::get_my_instance(), IDS_8X6_FAST, buf, 256);
    AddItem(ctrl, buf, 8);
    LoadString(core_api::get_my_instance(), IDS_16X12_FAST, buf, 256);
    AddItem(ctrl, buf, 16);
    LoadString(core_api::get_my_instance(), IDS_24X18, buf, 256);
    AddItem(ctrl, buf, 24);
    LoadString(core_api::get_my_instance(), IDS_32X24, buf, 256);
    AddItem(ctrl, buf, 32);
    LoadString(core_api::get_my_instance(), IDS_40X30, buf, 256);
    AddItem(ctrl, buf, 40);
    LoadString(core_api::get_my_instance(), IDS_48X36_DEFAULT, buf, 256);
    AddItem(ctrl, buf, 48);
    LoadString(core_api::get_my_instance(), IDS_64X48_SLOW, buf, 256);
    AddItem(ctrl, buf, 64);
    LoadString(core_api::get_my_instance(), IDS_80X60_SLOW, buf, 256);
    AddItem(ctrl, buf, 80);
    LoadString(core_api::get_my_instance(), IDS_96X72_SLOW, buf, 256);
    AddItem(ctrl, buf, 96);
    LoadString(core_api::get_my_instance(), IDS_128X96_SLOW, buf, 256);
    AddItem(ctrl, buf, 128);
    LoadString(core_api::get_my_instance(), IDS_160X120_SLOW, buf, 256);
    AddItem(ctrl, buf, 160);
    LoadString(core_api::get_my_instance(), IDS_192X144_SLOW, buf, 256);
    AddItem(ctrl, buf, 192);
    SelectItemByPos(ctrl, 0); // as a safe default
    SelectItemByValue(ctrl, static_cast<DWORD>(cfg_nGridX));

    // Canvas stretch.
    ctrl = GetDlgItem(IDC_STRETCH2);
    LoadString(core_api::get_my_instance(), IDS_AUTO, buf, 256);
    AddItem(ctrl, buf, 0);
    LoadString(core_api::get_my_instance(), IDS_NONE_BEST_IMAGE_QUALITY, buf, 256);
    AddItem(ctrl, buf, 100);
    LoadString(core_api::get_my_instance(), IDS_1_25_X, buf, 256);
    AddItem(ctrl, buf, 125);
    LoadString(core_api::get_my_instance(), IDS_1_33_X, buf, 256);
    AddItem(ctrl, buf, 133);
    LoadString(core_api::get_my_instance(), IDS_1_5_X, buf, 256);
    AddItem(ctrl, buf, 150);
    LoadString(core_api::get_my_instance(), IDS_1_67_X, buf, 256);
    AddItem(ctrl, buf, 167);
    LoadString(core_api::get_my_instance(), IDS_2_X, buf, 256);
    AddItem(ctrl, buf, 200);
    LoadString(core_api::get_my_instance(), IDS_3_X, buf, 256);
    AddItem(ctrl, buf, 300);
    LoadString(core_api::get_my_instance(), IDS_4_X, buf, 256);
    AddItem(ctrl, buf, 400);
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

#pragma region Helper Methods
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
#pragma endregion

static preferences_page_factory_t<preferences_page_milk2> g_preferences_page_milk2_factory;

milk2_config::milk2_config()
{
    initialize_paths();
    reset();
}

uint32_t milk2_config::g_get_version() const
{
    return m_version;
}

uint32_t milk2_config::get_sentinel() const
{
    return m_sentinel;
}

void milk2_config::reset()
{
    //--- CPluginShell::ReadConfig()
    settings.m_multisample_fullscreen = {1u, 0u};

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
    std::string base_path = core_api::get_my_full_path();
    std::string::size_type t = base_path.rfind('\\');
    if (t != std::string::npos)
        base_path.erase(t + 1);

    swprintf_s(default_szPluginsDirPath, L"%hs", const_cast<char*>(base_path.c_str()));
    swprintf_s(default_szConfigIniFile, L"%hs%ls", const_cast<char*>(base_path.c_str()), INIFILE);
    swprintf_s(default_szMilkdrop2Path, L"%ls%ls", default_szPluginsDirPath, SUBDIR);
    swprintf_s(default_szPresetDir, L"%lspresets\\", default_szMilkdrop2Path);
    wcstombs_s(nullptr, default_szPresetDirA, default_szPresetDir, MAX_PATH);
    wchar_t szConfigDir[MAX_PATH] = {0};
    wcscpy_s(szConfigDir, default_szConfigIniFile);
    wchar_t* p = wcsrchr(szConfigDir, L'\\');
    if (p)
        *(p + 1) = 0;
    swprintf_s(default_szMsgIniFile, L"%ls%ls", szConfigDir, MSG_INIFILE);
    swprintf_s(default_szImgIniFile, L"%ls%ls", szConfigDir, IMG_INIFILE);
}

void milk2_config::update_paths()
{
    if (cfg_szPresetDir.get().empty())
    {
        wcscpy_s(settings.m_szPresetDir, default_szPresetDir);
        cfg_szPresetDir.set(default_szPresetDirA);
        strcpy_s(settings.m_szPresetDirA, default_szPresetDirA);
    }
    else
    {
        size_t convertedChars;
        mbstowcs_s(&convertedChars, settings.m_szPresetDir, cfg_szPresetDir.get().c_str(), _TRUNCATE);
        strcpy_s(settings.m_szPresetDirA, cfg_szPresetDir.get().c_str());
    }
    wcscpy_s(settings.m_szPluginsDirPath, default_szPluginsDirPath);
    wcscpy_s(settings.m_szConfigIniFile, default_szConfigIniFile);
    wcscpy_s(settings.m_szMilkdrop2Path, default_szMilkdrop2Path);
    wcscpy_s(settings.m_szMsgIniFile, default_szMsgIniFile);
    wcscpy_s(settings.m_szImgIniFile, default_szImgIniFile);
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
            case 0:
                //parser >> m_multisample_fullscreen;
                //parser >> m_multisample_windowed;

                //parser >> m_start_fullscreen;
                //parser >> m_max_fps_fs;
                //parser >> m_show_press_f1_msg;
                //parser >> m_allow_page_tearing_fs;
                //parser >> m_minimize_winamp;
                //parser >> m_dualhead_horz;
                //parser >> m_dualhead_vert;
                //parser >> m_save_cpu;
                //parser >> m_skin;
                //parser >> m_fix_slow_text;

                //parser >> m_disp_mode_fs;

                //parser >> settings.m_bEnableRating;
                //parser >> settings.m_bHardCutsDisabled;
                //parser >> settings.g_bDebugOutput;
                //parser >> settings.m_bShowSongInfo;
                //parser >> settings.m_bShowPressF1ForHelp;
                //parser >> settings.m_bShowMenuToolTips;
                //parser >> settings.m_bSongTitleAnims;

                parser >> settings.m_bShowFPS;
                parser >> settings.m_bShowRating;
                parser >> settings.m_bShowPresetInfo;
                //parser >> settings.m_bShowDebugInfo;
                parser >> settings.m_bShowSongTitle;
                parser >> settings.m_bShowSongTime;
                parser >> settings.m_bShowSongLen;
                parser >> settings.m_bShowShaderHelp;

                //parser >> settings.m_bFixPinkBug;
                //parser >> settings.m_n16BitGamma;
                //parser >> settings.m_bAutoGamma;
                //parser >> settings.m_bAlways3D;
                //parser >> settings.m_fStereoSep;
                //parser >> settings.m_bFixSlowText;
                //parser >> settings.m_bAlwaysOnTop;
                //parser >> settings.m_bWarningsDisabled;
                //parser >> settings.m_bWarningsDisabled2;
                //parser >> settings.m_bAnisotropicFiltering;
                //parser >> settings.m_bPresetLockOnAtStartup;
                //parser >> settings.m_bPreventScollLockHandling;

                //parser >> settings.m_nCanvasStretch;
                //parser >> settings.m_nTexSizeX;
                parser >> settings.m_nTexSizeY;
                parser >> settings.m_bTexSizeWasAutoPow2;
                parser >> settings.m_bTexSizeWasAutoExact;
                //parser >> settings.m_nTexBitsPerCh;
                //parser >> settings.m_nGridX;
                parser >> settings.m_nGridY;
                //parser >> settings.m_nMaxPSVersion;
                //parser >> settings.m_nMaxImages;
                //parser >> settings.m_nMaxBytes;

                //parser >> settings.m_fBlendTimeUser;
                //parser >> settings.m_fBlendTimeAuto;
                //parser >> settings.m_fTimeBetweenPresets;
                //parser >> settings.m_fTimeBetweenPresetsRand;
                //parser >> settings.m_fHardCutLoudnessThresh;
                //parser >> settings.m_fHardCutHalflife;
                //parser >> settings.m_fSongTitleAnimDuration;
                //parser >> settings.m_fTimeBetweenRandomSongTitles;
                //parser >> settings.m_fTimeBetweenRandomCustomMsgs;

                parser >> settings.m_bPresetLockedByCode;
                //parser >> settings.m_bPresetLockedByUser;
                //parser >> settings.m_bMilkdropScrollLockState;
                //parser >> settings.m_nFpsLimit;

                parser >> settings.m_bEnableDownmix;
                parser >> settings.m_bEnableHDR;
                parser >> settings.m_nBackBufferFormat;
                parser >> settings.m_nDepthBufferFormat;
                parser >> settings.m_nBackBufferCount;
                parser >> settings.m_nMinFeatureLevel;

                parser >> settings.m_szPluginsDirPath;
                parser >> settings.m_szConfigIniFile;
                parser >> settings.m_szMilkdrop2Path;
                parser >> settings.m_szPresetDir;
                parser >> settings.m_szPresetDirA;
                parser >> settings.m_szMsgIniFile;
                parser >> settings.m_szImgIniFile;
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
    builder << g_get_version();

    //builder << m_multisample_fullscreen;
    //builder << m_multisample_windowed;

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
    cfg_bDebugOutput = settings.g_bDebugOutput;
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

    builder << settings.m_szPluginsDirPath;
    builder << settings.m_szConfigIniFile;
    builder << settings.m_szMilkdrop2Path;
    builder << settings.m_szPresetDir;
    builder << settings.m_szPresetDirA;
    builder << settings.m_szMsgIniFile;
    builder << settings.m_szImgIniFile;
}