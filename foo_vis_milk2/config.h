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
#include "version.h"

static constexpr GUID guid_milk2 = {
    0x204b0345, 0x4df5, 0x4b47, {0xad, 0xd3, 0x98, 0x9f, 0x81, 0x1b, 0xd9, 0xa5}
}; // {204B0345-4DF5-4B47-ADD3-989F811BD9A5}

static constexpr GUID guid_milk2_preferences = {
    0x5feadec6, 0x37f3, 0x4ebf, {0xb3, 0xfd, 0x57, 0x11, 0xfa, 0xe3, 0xa3, 0xe8}
}; // {5FEADEC6-37F3-4EBF-B3FD-5711FAE3A3E8}

//static constexpr GUID VisMilk2LangGUID = {
//    0xc5d175f1, 0xe4e4, 0x47ee, {0xb8, 0x5c, 0x4e, 0xdc, 0x6b, 0x2, 0x6a, 0x35}
//}; // {C5D175F1-E4E4-47EE-B85C-4EDC6B026A35}

namespace
{
// Settings controlled in preferences page.
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
static constexpr GUID guid_cfg_bHardCutsDisabled = {
    0x5d1a0e92, 0x3f95, 0x410c, {0xad, 0x66, 0xb4, 0xa5, 0xfa, 0x4f, 0xdc, 0xf4}
}; // {5D1A0E92-3F95-410C-AD66-B4A5FA4FDCF4}
static constexpr GUID guid_cfg_n16BitGamma = {
    0x350ba0d5, 0xee5b, 0x4886, {0x96, 0x3, 0x7f, 0xd4, 0xf5, 0xf8, 0x1b, 0x10}
}; // {350BA0D5-EE5B-4886-9603-7FD4F5F81B10}
static constexpr GUID guid_cfg_bAutoGamma = {
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
static constexpr GUID guid_cfg_bSongTitleAnims = {
    0x7ff565aa, 0x8402, 0x4ae0, {0x99, 0xc7, 0x11, 0x18, 0x44, 0x1d, 0xee, 0xc2}
}; // {7FF565AA-8402-4AE0-99C7-1118441DEEC2}
static constexpr GUID guid_cfg_fSongTitleAnimDuration = {
    0xe539e22c, 0x2c41, 0x4238, {0xae, 0xa9, 0x52, 0x25, 0xda, 0x29, 0xba, 0xcf}
}; // {E539E22C-2C41-4238-AEA9-5225DA29BACF}
static constexpr GUID guid_cfg_fTimeBetweenRandomSongTitles = {
    0xbea5f8e5, 0x48d1, 0x4063, {0xa1, 0xf2, 0x9d, 0x2, 0xfd, 0x33, 0xce, 0x4d}
}; // {BEA5F8E5-48D1-4063-A1F2-9D02FD33CE4D}
static constexpr GUID guid_cfg_fTimeBetweenRandomCustomMsgs = {
    0xd7778394, 0x8ed4, 0x4d0b, {0xb8, 0xec, 0x2e, 0x1d, 0xfb, 0x25, 0x49, 0x74}
}; // {D7778394-8ED4-4D0B-B8EC-2E1DFB254974}
static constexpr GUID guid_cfg_nCanvasStretch = {
    0xce121917, 0xc83d, 0x4a0b, {0xb7, 0x4c, 0x56, 0xf8, 0x3c, 0x97, 0xbe, 0x6c}
}; // {CE121917-C83D-4A0B-B74C-56F83C97BE6C}
static constexpr GUID guid_cfg_nGridX = {
    0xacf37191, 0xfbb4, 0x4ffa, {0x95, 0x24, 0xd1, 0x17, 0xba, 0x11, 0x4, 0x47}
}; // {ACF37191-FBB4-4FFA-9524-D117BA110447}
static constexpr GUID guid_cfg_nTexSizeX = {
    0xa2cd1e44, 0x9056, 0x4a2c, {0x97, 0x9e, 0x5b, 0xa4, 0x52, 0x34, 0x80, 0x3e}
}; // {A2CD1E44-9056-4A2C-979E-5BA45234803E}
static constexpr GUID guid_cfg_nTexBitsPerCh = {
    0xab5a6d53, 0xb4c9, 0x41c3, {0xa5, 0x66, 0x1d, 0x83, 0x90, 0xdb, 0x1e, 0xfa}
}; // {AB5A6D53-B4C9-41C3-A566-1D8390DB1EFA}
static constexpr GUID guid_cfg_szTitleFormat = {
    0xca37e590, 0xb292, 0x43b5, {0xa2, 0x34, 0x3d, 0xba, 0x21, 0xa7, 0xa0, 0xdb}
}; // {CA37E590-B292-43B5-A234-3DBA21A7A0DB}
static constexpr GUID guid_cfg_szArtworkFormat = {
    0xdcbf8993, 0x9fe7, 0x4b34, {0x8b, 0x6e, 0x34, 0xa8, 0x5b, 0x22, 0xe1, 0xd4}
}; // {DCBF8993-9FE7-4B34-8B6E-34A85B22E1D4}
static constexpr GUID guid_cfg_bSkipCompShader = {
    0xa9220355, 0x1382, 0x41f3, {0xbd, 0x41, 0x7e, 0xb3, 0xe, 0x94, 0xa6, 0x42}
}; // {A9220355-1382-41F3-BD41-7EB30E94A642}
static constexpr GUID guid_cfg_stFontInfo = {
    0x9794db85, 0xa1cf, 0x4532, {0x9e, 0x1b, 0x56, 0xec, 0xed, 0x22, 0x24, 0x3a}
}; // {9794DB85-A1CF-4532-9E1B-56ECED22243A}

// Settings controlled in advanced preferences.
static constexpr GUID guid_advconfig_branch = {
    0xd7ab1cd7, 0x7956, 0x4497, {0x9b, 0x1d, 0x74, 0x78, 0x7e, 0xde, 0x1d, 0xbc}
}; // {D7AB1CD7-7956-4497-9B1D-74787EDE1DBC}
static constexpr GUID guid_cfg_bDebugOutput = {
    0x808a73c, 0x8857, 0x4472, {0xad, 0x49, 0xdb, 0x1a, 0x48, 0x4e, 0x3f, 0x5}
}; // {0808A73C-8857-4472-AD49-DB1A484E3F05}
static constexpr GUID guid_cfg_szPresetDir = {
    0xfa9e467b, 0xfe6d, 0x4d79, {0x83, 0x98, 0xcd, 0x3d, 0x8b, 0xf4, 0x7a, 0x63}
}; // {FA9E467B-FE6D-4D79-8398-CD3D8BF47A63}

// State settings saved on close and restored on launch.
// Controlled in either the context menu or via the keyboard shortcuts.
static constexpr GUID guid_cfg_bShowFPS = {
    0xbc6670cd, 0x85ac, 0x48d9, {0x87, 0xb8, 0xf4, 0x7c, 0x63, 0xd4, 0x64, 0x5b}
}; // {BC6670CD-85AC-48D9-87B8-F47C63D4645B}
static constexpr GUID guid_cfg_bShowRating = {
    0xfefa4368, 0x2ee3, 0x4774, {0xa9, 0x44, 0xdf, 0xbe, 0x99, 0x3f, 0xe, 0xb3}
}; // {FEFA4368-2EE3-4774-A944-DFBE993F0EB3}
static constexpr GUID guid_cfg_bShowPresetInfo = {
    0x99648b4, 0xc44a, 0x490a, {0xa2, 0xd7, 0x63, 0xd, 0x7f, 0x7a, 0xbf, 0xae}
}; // {099648B4-C44A-490A-A2D7-630D7F7ABFAE}
static constexpr GUID guid_cfg_bShowDebugInfo = {
    0xf3b1b50f, 0xc81a, 0x42f3, {0xbf, 0x10, 0xc1, 0xb1, 0x12, 0xe9, 0x83, 0x78}
}; // {F3B1B50F-C81A-42F3-BF10-C1B112E98378}
static constexpr GUID guid_cfg_bShowSongTitle = {
    0x7e23ca07, 0x6023, 0x47d7, {0xae, 0x0, 0xbe, 0xc7, 0x6d, 0xc3, 0x43, 0xcb}
}; // {7E23CA07-6023-47D7-AE00-BEC76DC343CB}
static constexpr GUID guid_cfg_bShowSongTime = {
    0x26f61e81, 0x6fd6, 0x4edb, {0x81, 0x39, 0x4a, 0x96, 0xef, 0x51, 0x51, 0xf0}
}; // {26F61E81-6FD6-4EDB-8139-4A96EF5151F0}
static constexpr GUID guid_cfg_bShowSongLen = {
    0x8b9b394a, 0xdb91, 0x4f29, {0xbd, 0xaf, 0x3e, 0xaf, 0x63, 0xed, 0x9b, 0x5b}
}; // {8B9B394A-DB91-4F29-BDAF-3EAF63ED9B5B}
static constexpr GUID guid_cfg_bShowShaderHelp = {
    0xa259a73, 0x17e2, 0x4ecd, {0x86, 0x2a, 0x19, 0xe9, 0x65, 0xec, 0xcf, 0xd7}
}; // {0A259A73-17E2-4ECD-862A-19E965ECCFD7}
static constexpr GUID guid_cfg_bPresetLockedByCode = {
    0x32be30f, 0x5320, 0x429c, {0xb0, 0x7e, 0xf8, 0x9f, 0xe2, 0x9a, 0x92, 0xc9}
}; // {032BE30F-5320-429C-B07E-F89FE29A92C9}
static constexpr GUID guid_cfg_bEnableDownmix = {
    0x46e50b73, 0xb359, 0x404b, {0xab, 0xf4, 0x8, 0x7f, 0x4, 0x9f, 0x2, 0xf2}
}; // {46E50B73-B359-404B-ABF4-087F049F02F2}
static constexpr GUID guid_cfg_bShowAlbum = {
    0x2caa50c, 0x6910, 0x42c6, {0x9a, 0xff, 0xa5, 0xa5, 0x52, 0xb5, 0xd2, 0x6b}
}; // {02CAA50C-6910-42C6-9AFF-A5A552B5D26B}

// Preferences derived from other settings or hidden.
static constexpr GUID guid_cfg_bTexSizeWasAutoPow2 = {
    0xb85e5868, 0xde29, 0x40a1, {0x84, 0x14, 0x62, 0x2e, 0xb6, 0x82, 0xbc, 0xff}
}; // {B85E5868-DE29-40A1-8414-622EB682BCFF}
static constexpr GUID guid_cfg_bTexSizeWasAutoExact = {
    0x4f23b406, 0x291, 0x4e0e, {0xb9, 0x8a, 0xf0, 0xd3, 0x12, 0x30, 0x53, 0xed}
}; // {4F23B406-0291-4E0E-B98A-F0D3123053ED}
static constexpr GUID guid_cfg_nTexSizeY = {
    0x2935ac10, 0x37ca, 0x4806, {0x9d, 0x52, 0xaf, 0x7a, 0x3d, 0x5b, 0x68, 0xe6}
}; // {2935AC10-37CA-4806-9D52-AF7A3D5B68E6}
static constexpr GUID guid_cfg_nGridY = {
    0xaf0caf32, 0x1e7d, 0x49b1, {0xb1, 0xd3, 0x51, 0xb0, 0xbc, 0x5f, 0xf7, 0xcb}
}; // {AF0CAF32-1E7D-49B1-B1D3-51B0BC5FF7CB}
static constexpr GUID guid_cfg_nBackBufferFormat = {
    0x80def434, 0xa974, 0x455e, {0x9b, 0xfd, 0x36, 0xb9, 0x48, 0x32, 0x8b, 0xf8}
}; // {80DEF434-A974-455E-9BFD-36B948328BF8}
static constexpr GUID guid_cfg_nDepthBufferFormat = {
    0xa82c7618, 0x9248, 0x4891, {0xa8, 0x52, 0x2a, 0x27, 0xe2, 0xad, 0x63, 0xc1}
}; // {A82C7618-9248-4891-A852-2A27E2AD63C1}
static constexpr GUID guid_cfg_nBackBufferCount = {
    0xb8f48320, 0xa275, 0x4872, {0x93, 0xe3, 0x88, 0x2, 0x37, 0xfc, 0x5e, 0x3b}
}; // {B8F48320-A275-4872-93E3-880237FC5E3B}
static constexpr GUID guid_cfg_nMinFeatureLevel = {
    0x1320978f, 0x4cad, 0x4700, {0x86, 0xa9, 0xf6, 0xa, 0xf0, 0xba, 0x82, 0x58}
}; // {1320978F-4CAD-4700-86A9-F60AF0BA8258}
static constexpr GUID guid_cfg_bEnableHDR = {
    0x7b10342f, 0x826f, 0x4e24, {0x80, 0xef, 0x61, 0x46, 0x94, 0x30, 0xba, 0x7c}
}; // {7B10342F-826F-4E24-80EF-61469430BA7C}

// Defaults
// `milk2.ini` defaults
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
static constexpr bool default_bShowAlbum = false;
static constexpr bool default_bEnableHDR = false;
static constexpr int default_nBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
static constexpr int default_nDepthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
static constexpr int default_nBackBufferCount = 2;
static constexpr int default_nMinFeatureLevel = D3D_FEATURE_LEVEL_9_1;
static constexpr UINT default_max_fps_fs = 30;
static constexpr bool default_allow_page_tearing_fs = false;
static constexpr const char* default_szTitleFormat = "%title%";
static constexpr const char* default_szArtworkFormat = "";
static constexpr bool default_bSkipCompShader = false;
static constexpr auto _stFonts = F<NUM_BASIC_FONTS + NUM_EXTRA_FONTS>();
static constexpr const td_fontinfo* default_stFontInfo = _stFonts.fontinfo;

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
} // namespace

class milk2_preferences_page : public preferences_page_instance, public CDialogImpl<milk2_preferences_page>
{
  public:
    milk2_preferences_page(preferences_page_callback::ptr callback) : m_callback(callback), m_bMsgHandled(TRUE) {}

