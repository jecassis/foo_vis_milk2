//
// preferences.cpp: Configuration settings accessible through a preferences page and advanced preferences.
//

#include "pch.h"
#include "resource.h"
#include "config.h"
#include <vis_milk2/defines.h>
#include <vis_milk2/md_defines.h>

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
constexpr int default_cfg_bogo2 = 666;
constexpr int default_cfg_bogo3 = 42;
static constexpr char default_cfg_szPresetDir[] = "foobar";
// milk.ini
//constexpr int default_nFpsLimit = -1;
constexpr int default_nSongTitlesSpawned = 0;
constexpr bool default_nCustMsgsSpawned = 0;
constexpr bool default_nFramesSinceResize = 0;
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
//constexpr int default_nTexSizeY = -1; // -1 means "auto"
//static constexpr bool default_bTexSizeWasAutoPow2 = false;
//static constexpr bool default_bTexSizeWasAutoExact = false;
constexpr int default_nTexBitsPerCh = 8;
constexpr int default_nGridX = 48; //32;
//constexpr int default_nGridY = 36; //24;
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
    order_szPresetDir,
};

// clang-format off
static cfg_bool cfg_bPresetLockOnAtStartup(guid_cfg_bPresetLockOnAtStartup, default_bPresetLockOnAtStartup);
static cfg_bool cfg_bPreventScollLockHandling(guid_cfg_bPreventScollLockHandling, default_bPreventScollLockHandling);
static advconfig_branch_factory g_advconfigBranch("MilkDrop", guid_advconfig_branch, advconfig_branch::guid_branch_vis, 0);
static advconfig_checkbox_factory cfg_bDebugOutput("Debug Output", "milk2.bDebugOutput", guid_cfg_bDebugOutput, guid_advconfig_branch, order_bDebugOutput, default_bDebugOutput);
static advconfig_signed_integer_factory cfg_nMaxPSVersion("Max. Pixel Shader Version", "milk2.nMaxPSVersion", guid_cfg_nMaxPSVersion, guid_advconfig_branch, order_nMaxPSVersion, default_nMaxPSVersion, -1, 3);
static advconfig_integer_factory cfg_nMaxImages("Max. Images", "milk2.nMaxImages", guid_cfg_nMaxImages, guid_advconfig_branch, order_nMaxImages, default_nMaxImages, 1, 64);
static advconfig_integer_factory cfg_nMaxBytes("Max. Bytes", "milk2.nMaxBytes", guid_cfg_nMaxBytes, guid_advconfig_branch, order_nMaxBytes, default_nMaxBytes, 1, 32000000);
static advconfig_string_factory cfg_szPresetDir("Preset Directory", "milk2.szPresetDir", guid_cfg_szPresetDir, guid_advconfig_branch, order_szPresetDir, default_cfg_szPresetDir);
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
    void apply();
    void reset();

    // clang-format off
    BEGIN_MSG_MAP_EX(milk2_preferences_page)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_CB_SCROLLON3, BN_CLICKED, OnButtonClicked)
        COMMAND_HANDLER_EX(IDC_CB_SCROLLON4, BN_CLICKED, OnButtonClicked)
    END_MSG_MAP()
    // clang-format on

  private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnEditChange(UINT, int, CWindow);
    void OnButtonClicked(UINT, int, CWindow);
    bool HasChanged() const;
    void OnChanged();

    const preferences_page_callback::ptr m_callback;

    fb2k::CDarkModeHooks m_dark;
};

BOOL milk2_preferences_page::OnInitDialog(CWindow, LPARAM)
{
    // Enable dark mode
    // One call does it all, applies all relevant hacks automatically.
    m_dark.AddDialogWithControls(*this);

    CheckDlgButton(IDC_CB_SCROLLON3, static_cast<UINT>(cfg_bPresetLockOnAtStartup));
    CheckDlgButton(IDC_CB_SCROLLON4, static_cast<UINT>(cfg_bPreventScollLockHandling));
    return FALSE;
}

void milk2_preferences_page::OnEditChange(UINT, int, CWindow)
{
    OnChanged();
}

