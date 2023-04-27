//
// preferences.cpp: Configuration settings accessible through a preferences page and advanced preferences.
//

#include "pch.h"
#include "resource.h"
#include "config.h"

#include <helpers/atl-misc.h>
#include <helpers/advconfig_impl.h>
#include <sdk/cfg_var.h>
#include <helpers/DarkMode.h>
#include <vis_milk2/defines.h>

// Dark Mode:
// (1) Add fb2k::CDarkModeHooks member.
// (2) Initialize it in WM_INITDIALOG handler.
// (3) Tell foobar2000 that this preferences page supports dark mode, by returning correct get_state() flags.

static constexpr GUID guid_cfg_bogoSetting1 = {
    0x8a6c8c08, 0xc298, 0x4485, {0xb9, 0xb5, 0xa3, 0x1d, 0xd4, 0xed, 0xfa, 0x4b}
}; // {8A6C8C08-C298-4485-B9B5-A31DD4EDFA4B}

static constexpr GUID guid_cfg_bogoSetting2 = {
    0x96ba1b84, 0x1ff3, 0x47d3, {0x81, 0x9f, 0x35, 0xa7, 0xdb, 0xff, 0xde, 0x37}
}; // {96BA1B84-1FF3-47D3-819F-35A7DBFFDE37}

static constexpr GUID guid_advconfig_branch = {
    0xd7ab1cd7, 0x7956, 0x4497, {0x9b, 0x1d, 0x74, 0x78, 0x7e, 0xde, 0x1d, 0xbc}
}; // {D7AB1CD7-7956-4497-9B1D-74787EDE1DBC}

static constexpr GUID guid_cfg_bogoSetting3 = {
    0xca68c998, 0x3e53, 0x4acc, {0x98, 0xa2, 0x41, 0x46, 0x7a, 0xbc, 0x67, 0xef}
}; // {CA68C998-3E53-4ACC-98A2-41467ABC67EF}

static constexpr GUID guid_cfg_bogoSetting4 = {
    0x99e206f8, 0xd7be, 0x47e6, {0xb5, 0xfe, 0xf4, 0x41, 0x5f, 0x6c, 0x16, 0xed}
};

static constexpr GUID guid_cfg_bogoSetting5 = {
    0x7369ad25, 0x9e81, 0x4e2f, {0x8b, 0xe7, 0x95, 0xcb, 0x81, 0x99, 0x9b, 0x1b}
};

static constexpr GUID guid_cfg_bogoRadio = {
    0x4c18b9ab, 0xf823, 0x4d05, {0xb6, 0x18, 0x14, 0xb9, 0x4c, 0xad, 0xa2, 0xaa}
};

static constexpr GUID guid_cfg_bogoRadio1 = {
    0xdd0a3a95, 0x1546, 0x4f25, {0xa6, 0x8c, 0x23, 0xcf, 0x3d, 0xa5, 0x8f, 0x59}
};

static constexpr GUID guid_cfg_bogoRadio2 = {
    0xc35e1689, 0xb96f, 0x4bf4, {0x95, 0xfb, 0xb7, 0x8e, 0x83, 0xc5, 0x2c, 0x7d}
};

static constexpr GUID guid_cfg_bogoRadio3 = {
    0x790fe179, 0x9908, 0x47e2, {0x9f, 0xde, 0xce, 0xe1, 0x3f, 0x9a, 0xfc, 0x5b}
};

//static constexpr GUID guid_cfg_preset_lock = {
//    0xe5be745e, 0xab65, 0x4b69, {0xa1, 0xf3, 0x1e, 0xfb, 0x08, 0xff, 0x4e, 0xcf}
//};
//
//static constexpr GUID guid_cfg_preset_shuffle = {
//    0x659c6787, 0x97bb, 0x485b, {0xa0, 0xfc, 0x45, 0xfb, 0x12, 0xb7, 0x3a, 0xa0}
//};
//
//static constexpr GUID guid_cfg_preset_name = {
//    0x186c5741, 0x701e, 0x4f2c, {0xb4, 0x41, 0xe5, 0x57, 0x5c, 0x18, 0xb0, 0xa8}
//};
//
//static constexpr GUID guid_cfg_preset_duration = {
//    0x48d9b7f5, 0x4446, 0x4ab7, {0xb8, 0x71, 0xef, 0xc7, 0x59, 0x43, 0xb9, 0xcd}
//};

