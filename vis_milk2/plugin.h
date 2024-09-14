/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
  Copyright 2021-2024 Jimmy Cassis
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of Nullsoft nor the names of its contributors may be used to
      endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __NULLSOFT_DX_PLUGIN_H__
#define __NULLSOFT_DX_PLUGIN_H__

#include <list>
#include <vector>
#include "md_defines.h"
#include "pluginshell.h"
#include "support.h"
#include "texmgr.h"
#include "state.h"
#include "menu.h"
#include "constanttable.h"
#ifdef _FOOBAR
#include <foo_vis_milk2/settings.h>
#include <foo_vis_milk2/supertext.h>
#endif

int warand();

#define FRAND ((warand() % 7381) / 7380.0f)

// clang-format off
typedef enum { TEX_DISK, TEX_VS, TEX_BLUR0, TEX_BLUR1, TEX_BLUR2, TEX_BLUR3, TEX_BLUR4, TEX_BLUR5, TEX_BLUR6, TEX_BLUR_LAST } tex_code;
typedef enum { UI_REGULAR, UI_MENU, UI_LOAD, UI_LOAD_DEL, UI_LOAD_RENAME, UI_SAVEAS, UI_SAVE_OVERWRITE, UI_EDIT_MENU_STRING, UI_CHANGEDIR, UI_IMPORT_WAVE, UI_EXPORT_WAVE, UI_IMPORT_SHAPE, UI_EXPORT_SHAPE, UI_UPGRADE_PIXEL_SHADER, UI_MASHUP } ui_mode;
typedef struct { float rad; float ang; float a; float c; } td_vertinfo; //blending: mix = std::max(0, std::min(1, a * t + c));
// clang-format on

typedef struct
{
    float imm[3];      // bass, mids, treble (absolute)
    float imm_rel[3];  // bass, mids, treble (relative to song; 1=avg, 0.9~below, 1.1~above)
    float avg[3];      // bass, mids, treble (absolute)
    float avg_rel[3];  // bass, mids, treble (relative to song; 1=avg, 0.9~below, 1.1~above)
    float long_avg[3]; // bass, mids, treble (absolute)
    std::array<std::array<float, NUM_AUDIO_BUFFER_SAMPLES>, 2> fWave;
    std::vector<float> fSpecLeft; //[NUM_FFT_SAMPLES]
} td_mdsounddata;

typedef struct
{
    int bActive;
    int bFilterBadChars;  // if true, it will filter out any characters that don't belong in a filename, plus the & symbol (because it doesn't display properly with DrawText)
    int bDisplayAsCode;   // if true, semicolons will be followed by a newline, for display
    size_t nMaxLen;       // cannot be more than 511
    size_t nCursorPos;
    int nSelAnchorPos;    // -1 if no selection made
    int bOvertypeMode;
    wchar_t szText[48000];
    wchar_t szPrompt[512];
    wchar_t szToolTip[512];
    char szClipboardA[48000];
    wchar_t szClipboard[48000];
} td_waitstr;

typedef struct
{
    int bBold;
    int bItal;
    wchar_t szFace[128];
    int nColorR; // 0..255
    int nColorG; // 0..255
    int nColorB; // 0..255
} td_custom_msg_font;

enum
{
    MD2_PS_NONE = 0,
    MD2_PS_2_0 = 2,
    MD2_PS_2_X = 3,
    MD2_PS_3_0 = 4,
    MD2_PS_4_0 = 5,
    MD2_PS_5_0 = 6,
};

typedef struct
{
    int nFont;
    float fSize; // 0..100
    float x;
    float y;
    float randx;
    float randy;
    float growth;
    float fTime; // total time to display the message, in seconds
    float fFade; // % (0..1) of the time that is spent fading in

    // Overrides.
    int bOverrideBold;
    int bOverrideItal;
    int bOverrideFace;
    int bOverrideColorR;
    int bOverrideColorG;
    int bOverrideColorB;
    int nColorR; // 0..255
    int nColorG; // 0..255
    int nColorB; // 0..255
    int nRandR;
    int nRandG;
    int nRandB;
    int bBold;
    int bItal;
    wchar_t szFace[128];

    wchar_t szText[256];
} td_custom_msg;

