# build-fb2k_component.ps1
#
# Copyright (c) 2023-2024 Jimmy Cassis
# SPDX-License-Identifier: MPL-2.0

<#
    .SYNOPSIS
        Builds the foobar2000 component package.
    .DESCRIPTION
        Helper script that creates the foobar2000 component archive.
    .EXAMPLE
        ..\tools\build-fb2k_component.ps1
    .EXAMPLE
        .\tools\build-fb2k_component.ps1 -TargetName foo_vis_milk2 -TargetFileName foo_vis_milk2.dll -OutputPath Bin -Version 0.0.228.65533 -Verbose
    .EXAMPLE
        .\tools\build-fb2k_component.ps1 -RunBuild -Configuration Release -TargetName foo_vis_milk2 -TargetFileName foo_vis_milk2.dll -OutputPath "$(Get-Location)\Bin" -Version 0.0.251.65533 -SavePDB -Verbose
    .OUTPUTS
        *.fb2k-component
#>

[CmdletBinding()]
param
(
    [parameter(HelpMessage = 'Run MSBuild')]
    [switch] $RunBuild,
    [parameter(HelpMessage = 'Build Configuration')]
    [string] $Configuration = 'Release',
    [parameter(HelpMessage = 'Platforms')]
    [string[]] $Platforms = @('x86', 'x64', 'ARM64', 'ARM64EC'),
    [parameter(HelpMessage = 'Target Name')]
    [string] $TargetName = 'foo_vis_milk2',
    [parameter(HelpMessage = 'Target File Name')]
    [string] $TargetFileName,
    [parameter(HelpMessage = 'Output Path')]
    [string] $OutputPath = "$(Get-Location)\Bin",
    [parameter(HelpMessage = 'Version')]
    [string] $Version,
    [parameter(HelpMessage = 'Save PDBs')]
    [switch] $SavePDB
)

#Requires -Version 7.2

Set-StrictMode -Version Latest
Set-PSDebug -Strict

$ErrorActionPreference = 'Stop'

# Note: The working directory is the solution directory.

if (-not (Test-Path Env:VCToolsInstallDir))
{
    Write-Host "FATAL: Run in Developer PowerShell."
    exit 1
}

if (-not $TargetFileName)
{
    $TargetFileName = "${TargetName}.dll"
}