static const GUID guid_cfg_bDebugOutput = {
    0x808a73c, 0x8857, 0x4472, {0xad, 0x49, 0xdb, 0x1a, 0x48, 0x4e, 0x3f, 0x5}
}; // {0808A73C-8857-4472-AD49-DB1A484E3F05}
// {CB39278B-4C19-4931-9263-631F6924F1C4}
static const GUID guid_cfg_nMaxPSVersion = {
    0xcb39278b, 0x4c19, 0x4931, {0x92, 0x63, 0x63, 0x1f, 0x69, 0x24, 0xf1, 0xc4}
};
// {779DA878-F890-4DD5-9BB1-B8FC643D0AFB}
static const GUID guid_cfg_nMaxImages = {
    0x779da878, 0xf890, 0x4dd5, {0x9b, 0xb1, 0xb8, 0xfc, 0x64, 0x3d, 0xa, 0xfb}
};
// {904342AE-2844-4970-9C02-8EC4CC75269F}
static const GUID guid_cfg_nMaxBytes = {
    0x904342ae, 0x2844, 0x4970, {0x9c, 0x2, 0x8e, 0xc4, 0xcc, 0x75, 0x26, 0x9f}
};
//// {0C095C95-CBD5-4CA5-A097-84FE1D56B68F}
//static const GUID << name >> = {
//                                 0xc095c95, 0xcbd5, 0x4ca5, {0xa0, 0x97, 0x84, 0xfe, 0x1d, 0x56, 0xb6, 0x8f}
//};

