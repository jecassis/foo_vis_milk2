# Testing the MilkDrop 2 Visualization Component for foobar2000

In the examples shown below, foobar2000 is installed in the `$Env:LOCALAPPDATA\Programs\foobar2000_x64\` folder.

## Runtime testing and debugging

### In foobar2000

See [BUILDING](BUILDING.md) for a sample `foo_vis_milk2/foo_vis_milk2.vcxproj.user`.

## Unit Testing

### Using VSTest in CLI

```powershell
VSTest.Console.exe /Platform:x64 "$(Get-Location)\test\test.dll"
```

## Using VSTest in GUI

Use Test Explorer via **Test > Test Explorer** (Ctrl+E, T).

## Coverage Collection

### At Runtime

As of Visual Studio 2022 version 17.2, the native DLL binary can be instrumented statically (on disk). Consequently, the Debug configurations link with the `/PROFILE` option. This enables coverage collection.

Enable native static instrumentation by enabling the preview feature "_Code Coverage experience improvements_" in **Tools > Options > Environment > Preview Features**. Alternatively, the [`coverage.runsettings`](test/coverage.runsettings) file sets:

```xml
<CodeCoverage>
  <EnableStaticNativeInstrumentation>True</EnableStaticNativeInstrumentation>
</CodeCoverage>
```

To instrument the component DLL (optional):

```powershell
Microsoft.CodeCoverage.Console.exe instrument "$Env:LOCALAPPDATA\Programs\foobar2000_x64\profile\user-components-x64\foo_vis_milk2\foo_vis_milk2.dll"
```

To run coverage collection while foobar2000 is running, launch it like so:

```powershell
Microsoft.CodeCoverage.Console.exe collect "$Env:LOCALAPPDATA\Programs\foobar2000_x64\foobar2000.exe" --settings "$(Get-Location)\test\coverage.runsettings" --output "$(Get-Location)\test\CodeCoverage\foo_vis_milk2.coverage"
```

Once foobar2000 is closed, open `foo_vis_milk2.coverage` in Visual Studio.

## DirectX "Device Lost" Testing

A "device lost" will not happen often. In Direct3D 9, a 'device lost' would routinely appear from `ALT + TAB` because the GPU used to be an 'exclusive' rather than 'shared' resource.

`DXGI_ERROR_DEVICE_RESET` occurs if the driver crashes or the video hardware hangs.
`DXGI_ERROR_DEVICE_REMOVED` occurs if a new driver is installed while the component is running or if you are running on a 'GPU is in the dock' style laptop and the laptop is undocked. This event can be triggered from the Developer Command Prompt for Visual Studio as 'administrator':

```pwsh
dxcap -forcetdr
```

> It will immediately cause all currently running Direct3D apps to get a `DXGI_ERROR_DEVICE_REMOVED` event.

#### References

- [Customize Code Coverage Analysis](https://learn.microsoft.com/en-us/visualstudio/test/customizing-code-coverage-analysis?view=vs-2022#static-and-dynamic-native-instrumentation)