typedef struct
{
    int bRedrawSuperText; // true if it needs redraw
    int bIsSongTitle;     // false for custom message, true for song title
    wchar_t szText[256];
    wchar_t nFontFace[128];
    int bBold;
    int bItal;
    float fX;
    float fY;
    float fFontSize;   // [0..100] for custom messages, [0..4] for song titles
    float fGrowth;     // applies to custom messages only
    int nFontSizeUsed; // height in pixels
    float fStartTime;
    float fDuration;
    float fFadeTime; // applies to custom messages only; song title fade times are handled specially
    int nColorR;
    int nColorG;
    int nColorB;
} td_supertext;

typedef struct
{
    wchar_t texname[256]; // ~filename, but without path or extension!
    ID3D11Resource* texptr;
    int w, h, d;
    //D3DXHANDLE texsize_param;
    bool bEvictable;
    int nAge;         // only valid if bEvictable is true
    int nSizeInBytes; // only valid if bEvictable is true
} TexInfo;

typedef struct
{
    std::wstring texname; // just for ref
    LPCSTR texsize_param;
    int w, h;
} TexSizeParamInfo;

typedef struct
{
    ID3D11Resource* texptr;
    bool bBilinear;
    bool bWrap;
    UINT bindPoint;
} SamplerInfo;

enum ErrorCategory
{
    ERR_ALL = 0,
    ERR_INIT = 1, // specifically, loading a preset
    ERR_PRESET = 2, // specifically, loading a preset
    ERR_MISC = 3,
    ERR_NOTIFY = 4, // a simple notification - not an error at all. ("shuffle is now ON." etc.)
                    // note: each NOTIFY msg clears all the old NOTIFY messages!
    ERR_SCANNING_PRESETS = 5,
};

typedef struct
{
    std::wstring msg;
    bool bold; // true -> red background, false -> black background
    bool printed;
    float birthTime;
    float expireTime;
    TextElement text;
    ErrorCategory category;
} ErrorMsg;
typedef std::list<ErrorMsg> ErrorMsgList;

typedef struct
{
    wchar_t error[1024];
} ErrorCopy;

class CShaderParams
{
  public:
    // float4 handles:
    LPCSTR rand_frame;
    LPCSTR rand_preset;
    LPCSTR const_handles[24];
    LPCSTR q_const_handles[(NUM_Q_VAR + 3) / 4];
    LPCSTR rot_mat[24];

    typedef std::vector<TexSizeParamInfo> TexSizeParamInfoList;
    TexSizeParamInfoList texsize_params;

    // sampler stages for various PS texture bindings:
    //int texbind_vs;
    //int texbind_disk[32];
    //int texbind_voronoi;
    //...
    SamplerInfo m_texture_bindings[16]; // an entry for each sampler slot.  These are ALIASES - DO NOT DELETE.
    tex_code m_texcode[16];             // if ==TEX_VS, forget the pointer - texture bound @ that stage is the double-buffered VS.

    void Clear();
    void CacheParams(CConstantTable* pCT, bool bHardErrors);
    void OnTextureEvict(ID3D11Resource* texptr);
    CShaderParams();
    ~CShaderParams();
};
typedef std::vector<CShaderParams*> CShaderParamsList;

class VShaderInfo
{
  public:
    ID3D11VertexShader* ptr;
    CConstantTable* CT;
    CShaderParams params;
    VShaderInfo() { ptr = NULL; CT = NULL; params.Clear(); }
    ~VShaderInfo() { Clear(); }
    void Clear();
};