// defaults
constexpr int default_cfg_bogo1 = 1337;
constexpr int default_cfg_bogo2 = 666;
constexpr int default_cfg_bogo3 = 42;
static constexpr char default_cfg_bogo4[] = "foobar";
static constexpr bool default_cfg_bogo5 = false;
// milk.ini
//constexpr int default_nFpsLimit = -1;
constexpr int default_nSongTitlesSpawned = 0;
constexpr bool default_nCustMsgsSpawned = 0;
constexpr bool default_nFramesSinceResize = 0;
static constexpr bool default_bEnableRating = true;
static constexpr bool default_bHardCutsDisabled = true;
static constexpr bool default_bDebugOutput = false;
//static constexpr bool default_bShowSongInfo = GetPrivateProfileBoolW;
//static constexpr bool default_bShowPresetInfo = false;
static constexpr bool default_bShowPressF1ForHelp = true;
//static constexpr bool default_bShowMenuToolTips = GetPrivateProfileBoolW;
static constexpr bool default_bSongTitleAnims = true;
static constexpr bool default_bShowFPS = false;
static constexpr bool default_bShowRating = false;
static constexpr bool default_bShowPresetInfo = false;
//static constexpr bool default_bShowDebugInfo = false;
static constexpr bool default_bShowSongTitle = false;
static constexpr bool default_bShowSongTime = false;
static constexpr bool default_bShowSongLen = false;
//static constexpr bool default_bFixPinkBug = -1;
constexpr int default_n16BitGamma = 2;
static constexpr bool default_bAutoGamma = true;
//static constexpr bool default_bAlways3D = false;
constexpr float default_fStereoSep = 1.0f;
//static constexpr bool default_bFixSlowText = true;
//static constexpr bool default_bAlwaysOnTop = false;
//static constexpr bool default_bWarningsDisabled = false;
static constexpr bool default_bWarningsDisabled2 = false;
//static constexpr bool default_bAnisotropicFiltering = true;
static constexpr bool default_bPresetLockOnAtStartup = false;
static constexpr bool default_bPreventScollLockHandling = false;
constexpr int default_nCanvasStretch = 0;
constexpr int default_nTexSizeX = -1; // -1 means "auto"
constexpr int default_nTexSizeY = -1; // -1 means "auto"
static constexpr bool default_bTexSizeWasAutoPow2 = false; //= (m_nTexSizeX == -2);
static constexpr bool default_bTexSizeWasAutoExact = false; //= (m_nTexSizeX == -1);
constexpr int default_nTexBitsPerCh = 8;
constexpr int default_nGridX = 48; //32;
constexpr int default_nGridY = 36; // = m_nGridX * 3 / 4; //24;
constexpr int default_nMaxPSVersion = -1; // -1 = auto, 0 = disable shaders, 2 = ps_2_0, 3 = ps_3_0
constexpr int default_nMaxImages = 32;
constexpr int default_nMaxBytes = 16000000;
static constexpr bool default_bPresetLockedByCode = false;
constexpr float default_fStartTime = 0.0f;
constexpr float default_fPresetStartTime = 0.0f;
constexpr float default_fNextPresetTime = -1.0f; // negative value means no time set (...it will be auto-set on first call to UpdateTime)
static constexpr bool default_nLoadingPreset = 0;
static constexpr bool default_nPresetsLoadedTotal = 0;
constexpr float default_fSnapPoint = 0.5f;
static constexpr bool default_bShowShaderHelp = false;
constexpr float default_fMotionVectorsTempDx = 0.0f;
constexpr float default_fMotionVectorsTempDy = 0.0f;
constexpr float default_fBlendTimeUser = 1.7f;
constexpr float default_fBlendTimeAuto = 2.7f;
constexpr float default_fTimeBetweenPresets = 16.0f;
constexpr float default_fTimeBetweenPresetsRand = 10.0f;
constexpr float default_fHardCutLoudnessThresh = 2.5f;
constexpr float default_fHardCutHalflife = 60.0f;
constexpr float default_fSongTitleAnimDuration = 1.7f;
constexpr float default_fTimeBetweenRandomSongTitles = -1.0f;
constexpr float default_fTimeBetweenRandomCustomMsgs = -1.0f;
//static constexpr char default_szMilkdrop2Path[]=  L"%ls%ls", GetPluginsDirPath(), SUBDIR),
//static constexpr char default_szMsgIniFile[]= L"%ls%ls", szConfigDir, MSG_INIFILE);
//static constexpr char default_szImgIniFile[]= L"%ls%ls", szConfigDir, IMG_INIFILE),
//static constexpr bool default_szPresetDir = GetPrivateProfileStringW
static constexpr bool default_bEnableDownmix = false;
static constexpr bool default_bAllowTearing = false;
static constexpr bool default_bEnableHDR = false;
constexpr int default_nBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
constexpr int default_nDepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
constexpr int default_nBackBufferCount = 2;
constexpr int default_nMinFeatureLevel = D3D_FEATURE_LEVEL_9_1;

// Advanced preferences order
enum
{
    order_bDebugOutput,
    order_nMaxPSVersion,
    order_nMaxImages,
    order_nMaxBytes,
    order_bogo4,
    order_bogo5,
    order_bogoRadio,
};

enum
{
    order_bogoRadio1,
    order_bogoRadio2,
    order_bogoRadio3,
};

// clang-format off
//static advconfig_string_factory cfg_szPresetDir("Preset Directory", guid_advconfig_branch, 0.0, wctombs(InitializeDefaults()));
//(const char* p_name, const GUID& p_guid, const GUID& p_parent, double p_priority, const char* p_initialstate,
//t_uint32 p_prefFlags = 0)
//static advconfig_string_factory mystring("name goes here", "configStore var name goes here", myguid, parentguid, 0, "asdf");
static cfg_uint cfg_bogoSetting1(guid_cfg_bogoSetting1, default_cfg_bogo1), cfg_bogoSetting2(guid_cfg_bogoSetting2, default_cfg_bogo2);

static advconfig_branch_factory g_advconfigBranch("MilkDrop", guid_advconfig_branch, advconfig_branch::guid_branch_vis, 0);
static advconfig_checkbox_factory cfg_bDebugOutput("Debug Output", "milk2.bDebugOutput", guid_cfg_bDebugOutput, guid_advconfig_branch, order_bDebugOutput, false);
static advconfig_signed_integer_factory cfg_nMaxPSVersion("Max. Pixel Shader Version", "milk2.nMaxPSVersion", guid_cfg_nMaxPSVersion, guid_advconfig_branch, order_nMaxPSVersion, default_nMaxPSVersion, -1, 3);
static advconfig_integer_factory cfg_nMaxImages("Max. Images", "milk2.nMaxImages", guid_cfg_nMaxImages, guid_advconfig_branch, order_nMaxImages, default_nMaxImages, 1, 64);
static advconfig_integer_factory cfg_nMaxBytes("Max. Bytes", "milk2.nMaxBytes", guid_cfg_nMaxBytes, guid_advconfig_branch, order_nMaxBytes, default_nMaxBytes, 1, 32000000);

