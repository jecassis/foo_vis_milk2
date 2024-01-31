//
// preferences.cpp: Configuration settings accessible through a preferences page and advanced preferences.
//

#include "pch.h"
#include "resource.h"
#include "config.h"
#include <vis_milk2/defines.h>
#include <vis_milk2/md_defines.h>

extern HWND g_hWindow;

// Dark Mode:
// (1) Add fb2k::CDarkModeHooks member.
// (2) Initialize it in WM_INITDIALOG handler.
// (3) Tell foobar2000 that this preferences page supports dark mode, by returning correct get_state() flags.

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
static constexpr GUID guid_cfg_max_fps_w = {
    0x2ffda0b0, 0x632a, 0x49c0, {0x90, 0xc7, 0x52, 0xd1, 0x6c, 0x93, 0xee, 0x5a}
}; // {2FFDA0B0-632A-49C0-90C7-52D16C93EE5A}
static constexpr GUID guid_cfg_max_fps_fs = {
    0x2c669890, 0x66fb, 0x4419, {0xa9, 0xb7, 0x93, 0xc8, 0x24, 0xf2, 0xfd, 0x19}
}; // {2C669890-66FB-4419-A9B7-93C824F2FD19}
static constexpr GUID guid_cfg_bShowPressF1ForHelp = {
    0x4c2e81ab, 0xfbde, 0x4c22, {0x81, 0x95, 0xe9, 0x1f, 0x74, 0x9, 0x59, 0x9c}
}; // {4C2E81AB-FBDE-4C22-8195-E91F7409599C}
static constexpr GUID guid_cfg_allow_page_tearing_w = {
    0x3b384b04, 0x7a0c, 0x4634, {0xab, 0xef, 0xa2, 0xf6, 0x9d, 0x33, 0xbb, 0x15}
}; // {3B384B04-7A0C-4634-ABEF-A2F69D33BB15}
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
static constexpr GUID guid_advconfig_branch = {
    0xd7ab1cd7, 0x7956, 0x4497, {0x9b, 0x1d, 0x74, 0x78, 0x7e, 0xde, 0x1d, 0xbc}
}; // {D7AB1CD7-7956-4497-9B1D-74787EDE1DBC}
static constexpr GUID guid_cfg_bDebugOutput = {
    0x808a73c, 0x8857, 0x4472, {0xad, 0x49, 0xdb, 0x1a, 0x48, 0x4e, 0x3f, 0x5}
}; // {0808A73C-8857-4472-AD49-DB1A484E3F05}
static constexpr GUID guid_cfg_nMaxPSVersion = {
    0xcb39278b, 0x4c19, 0x4931, {0x92, 0x63, 0x63, 0x1f, 0x69, 0x24, 0xf1, 0xc4}
}; // {CB39278B-4C19-4931-9263-631F6924F1C4}
static constexpr GUID guid_cfg_nMaxImages = {
    0x779da878, 0xf890, 0x4dd5, {0x9b, 0xb1, 0xb8, 0xfc, 0x64, 0x3d, 0xa, 0xfb}
}; // {779DA878-F890-4DD5-9BB1-B8FC643D0AFB}
static constexpr GUID guid_cfg_nMaxBytes = {
    0x904342ae, 0x2844, 0x4970, {0x9c, 0x2, 0x8e, 0xc4, 0xcc, 0x75, 0x26, 0x9f}
}; // {904342AE-2844-4970-9C02-8EC4CC75269F}
static constexpr GUID guid_cfg_szPresetDir = {
    0xfa9e467b, 0xfe6d, 0x4d79, {0x83, 0x98, 0xcd, 0x3d, 0x8b, 0xf4, 0x7a, 0x63}
}; // {FA9E467B-FE6D-4D79-8398-CD3D8BF47A63}

