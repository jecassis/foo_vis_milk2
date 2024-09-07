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

#ifndef __NULLSOFT_DX_PLUGIN_SUPPORT_H__
#define __NULLSOFT_DX_PLUGIN_SUPPORT_H__

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include "d3d11shim.h"

using namespace DirectX;

void PrepareFor3DDrawing(D3D11Shim* pDevice, int viewport_width, int viewport_height,
                         float fov_in_degrees, float near_clip, float far_clip,
                         XMVECTOR* pvEye, XMVECTOR* pvLookat, XMVECTOR* pvUp);
void PrepareFor2DDrawing(D3D11Shim* pDevice);
void MakeWorldMatrix(XMMATRIX* pOut, float xpos, float ypos, float zpos, float sx, float sy, float sz, float pitch, float yaw, float roll);
void MakeProjectionMatrix(XMMATRIX* pOut,
                          const float near_plane, // Distance to near clipping plane
                          const float far_plane, // Distance to far clipping plane
                          const float fov_horiz, // Horizontal field of view angle, in radians
                          const float fov_vert // Vertical field of view angle, in radians
);

void GetWinampSongTitle(HWND hWndWinamp, wchar_t* szSongTitle, size_t nSize);
void GetWinampSongPosAsText(HWND hWndWinamp, wchar_t* szSongPos);
void GetWinampSongLenAsText(HWND hWndWinamp, wchar_t* szSongLen);
float GetWinampSongPos(HWND hWndWinamp); // returns answer in seconds
float GetWinampSongLen(HWND hWndWinamp); // returns answer in seconds

int GetDX11TexFormatBitsPerPixel(DXGI_FORMAT fmt);

// Define vertex formats.
// Note: Layout must match the vertex declarations in "d3d11shim.cpp"!
typedef struct _MDVERTEX
{
    float x, y, z; // screen position + Z-buffer depth
    float r, g, b, a; // diffuse color
    float tu, tv; // DYNAMIC: texture coordinates for texture #0
    float tu_orig, tv_orig; // STATIC
    float rad, ang; // STATIC: texture coordinates for texture #2
} MDVERTEX, *LPMDVERTEX;

typedef struct _WFVERTEX
{
    float x, y, z;
    float r, g, b, a; // diffuse color, also acts as filler to 16 bytes
} WFVERTEX, *LPWFVERTEX;

typedef struct _SPRITEVERTEX
{
    float x, y, z; // screen position + Z-buffer depth
    float r, g, b, a; // diffuse color, also acts as filler to 16 bytes
    float tu, tv; // texture coordinates for texture #0
} SPRITEVERTEX, *LPSPRITEVERTEX;

typedef struct _HELPVERTEX
{
    float x, y, z; // screen position + Z-buffer depth
    float r, g, b, a; // diffuse color, also acts as filler to 16 bytes
    float tu, tv; // texture coordinates for texture #0
} HELPVERTEX, *LPHELPVERTEX;

typedef struct _SIMPLEVERTEX
{
    float x, y, z; // screen position + Z-buffer depth
    DWORD Diffuse;  // diffuse color, also acts as filler to 16 bytes
} SIMPLEVERTEX, *LPSIMPLEVERTEX;

#ifdef PROFILING
#define PROFILE_BEGIN \
    LARGE_INTEGER tx, freq, ty; \
    QueryPerformanceCounter(&tx); \
    QueryPerformanceFrequency(&freq);
#define PROFILE_END(s) { \
    QueryPerformanceCounter(&ty); \
    float dt = static_cast<float>(static_cast<double>(ty.QuadPart - tx.QuadPart) / static_cast<double>(freq.QuadPart)); \
    char buf[256]; \
    sprintf_s(buf, "  %s = %.1f ms\n", s, dt * 1000); \
    OutputDebugString(buf); \
    tx = ty; \
}
#else
#define PROFILE_BEGIN
#define PROFILE_END(s)
#endif

#endif