void milk2_preferences_page::OnButtonClicked(UINT, int, CWindow)
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
    CheckDlgButton(IDC_CB_SCROLLON3, static_cast<UINT>(cfg_bPresetLockOnAtStartup));
    CheckDlgButton(IDC_CB_SCROLLON4, static_cast<UINT>(cfg_bPreventScollLockHandling));
    OnChanged();
}

void milk2_preferences_page::apply()
{
    cfg_bPresetLockOnAtStartup = static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON3));
    cfg_bPreventScollLockHandling = static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON4));

    OnChanged(); // The dialog content has not changed but the flags have; the currently shown values now match the settings so the apply button can be disabled.
}

bool milk2_preferences_page::HasChanged() const
{
    // Returns whether the dialog content is different from the current configuration (whether the apply button should be enabled or not).
    return static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON3)) != cfg_bPresetLockOnAtStartup ||
           static_cast<bool>(IsDlgButtonChecked(IDC_CB_SCROLLON4)) != cfg_bPreventScollLockHandling;
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
        p_out << "https://www.geisswerks.com/milkdrop/milkdrop.html"; //"http://help.foobar2000.org/" << core_version_info::g_get_version_string() << "/" << "preferences" << "/" << pfc::print_guid(get_guid()) << "/" << get_name();
        return true;
    }
};

static preferences_page_factory_t<preferences_page_milk2> g_preferences_page_milk2_factory;

milk2_config::milk2_config()
{
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

    m_bEnableRating = default_bEnableRating;
    m_bHardCutsDisabled = default_bHardCutsDisabled;
    g_bDebugOutput = cfg_bDebugOutput.get();
    //m_bShowSongInfo;
    m_bShowPressF1ForHelp = default_bShowPressF1ForHelp;
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
    m_bPresetLockOnAtStartup = default_bPresetLockOnAtStartup;
    m_bPreventScollLockHandling = default_bPreventScollLockHandling;

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
    m_fTimeBetweenPresets = default_fTimeBetweenPresets;
    m_fTimeBetweenPresetsRand = default_fTimeBetweenPresetsRand;
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
    if (m_fTimeBetweenPresetsRand < 0.0f)
        m_fTimeBetweenPresetsRand = 0.0f;
    if (m_fTimeBetweenPresets < 0.1f)
        m_fTimeBetweenPresets = 0.1f;

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

    InitializePaths();
}

void milk2_config::InitializePaths()
{
    std::string base_path = core_api::get_my_full_path();
    std::string::size_type t = base_path.rfind('\\');
    if (t != std::string::npos)
        base_path.erase(t + 1);

    swprintf_s(m_szPluginsDirPath, L"%hs", const_cast<char*>(base_path.c_str()));
    swprintf_s(m_szConfigIniFile, L"%hs%ls", const_cast<char*>(base_path.c_str()), INIFILE);
    swprintf_s(m_szMilkdrop2Path, L"%ls%ls", m_szPluginsDirPath, SUBDIR);
    swprintf_s(m_szPresetDir, L"%lspresets\\", m_szMilkdrop2Path);
    wcstombs_s(nullptr, m_szPresetDirA, m_szPresetDir, MAX_PATH);
    wchar_t szConfigDir[MAX_PATH] = {0};
    wcscpy_s(szConfigDir, m_szConfigIniFile);
    wchar_t* p = wcsrchr(szConfigDir, L'\\');
    if (p)
        *(p + 1) = 0;
    swprintf_s(m_szMsgIniFile, L"%ls%ls", szConfigDir, MSG_INIFILE);
    swprintf_s(m_szImgIniFile, L"%ls%ls", szConfigDir, IMG_INIFILE);
}

// Reads the configuration.
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
    builder << g_get_version();
    //builder << m_nFpsLimit;
    builder << m_nSongTitlesSpawned;
    builder << m_nCustMsgsSpawned;
    builder << m_nFramesSinceResize;

    builder << m_bEnableRating;
    builder << m_bHardCutsDisabled;
    builder << cfg_bDebugOutput.get();
    //builder << m_bShowSongInfo;
    builder << m_bShowPressF1ForHelp;
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
    builder << m_bWarningsDisabled2;
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

    builder << m_fBlendTimeUser;
    builder << m_fBlendTimeAuto;
    builder << m_fTimeBetweenPresets;
    builder << m_fTimeBetweenPresetsRand;
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