// Defaults
// `milk.ini` defaults
//constexpr int default_nFpsLimit = -1;
static constexpr int default_nSongTitlesSpawned = 0;
static constexpr bool default_nCustMsgsSpawned = 0;
static constexpr bool default_nFramesSinceResize = 0;
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
static constexpr bool default_bPreventScollLockHandling = false;
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
static constexpr float default_fStartTime = 0.0f;
static constexpr float default_fPresetStartTime = 0.0f;
static constexpr float default_fNextPresetTime = -1.0f; // negative value means no time set (...it will be auto-set on first call to UpdateTime)
static constexpr bool default_nLoadingPreset = 0;
static constexpr bool default_nPresetsLoadedTotal = 0;
static constexpr float default_fSnapPoint = 0.5f;
static constexpr bool default_bShowShaderHelp = false;
static constexpr float default_fMotionVectorsTempDx = 0.0f;
static constexpr float default_fMotionVectorsTempDy = 0.0f;
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
static constexpr bool default_bAllowTearing = false;
static constexpr bool default_bEnableHDR = false;
static constexpr int default_nBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
static constexpr int default_nDepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
static constexpr int default_nBackBufferCount = 2;
static constexpr int default_nMinFeatureLevel = D3D_FEATURE_LEVEL_9_1;
static constexpr UINT default_max_fps_w = 30;
static constexpr UINT default_max_fps_fs = 30;
static constexpr bool default_allow_page_tearing_w = true;
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
    order_nMaxPSVersion,
    order_nMaxImages,
    order_nMaxBytes,
    order_szPresetDir,
};

// clang-format off
static cfg_bool cfg_bPresetLockOnAtStartup(guid_cfg_bPresetLockOnAtStartup, default_bPresetLockOnAtStartup);
static cfg_bool cfg_bPreventScollLockHandling(guid_cfg_bPreventScollLockHandling, default_bPreventScollLockHandling);
static cfg_bool cfg_bWarningsDisabled(guid_cfg_bWarningsDisabled, default_bWarningsDisabled2);
static cfg_bool cfg_bEnableRating(guid_cfg_bEnableRating, !default_bEnableRating);
static cfg_bool cfg_bShowPressF1ForHelp(guid_cfg_bShowPressF1ForHelp, default_bShowPressF1ForHelp);
static cfg_bool cfg_allow_page_tearing_w(guid_cfg_allow_page_tearing_w, default_allow_page_tearing_w);
static cfg_bool cfg_allow_page_tearing_fs(guid_cfg_allow_page_tearing_fs, default_allow_page_tearing_fs);
static cfg_int cfg_max_fps_w(guid_cfg_max_fps_w, static_cast<int64_t>(default_max_fps_fs));
static cfg_int cfg_max_fps_fs(guid_cfg_max_fps_fs, static_cast<int64_t>(default_max_fps_fs));
static cfg_float cfg_fTimeBetweenPresets(guid_cfg_fTimeBetweenPresets, static_cast<double>(default_fTimeBetweenPresets));
static cfg_float cfg_fTimeBetweenPresetsRand(guid_cfg_fTimeBetweenPresetsRand, static_cast<double>(default_fTimeBetweenPresetsRand));
static cfg_float cfg_fBlendTimeAuto(guid_cfg_fBlendTimeAuto, static_cast<double>(default_fBlendTimeAuto));
static cfg_float cfg_fBlendTimeUser(guid_cfg_fBlendTimeUser, static_cast<double>(default_fBlendTimeUser));
static advconfig_branch_factory g_advconfigBranch("MilkDrop", guid_advconfig_branch, advconfig_branch::guid_branch_vis, 0);
static advconfig_checkbox_factory cfg_bDebugOutput("Debug Output", "milk2.bDebugOutput", guid_cfg_bDebugOutput, guid_advconfig_branch, order_bDebugOutput, default_bDebugOutput);
static advconfig_signed_integer_factory cfg_nMaxPSVersion("Max. Pixel Shader Version", "milk2.nMaxPSVersion", guid_cfg_nMaxPSVersion, guid_advconfig_branch, order_nMaxPSVersion, default_nMaxPSVersion, -1, 3);
static advconfig_integer_factory cfg_nMaxImages("Max. Images", "milk2.nMaxImages", guid_cfg_nMaxImages, guid_advconfig_branch, order_nMaxImages, default_nMaxImages, 1, 64);
static advconfig_integer_factory cfg_nMaxBytes("Max. Bytes", "milk2.nMaxBytes", guid_cfg_nMaxBytes, guid_advconfig_branch, order_nMaxBytes, default_nMaxBytes, 1, 32000000);
static advconfig_string_factory cfg_szPresetDir("Preset Directory", "milk2.szPresetDir", guid_cfg_szPresetDir, guid_advconfig_branch, order_szPresetDir, "");
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
        COMMAND_HANDLER_EX(IDC_CB_SCROLLON3, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_SCROLLON4, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_NOWARN3, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_NORATING2, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_PRESS_F1_MSG, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_W_MAXFPS2, CBN_SELCHANGE, OnComboChange)
        COMMAND_HANDLER_EX(IDC_CB_WPT, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_BETWEEN_TIME, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_BETWEEN_TIME_RANDOM, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_BLEND_AUTO, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_BLEND_USER, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_CB_HARDCUTS, BN_CLICKED, OnButtonClick)
    END_MSG_MAP()
    // clang-format on

  private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnEditChange(UINT, int, CWindow);
    void OnButtonClick(UINT, int, CWindow);
    void OnComboChange(UINT, int, CWindow);
    bool HasChanged() const;
    void OnChanged();
    void UpdateMaxFps(int screenmode);
    void SaveMaxFps(int screenmode);

    const preferences_page_callback::ptr m_callback;

    fb2k::CDarkModeHooks m_dark;
};