    enum milk2_dialog_id
    {
        IDD = IDD_PREFS
    };

    uint32_t get_state();
    HWND get_wnd() { return *this; }
    void apply();
    void reset();

    // clang-format off
    BEGIN_MSG_MAP_EX(milk2_preferences_page)
#ifdef _DEBUG
        char buf[32];
        strncpy_s(buf, 32, core_api::get_my_file_name(), 32);
        strncat_s(buf, "_prefs: ", 32);
        OutputDebugMessage(buf, get_wnd(), uMsg, wParam, lParam);
#endif
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_NOTIFY(OnNotify)
        MSG_WM_HSCROLL(OnHScroll)
        MSG_WM_CLOSE(OnClose)
        MSG_WM_DESTROY(OnDestroy)
        COMMAND_HANDLER_EX(IDC_CB_SCROLLON3, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_SCROLLON4, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_NOWARN3, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_NORATING2, BN_CLICKED, OnButtonClick)
        COMMAND_HANDLER_EX(IDC_CB_PRESS_F1_MSG, BN_CLICKED, OnButtonClick)
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
        COMMAND_HANDLER_EX(IDC_ARTWORK_FORMAT, EN_CHANGE, OnEditNotification)
        COMMAND_HANDLER_EX(ID_SPRITE, BN_CLICKED, OnButtonPushed)
        COMMAND_HANDLER_EX(ID_MSG, BN_CLICKED, OnButtonPushed)
        COMMAND_HANDLER_EX(ID_FONTS, BN_CLICKED, OnButtonPushed)
    END_MSG_MAP()
    // clang-format on