//static advconfig_string_factory cfg_bogoSetting4("Bogo setting 4", "milk2.bogo4", guid_cfg_bogoSetting4, guid_advconfig_branch, order_bogo4, default_cfg_bogo4);
//static advconfig_checkbox_factory cfg_bogoSetting5("Bogo setting 5", "milk2.bogo5", guid_cfg_bogoSetting5, guid_advconfig_branch, order_bogo5, default_cfg_bogo5);
//static advconfig_branch_factory cfg_bogoRadioBranch("Bogo radio", guid_cfg_bogoRadio, guid_advconfig_branch, order_bogoRadio);
//static advconfig_radio_factory cfg_bogoRadio1("Radio 1", "milk2.bogoRaidio.1", guid_cfg_bogoRadio1, guid_cfg_bogoRadio, order_bogoRadio1, true);
//static advconfig_radio_factory cfg_bogoRadio2("Radio 2", "milk2.bogoRaidio.2", guid_cfg_bogoRadio2, guid_cfg_bogoRadio, order_bogoRadio2, false);
//static advconfig_radio_factory cfg_bogoRadio3("Radio 3", "milk2.bogoRaidio.3", guid_cfg_bogoRadio3, guid_cfg_bogoRadio, order_bogoRadio3, false);

//static wchar_t default_szPluginsDirPath[MAX_PATH];
//static wchar_t default_szConfigIniFile[MAX_PATH];
//static wchar_t default_szMilkdrop2Path[MAX_PATH];
//static wchar_t default_szPresetDir[MAX_PATH];
//static char default_szPresetDirA[MAX_PATH];
//
//wchar_t* InitializeDefaults()
//{
//    std::string base_path = core_api::get_my_full_path();
//    std::string::size_type t = base_path.rfind('\\');
//    if (t != std::string::npos)
//        base_path.erase(t + 1);
//
//    swprintf_s(default_szPluginsDirPath, L"%hs", const_cast<char*>(base_path.c_str()));
//    swprintf_s(default_szConfigIniFile, L"%hs%ls", const_cast<char*>(base_path.c_str()), INIFILE);
//    swprintf_s(default_szMilkdrop2Path, L"%ls%ls", default_szPluginsDirPath, SUBDIR);
//    swprintf_s(default_szPresetDir, L"%lspresets\\", default_szMilkdrop2Path);
//    size_t* pReturnValue;
//    wcstombs_s(pReturnValue, char(&default_szPresetDirA)[MAX_PATH], default_szPresetDir, MAX_PATH);
//    return default_szPresetDir;
//}
// clang-format on

class milk2_preferences_page_instance : public preferences_page_instance, public CDialogImpl<milk2_preferences_page_instance>
{
  public:
    milk2_preferences_page_instance(preferences_page_callback::ptr callback) : m_callback(callback) {}

    enum milk2_dialog_id
    {
        IDD = IDD_PROPPAGE_0 // Dialog resource ID
    };

    // preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
    uint32_t get_state();
    void apply();
    void reset();

    //WTL message map
    // clang-format off
    BEGIN_MSG_MAP_EX(milk2_preferences_page_instance)
    MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_BETWEEN_TIME, EN_CHANGE, OnEditChange)
        COMMAND_HANDLER_EX(IDC_BLEND_AUTO, EN_CHANGE, OnEditChange)
    END_MSG_MAP()
    // clang-format on

  private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnEditChange(UINT, int, CWindow);
    bool HasChanged();
    void OnChanged();

    const preferences_page_callback::ptr m_callback;

    // Dark mode hooks object, must be a member of dialog class.
    fb2k::CDarkModeHooks m_dark;
};

