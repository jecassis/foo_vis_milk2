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
$x86Version = $null
$x64Version = $null
$arm64Version = $null
$arm64ecVersion = $null


# Create the package directory
if (Test-Path -Path "$PackagePath")
{
    Remove-Item -Path "${PackagePath}" -Recurse -Force
}
Write-Host "INFO: Creating directory `"$PackagePath`"...";
$null = New-Item -Path `
    "${PackagePath}", `
    "${PackagePath}\x64", `
    "${PackagePath}\arm64", `
    "${PackagePath}\arm64ec", `
    "${PackagePath}\data" -Type Directory -Force

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
    $x86Version = (Get-ItemProperty "${OutputPath}\Win32\Release\${TargetFileName}").VersionInfo.FileVersionRaw
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
    $x64Version = (Get-ItemProperty "${OutputPath}\x64\Release\${TargetFileName}").VersionInfo.FileVersionRaw
    Write-Host "INFO: Copying x64 `"$TargetFileName`" to `"${PackagePath}\x64`"..."
    Copy-Item "${OutputPath}\x64\Release\${TargetFileName}" -Destination "${PackagePath}\x64" -Force
}
else
{
    Write-Host "FATAL: Missing x64 build output."
    exit 1
}

# Copy ARM64 build output.
if (Test-Path -Path "${OutputPath}\ARM64\Release\${TargetFileName}")
{
    if ($VerbosePreference)
    {
        Write-Host "DEBUG: Dumping ARM64 `"$TargetFileName`" information..."
        Write-Host -NoNewline 'File Size: '
        Write-Host ((Get-ItemProperty "${OutputPath}\ARM64\Release\${TargetFileName}").length/1KB) KiB -Separator ' '
        #(Get-ItemProperty "${OutputPath}\ARM64\Release\${TargetFileName}").VersionInfo.FileVersion
        #(Get-ItemProperty "${OutputPath}\ARM64\Release\${TargetFileName}").VersionInfo.ProductVersion
        (Get-ItemProperty "${OutputPath}\ARM64\Release\${TargetFileName}").VersionInfo | Format-List
        link.exe /dump /exports /nologo "${OutputPath}\ARM64\Release\${TargetFileName}"
    }
    $arm64Version = (Get-ItemProperty "${OutputPath}\ARM64\Release\${TargetFileName}").VersionInfo.FileVersionRaw
    Write-Host "INFO: Copying ARM64 `"$TargetFileName`" to `"${PackagePath}\arm64`"..."
    Copy-Item "${OutputPath}\ARM64\Release\${TargetFileName}" -Destination "${PackagePath}\arm64" -Force
}
else
{
    Write-Host "WARNING: Missing ARM64 build output."
    Remove-Item -Path "${PackagePath}\arm64" -Recurse -Force
}

# Copy ARM64EC build output.
if (Test-Path -Path "${OutputPath}\ARM64EC\Release\${TargetFileName}")
{
    if ($VerbosePreference)
    {
        Write-Host "DEBUG: Dumping ARM64EC `"$TargetFileName`" information..."
        Write-Host -NoNewline 'File Size: '
        Write-Host ((Get-ItemProperty "${OutputPath}\ARM64EC\Release\${TargetFileName}").length/1KB) KiB -Separator ' '
        #(Get-ItemProperty "${OutputPath}\ARM64EC\Release\${TargetFileName}").VersionInfo.FileVersion
        #(Get-ItemProperty "${OutputPath}\ARM64EC\Release\${TargetFileName}").VersionInfo.ProductVersion
        (Get-ItemProperty "${OutputPath}\ARM64EC\Release\${TargetFileName}").VersionInfo | Format-List
        link.exe /dump /exports /nologo "${OutputPath}\ARM64EC\Release\${TargetFileName}"
    }
    $arm64ecVersion = (Get-ItemProperty "${OutputPath}\ARM64EC\Release\${TargetFileName}").VersionInfo.FileVersionRaw
    Write-Host "INFO: Copying ARM64EC `"$TargetFileName`" to `"${PackagePath}\arm64ec`"..."
    Copy-Item "${OutputPath}\ARM64EC\Release\${TargetFileName}" -Destination "${PackagePath}\arm64ec" -Force
}
else
{
    Write-Host "WARNING: Missing ARM64EC build output."
    Remove-Item -Path "${PackagePath}\arm64ec" -Recurse -Force
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
if ($x86Version -ne $x64Version)
{
    Write-Host "FATAL: Win32 (${x86Version}) and x64 (${x64Version}) DLL versions mismatch."
    exit 1
}
if ($arm64Version -and ($x86Version -ne $arm64Version))
{
    Write-Host "FATAL: Win32 (${x86Version}) and ARM64 (${$arm64Version}) DLL versions mismatch."
    exit 1
}
if ($arm64ecVersion -and ($x86Version -ne $arm64ecVersion))
{
    Write-Host "FATAL: Win32 (${x86Version}) and ARM64EC (${$arm64ecVersion}) DLL versions mismatch."
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
Compress-Archive -Path "${PackagePath}\*" -DestinationPath "$ArchivePath" -CompressionLevel Optimal -Force

Write-Host "INFO: Done."

exit 0