    BOOL PluginShellFontDialogProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static void ScootControl(HWND hwnd, int ctrl_id, int dx, int dy);

  private:
    BOOL OnInitDialog(CWindow, LPARAM);
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
    void OnClose();
    void OnDestroy();
    void OnButtonPushed(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnEditNotification(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnButtonClick(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnComboChange(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
    void AutoHideGamma16();
    bool HasChanged() const;
    void OnChanged();

    inline void AddItem(HWND ctrl, LPWSTR buffer, UINT id, DWORD itemdata);
    inline void AddItem(HWND ctrl, LPCWSTR text, DWORD itemdata);
    inline void SelectItemByPos(HWND ctrl, int pos);
    int SelectItemByValue(HWND ctrl, LRESULT value);
    int64_t ReadCBValue(DWORD ctrl_id) const;
    bool IsComboDiff(DWORD ctrl_id, int64_t previous) const;
    wchar_t* FormImageCacheSizeString(LPCWSTR itemStr, const UINT sizeID);
    void UpdateMaxFps(int screenmode) const;
    void SaveMaxFps(int screenmode) const;
    void OpenToEdit(LPWSTR szDefault, LPCWSTR szFilename);
    void InitFontI(td_fontinfo* fi, DWORD ctrl1, DWORD ctrl2, DWORD bold_id, DWORD ital_id, DWORD aa_id, HWND hdlg, DWORD ctrl4, wchar_t* szFontName);
    void SaveFontI(td_fontinfo* fi, DWORD ctrl1, DWORD ctrl2, DWORD bold_id, DWORD ital_id, DWORD aa_id, HWND hdlg);

    const preferences_page_callback::ptr m_callback;

    CToolTipCtrl m_tooltips;

    fb2k::CDarkModeHooks m_dark;
};

class preferences_page_milk2 : public preferences_page_impl<milk2_preferences_page>
{
  public:
    const char* get_name() { return "MilkDrop"; }
    GUID get_guid() { return guid_milk2_preferences; }
    GUID get_parent_guid() { return guid_visualisations; }
    bool get_help_url(pfc::string_base& p_out)
    {
        p_out.reset();
        p_out << APPLICATION_DOCUMENTATION_URL;
        //"http://help.foobar2000.org/" << core_version_info::g_get_version_string() << "/" << "preferences" << "/" << pfc::print_guid(get_guid()) << "/" << get_name();
        return true;
    }
};

class FontDlg : public CDialogImpl<FontDlg>
{
  public:
    enum font_dialog_id
    {
        IDD = IDD_FONTDIALOG
    };

    static INT_PTR CALLBACK StartDialogProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_INITDIALOG && lParam > 0 && ::GetWindowLongPtr(hdlg, GWLP_USERDATA) == 0)
            ::SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)lParam);

        milk2_preferences_page* p = reinterpret_cast<milk2_preferences_page*>(::GetWindowLongPtr(hdlg, GWLP_USERDATA));

        if (p)
            return p->PluginShellFontDialogProc(hdlg, msg, wParam, lParam);
        else
            return FALSE;
    }

    BEGIN_MSG_MAP(FontDlg)
    END_MSG_MAP()
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

    static void resolve_profile();
    static void initialize_paths();

    // `milk2.ini`
    plugin_config settings{};

  private:
    uint32_t m_sentinel = MAKEFOURCC('M', 'I', 'L', 'K'); //('K' << 24 | 'L' << 16 | 'I' << 8 | 'M');
    uint32_t m_version = 3;

    void update_fonts();
    void update_paths();
};