class PShaderInfo
{
  public:
    ID3D11PixelShader* ptr;
    CConstantTable* CT;
    CShaderParams params;
    PShaderInfo() { ptr = NULL; CT = NULL; params.Clear(); }
    ~PShaderInfo() { Clear(); }
    void Clear();
};

typedef struct
{
    VShaderInfo vs;
    PShaderInfo ps;
} ShaderPairInfo;

typedef struct
{
    PShaderInfo warp;
    PShaderInfo comp;
} PShaderSet;

typedef struct
{
    VShaderInfo warp;
    VShaderInfo comp;
} VShaderSet;

typedef struct
{
    std::wstring szFilename; // without path
    float fRatingThis;
    float fRatingCum;
} PresetInfo;
typedef std::vector<PresetInfo> PresetList;

class CPlugin : public CPluginShell
{
  public:
    //====[ 1. Members added for MilkDrop ]================================================
    /// CONFIGURATION PANEL SETTINGS (TAB #2)
    float m_fBlendTimeAuto;          // blend time when preset auto-switches
    float m_fBlendTimeUser;          // blend time when user loads a new preset
    float m_fTimeBetweenPresets;     // <- this is in addition to m_fBlendTimeAuto
    float m_fTimeBetweenPresetsRand; // <- this is in addition to m_fTimeBetweenPresets
    bool m_bSequentialPresetOrder;
    bool m_bHardCutsDisabled;
    float m_fHardCutLoudnessThresh;
    float m_fHardCutHalflife;
    float m_fHardCutThresh;
    //int m_nWidth;
    //int m_nHeight;
    //int m_nDispBits;
    int m_nCanvasStretch; // 0=Auto, 100=None, 125 = 1.25X, 133, 150, 167, 200, 300, 400 (4X).
    int m_nTexSizeX;      // -1 = exact match to screen; -2 = nearest power of 2.
    int m_nTexSizeY;
    float m_fAspectX;
    float m_fAspectY;
    float m_fInvAspectX;
    float m_fInvAspectY;
    int m_nTexBitsPerCh;
    int m_nGridX;
    int m_nGridY;

    bool m_bShowPressF1ForHelp;
    bool m_bShowMenuToolTips;
    int m_n16BitGamma;
    bool m_bAutoGamma;
    //int m_nFpsLimit;
    //int m_cLeftEye3DColor[3];
    //int m_cRightEye3DColor[3];
    bool m_bEnableRating;
    bool m_bSongTitleAnims;
    float m_fSongTitleAnimDuration;
    float m_fTimeBetweenRandomSongTitles;
    float m_fTimeBetweenRandomCustomMsgs;
    int m_nSongTitlesSpawned;
    int m_nCustMsgsSpawned;

    //bool m_bAlways3D;
    //float m_fStereoSep;
    //bool m_bAlwaysOnTop;
    //bool m_bFixSlowText;
    //bool m_bWarningsDisabled; // message boxes
    bool m_bWarningsDisabled2; // warnings/errors in upper-right corner (m_szUserMessage)
    //bool m_bAnisotropicFiltering;
    bool m_bPresetLockOnAtStartup;
    bool m_bPreventScollLockHandling;
    int m_nMaxPSVersion_ConfigPanel; // -1 = auto, 0 = disable shaders, 2 = ps_2_0, 3 = ps_2_x, 4 = ps_3_0, 5 = ps_4_0
    int m_nMaxPSVersion_DX; // 0 = no shader support, 2 = ps_2_0, 3 = ps_2_x, 4 = ps_3_0, 5 = ps_4_0
    int m_nMaxPSVersion; // the minimum of the other two
    int m_nMaxImages;
    int m_nMaxBytes;

