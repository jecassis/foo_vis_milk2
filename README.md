# MilkDrop 2 Visualization Component for foobar2000

Port of Winamp's MilkDrop 2 visualization library from its original DirectX 9 version to use DirectX 11.

Prerequisites to build the DirectX 11 `foo_vis_milk2.dll` component for foobar2000:

- [foobar2000 SDK](https://www.foobar2000.org/SDK): download the latest version and uncompress the contents in the `external/` folder.
- [NS-EEL2](https://github.com/justinfrankel/WDL/tree/main/WDL/eel2) (included in [WDL](https://www.cockos.com/wdl/)): the files required to build the DLL are included in this repository.
- [DirectXTK](https://github.com/Microsoft/DirectXTK): the files required to build the DLL are fetched via the NuGet package manager.
- [Windows Template Library (WTL)](https://wtl.sourceforge.io/): the files required to build the DLL are imported .
- [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) with [Active Template Library (ATL)](https://learn.microsoft.com/en-us/cpp/mfc/mfc-and-atl): open the [`foo_vis_milk2`](foo_vis_milk2.sln) solution, set `foo_vis_milk2` as the Startup Project, select a configuration, and build.

Refer to the [build pipeline](.github/workflows/build.yml) jobs for a step-by-step guide on how to build. _Only x86 and x64 Intel architecture platforms are functional._

See [CHANGELOG](CHANGELOG.md) for additional details.

## MilkDrop 2

MilkDrop 2 (`vis_milk2`) is a music visualizer - a "plug-in" to the Winamp music player. The changes to the [MilkDrop 2 source code release](https://sourceforge.net/projects/milkdrop2/) from 5/13/13 (version 2.25c) include:

- Porting VMS from DirectX 9 to Direct X 11.1.
<!--- Porting text layout and rendering from D3DX9 and GDI to DirectWrite and Direct2D, respectively.-->
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