# Building the MilkDrop 2 Visualization Component for foobar2000

## Using the GUI

A step-by-step on how to compile on Windows:

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) and [Git](https://git-scm.com/).
   1. Clone the repository.
   ```powershell
   git.exe clone https://github.com/jecassis/foo_vis_milk2.git -b master --depth 1 foo_vis_milk2
   ```
   2. In Visual Studio "_Open a project or solution_" and point it to the [`foo_vis_milk2.sln`](foo_vis_milk2.sln) file in the repository.
   3. Ensure all additional tools for Visual Studio are installed by running the installer using **Tools > Get Tools and Features...**. In the installer that comes up hit the "_More_" dropdown and select "_Import configuration_". In the file selection dialog, point it to [`.vsconfig`](.vsconfig) in the repository to install things like NuGet, ATL and a few other prerequisites.
      > Note: Additional debug tools such as those for graphics and memory debugging are not included in this `.vsconfig`, however they might be useful for development.
   4. In Solution Explorer, if the "*foo_vis_milk2*" project is not bold font, right click on it and select "_Set as Starter Project_".
   5. In Solution Explorer again, right click on the "*Solution 'foo_vis_milk2'*" tree root at the top and select "_Manage NuGet Packages for Solution_". Use the tab that comes up to install DirectXTK and WTL for the various projects; they should be pre-populated but you can search if not.
      > Note: It is possible to ignore dependencies, but installing XAudio2 might be simpler than tinkering with the options.
2. Download the foobar2000 SDK and place it in the `external\` directory according to the file layout in the [README](README.md#repository-notes). Then use the CLI to patch it:

```powershell
Set-Location -Path "path\to\repo"
Set-Location -Path "$(Get-Location)\external"
git.exe init
git.exe add foobar2000\ libppui\ pfc\
git.exe commit -m 'Initial commit'
git.exe apply --ignore-whitespace --whitespace=nowarn --verbose "$(Get-Location)\fb2ksdk.patch"
Remove-Item -Path "$(Get-Location)\.git" -Recurse -Force
```

3. Clone the projectm-eval repo into the `external\` directory and patch it in a similar manner to the SDK:

```powershell
Set-Location -Path "path\to\repo"
Set-Location -Path "$(Get-Location)\external"
git.exe clone https://github.com/projectM-visualizer/projectm-eval.git
Set-Location -Path "$(Get-Location)\external\projectm-eval"
git.exe apply --ignore-whitespace --whitespace=nowarn --verbose "$(Get-Location)\..\pmeel.patch"
```

4. Then just select your desired build configuration and platform in Visual Studio, for example "_Debug x64_" and do **Build > Build Solution** (**Ctrl+Shift+B**).
5. For quick debugging here is a suggestion for `foo_vis_milk2/foo_vis_milk2.vcxproj.user` (only showing Debug x64 for brevity and your install locations may be different). It includes how to attach foobar2000 to the debugger and update the version of the component DLL with the most recent build:

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LocalDebuggerCommand>$(LocalAppData)\Programs\foobar2000_x64\foobar2000.exe</LocalDebuggerCommand>
    <LocalDebuggerDebuggerType>NativeOnly</LocalDebuggerDebuggerType>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <PostBuildEvent>
      <Command>copy /y "$(TargetPath)" "$(LocalAppData)\Programs\foobar2000_x64\profile\user-components-x64\foo_vis_milk2"</Command>
    </PostBuildEvent>
    <PostBuildEvent>
      <Message>Copying "$(TargetFileName)" to components directory...</Message>
    </PostBuildEvent>
  </ItemDefinitionGroup>
</Project>
```

6. To launch foobar2000 while attaching the Visual Studio debugger do **Debug > Start Debugging** (**F5**).

## Using the CLI

See [build.yml](.github/workflows/build.yml).