    // PIXEL SHADERS
    UINT m_dwShaderFlags; // Shader compilation/linking flags
    ID3DBlob* m_pShaderCompileErrors;
    VShaderSet m_fallbackShaders_vs; // *these are the only vertex shaders used for the whole application*
    PShaderSet m_fallbackShaders_ps; // these are just used when the preset's pixel shaders fail to compile
    PShaderSet m_shaders;            // includes shader pointers and constant tables for warp & comp shaders, for current preset
    PShaderSet m_OldShaders;         // includes shader pointers and constant tables for warp & comp shaders, for previous preset
    PShaderSet m_NewShaders;         // includes shader pointers and constant tables for warp & comp shaders, for upcoming preset
    ShaderPairInfo m_BlurShaders[2];
    bool m_bWarpShaderLock;
    bool m_bCompShaderLock;

#define SHADER_WARP 0
#define SHADER_COMP 1
#define SHADER_BLUR 2
#define SHADER_OTHER 3
    bool LoadShaderFromMemory(const char* szOrigShaderText, const char* szFn, const char* szProfile, CConstantTable** ppConstTable,
                              void** ppShader, const int shaderType, const bool bHardErrors);
    bool RecompileVShader(const char* szShadersText, VShaderInfo* si, int shaderType, bool bHardErrors);
    bool RecompilePShader(const char* szShadersText, PShaderInfo* si, int shaderType, bool bHardErrors, int PSVersion);
    bool EvictSomeTexture();
    typedef std::vector<TexInfo> TexInfoList;
    TexInfoList m_textures;
    bool m_bNeedRescanTexturesDir;

    // Input layouts.
    //ID3D11InputLayout* m_pMilkDropVertDecl;
    //ID3D11InputLayout* m_pWfVertDecl;
    //ID3D11InputLayout* m_pSpriteVertDecl;

    XMFLOAT4 m_rand_frame; // 4 random floats (0..1); randomized once per frame; fed to pixel shaders.

    // ADDITIONAL RUNTIME SETTINGS.
    float m_prev_time;
    bool m_bTexSizeWasAutoPow2;
    bool m_bTexSizeWasAutoExact;
    bool m_bPresetLockedByUser;
    bool m_bPresetLockedByCode;
    float m_fAnimTime;
    float m_fStartTime;
    float m_fPresetStartTime;
    float m_fNextPresetTime;
    float m_fSnapPoint;
    CState* m_pState;    // points to current CState
    CState* m_pOldState; // points to previous CState
    CState* m_pNewState; // points to upcoming CState; not yet blending to it because still compiling the shaders for it!
    int m_nLoadingPreset;
    wchar_t m_szLoadingPreset[MAX_PATH];
    float m_fLoadingPresetBlendTime;
    int m_nPresetsLoadedTotal;    // important for texture eviction age-tracking...
    CState m_state_DO_NOT_USE[3]; // do not use; use pState and pOldState instead.
    ui_mode m_UI_mode;            // can be UI_REGULAR, UI_LOAD, UI_SAVEHOW, or UI_SAVEAS

#define MASH_SLOTS 5
#define MASH_APPLY_DELAY_FRAMES 1
    WPARAM m_nMashSlot; // 0..MASH_SLOTS-1
    //char m_szMashDir[MASH_SLOTS][MAX_PATH];
    int m_nMashPreset[MASH_SLOTS];
    int m_nLastMashChangeFrame[MASH_SLOTS];

    //td_playlist_entry *m_szPlaylist; // array of 128-char strings
    //int m_nPlaylistCurPos;
    //int m_nPlaylistLength;
    //int m_nTrackPlaying;
    //int m_nSongPosMS;
    //int m_nSongLenMS;
    bool m_bUserPagedUp;
    bool m_bUserPagedDown;
    float m_fMotionVectorsTempDx;
    float m_fMotionVectorsTempDy;

    td_waitstr m_waitstring;
    void WaitString_NukeSelection();
    void WaitString_Cut();
    void WaitString_Copy();
    void WaitString_Paste();
    void WaitString_SeekLeftWord();
    void WaitString_SeekRightWord();
    size_t WaitString_GetCursorColumn() const;
    int WaitString_GetLineLength() const;
    void WaitString_SeekUpOneLine();
    void WaitString_SeekDownOneLine();

