/*
 * settings.h - Defines common settings structure.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#pragma once

static constexpr int g_nFontSize[] = {
    6,   7,   8,  9,  10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24, 26,
    28,  30,  32, 36, 40, 44, 48, 52, 56, 60, 64, 72, 80, 88, 96, 104,
    112, 120, 128
};

template <int N>
struct F
{
    constexpr wchar_t* __wcscpy(wchar_t* dst, const wchar_t* src)
    {
        wchar_t* cp = dst;
        wchar_t c;
        do
        {
            c = *src++;
            *cp++ = c;
        } while (c != L'\0');
        return dst;
    }

    constexpr F() : fontinfo()
    {
        __wcscpy(fontinfo[SIMPLE_FONT].szFace, SIMPLE_FONT_DEFAULT_FACE);
        fontinfo[SIMPLE_FONT].nSize = SIMPLE_FONT_DEFAULT_SIZE;
        fontinfo[SIMPLE_FONT].bBold = SIMPLE_FONT_DEFAULT_BOLD;
        fontinfo[SIMPLE_FONT].bItalic = SIMPLE_FONT_DEFAULT_ITAL;
        fontinfo[SIMPLE_FONT].bAntiAliased = SIMPLE_FONT_DEFAULT_AA;
        __wcscpy(fontinfo[DECORATIVE_FONT].szFace, DECORATIVE_FONT_DEFAULT_FACE);
        fontinfo[DECORATIVE_FONT].nSize = DECORATIVE_FONT_DEFAULT_SIZE;
        fontinfo[DECORATIVE_FONT].bBold = DECORATIVE_FONT_DEFAULT_BOLD;
        fontinfo[DECORATIVE_FONT].bItalic = DECORATIVE_FONT_DEFAULT_ITAL;
        fontinfo[DECORATIVE_FONT].bAntiAliased = DECORATIVE_FONT_DEFAULT_AA;
        __wcscpy(fontinfo[HELPSCREEN_FONT].szFace, HELPSCREEN_FONT_DEFAULT_FACE);
        fontinfo[HELPSCREEN_FONT].nSize = HELPSCREEN_FONT_DEFAULT_SIZE;
        fontinfo[HELPSCREEN_FONT].bBold = HELPSCREEN_FONT_DEFAULT_BOLD;
        fontinfo[HELPSCREEN_FONT].bItalic = HELPSCREEN_FONT_DEFAULT_ITAL;
        fontinfo[HELPSCREEN_FONT].bAntiAliased = HELPSCREEN_FONT_DEFAULT_AA;
        __wcscpy(fontinfo[PLAYLIST_FONT].szFace, PLAYLIST_FONT_DEFAULT_FACE);
        fontinfo[PLAYLIST_FONT].nSize = PLAYLIST_FONT_DEFAULT_SIZE;
        fontinfo[PLAYLIST_FONT].bBold = PLAYLIST_FONT_DEFAULT_BOLD;
        fontinfo[PLAYLIST_FONT].bItalic = PLAYLIST_FONT_DEFAULT_ITAL;
        fontinfo[PLAYLIST_FONT].bAntiAliased = PLAYLIST_FONT_DEFAULT_AA;

#if (NUM_EXTRA_FONTS >= 1)
        __wcscpy(fontinfo[NUM_BASIC_FONTS + 0].szFace, EXTRA_FONT_1_DEFAULT_FACE);
        fontinfo[NUM_BASIC_FONTS + 0].nSize = EXTRA_FONT_1_DEFAULT_SIZE;
        fontinfo[NUM_BASIC_FONTS + 0].bBold = EXTRA_FONT_1_DEFAULT_BOLD;
        fontinfo[NUM_BASIC_FONTS + 0].bItalic = EXTRA_FONT_1_DEFAULT_ITAL;
        fontinfo[NUM_BASIC_FONTS + 0].bAntiAliased = EXTRA_FONT_1_DEFAULT_AA;
#endif
#if (NUM_EXTRA_FONTS >= 2)
        __wcscpy(fontinfo[NUM_BASIC_FONTS + 1].szFace, EXTRA_FONT_2_DEFAULT_FACE);
        fontinfo[NUM_BASIC_FONTS + 1].nSize = EXTRA_FONT_2_DEFAULT_SIZE;
        fontinfo[NUM_BASIC_FONTS + 1].bBold = EXTRA_FONT_2_DEFAULT_BOLD;
        fontinfo[NUM_BASIC_FONTS + 1].bItalic = EXTRA_FONT_2_DEFAULT_ITAL;
        fontinfo[NUM_BASIC_FONTS + 1].bAntiAliased = EXTRA_FONT_2_DEFAULT_AA;
#endif
#if (NUM_EXTRA_FONTS >= 3)
        __wcscpy(fontinfo[NUM_BASIC_FONTS + 2].szFace, EXTRA_FONT_3_DEFAULT_FACE);
        fontinfo[NUM_BASIC_FONTS + 2].nSize = EXTRA_FONT_3_DEFAULT_SIZE;
        fontinfo[NUM_BASIC_FONTS + 2].bBold = EXTRA_FONT_3_DEFAULT_BOLD;
        fontinfo[NUM_BASIC_FONTS + 2].bItalic = EXTRA_FONT_3_DEFAULT_ITAL;
        fontinfo[NUM_BASIC_FONTS + 2].bAntiAliased = EXTRA_FONT_3_DEFAULT_AA;
#endif
#if (NUM_EXTRA_FONTS >= 4)
        __wcscpy(fontinfo[NUM_BASIC_FONTS + 3].szFace, EXTRA_FONT_4_DEFAULT_FACE);
        fontinfo[NUM_BASIC_FONTS + 3].nSize = EXTRA_FONT_4_DEFAULT_SIZE;
        fontinfo[NUM_BASIC_FONTS + 3].bBold = EXTRA_FONT_4_DEFAULT_BOLD;
        fontinfo[NUM_BASIC_FONTS + 3].bItalic = EXTRA_FONT_4_DEFAULT_ITAL;
        fontinfo[NUM_BASIC_FONTS + 3].bAntiAliased = EXTRA_FONT_4_DEFAULT_AA;
#endif
#if (NUM_EXTRA_FONTS >= 5)
        __wcscpy(fontinfo[NUM_BASIC_FONTS + 4].szFace, EXTRA_FONT_5_DEFAULT_FACE);
        fontinfo[NUM_BASIC_FONTS + 4].nSize = EXTRA_FONT_5_DEFAULT_SIZE;
        fontinfo[NUM_BASIC_FONTS + 4].bBold = EXTRA_FONT_5_DEFAULT_BOLD;
        fontinfo[NUM_BASIC_FONTS + 4].bItalic = EXTRA_FONT_5_DEFAULT_ITAL;
        fontinfo[NUM_BASIC_FONTS + 4].bAntiAliased = EXTRA_FONT_5_DEFAULT_AA;
#endif
    }

    td_fontinfo fontinfo[N];
};

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
    bool m_bShowAlbum;
    bool m_bEnableHDR;
    bool m_bSkipCompShader;
    uint32_t m_nBackBufferFormat;
    uint32_t m_nDepthBufferFormat;
    uint32_t m_nBackBufferCount;
    uint32_t m_nMinFeatureLevel;

    //int32_t m_nFpsLimit;

    //--- Paths
    wchar_t m_szPluginsDirPath[MAX_PATH];
    wchar_t m_szConfigIniFile[MAX_PATH];
    wchar_t m_szMsgIniFile[MAX_PATH];
    wchar_t m_szImgIniFile[MAX_PATH];
    wchar_t m_szPresetDir[MAX_PATH];

    //--- Formats
    wchar_t m_szTitleFormat[256];
    wchar_t m_szArtworkFormat[256];

    //--- Artwork
    artFetchData* m_artData;

    //--- Fonts
    td_fontinfo m_fontinfo[NUM_BASIC_FONTS + NUM_EXTRA_FONTS];
} plugin_config;