BOOL milk2_preferences_page::OnInitDialog(CWindow, LPARAM)
{
    // Enable dark mode
    // One call does it all, applies all relevant hacks automatically.
    m_dark.AddDialogWithControls(*this);

    WCHAR buf[256]{};
    HWND ctrl = nullptr;

    // Set checkboxes.
    CheckDlgButton(IDC_CB_SCROLLON3, static_cast<UINT>(cfg_bPresetLockOnAtStartup));
    CheckDlgButton(IDC_CB_SCROLLON4, static_cast<UINT>(cfg_bPreventScollLockHandling));
    CheckDlgButton(IDC_CB_NOWARN3, static_cast<UINT>(cfg_bWarningsDisabled));
    CheckDlgButton(IDC_CB_NORATING2, static_cast<UINT>(!cfg_bEnableRating));
    CheckDlgButton(IDC_CB_PRESS_F1_MSG, static_cast<UINT>(cfg_bShowPressF1ForHelp));
    CheckDlgButton(IDC_CB_WPT, static_cast<UINT>(cfg_allow_page_tearing_w));
    //CheckDlgButton(IDC_CB_FSPT, static_cast<UINT>(cfg_allow_page_tearing_fs));
    UpdateMaxFps(WINDOWED);
    //void SaveMaxFps(FULLSCREEN);
    swprintf_s(buf, L"%.1f", static_cast<float>(cfg_fTimeBetweenPresets));
    SetDlgItemText(IDC_BETWEEN_TIME, buf);
    swprintf_s(buf, L"%.1f", static_cast<float>(cfg_fTimeBetweenPresetsRand));
    SetDlgItemText(IDC_BETWEEN_TIME_RANDOM, buf);
    swprintf_s(buf, L"%.1f", static_cast<float>(cfg_fBlendTimeAuto));
    SetDlgItemText(IDC_BLEND_AUTO, buf);
    swprintf_s(buf, L"%.1f", static_cast<float>(cfg_fBlendTimeUser));
    SetDlgItemText(IDC_BLEND_USER, buf);

    return FALSE;
}

void milk2_preferences_page::OnEditChange(UINT, int, CWindow)
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
    WCHAR buf[256];
    CheckDlgButton(IDC_CB_SCROLLON3, static_cast<UINT>(cfg_bPresetLockOnAtStartup));
    CheckDlgButton(IDC_CB_SCROLLON4, static_cast<UINT>(cfg_bPreventScollLockHandling));
    CheckDlgButton(IDC_CB_NOWARN3, static_cast<UINT>(cfg_bWarningsDisabled));
    CheckDlgButton(IDC_CB_NORATING2, static_cast<UINT>(!cfg_bEnableRating));
    CheckDlgButton(IDC_CB_PRESS_F1_MSG, static_cast<UINT>(cfg_bShowPressF1ForHelp));
    CheckDlgButton(IDC_CB_WPT, static_cast<UINT>(cfg_allow_page_tearing_w));
    //CheckDlgButton(IDC_CB_FSPT, static_cast<UINT>(cfg_allow_page_tearing_fs));
    UpdateMaxFps(WINDOWED);
    //UpdateMaxFps(FULLSCREEN);
    swprintf_s(buf, L"%.1f", static_cast<float>(cfg_fTimeBetweenPresets));
    SetDlgItemText(IDC_BETWEEN_TIME, buf);
    swprintf_s(buf, L"%.1f", static_cast<float>(cfg_fTimeBetweenPresetsRand));
    SetDlgItemText(IDC_BETWEEN_TIME_RANDOM, buf);
    swprintf_s(buf, L"%.1f", static_cast<float>(cfg_fBlendTimeAuto));
    SetDlgItemText(IDC_BLEND_AUTO, buf);
    swprintf_s(buf, L"%.1f", static_cast<float>(cfg_fBlendTimeUser));
    SetDlgItemText(IDC_BLEND_USER, buf);

    OnChanged();
}

