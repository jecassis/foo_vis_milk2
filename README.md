# MilkDrop 2 Visualization Component for foobar2000

Port of Winamp's MilkDrop 2 visualization library from its original DirectX 9 version to use DirectX 11.

Prerequisites to build the DirectX 11 `foo_vis_milk2.dll` component for foobar2000:

- [foobar2000 SDK](https://www.foobar2000.org/SDK): download version SDK2023-02-22 or later and place the contents in the `external/` folder.
- [NS-EEL2](https://github.com/justinfrankel/WDL/tree/main/WDL/eel2) (included in [WDL](https://www.cockos.com/wdl/)): the files required to build the DLL are included in this repository.
- [DirectXTK](https://github.com/Microsoft/DirectXTK): the files required to build the DLL are fetched using NuGet.
- [WTL](https://sourceforge.net/projects/wtl/): the files required to build the DLL are fetched using NuGet.
- [Visual Studio 2022](https://visualstudio.microsoft.com/vs/): open the [`milkdrop2`](foo_vis_milk2.sln) solution, select a configuration for the `foo_vis_milk2` project, and build.

Refer to [CI pipeline](.github\workflows\build.yml) jobs for a step-by-step guide on how to build. Only x86 and x64 Intel architecture platforms are functional.

## MilkDrop 2

MilkDrop 2 (`vis_milk2`) is a music visualizer - a "plug-in" to the Winamp music player.

Open source release's `README.txt`:

```text
MilkDrop 2 development README

Author:       Ryan Geiss
Last updated: 18 May 2013

GETTING STARTED

To get started, either download the .zip file here (which contains a snapshot
of the MilkDrop 2 source code on 5/13/13, the day it was open-sourced), or
go to 'code' tab and execute the 'git clone' command given there to pull down
the source.

To build the Winamp / Windows version (which is the only build supported
so far), you'll need Visual Studio [Visual C++] 2008 or later.  (The free
'Express' editions of Visual Studio will work just fine, and they give you
all the functionality.)

Once it's installed, open Visual Studio and open the project
"src/vis_milk2/milkdrop_DX9.sln".  Then select either the Debug or Release
configuration, whichever you want.  Then build it.

If it gets through the compile and link but then gives an error when
trying to write the final binary (vis_milk2.dll) to disk, do the following:
In the Solution Explorer, right click on the "vis_milk2" project and click
Properties.  Then, under Configuration Properties, click on Linker.  To the
right, the first item you'll see is "Output File", and it will be set to
"$(ProgramFiles)\Winamp\plugins\vis_milk2.dll" (without the double quotes).
You might have to change this to write the file to some "normal" directory,
rather than the (proteted) Program Files directory.  Then, after building,
you'll want to manually copy vis_milk2.dll to the Winamp\Plugins directory
(and repeat this each time you build) (or maybe try starting Visual Studio
as an Administrator, so it can just write the file to where you want it,
in the first place).

Once the .DLL under Program Files is updated, you can start Winamp.  Hit
CTRL+K to select MilkDrop 2 as your visualizer; hit ALT+K to configure it
(optional); or play some music and then click the 'visualization' tab to
start it.  Double-click the visualization to go full-screen.

You can attach the Visual Studio debugger to MilkDrop while it is running,
as long as the DLL that's running matches the source code in Visual Studio.
From within Visual Studio, just go to the Debug menu and select 'Attach
to Process'.  Then find Winamp.exe and it should start.  You can then
see debug output in the visual studio window, set breakpoints, move the
instruction pointer around, look at variable values, and even
modify code once a breakpoint is hit -- the compiler will recompile it
on-the-fly and you'll just keep going.  (The Visual Studio debugger is
absolutely mind-blowingly awesome.)

You'll also need to have some preset files (*.milk) in
$(ProgramFiles)\Winamp\Plugins\Milkdrop2\presets, but if you installed Winamp,
then there will already be some there for you to play with.

To learn how to alter or author new MilkDrop presets, see
$(ProgramFiles)\Winamp\Plugins\Milkdrop2\docs\milkdrop_preset_authoring.html.
```