BOOL milk2_preferences_page_instance::OnInitDialog(CWindow, LPARAM)
{
    // Enable dark mode
    // One call does it all, applies all relevant hacks automatically
    m_dark.AddDialogWithControls(*this);

    SetDlgItemInt(IDC_BETWEEN_TIME, (UINT)cfg_bogoSetting1, FALSE);
    SetDlgItemInt(IDC_BLEND_AUTO, (UINT)cfg_bogoSetting2, FALSE);
    return FALSE;
}

void milk2_preferences_page_instance::OnEditChange(UINT, int, CWindow)
{
    OnChanged();
}

uint32_t milk2_preferences_page_instance::get_state()
{
    // IMPORTANT: Always return dark_mode_supported - tell foobar2000 that this preferences page is dark mode compliant.
    uint32_t state = preferences_state::resettable | preferences_state::dark_mode_supported;
    if (HasChanged())
        state |= preferences_state::changed;
    return state;
}

void milk2_preferences_page_instance::reset()
{
    SetDlgItemInt(IDC_BETWEEN_TIME, default_cfg_bogo1, FALSE);
    SetDlgItemInt(IDC_BLEND_AUTO, default_cfg_bogo2, FALSE);
    OnChanged();
}

void milk2_preferences_page_instance::apply()
{
    cfg_bogoSetting1 = GetDlgItemInt(IDC_BETWEEN_TIME, NULL, FALSE);
    cfg_bogoSetting2 = GetDlgItemInt(IDC_BLEND_AUTO, NULL, FALSE);

    OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool milk2_preferences_page_instance::HasChanged()
{
    //returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
    return GetDlgItemInt(IDC_BETWEEN_TIME, NULL, FALSE) != cfg_bogoSetting1 ||
           GetDlgItemInt(IDC_BLEND_AUTO, NULL, FALSE) != cfg_bogoSetting2;
}

// Tells the host that state has changed to enable or disable the "Apply" button appropriately.
void milk2_preferences_page_instance::OnChanged()
{
    m_callback->on_state_changed();
}

class milk2_preferences_page : public preferences_page_impl<milk2_preferences_page_instance>
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
};

static preferences_page_factory_t<milk2_preferences_page> g_preferences_page_milk2_factory;

milk2_config::milk2_config()
{
    reset();
}

uint32_t milk2_config::g_get_version()
{
    return 0u;
}