void milk2_preferences_page::apply()
{
    WCHAR buf[256], *stop;
    cfg_bPresetLockOnAtStartup = static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON3));
    cfg_bPreventScollLockHandling = static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON4));
    cfg_bWarningsDisabled = static_cast<bool>(IsDlgButtonChecked(IDC_CB_NOWARN3));
    cfg_bEnableRating = !static_cast<bool>(IsDlgButtonChecked(IDC_CB_NORATING2));
    cfg_bShowPressF1ForHelp = static_cast<bool>(IsDlgButtonChecked(IDC_CB_PRESS_F1_MSG));
    cfg_allow_page_tearing_w = static_cast<bool>(IsDlgButtonChecked(IDC_CB_WPT));
    SaveMaxFps(WINDOWED);
    //SaveMaxFps(FULLSCREEN);
    GetDlgItemText(IDC_BETWEEN_TIME, buf, 256);
    cfg_fTimeBetweenPresets = wcstof(buf, &stop);
    if (cfg_fTimeBetweenPresets < 0.1f)
        cfg_fTimeBetweenPresets = 0.1f;
    GetDlgItemText(IDC_BETWEEN_TIME, buf, 256);
    cfg_fTimeBetweenPresetsRand = wcstof(buf, &stop);
    if (cfg_fTimeBetweenPresetsRand < 0.0f)
        cfg_fTimeBetweenPresetsRand = 0.0f;
    GetDlgItemText(IDC_BLEND_AUTO, buf, 256);
    cfg_fBlendTimeAuto = wcstof(buf, &stop);
    GetDlgItemText(IDC_BLEND_AUTO, buf, 256);
    cfg_fBlendTimeUser = wcstof(buf, &stop);

    OnChanged(); // The dialog content has not changed but the flags have; the currently shown values now match the settings so the apply button can be disabled.
    ::SendMessage(g_hWindow, WM_CONFIG_CHANGE, (WPARAM)0, (LPARAM)0);
}

