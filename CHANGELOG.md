# MilkDrop 2 Visualization Library Notes

## Current foobar2000 Component Status

In order to complete the port from Winamp large amounts of functionality has been removed, lost, or untested:

- Some text, image and texture rendering.
- Customization through INI file.
- Older presets may not fully work.
- Interactive preset and shader editing.
- Desktop mode.
- Fake fullscreen mode and dual header functionality.
- VJ mode.
- Custom messages.

## MilkDrop 2 Library Notes

Prerequisites to build `vis_milk2.dll`:

- [Winamp 5.55 SDK](http://forums.winamp.com/showthread.php?t=252090): the files required to build the DLL are included in this repository.
- [NS-EEL2 Library](https://github.com/justinfrankel/WDL/tree/main/WDL/eel2) (included in [WDL](https://www.cockos.com/wdl/)): the files required to build the DLL are included in this repository.
- [DirectXTK Library](https://github.com/Microsoft/DirectXTK): the files required to build the DLL are imported via the NuGet package manager.

Breaking changes, relative to version 2.25c:

- The shader files in the `Winamp\Plugins\Milkdrop2\data` have been updated to match the DirectX 11 data layout.
- The [Legacy DirectX SDK](https://www.microsoft.com/en-us/download/details.aspx?id=6812) is no longer required to build due the update to DirectX 11 (see [Where is the DirectX SDK (2021 Edition)?](https://walbourn.github.io/where-is-the-directx-sdk-2021-edition/)).
- The minimum OS version supported is Windows 7. Consequently, the [DirectX End-User Runtime](https://www.microsoft.com/en-us/download/details.aspx?id=8109) installation is not required as it is included as a core component of the OS.
- Removed system callback API, which was mainly used to open websites in the internal browser.
- NS-EEL2 is _not_ built in EEL1 compatibility mode. So presets that use that syntax will not compile. Refer to the [WDL documentation](https://www.cockos.com/EEL2/) for syntax. Known EEL1 functions that have alternate EEL2 syntax or were deprecated are:
  `assign`, `if`, `equal`, `below`, `above`, `bnot`, `megabuf`, `gmegabuf`, `sigmoid`, `band`, `bor`
  <br />Example of preset change to work in EEL2:

```c
per_frame_22=vx2 = if(above(x2,0),vx2,abs(vx2)*0.5);
```

Should be:

```c
per_frame_22=vx2 = x2 > 0 ? vx2 : abs(vx2) * 0.5;
```

## MilkDrop 2 Release Notes

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