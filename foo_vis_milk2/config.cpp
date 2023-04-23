#include "pch.h"
#include "config.h"

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
    g_bDebugOutput = false;
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
    m_nMaxPSVersion = -1; // -1 = auto, 0 = disable shaders, 2 = ps_2_0, 3 = ps_3_0
    m_nMaxImages = 32;
    m_nMaxBytes = 16000000;
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