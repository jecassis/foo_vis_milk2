/*
 *  settings.h - Defines common settings structure.
 */

#pragma once

typedef struct
{
    //--- CPluginShell::ReadConfig()
    DXGI_SAMPLE_DESC m_multisample_fs;

    //uint32_t m_start_fullscreen;
    uint32_t m_max_fps_fs;
    uint32_t m_show_press_f1_msg;
    uint32_t m_allow_page_tearing_fs;
    //uint32_t m_save_cpu;
    //uint32_t m_skin;
    //uint32_t m_fix_slow_text;

    //DXGI_MODE_DESC1 m_disp_mode_fs;

    //--- CPlugin::MilkDropReadConfig()
    bool m_bEnableRating;
    bool m_bHardCutsDisabled;
    bool g_bDebugOutput;
    //bool m_bShowSongInfo;
    //bool m_bShowPressF1ForHelp;
    //bool m_bShowMenuToolTips;
    bool m_bSongTitleAnims;

    bool m_bShowFPS;
    bool m_bShowRating;
    bool m_bShowPresetInfo;
    //bool m_bShowDebugInfo;
    bool m_bShowSongTitle;
    bool m_bShowSongTime;
    bool m_bShowSongLen;
    bool m_bShowShaderHelp;

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

    float m_fBlendTimeUser;
    float m_fBlendTimeAuto;
    float m_fTimeBetweenPresets;
    float m_fTimeBetweenPresetsRand;
    float m_fHardCutLoudnessThresh;
    float m_fHardCutHalflife;
    float m_fSongTitleAnimDuration;
    float m_fTimeBetweenRandomSongTitles;
    float m_fTimeBetweenRandomCustomMsgs;

    bool m_bPresetLockedByCode;
    bool m_bPresetLockedByUser;
    //bool m_bMilkdropScrollLockState;

    //--- Extras
    bool m_bEnableDownmix;
    bool m_bEnableHDR;
    uint32_t m_nBackBufferFormat;
    uint32_t m_nDepthBufferFormat;
    uint32_t m_nBackBufferCount;
    uint32_t m_nMinFeatureLevel;

    //int32_t m_nFpsLimit;

    //--- Paths
    wchar_t m_szPluginsDirPath[MAX_PATH];
    wchar_t m_szConfigIniFile[MAX_PATH];
    wchar_t m_szPresetDir[MAX_PATH];
} plugin_config;