    int m_nPresets;          // number of entries in the file listing. Includes directories and then files, sorted alphabetically.
    int m_nDirs;             // number of presets that are actually directories. Always between 0 and m_nPresets.
    int m_nPresetListCurPos; // index of the currently-HIGHLIGHTED preset (the user must press Enter on it to select it).
    int m_nCurrentPreset;    // index of the currently-RUNNING preset.
                             //   Note that this is NOT the same as the currently-highlighted preset! (that's m_nPresetListCurPos)
                             //   Be careful - this can be -1 if the user changed dir. & a new preset hasn't been loaded yet.
    wchar_t m_szCurrentPresetFile[512]; // w/o path.  this is always valid (unless no presets were found)
    PresetList m_presets;
    void UpdatePresetList(bool bBackground = false, bool bForce = false, bool bTryReselectCurrentPreset = true);
    wchar_t m_szUpdatePresetMask[MAX_PATH];
    volatile bool m_bPresetListReady;
    //void UpdatePresetRatings();
    //int m_nRatingReadProgress;  // equals 'm_nPresets' if all ratings are read in & ready to go; -1 if uninitialized; otherwise, it's still reading them in, and range is: [0 .. m_nPresets-1]
    bool m_bInitialPresetSelected;

    // PRESET HISTORY
#define PRESET_HIST_LEN (64 + 2) // this is 2 more than the DESIRED number to be able to go back
    std::wstring m_presetHistory[PRESET_HIST_LEN]; // circular
    int m_presetHistoryPos;
    int m_presetHistoryBackFence;
    int m_presetHistoryFwdFence;
    void PrevPreset(float fBlendTime);
    void NextPreset(float fBlendTime); // if not retracing our former steps, it will choose a random one.
    void OnFinishedLoadingPreset();

    FFT mdfft{NUM_AUDIO_BUFFER_SAMPLES, NUM_FFT_SAMPLES, true, 1.0f};
    td_mdsounddata mdsound;

    // Displaying text to user.
    bool m_bShowFPS;
    bool m_bShowRating;
    bool m_bShowPresetInfo;
    bool m_bShowDebugInfo;
    bool m_bShowSongTitle;
    bool m_bShowSongTime;
    bool m_bShowSongLen;
    float m_fShowRatingUntilThisTime;
    //float m_fShowUserMessageUntilThisTime;
    //char m_szUserMessage[512];
    //bool m_bUserMessageIsError;

    ErrorMsgList m_errors;
    void AddError(wchar_t* szMsg, float fDuration, ErrorCategory category = ERR_ALL, bool bBold = true);
    void ClearErrors(int category = ERR_ALL);

    wchar_t m_szSongTitle[256];
    wchar_t m_szSongTitlePrev[256];

    // Stuff for the menu system.
    CMilkMenu* m_pCurMenu; // should always be valid!
    CMilkMenu m_menuPreset;
    CMilkMenu m_menuWave;
    CMilkMenu m_menuAugment;
    CMilkMenu m_menuCustomWave;
    CMilkMenu m_menuCustomShape;
    CMilkMenu m_menuMotion;
    CMilkMenu m_menuPost;
    CMilkMenu m_menuWavecode[MAX_CUSTOM_WAVES];
    CMilkMenu m_menuShapecode[MAX_CUSTOM_SHAPES];
    bool m_bShowShaderHelp;

    wchar_t m_szMilkdrop2Path[MAX_PATH]; // ends in a backslash
    wchar_t m_szMsgIniFile[MAX_PATH];
    wchar_t m_szImgIniFile[MAX_PATH];
    wchar_t m_szPresetDir[MAX_PATH];

    float m_fRandStart[4];

