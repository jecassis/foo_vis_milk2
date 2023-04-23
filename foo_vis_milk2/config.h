#pragma once

class milk2_config
{
  public:
    milk2_config();

    uint32_t g_get_version();
    void reset();
    void parse(ui_element_config_parser& parser);
    void build(ui_element_config_builder& builder);

    //bool m_hw_rendering_enabled;
    //bool m_trigger_enabled;
    //bool m_resample_enabled;
    //bool m_low_quality_enabled;
    //uint32_t m_window_duration_millis;
    //uint32_t m_zoom_percent;
    //uint32_t m_refresh_rate_limit_hz;
    //uint32_t m_line_stroke_width;

    // "milk2.ini" settings
    //int32_t m_nFpsLimit;
    uint32_t m_nSongTitlesSpawned;
    uint32_t m_nCustMsgsSpawned;
    uint32_t m_nFramesSinceResize;

    bool m_bEnableRating;
    bool m_bHardCutsDisabled;
    bool g_bDebugOutput;
    //bool m_bShowSongInfo;
    //bool m_bShowPresetInfo;
    bool m_bShowPressF1ForHelp;
    //bool m_bShowMenuToolTips;
    bool m_bSongTitleAnims;

    bool m_bShowFPS;
    bool m_bShowRating;
    bool m_bShowPresetInfo;
    //bool m_bShowDebugInfo;
    bool m_bShowSongTitle;
    bool m_bShowSongTime;
    bool m_bShowSongLen;

    //int32_t m_bFixPinkBug;
    uint32_t m_n16BitGamma;
    bool m_bAutoGamma;

    //bool m_bAlways3D;
    //float m_fStereoSep;
    //bool m_bFixSlowText;
    //bool m_bAlwaysOnTop;
    //bool m_bWarningsDisabled;
    bool m_bWarningsDisabled2;
    //bool m_bAnisotropicFiltering;
    bool m_bPresetLockOnAtStartup;
    bool m_bPreventScollLockHandling;

    uint32_t m_nCanvasStretch;
    int32_t m_nTexSizeX;
    int32_t m_nTexSizeY;
    bool m_bTexSizeWasAutoPow2;
    bool m_bTexSizeWasAutoExact;
    uint32_t m_nTexBitsPerCh;
    uint32_t m_nGridX;
    uint32_t m_nGridY;
    int32_t m_nMaxPSVersion;
    uint32_t m_nMaxImages;
    uint32_t m_nMaxBytes;
    bool m_bPresetLockedByCode;
    float m_fStartTime;
    float m_fPresetStartTime;
    float m_fNextPresetTime;
    uint32_t m_nLoadingPreset;
    uint32_t m_nPresetsLoadedTotal;
    float m_fSnapPoint;
    bool m_bShowShaderHelp;

    float m_fMotionVectorsTempDx;
    float m_fMotionVectorsTempDy;
    float m_fBlendTimeUser;
    float m_fBlendTimeAuto;
    float m_fTimeBetweenPresets;
    float m_fTimeBetweenPresetsRand;
    float m_fHardCutLoudnessThresh;
    float m_fHardCutHalflife;
    float m_fSongTitleAnimDuration;
    float m_fTimeBetweenRandomSongTitles;
    float m_fTimeBetweenRandomCustomMsgs;

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
    bool m_bPresetLockedByUser = m_bPresetLockOnAtStartup;
    //bool m_bMilkdropScrollLockState = m_bPresetLockOnAtStartup;

    bool m_bEnableDownmix;
    bool m_bAllowTearing;
    bool m_bEnableHDR;
    uint32_t m_nBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    uint32_t m_nDepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    uint32_t m_nBackBufferCount = 2;
    uint32_t m_nMinFeatureLevel = D3D_FEATURE_LEVEL_9_1;
};