// Returns whether the dialog content is different from the current configuration;
// i.e., whether the "Apply" button should be enabled or not.
bool milk2_preferences_page::HasChanged() const
{
    bool checkbox_changes = static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON3)) != cfg_bPresetLockOnAtStartup ||
                            static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON4)) != cfg_bPreventScollLockHandling ||
                            static_cast<bool>(IsDlgButtonChecked(IDC_CB_NOWARN3)) != cfg_bWarningsDisabled ||
                            static_cast<bool>(IsDlgButtonChecked(IDC_CB_NORATING2)) == cfg_bEnableRating ||
                            static_cast<bool>(IsDlgButtonChecked(IDC_CB_PRESS_F1_MSG)) != cfg_bShowPressF1ForHelp ||
                            static_cast<bool>(IsDlgButtonChecked(IDC_CB_WPT)) != cfg_allow_page_tearing_w;

    bool combobox_changes = false;
    LRESULT n = SendMessage(GetDlgItem(IDC_W_MAXFPS2), CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (n != CB_ERR)
    {
        if (n > 0)
            n = MAX_MAX_FPS + 1 - n;

        combobox_changes = combobox_changes || static_cast<UINT>(n) != cfg_max_fps_w;
    }

    WCHAR buf[256], *stop;
    bool editcontrol_changes = false;
    GetDlgItemText(IDC_BETWEEN_TIME, buf, 256);
    editcontrol_changes = editcontrol_changes || wcstof(buf, &stop) != cfg_fTimeBetweenPresets;
    GetDlgItemText(IDC_BETWEEN_TIME, buf, 256);
    editcontrol_changes = editcontrol_changes || wcstof(buf, &stop) != cfg_fTimeBetweenPresetsRand;
    GetDlgItemText(IDC_BLEND_AUTO, buf, 256);
    editcontrol_changes = editcontrol_changes || wcstof(buf, &stop) != cfg_fBlendTimeAuto;
    GetDlgItemText(IDC_BLEND_AUTO, buf, 256);
    editcontrol_changes = editcontrol_changes || wcstof(buf, &stop) != cfg_fBlendTimeUser;

    return checkbox_changes || combobox_changes || editcontrol_changes;
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

// Initializes FPS combo boxes.
void milk2_preferences_page::UpdateMaxFps(int screenmode)
{
    HWND ctrl = nullptr;
    switch (screenmode)
    {
        case WINDOWED:
            ctrl = GetDlgItem(IDC_W_MAXFPS2);
            break;
        case FULLSCREEN:
            ctrl = GetDlgItem(IDC_FS_MAXFPS);
            break;
    }

    if (!ctrl)
        return;

    SendMessage(ctrl, CB_RESETCONTENT, 0, 0);
    for (int j = 0; j <= MAX_MAX_FPS; ++j)
    {
        WCHAR buf[256];
        if (j == 0)
            LoadString(core_api::get_my_instance(), IDS_UNLIMITED, buf, 256);
        else
        {
            LoadString(core_api::get_my_instance(), IDS_X_FRAME_SEC, buf, 256);
            swprintf_s(buf, buf, MAX_MAX_FPS + 1 - j);
        }

        SendMessage(ctrl, CB_ADDSTRING, j, (LPARAM)buf);
    }

    // Set previous selection.
    UINT max_fps = 0;
    switch (screenmode)
    {
        case WINDOWED:
            max_fps = static_cast<UINT>(cfg_max_fps_w);
            break;
        case FULLSCREEN:
            max_fps = static_cast<UINT>(cfg_max_fps_w);
            break;
    }
    if (max_fps == 0)
        SendMessage(ctrl, CB_SETCURSEL, 0, 0);
    else
        SendMessage(ctrl, CB_SETCURSEL, MAX_MAX_FPS - max_fps + 1, 0);
}

void milk2_preferences_page::SaveMaxFps(int screenmode)
{
    HWND ctrl = nullptr;
    switch (screenmode)
    {
        case WINDOWED:
            ctrl = GetDlgItem(IDC_W_MAXFPS2);
            break;
        case FULLSCREEN:
            ctrl = GetDlgItem(IDC_FS_MAXFPS);
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
                case WINDOWED:
                    cfg_max_fps_w = static_cast<UINT>(n);
                    break;
                case FULLSCREEN:
                    cfg_max_fps_fs = static_cast<UINT>(n);
                    break;
            }
        }
    }
}

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