Write-Host "INFO: Building `"$TargetName`" component package..."

$PackagePath = "$(Get-Location)\component"
$DataPath = "$(Get-Location)\external\winamp"
$versions = @()
$x86Version = $null
$x64Version = $null
$arm64Version = $null
$arm64ecVersion = $null
$SolutionPath = "$(Get-Location)\foo_vis_milk2.sln"

# Create the package directory.
if (Test-Path -Path $PackagePath)
{
    Remove-Item -Path $PackagePath -Recurse -Force
}
Write-Host "INFO: Creating directory `"$PackagePath`"..."
$null = New-Item -Path `
    "${PackagePath}\", `
    "${PackagePath}\x64", `
    "${PackagePath}\arm64", `
    "${PackagePath}\arm64ec", `
    "${PackagePath}\data" -Type Directory -Force

# Build DLL and copy build output.
foreach ($platform in $Platforms)
{
    if ($RunBuild)
    {
        Write-Host 'INFO: Running' ${platform}.Replace('x86', 'Win32') $Configuration 'build...'
        msbuild.exe $SolutionPath -m:8 -t:Build -p:"Configuration=${Configuration};Platform=${platform}" #--% -verbosity:diagnostic
    }
    if (Test-Path -Path ("${OutputPath}\" + (${platform}.Replace('x86', 'Win32')) + "\${Configuration}\${TargetFileName}"))
    {
        if ($VerbosePreference)
        {
            Write-Host "DEBUG: Dumping $(${platform}.Replace('x86', 'Win32')) `"$TargetFileName`" information..."
            Write-Host -NoNewline 'DEBUG: File Size: '
            Write-Host ((Get-ItemProperty ("${OutputPath}\" + (${platform}.Replace('x86', 'Win32')) + "\${Configuration}\${TargetFileName}")).length / 1KB) KiB -Separator ' '
            #Write-Host -NoNewline 'DEBUG: File Version: '
            #Write-Host (Get-ItemProperty ("${OutputPath}\" + (${platform}.Replace('x86', 'Win32')) + "\${Configuration}\${TargetFileName}")).VersionInfo.FileVersion
            #Write-Host -NoNewline 'DEBUG: Product Version: '
            #Write-Host (Get-ItemProperty ("${OutputPath}\" + (${platform}.Replace('x86', 'Win32')) + "\${Configuration}\${TargetFileName}")).VersionInfo.ProductVersion
            (Get-ItemProperty ("${OutputPath}\" + (${platform}.Replace('x86', 'Win32')) + "\${Configuration}\${TargetFileName}")).VersionInfo | Format-List
            #$vcversion = (Get-ChildItem "C:\Program Files\Microsoft Visual Studio\2022\VC\Tools\MSVC\" | Where-Object { $_.PSIsContainer } | Sort-Object -Property Name -Descending | Select-Object -First 1).Name
            #$toolhost = ($platform -ireplace 'ARM64(?:EC)?', 'X86').ToUpper()
            #$tooltarget = ($platform -ireplace 'ARM64EC', 'arm64').ToLower()
            link.exe /dump /exports /nologo ("${OutputPath}\" + (${platform}.Replace('x86', 'Win32')) + "\${Configuration}\foo_vis_milk2.dll")
        }
        $versions += (Get-ItemProperty ("${OutputPath}\" + (${platform}.Replace('x86', 'Win32')) + "\${Configuration}\${TargetFileName}")).VersionInfo.FileVersionRaw
        Write-Host "INFO: Copying $(${platform}.Replace('x86', 'Win32')) `"$TargetFileName`" to `"$("${PackagePath}\" + ${platform}.Replace('x86', '').ToLower())`"..."
        Copy-Item ("${OutputPath}\" + (${platform}.Replace('x86', 'Win32')) + "\${Configuration}\${TargetFileName}") -Destination ("${PackagePath}\" + ${platform}.Replace('x86', '').ToLower()) -Force
    }
    else
    {
        Write-Host "FATAL: Missing `"${platform}.Replace('x86', 'Win32')`" build output."
        exit 1
    }
}

# Copy data and presets.
if (Test-Path -Path "${DataPath}\data\*")
{
    Write-Host "INFO: Copying shaders to `"${PackagePath}\data`"..."
    Copy-Item "${DataPath}\data\*" -Destination "${PackagePath}\data" -Force
}
else
{
    Write-Host "FATAL: Missing or empty shader data."
    exit 1
}

# Check versions.
$x86Version, $x64Version, $arm64Version, $arm64ecVersion = $versions
if ($x86Version -ne $x64Version)
{
    Write-Host "FATAL: Win32 (${x86Version}) and x64 (${x64Version}) DLL versions mismatch."
    exit 1
}
if ($arm64Version -and ($x86Version -ne $arm64Version))
{
    Write-Host "FATAL: Win32 (${x86Version}) and ARM64 (${arm64Version}) DLL versions mismatch."
    exit 1
}
if ($arm64ecVersion -and ($x86Version -ne $arm64ecVersion))
{
    Write-Host "FATAL: Win32 (${x86Version}) and ARM64EC (${arm64ecVersion}) DLL versions mismatch."
    exit 1
}

# Define component output file name.
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
Compress-Archive -Path "${PackagePath}\*" -DestinationPath $ArchivePath -CompressionLevel Optimal -Force

# Save PDBs.
if ($SavePDB && $Version && $Version -ne '')
{
    $BackupPath = "$(Get-Location)\data"
    $PdbPath = "${BackupPath}\pdbs\${Version}"
    Write-Host "INFO: Copying PDBs to `"${PdbPath}`"..."
    
    # Create the PDB directory.
    if (-not (Test-Path -Path $BackupPath))
    {
        $null = New-Item -Path $BackupPath -Type Directory -Force
    }
    if (Test-Path -Path $PdbPath)
    {
        Remove-Item -Path $PdbPath -Recurse -Force
    }
    $null = New-Item -Path `
        "${PdbPath}\", `
        "${PdbPath}\Win32", `
        "${PdbPath}\x64", `
        "${PdbPath}\ARM64", `
        "${PdbPath}\ARM64EC" -Type Directory -Force
    foreach ($platform in $Platforms)
    {
        Copy-Item $ArchivePath -Destination $BackupPath -Force
        Copy-Item ("${OutputPath}\" + (${platform}.Replace('x86', 'Win32')) + "\${Configuration}\*") -Destination ("${PdbPath}\" + ${platform}.Replace('x86', 'Win32')) -Recurse -Force
    }
}

Write-Host "INFO: Done."

exit 0