    // DirectX.
    ID3D11Texture2D* m_lpVS[2];
#define NUM_BLUR_TEX 6
#if (NUM_BLUR_TEX > 0)
    ID3D11Texture2D* m_lpBlur[NUM_BLUR_TEX]; // each is successively 1/2 size of previous one
    int m_nBlurTexW[NUM_BLUR_TEX];
    int m_nBlurTexH[NUM_BLUR_TEX];
#endif
    int m_nHighestBlurTexUsedThisFrame;
    ID3D11Texture2D* m_lpDDSTitle;
    int m_nTitleTexSizeX, m_nTitleTexSizeY;
    MDVERTEX* m_verts;
    MDVERTEX* m_verts_temp;
    td_vertinfo* m_vertinfo;
    int* m_indices_strip;
    int* m_indices_list;

    // Final composite grid.
#define FCGSX 32 // final composite grid size - number vertices - should be EVEN.
#define FCGSY 24 // final composite grid size - number vertices - should be EVEN.
                 // number of grid *cells* is two less,
                 // since we have redundant verts along the center line in X and Y (...for clean 'ang' interp)
    MDVERTEX m_comp_verts[FCGSX * FCGSY];
    int m_comp_indices[(FCGSX - 2) * (FCGSY - 2) * 2 * 3];

    bool m_bHasFocus;
    bool m_bHadFocus;
    bool m_bOrigScrollLockState;
    //bool m_bMilkdropScrollLockState; // saved when focus is lost; restored when focus is regained

    int m_nNumericInputMode; // NUMERIC_INPUT_MODE_CUST_MSG, NUMERIC_INPUT_MODE_SPRITE
    int m_nNumericInputNum;
    int m_nNumericInputDigits;
    td_custom_msg_font m_customMessageFont[MAX_CUSTOM_MESSAGE_FONTS];
    td_custom_msg m_customMessage[MAX_CUSTOM_MESSAGES];

    texmgr m_texmgr; // for user sprites

    td_supertext m_supertext; // **contains info about current Song Title or Custom Message.**
#ifdef _SUPERTEXT
    std::unique_ptr<SuperText> m_superTitle;
#endif

    ID3D11Texture2D* m_tracer_tex;

    int m_nFramesSinceResize;

    char m_szShaderIncludeText[32768];      // note: this still has char 13's and 10's in it - it's never edited on screen or loaded/saved with a preset.
    size_t m_nShaderIncludeTextLen;         // number of characters, not including the final NULL.
    char m_szDefaultWarpVShaderText[32768]; // THIS HAS CHAR 13/10 CONVERTED TO LINEFEED_CONTROL_CHAR
    char m_szDefaultWarpPShaderText[32768]; // THIS HAS CHAR 13/10 CONVERTED TO LINEFEED_CONTROL_CHAR
    char m_szDefaultCompVShaderText[32768]; // THIS HAS CHAR 13/10 CONVERTED TO LINEFEED_CONTROL_CHAR
    char m_szDefaultCompPShaderText[32768]; // THIS HAS CHAR 13/10 CONVERTED TO LINEFEED_CONTROL_CHAR
    char m_szBlurVS[32768];
    char m_szBlurPSX[32768];
    char m_szBlurPSY[32768];
    //const char* GetDefaultWarpShadersText() { return m_szDefaultWarpShaderText; }
    //const char* GetDefaultCompShadersText() { return m_szDefaultCompShaderText; }
    void GenWarpPShaderText(char* szShaderText, float decay, bool bWrap);
    void GenCompPShaderText(char* szShaderText, float brightness, float ve_alpha, float ve_zoom, int ve_orient, float hue_shader, bool bBrighten, bool bDarken, bool bSolarize, bool bInvert);