void milk2_config::reset()
{
    // milk.ini
    //m_nFpsLimit = default_nFpsLimit;
    m_nSongTitlesSpawned = default_nSongTitlesSpawned;
    m_nCustMsgsSpawned = default_nCustMsgsSpawned;
    m_nFramesSinceResize = default_nFramesSinceResize;

    m_bEnableRating = cfg_bEnableRating.get();
    m_bHardCutsDisabled = default_bHardCutsDisabled;
    g_bDebugOutput = cfg_bDebugOutput.get();
    //m_bShowSongInfo;
    m_bShowPressF1ForHelp = cfg_bShowPressF1ForHelp.get();
    //m_bShowMenuToolTips;
    m_bSongTitleAnims = default_bSongTitleAnims;

    m_bShowFPS = default_bShowFPS;
    m_bShowRating = default_bShowRating;
    m_bShowPresetInfo = default_bShowPresetInfo;
    //m_bShowDebugInfo = false;
    m_bShowSongTitle = default_bShowSongTitle;
    m_bShowSongTime = default_bShowSongTime;
    m_bShowSongLen = default_bShowSongLen;

    //m_bFixPinkBug = default_bFixPinkBug;
    m_n16BitGamma = default_n16BitGamma;
    m_bAutoGamma = default_bAutoGamma;
    //m_bAlways3D = default_bAlways3D;
    //m_fStereoSep = default_fStereoSep;
    //m_bFixSlowText = default_bFixSlowText;
    //m_bAlwaysOnTop = default_bAlwaysOnTop;
    //m_bWarningsDisabled = default_bWarningsDisabled;
    m_bWarningsDisabled2 = default_bWarningsDisabled2;
    //m_bAnisotropicFiltering = default_bAnisotropicFiltering;
    m_bPresetLockOnAtStartup = cfg_bPresetLockOnAtStartup.get();
    m_bPreventScollLockHandling = cfg_bPreventScollLockHandling.get();

    m_nCanvasStretch = default_nCanvasStretch;
    m_nTexSizeX = default_nTexSizeX;
    m_nTexSizeY = m_nTexSizeX;
    m_bTexSizeWasAutoPow2 = (m_nTexSizeX == -2);
    m_bTexSizeWasAutoExact = (m_nTexSizeX == -1);
    m_nTexBitsPerCh = default_nTexBitsPerCh;
    m_nGridX = default_nGridX;
    m_nGridY = m_nGridX * 3 / 4;
    m_nMaxPSVersion = static_cast<uint32_t>(cfg_nMaxPSVersion.get());
    m_nMaxImages = static_cast<uint32_t>(cfg_nMaxImages.get());
    m_nMaxBytes = static_cast<uint32_t>(cfg_nMaxBytes.get());

    m_fBlendTimeUser = default_fBlendTimeUser;
    m_fBlendTimeAuto = default_fBlendTimeAuto;
    m_fTimeBetweenPresets = static_cast<float>(cfg_fTimeBetweenPresets.get());
    m_fTimeBetweenPresetsRand = static_cast<float>(cfg_fTimeBetweenPresetsRand.get());
    m_fHardCutLoudnessThresh = default_fHardCutLoudnessThresh;
    m_fHardCutHalflife = default_fHardCutHalflife;
    m_fSongTitleAnimDuration = default_fSongTitleAnimDuration;
    m_fTimeBetweenRandomSongTitles = default_fTimeBetweenRandomSongTitles;
    m_fTimeBetweenRandomCustomMsgs = default_fTimeBetweenRandomCustomMsgs;

    m_bTexSizeWasAutoPow2 = (m_nTexSizeX == -2);
    m_bTexSizeWasAutoExact = (m_nTexSizeX == -1);

    m_bPresetLockedByCode = default_bPresetLockedByCode;
    m_fStartTime = default_fStartTime;
    m_fPresetStartTime = default_fPresetStartTime;
    m_fNextPresetTime = default_fNextPresetTime;
    m_nLoadingPreset = default_nLoadingPreset;
    m_nPresetsLoadedTotal = default_nPresetsLoadedTotal;
    m_fSnapPoint = default_fSnapPoint;
    m_bShowShaderHelp = default_bShowShaderHelp;
    m_fMotionVectorsTempDx = default_fMotionVectorsTempDx;
    m_fMotionVectorsTempDy = default_fMotionVectorsTempDy;

    // Bounds checking
    if (m_nGridX > MAX_GRID_X)
        m_nGridX = MAX_GRID_X;
    if (m_nGridY > MAX_GRID_Y)
        m_nGridY = MAX_GRID_Y;

    // Derived settings
    m_bPresetLockedByUser = m_bPresetLockOnAtStartup;
    //m_bMilkdropScrollLockState = m_bPresetLockOnAtStartup;

    m_bEnableDownmix = default_bEnableDownmix;
    m_bAllowTearing = default_bAllowTearing;
    m_bEnableHDR = default_bEnableHDR;
    m_nBackBufferFormat = default_nBackBufferFormat;
    m_nDepthBufferFormat = default_nDepthBufferFormat;
    m_nBackBufferCount = default_nBackBufferCount;
    m_nMinFeatureLevel = default_nMinFeatureLevel;

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
        wcscpy_s(m_szPresetDir, default_szPresetDir);
        cfg_szPresetDir.set(default_szPresetDirA);
        strcpy_s(m_szPresetDirA, default_szPresetDirA);
    }
    else
    {
        size_t convertedChars;
        mbstowcs_s(&convertedChars, m_szPresetDir, cfg_szPresetDir.get().c_str(), _TRUNCATE);
        strcpy_s(m_szPresetDirA, cfg_szPresetDir.get().c_str());
    }
    wcscpy_s(m_szPluginsDirPath, default_szPluginsDirPath);
    wcscpy_s(m_szConfigIniFile, default_szConfigIniFile);
    wcscpy_s(m_szMilkdrop2Path, default_szMilkdrop2Path);
    wcscpy_s(m_szMsgIniFile, default_szMsgIniFile);
    wcscpy_s(m_szImgIniFile, default_szImgIniFile);
}