void milk2_config::reset()
{
    // milk.ini
    //m_nFpsLimit = -1;
    m_nSongTitlesSpawned = 0;
    m_nCustMsgsSpawned = 0;
    m_nFramesSinceResize = 0;

    m_bEnableRating = true;
    m_bHardCutsDisabled = true;
    g_bDebugOutput = cfg_bDebugOutput.get();
    //m_bShowSongInfo = GetPrivateProfileBoolW
    //m_bShowPresetInfo = false;
    m_bShowPressF1ForHelp = true;
    //m_bShowMenuToolTips = GetPrivateProfileBoolW
    m_bSongTitleAnims = true;

    m_bShowFPS = false;
    m_bShowRating = false;
    m_bShowPresetInfo = false;
    //m_bShowDebugInfo = false;
    m_bShowSongTitle = false;
    m_bShowSongTime = false;
    m_bShowSongLen = false;

    //m_bFixPinkBug = -1;
    m_n16BitGamma = 2;

    m_bAutoGamma = true;

    //m_bAlways3D = false;
    //m_fStereoSep = 1.0f;
    //m_bFixSlowText = true;
    //m_bAlwaysOnTop = false;
    //m_bWarningsDisabled = false;
    m_bWarningsDisabled2 = false;
    //m_bAnisotropicFiltering = true;
    m_bPresetLockOnAtStartup = false;
    m_bPreventScollLockHandling = false;

    m_nCanvasStretch = 0;
    m_nTexSizeX = -1; // -1 means "auto"
    m_nTexSizeY = -1; // -1 means "auto"
    m_bTexSizeWasAutoPow2 = (m_nTexSizeX == -2); //= false;
    m_bTexSizeWasAutoExact = (m_nTexSizeX == -1); //= false;
    m_nTexBitsPerCh = 8;
    m_nGridX = 48; //32;
    m_nGridY = m_nGridX * 3 / 4; // = 36; //24;
    m_nMaxPSVersion = default_nMaxPSVersion; // -1 = auto, 0 = disable shaders, 2 = ps_2_0, 3 = ps_3_0
    m_nMaxImages = default_nMaxImages;
    m_nMaxBytes = default_nMaxBytes;
    m_bPresetLockedByCode = false;
    m_fStartTime = 0.0f;
    m_fPresetStartTime = 0.0f;
    m_fNextPresetTime = -1.0f; // negative value means no time set (...it will be auto-set on first call to UpdateTime)
    m_nLoadingPreset = 0;
    m_nPresetsLoadedTotal = 0;
    m_fSnapPoint = 0.5f;
    m_bShowShaderHelp = false;
    m_fMotionVectorsTempDx = 0.0f;
    m_fMotionVectorsTempDy = 0.0f;
    m_fBlendTimeUser = 1.7f;
    m_fBlendTimeAuto = 2.7f;
    m_fTimeBetweenPresets = 16.0f;
    m_fTimeBetweenPresetsRand = 10.0f;
    m_fHardCutLoudnessThresh = 2.5f;
    m_fHardCutHalflife = 60.0f;
    m_fSongTitleAnimDuration = 1.7f;
    m_fTimeBetweenRandomSongTitles = -1.0f;
    m_fTimeBetweenRandomCustomMsgs = -1.0f;

    //swprintf_s(m_szMilkdrop2Path, L"%ls%ls", GetPluginsDirPath(), SUBDIR);
    //swprintf_s(m_szPresetDir, L"%lspresets\\", m_szMilkdrop2Path);
    //swprintf_s(m_szMsgIniFile, L"%ls%ls", szConfigDir, MSG_INIFILE);
    //swprintf_s(m_szImgIniFile, L"%ls%ls", szConfigDir, IMG_INIFILE);
    //m_szPresetDir = GetPrivateProfileStringW

    //// Bounds checking.
    //if (m_nGridX > MAX_GRID_X)
    //    m_nGridX = MAX_GRID_X;
    //if (m_nGridY > MAX_GRID_Y)
    //    m_nGridY = MAX_GRID_Y;
    //if (m_fTimeBetweenPresetsRand < 0)
    //    m_fTimeBetweenPresetsRand = 0;
    //if (m_fTimeBetweenPresets < 0.1f)
    //    m_fTimeBetweenPresets = 0.1f;

    // DERIVED SETTINGS
    m_bPresetLockedByUser = m_bPresetLockOnAtStartup;
    //m_bMilkdropScrollLockState = m_bPresetLockOnAtStartup;

    m_bEnableDownmix = false;
    m_bAllowTearing = false;
    m_bEnableHDR = false;
    m_nBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    m_nDepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    m_nBackBufferCount = 2;
    m_nMinFeatureLevel = D3D_FEATURE_LEVEL_9_1;
}

void milk2_config::parse(ui_element_config_parser& parser)
{
    reset();

    //to retrieve state : pfc::string8 val;
    //mystring.get(val);

    //to retrieve state : myint.get();
    try
    {
        uint32_t version;
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
                parser >> g_bDebugOutput;
                parser >> m_bPresetLockedByUser;
                parser >> m_bEnableDownmix;
                break;
            default:
                console::formatter() << core_api::get_my_file_name() << ": Unknown configuration format version: " << version;
        }
    }
    catch (exception_io& exc)
    {
        console::formatter() << core_api::get_my_file_name() << ": Exception while reading configuration data: " << exc;
    }
}

void milk2_config::build(ui_element_config_builder& builder)
{
    builder << g_get_version();
    builder << g_bDebugOutput;
    builder << m_bPresetLockedByUser;
    //builder << m_line_stroke_width;
    //builder << m_low_quality_enabled;
    //builder << m_resample_enabled;
    //builder << m_refresh_rate_limit_hz;
    //builder << m_trigger_enabled;
    //builder << m_hw_rendering_enabled;
    builder << m_bEnableDownmix;
    //builder << m_window_duration_millis;
    //builder << m_zoom_percent;
}