    //====[ 2. Methods added ]=====================================================================================
    void RenderFrame(int bRedraw);
    void DrawTooltip(wchar_t* str, int xR, int yB);
    void ClearTooltip();
    void ClearText();
    void RandomizeBlendPattern();
    void GenPlasma(int x0, int x1, int y0, int y1, float dt);
    void LoadPerFrameEvallibVars(CState* pState);
    void LoadCustomWavePerFrameEvallibVars(CState* pState, int i);
    void LoadCustomShapePerFrameEvallibVars(CState* pState, int i, int instance);
#ifndef _FOOBAR
    void WriteRealtimeConfig(); // called on Finish()
#else
    bool PanelSettings(plugin_config* settings);
#endif
    void LoadRandomPreset(float fBlendTime);
    void LoadPreset(const wchar_t* szPresetFilename, float fBlendTime);
    void LoadPresetTick();
    void SetPresetListPosition(std::wstring search);
    void FindValidPresetDir();
    wchar_t* GetPresetDir() const { return const_cast<wchar_t*>(m_szPresetDir); };
    td_fontinfo* GetFontInfo() const {return const_cast<td_fontinfo*>(m_fontinfo); };
    void SavePresetAs(wchar_t* szNewFile); // overwrites the file if it was already there.
    void DeletePresetFile(wchar_t* szDelFile);
    void RenamePresetFile(wchar_t* szOldFile, wchar_t* szNewFile);
    void SetCurrentPresetRating(float fNewRating);
    void SeekToPreset(wchar_t cStartChar);
    bool ReversePropagatePoint(float fx, float fy, float* fx2, float* fy2) const;
    void ClearGraphicsWindow(); // for windowed mode only
    void LaunchCustomMessage(int nMsgNum);
    void ReadCustomMessages();
    void LaunchSongTitleAnim();

    bool RenderStringToTitleTexture();
    void ShowSongTitleAnim(int w, int h, float fProgress);
    void DrawWave(float* fL, float* fR);
    void DrawCustomWaves();
    void DrawCustomShapes();
    void DrawSprites();
    void ComputeGridAlphaValues();
    //void WarpedBlit(); // note: 'bFlipAlpha' just flips the alpha blending in fixed-fn pipeline - not the values for culling tiles.
    void WarpedBlit_Shaders(int nPass, bool bAlphaBlend, bool bFlipAlpha, bool bCullTiles, bool bFlipCulling);
    void WarpedBlit_NoShaders(int nPass, bool bAlphaBlend, bool bFlipAlpha, bool bCullTiles, bool bFlipCulling);
    void ShowToUser_Shaders(int nPass, bool bAlphaBlend, bool bFlipAlpha, bool bCullTiles, bool bFlipCulling);
    void ShowToUser_NoShaders();
    void BlurPasses();
    void GetSafeBlurMinMax(CState* pState, float* blur_min, float* blur_max);
    void RunPerFrameEquations(int code);
    void DrawUserSprites();
    void MergeSortPresets(int left, int right);
    void BuildMenus();
    void SetMenusForPresetVersion(int WarpPSVersion, int CompPSVersion);
    bool LaunchSprite(int nSpriteNum, int nSlot, const std::wstring& filename = L"", const std::vector<uint8_t>& vec = std::vector<uint8_t>());
    void KillSprite(int iSlot);
    void DoCustomSoundAnalysis();
    void DrawMotionVectors();

    bool LoadShaders(PShaderSet* sh, CState* pState, bool bTick);
    void UvToMathSpace(float u, float v, float* rad, float* ang);
    void ApplyShaderParams(CShaderParams* p, CConstantTable* pCT, CState* pState);
    void RestoreShaderParams();
    bool AddNoiseTex(const wchar_t* szTexName, int size, int zoom_factor);
    bool AddNoiseVol(const wchar_t* szTexName, int size, int zoom_factor);

    //====[ 3. Virtual functions ]===========================================================================
    virtual void OverrideDefaults();
    virtual void MilkDropPreInitialize();
    virtual void MilkDropReadConfig();
    virtual void MilkDropWriteConfig();
    virtual int AllocateMilkDropNonDX11();
    virtual void CleanUpMilkDropNonDX11();
    virtual int AllocateMilkDropDX11();
    virtual void CleanUpMilkDropDX11(int final_cleanup);
    virtual void MilkDropRenderFrame(int redraw);
    virtual void MilkDropRenderUI(int* upper_left_corner_y, int* upper_right_corner_y, int* lower_left_corner_y, int* lower_right_corner_y, int xL, int xR);
    //virtual LRESULT MilkDropWindowProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);
    //virtual BOOL MilkDropConfigTabProc(int nPage, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual void OnAltK();
    virtual void DumpDebugMessage(const wchar_t* s);
    virtual void PopupMessage(int message_id, int title_id, bool dump = false);
    virtual void ConsoleMessage(const wchar_t* function_name, int message_id, int title_id);