// Reads the configuration.
void milk2_config::parse(ui_element_config_parser& parser)
{
    // to retrieve state: pfc::string8 val; mystring.get(val); myint.get();
    try
    {
        uint32_t sentinel, version;
        parser >> sentinel;
        parser >> version;
        switch (version)
        {
            //case 6:
            //    parser >> m_line_stroke_width;
            //    m_line_stroke_width = pfc::clip_t<t_uint32>(m_line_stroke_width, 5, 30);
            //    // fall through
            //case 5:
            //    parser >> m_low_quality_enabled;
            //    // fall through
            //case 4:
            //    parser >> m_resample_enabled;
            //    // fall through
            //case 3:
            //    parser >> m_refresh_rate_limit_hz;
            //    m_refresh_rate_limit_hz = pfc::clip_t<t_uint32>(m_refresh_rate_limit_hz, 20, 200);
            //    // fall through
            //case 2:
            //    parser >> m_trigger_enabled;
            //    // fall through
            //case 1:
            //    parser >> m_hw_rendering_enabled;
            //    parser >> m_window_duration_millis;
            //    m_window_duration_millis = pfc::clip_t<t_uint32>(m_window_duration_millis, 50, 800);
            //    parser >> m_zoom_percent;
            //    m_zoom_percent = pfc::clip_t<t_uint32>(m_zoom_percent, 50, 800);
            //    // fall through
            case 0:
                // milk.ini
                //parser >> m_nFpsLimit;
                parser >> m_nSongTitlesSpawned;
                parser >> m_nCustMsgsSpawned;
                parser >> m_nFramesSinceResize;

                parser >> m_bEnableRating;
                parser >> m_bHardCutsDisabled;
                parser >> g_bDebugOutput;
                //parser >> m_bShowSongInfo;
                parser >> m_bShowPressF1ForHelp;
                //parser >> m_bShowMenuToolTips;
                parser >> m_bSongTitleAnims;

                parser >> m_bShowFPS;
                parser >> m_bShowRating;
                parser >> m_bShowPresetInfo;
                //parser >> m_bShowDebugInfo;
                parser >> m_bShowSongTitle;
                parser >> m_bShowSongTime;
                parser >> m_bShowSongLen;

                //parser >> m_bFixPinkBug;
                parser >> m_n16BitGamma;
                parser >> m_bAutoGamma;
                //parser >> m_bAlways3D;
                //parser >> m_fStereoSep;
                //parser >> m_bFixSlowText;
                //parser >> m_bAlwaysOnTop;
                //parser >> m_bWarningsDisabled;
                parser >> m_bWarningsDisabled2;
                //parser >> m_bAnisotropicFiltering;
                parser >> m_bPresetLockOnAtStartup;
                parser >> m_bPreventScollLockHandling;

                parser >> m_nCanvasStretch;
                parser >> m_nTexSizeX;
                parser >> m_nTexSizeY;
                parser >> m_bTexSizeWasAutoPow2;
                parser >> m_bTexSizeWasAutoExact;
                parser >> m_nTexBitsPerCh;
                parser >> m_nGridX;
                parser >> m_nGridY;
                parser >> m_nMaxPSVersion;
                parser >> m_nMaxImages;
                parser >> m_nMaxBytes;

                parser >> m_fBlendTimeUser;
                parser >> m_fBlendTimeAuto;
                parser >> m_fTimeBetweenPresets;
                parser >> m_fTimeBetweenPresetsRand;
                parser >> m_fHardCutLoudnessThresh;
                parser >> m_fHardCutHalflife;
                parser >> m_fSongTitleAnimDuration;
                parser >> m_fTimeBetweenRandomSongTitles;
                parser >> m_fTimeBetweenRandomCustomMsgs;

                parser >> m_bPresetLockedByCode;
                parser >> m_fStartTime;
                parser >> m_fPresetStartTime;
                parser >> m_fNextPresetTime;
                parser >> m_nLoadingPreset;
                parser >> m_nPresetsLoadedTotal;
                parser >> m_fSnapPoint;
                parser >> m_bShowShaderHelp;
                parser >> m_fMotionVectorsTempDx;
                parser >> m_fMotionVectorsTempDy;

                parser >> m_bPresetLockedByUser;
                //parser >> m_bMilkdropScrollLockState;

                parser >> m_bEnableDownmix;
                parser >> m_bAllowTearing;
                parser >> m_bEnableHDR;
                parser >> m_nBackBufferFormat;
                parser >> m_nDepthBufferFormat;
                parser >> m_nBackBufferCount;
                parser >> m_nMinFeatureLevel;

                parser >> m_szPluginsDirPath;
                parser >> m_szConfigIniFile;
                parser >> m_szMilkdrop2Path;
                parser >> m_szPresetDir;
                parser >> m_szPresetDirA;
                parser >> m_szMsgIniFile;
                parser >> m_szImgIniFile;
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
    //builder << m_nFpsLimit;
    builder << m_nSongTitlesSpawned;
    builder << m_nCustMsgsSpawned;
    builder << m_nFramesSinceResize;

    builder << cfg_bEnableRating.get();
    builder << m_bHardCutsDisabled;
    builder << cfg_bDebugOutput.get();
    //builder << m_bShowSongInfo;
    builder << cfg_bShowPressF1ForHelp.get();
    //builder << m_bShowMenuToolTips;
    builder << m_bSongTitleAnims;

    builder << m_bShowFPS;
    builder << m_bShowRating;
    builder << m_bShowPresetInfo;
    //builder << m_bShowDebugInfo;
    builder << m_bShowSongTitle;
    builder << m_bShowSongTime;
    builder << m_bShowSongLen;

    //builder << m_bFixPinkBug;
    builder << m_n16BitGamma;
    builder << m_bAutoGamma;
    //builder << m_bAlways3D;
    //builder << m_fStereoSep;
    //builder << m_bFixSlowText;
    //builder << m_bAlwaysOnTop;
    //builder << m_bWarningsDisabled;
    builder << cfg_bWarningsDisabled.get();
    //builder << m_bAnisotropicFiltering;
    builder << cfg_bPresetLockOnAtStartup.get();
    builder << cfg_bPreventScollLockHandling.get();

    builder << m_nCanvasStretch;
    builder << m_nTexSizeX;
    builder << m_nTexSizeY;
    builder << m_bTexSizeWasAutoPow2;
    builder << m_bTexSizeWasAutoExact;
    builder << m_nTexBitsPerCh;
    builder << m_nGridX;
    builder << m_nGridY;
    builder << static_cast<uint32_t>(cfg_nMaxPSVersion.get());
    builder << static_cast<uint32_t>(cfg_nMaxImages.get());
    builder << static_cast<uint32_t>(cfg_nMaxBytes.get());

    builder << static_cast<float>(cfg_fBlendTimeUser.get());
    builder << static_cast<float>(cfg_fBlendTimeAuto.get());
    builder << static_cast<float>(cfg_fTimeBetweenPresets.get());
    builder << static_cast<float>(cfg_fTimeBetweenPresetsRand.get());
    builder << m_fHardCutLoudnessThresh;
    builder << m_fHardCutHalflife;
    builder << m_fSongTitleAnimDuration;
    builder << m_fTimeBetweenRandomSongTitles;
    builder << m_fTimeBetweenRandomCustomMsgs;

    builder << m_bPresetLockedByCode;
    builder << m_fStartTime;
    builder << m_fPresetStartTime;
    builder << m_fNextPresetTime;
    builder << m_nLoadingPreset;
    builder << m_nPresetsLoadedTotal;
    builder << m_fSnapPoint;
    builder << m_bShowShaderHelp;
    builder << m_fMotionVectorsTempDx;
    builder << m_fMotionVectorsTempDy;

    builder << m_bPresetLockedByUser;
    //builder << m_bMilkdropScrollLockState;

    builder << m_bEnableDownmix;
    builder << m_bAllowTearing;
    builder << m_bEnableHDR;
    builder << m_nBackBufferFormat;
    builder << m_nDepthBufferFormat;
    builder << m_nBackBufferCount;
    builder << m_nMinFeatureLevel;

    builder << m_szPluginsDirPath;
    builder << m_szConfigIniFile;
    builder << m_szMilkdrop2Path;
    builder << m_szPresetDir;
    builder << m_szPresetDirA;
    builder << m_szMsgIniFile;
    builder << m_szImgIniFile;
}