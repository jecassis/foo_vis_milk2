<#
    .SYNOPSIS
        Builds the foobar2000 component package.
    .DESCRIPTION
        Helper script that creates the foobar2000 component archive.
    .EXAMPLE
        .\build-fb2k_component.ps1
    .EXAMPLE
        .\build-fb2k_component.ps1 -TargetName foo_vis_milk2 -TargetFileName foo_vis_milk2.dll -OutputPath Bin -Version 0.0.228.65533 -Verbose
    .OUTPUTS
        *.fb2k-component
#>

[CmdletBinding()]
param
(
    [parameter(HelpMessage='Target Name')]
        [string] $TargetName = 'foo_vis_milk2',
    [parameter(HelpMessage='Target File Name')]
        [string] $TargetFileName,
    [parameter(HelpMessage='Output Path')]
        [string] $OutputPath = 'Bin',
    [parameter(HelpMessage='Version')]
        [string] $Version
)

#Requires -Version 7.2

Set-StrictMode -Version Latest
Set-PSDebug -Strict

$ErrorActionPreference = 'Stop'

# Note: The working directory is the solution directory.

if (-not $TargetFileName)
{
    $TargetFileName = "${TargetName}.dll"
}

Write-Host "INFO: Building `"$TargetName`" component package..."

$PackagePath = "$(Get-Location)\component"
$DataPath = "$(Get-Location)\external\winamp"

# Create the package directory
if (Test-Path -Path "$PackagePath")
{
    Remove-Item -Path "${PackagePath}" -Recurse -Force
}
Write-Host "INFO: Creating directory `"$PackagePath`"...";
$null = New-Item -Path `
    "${PackagePath}", `
    "${PackagePath}\x64", `
    "${PackagePath}\milkdrop2", `
    "${PackagePath}\milkdrop2\data", `
    "${PackagePath}\milkdrop2\presets" -Type Directory -Force

# Copy x86 build output.
if (Test-Path -Path "${OutputPath}\Win32\Release\${TargetFileName}")
{
    if ($VerbosePreference)
    {
        Write-Host "DEBUG: Dumping Win32 `"$TargetFileName`" information..."
        Write-Host -NoNewline 'File Size: '
        Write-Host ((Get-ItemProperty "${OutputPath}\Win32\Release\${TargetFileName}").length/1KB) KiB -Separator ' '
        #(Get-ItemProperty "${OutputPath}\Win32\Release\${TargetFileName}").VersionInfo.FileVersion
        #(Get-ItemProperty "${OutputPath}\Win32\Release\${TargetFileName}").VersionInfo.ProductVersion
        (Get-ItemProperty "${OutputPath}\Win32\Release\${TargetFileName}").VersionInfo | Format-List
        link.exe /dump /exports /nologo "${OutputPath}\Win32\Release\${TargetFileName}"
    }
    Write-Host "INFO: Copying Win32 `"$TargetFileName`" to `"$PackagePath`"..."
    Copy-Item "${OutputPath}\Win32\Release\${TargetFileName}" -Destination "${PackagePath}" -Force
}
else
{
    Write-Host "FATAL: Missing Win32 build output."
    exit 1
}

# Copy x64 build output.
if (Test-Path -Path "${OutputPath}\x64\Release\${TargetFileName}")
{
    if ($VerbosePreference)
    {
        Write-Host "DEBUG: Dumping x64 `"$TargetFileName`" information..."
        Write-Host -NoNewline 'File Size: '
        Write-Host ((Get-ItemProperty "${OutputPath}\x64\Release\${TargetFileName}").length/1KB) KiB -Separator ' '
        #(Get-ItemProperty "${OutputPath}\x64\Release\${TargetFileName}").VersionInfo.FileVersion
        #(Get-ItemProperty "${OutputPath}\x64\Release\${TargetFileName}").VersionInfo.ProductVersion
        (Get-ItemProperty "${OutputPath}\x64\Release\${TargetFileName}").VersionInfo | Format-List
        link.exe /dump /exports /nologo "${OutputPath}\x64\Release\${TargetFileName}"
    }
    Write-Host "INFO: Copying x64 `"$TargetFileName`" to `"${PackagePath}/x64`"..."
    Copy-Item "${OutputPath}\x64\Release\${TargetFileName}" -Destination "${PackagePath}/x64" -Force
}
else
{
    Write-Host "FATAL: Missing x64 build output."
    exit 1
}

# Copy data and presets.
if (Test-Path -Path "${DataPath}/data/*")
{
    Write-Host "INFO: Copying shaders to `"${PackagePath}\milkdrop2\data`"..."
    Copy-Item "${DataPath}/data/*" -Destination "${PackagePath}\milkdrop2\data" -Force
}
else
{
    Write-Host "FATAL: Missing or empty shader data."
    exit 1
}

if (Test-Path -Path "${DataPath}/presets/*")
{
    Write-Host "INFO: Copying presets to `"${PackagePath}\milkdrop2\presets`"..."
    Copy-Item "${DataPath}/presets/*" -Destination "${PackagePath}\milkdrop2\presets" -Force
}
else
{
    Write-Host "WARNING: Missing or empty preset data."
    $null = New-Item -Path "${PackagePath}\milkdrop2\presets\dummy.milk" -Type File
}

# Check versions.
$x86Version = (Get-ItemProperty "${OutputPath}\Win32\Release\${TargetFileName}").VersionInfo.FileVersionRaw
$x64Version = (Get-ItemProperty "${OutputPath}\Win32\Release\${TargetFileName}").VersionInfo.FileVersionRaw
if ($x86Version -ne $x64Version)
{
    Write-Host "FATAL: Win32 (${x86Version}) and x64 (${x64Version}) DLL version mismatch."
    exit 1
}
if ($Version -eq '')
{
    $ArchivePath = "$(Get-Location)\${TargetName}.fb2k-component"
}
elseif ($Version)
{
    $ArchivePath = "$(Get-Location)\${TargetName}-${Version}.fb2k-component"
}
else
{
    $ArchivePath = "$(Get-Location)\${TargetName}-${x86Version}.fb2k-component"
}

# Create component archive.
Write-Host "INFO: Creating component archive `"$ArchivePath`"..."
Compress-Archive -Path "${PackagePath}/*" -DestinationPath "$ArchivePath" -CompressionLevel Optimal -Force

Write-Host "INFO: Done."

exit 0