    /*
    //====[ 4. Methods from base class ]===========================================================================
    // 'GET' METHODS
    // ------------------------------------------------------------
    int GetFrame();            // returns current frame number (starts at zero)
    float GetTime();           // returns current animation time (in seconds) (starts at zero) (updated once per frame)
    float GetFps();            // returns current estimate of framerate (frames per second)
    eScrMode GetScreenMode();  // returns WINDOWED, FULLSCREEN, FAKE_FULLSCREEN, or NOT_YET_KNOWN (if called before or during OverrideDefaults()).
    HWND GetWinampWindow();    // returns handle to Winamp main window
    HINSTANCE GetInstance();   // returns handle to the plugin DLL module; used for things like loading resources (dialogs, bitmaps, icons...) that are built into the plugin.
    char* GetPluginsDirPath(); // usually returns 'c:\\program files\\winamp\\plugins\\'
    char* GetConfigIniFile();  // usually returns 'c:\\program files\\winamp\\plugins\\something.ini' - filename is determined from identifiers in 'defines.h'

    // GET METHODS THAT ONLY WORK ONCE DIRECTX IS READY
    // ------------------------------------------------------------
    //  The following 'Get' methods are only available after DirectX has been initialized.
    //  If you call these from `OverrideDefaults()`, `MilkDropPreInitialize()`, or `MilkDropReadConfig()`,
    //  they will fail and return NULL (zero).
    // ------------------------------------------------------------
    HWND GetPluginWindow();          // returns handle to the plugin window.  NOT persistent; can change.
    int GetWidth();                  // returns width of plugin window interior, in pixels.
    int GetHeight();                 // returns height of plugin window interior, in pixels.
    DXGI_FORMAT GetBackBufFormat();  // returns the pixelformat of the back buffer (probably DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_B4G4R4A4_UNORM, or DXGI_FORMAT_UNKNOWN)
    DXGI_FORMAT GetBackBufZFormat(); // returns the pixelformat of the back buffer's Z buffer (probably DXGI_FORMAT_D16_UNORM, or DXGI_FORMAT_UNKNOWN)
    D3D11Shim* GetDevice();          // returns a pointer to the DirectX 11 Device. NOT persistent; can change.

    // FONTS & TEXT
    // ------------------------------------------------------------
    LPD3DXFONT GetFont(eFontIndex idx); // returns a handle to a D3DX font you can use to draw text on the screen
    int GetFontHeight(eFontIndex idx);  // returns the height of the font, in pixels

    // MISC
    // ------------------------------------------------------------
    td_soundinfo m_sound;           // a structure always containing the most recent sound analysis information; defined in pluginshell.h.
    void SuggestHowToFreeSomeMem(); // gives the user a 'smart' messagebox that suggests how they can free up some video memory.
    */

  private:
    TextElement m_presetName;
    TextElement m_presetRating;
    TextElement m_fpsDisplay;
    TextElement m_debugInfo;
    TextElement m_toolTip;
    TextElement m_songTitle;
    TextElement m_songStats;
    TextElement m_waitText[MAX_PRESETS_PER_PAGE];
    TextElement m_menuText[MAX_PRESETS_PER_PAGE / 2];
    TextElement m_loadPresetInstruction;
    TextElement m_loadPresetDir;
    TextElement m_loadPresetItem[MAX_PRESETS_PER_PAGE];
    TextElement m_ddsTitle;
};

#endif