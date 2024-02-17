# MilkDrop 2 Visualization Component for foobar2000

Port of Winamp's MilkDrop 2 visualization library from its original DirectX 9 version to use DirectX 11.1.
MilkDrop 2 takes you flying through visualizations of the soundwaves you're hearing, and uses beat detection to trigger myriad psychedelic effects, creating a rich visual journey through sound.

Prerequisites to build the `foo_vis_milk2.dll` component for foobar2000:

- [foobar2000 SDK](https://www.foobar2000.org/SDK): download the latest version and uncompress the contents in the `external/` folder.
- [NS-EEL2](https://github.com/justinfrankel/WDL/tree/main/WDL/eel2) (included in [WDL](https://www.cockos.com/wdl/)): the files required to build the DLL are included in this repository.
- [DirectXTK](https://github.com/Microsoft/DirectXTK): the files required to build the DLL are fetched via the NuGet package manager.
- [Windows Template Library (WTL)](https://wtl.sourceforge.io/): the files required to build the DLL are fetched via the NuGet package manager.
- [Visual Studio 2022](https://visualstudio.microsoft.com/vs/): open the [`foo_vis_milk2`](foo_vis_milk2.sln) solution, set `foo_vis_milk2` as the Startup Project, install WTL and DirectXTK as NuGet packages, select a configuration, and build the solution.

> Import the Visual Studio [installation configuration](.vsconfig) file to install required components such as the [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/), [Active Template Library (ATL)](https://learn.microsoft.com/en-us/cpp/mfc/mfc-and-atl) and [NuGet Package Manager](https://www.nuget.org/).

Refer to the [build pipeline](.github/workflows/build.yml) jobs for a step-by-step guide on how to build. _Only x86 and x64 Intel architecture platforms are tested._

See [CHANGELOG](CHANGELOG.md) for additional details.

## Features

- Uses DirectX 11.1 (Direct3D 11.1, DXGI 1.6, Direct2D 1.1, DirectWrite 1.1) for rendering.
- Supports the Default User Interface (Default UI) only.
- Configurable through foobar2000 preferences instead of `.ini` files.
- Can build 32-bit and 64-bit x86 component configurations as well as ARM64 and ARM64EC.
- Built for foobar2000 2.0 and later with latest Windows 11 SDK (10.0.22621.0) and MSVC (v143).
- Updated all library dependencies to their latest available releases.
- Deprecated or insecure functions have been rewritten and most unused functionality removed.
- `vis_milk2` has been upgraded to use more modern C++ alongside the move to DirectX 11. 
- Tested on foobar2000 v2.1.2 (x86 32-bit and x86 64-bit) and Microsoft Windows 11 (Build 22621).
- In theory, Intel architecture versions work with Windows 8 or later and ARM architecture versions with Windows 10 or later; mainly dictated by the DXGI version required to support hybrid graphics, high DPI displays and HDR.

## Run Requirements and Installation

- Download [foobar2000](https://www.foobar2000.org/download) and install.
- Import `foo_vis_milk2.fbk2-component` into foobar2000 using the "*File / Preferences / Components / Install...*" menu item.
- Download and extract presets into this component's directory of foobar2000. This should be `<foobar2000 install folder>\profile\user-components{,-x64}\foo_vis_milk2\milkdrop2\presets`. Some presets suggested by Ryan Geiss (the MilkDrop 2 author) can be downloaded from his [website](https://www.geisswerks.com/milkdrop/).

## Repository Notes

The build assumes the following directory structure:

```text
 .github\ -> contains the continuous integration build pipeline.
 Bin\ -> generated by Visual Studio to contain the final DLLs and PDBs.
 external\ -> contains external library dependencies.
     directxtk_desktop_2019.*\ -> from NuGet, contains the DirectX Tool Kit (DirectXTK) for x86, x64 and ARM64EC.
     directxtk_desktop_win10.*\ -> from NuGet, the DirectX Tool Kit (DirectXTK) for ARM64.
     eel2\ -> contains the Nullsoft Expression Evaluator Library (NS-EEL).
     foobar2000\ -> contains most of the foobar2000 SDK after downloading.
         component_client\ -> from the foobar2000 SDK, generates the DLL entrypoint function for the component.
         helpers\ -> from the foobar2000 SDK, constains a library of various helper code for the component.
         sdk\ -> from the foobar2000 SDK, contains declarations of services and various service-specific helper code.
         shared\ -> from the foobar2000 SDK, contains the various utility code used by foobar2000 components.
     libppui\ -> from the foobar2000 SDK, contains a library of helper code, mainly Windows UI code.
     nu\ -> contains the Nullsoft utilities.
     pfc\ -> from the foobar2000 SDK, a class library used by the foobar2000 SDK.
     winamp\ -> contains header files, shader files and documentation from the Winamp release.
     wtl.*\ -> from NuGet, the Windows Template Library (WTL).
 foo_vis_milk2\ -> contains the foobar2000 component code.
 Obj\ -> generated by Visual Studio to contain the intermediate build object files.
 tools\ -> contains the packaging and formatting scripts.
 vis_milk2\ -> contains the MilkDrop 2 visualization library code.
```

## MilkDrop 2

MilkDrop 2 (`vis_milk2`) is a music visualizer - a "plug-in" to the Winamp music player. The changes to the [MilkDrop 2 source code release](https://sourceforge.net/projects/milkdrop2/) from 5/13/13 (version 2.25c) include:

- Porting VMS from DirectX 9 to Direct X 11.1. DirectX 11.1 is Direct3D 11.1, Direct2D 1.1, DirectWrite 1.1, and DXGI 1.2.
- Porting text layout and rendering from D3DX9 and GDI to DirectWrite and Direct2D, respectively.
- Building DLL with Visual Studio 2022 (v143) Platform Toolset.
- Minor bug and typo fixing so that the plug-in can be used in Winamp and foobar2000 music players without crashing.
- Fixing of string resources to flow consistently with Segoe UI spacing and sizing.
- Minor cleaning and updating of configuration panel to match functioning features and UI modifications.
- Developer experience improvements, such as:
  - Updated dependencies to latest available versions.
  - Refactored EEL2 and DirectXTK into separate projects.
  - PCH and multiprocessor compile enabled for fast builds.
  - Buildable with C++20 compiler, including the address sanitizer and fuzzer. Builds clean using `/W4` (level 1,2,3,4 compiler warnings) and `/WX` (treat compiler warnings as errors) build options.
  - All character string and memory manipulation functions migrated to use their respective secure CRT versions.
  - Several utility functions and container classes replaced with their STL equivalents.
  - Enabled and added x86 64-bit builds (x64 platform) in addition to the upgraded x86 32-bit (Win32 platform) ones. Mostly tested the 64-bit DLL standalone as most music players are still 32-bit applications.
  - Added a minimal unit test with associated infrastructure and some API service mock classes to test DLL initialization.
  - Added CI pipeline (GitHub Actions).
  - Enforced more consistent formatting (with ClangFormat), line endings, and file encodings. Important notes:
    - "plugin.rc" encoding was changed from "Western European (Windows) - Codepage 1252" to "Unicode (UTF-8 without signature) - Codepage 65001".
    - "defines.h" encoding was changed from "Unicode (UTF-8 without signature) - Codepage 65001" to "Unicode (UTF-8 with signature/BOM) - Codepage 65001".
    - All other text files use "Unicode (UTF-8 without signature) - Codepage 65001". All text files use CRLF line endings.
  - Updated comments and consolidated documentation.