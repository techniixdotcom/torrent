#Requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet("x64", "x86")]
    [string] $Platform = "x64",
    [string] $Target   = "Build",
    [switch] $SkipInstall,
    [switch] $Elevated
)

$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
Set-Location $ProjectRoot

function Write-Step($m) { Write-Host "`n=== $m ===" -ForegroundColor Cyan }
function Write-Ok($m)   { Write-Host "  [ok] $m"   -ForegroundColor Green }
function Write-Note($m) { Write-Host "  [..] $m"   -ForegroundColor Yellow }

function Test-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    (New-Object Security.Principal.WindowsPrincipal($id)).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)
}

function Update-Path {
    $machine = [Environment]::GetEnvironmentVariable("Path", "Machine")
    $user    = [Environment]::GetEnvironmentVariable("Path", "User")
    $env:Path = (@($machine, $user) | Where-Object { $_ }) -join ';'
}

function Test-Cmd($name) {
    $null -ne (Get-Command $name -ErrorAction SilentlyContinue)
}

function Get-CMakeVersion {
    try {
        $line = & cmake --version 2>$null | Select-Object -First 1
        if ($line -match '(\d+)\.(\d+)') {
            return [version]("{0}.{1}" -f $Matches[1], $Matches[2])
        }
    } catch { }
    return $null
}

function Test-DotnetOk {
    if (-not (Test-Cmd dotnet)) { return $false }
    try {
        foreach ($line in (& dotnet --list-sdks 2>$null)) {
            if ($line -match '^(\d+)\.' -and [int]$Matches[1] -ge 8) { return $true }
        }
    } catch { }
    return $false
}

function Get-VSWhere {
    $p = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $p) { return $p } else { return $null }
}

function Get-VCInstallPath {
    $vw = Get-VSWhere
    if (-not $vw) { return $null }
    $p = & $vw -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($p) { return ($p | Select-Object -First 1) } else { return $null }
}

function Get-AnyVSInstallPath {
    $vw = Get-VSWhere
    if (-not $vw) { return $null }
    $p = & $vw -latest -prerelease -products * -property installationPath 2>$null
    if ($p) { return ($p | Select-Object -First 1) } else { return $null }
}

function Install-Winget($id, [string[]] $extra) {
    $wgArgs = @("install", "--id", $id, "-e", "--accept-package-agreements", "--accept-source-agreements")
    if ($extra) { $wgArgs += $extra }
    Write-Note "winget $($wgArgs -join ' ')"
    & winget @wgArgs
}

# ---------------------------------------------------------------------------
# 1. Detect prerequisites
# ---------------------------------------------------------------------------
Write-Step "Checking prerequisites"

$cmv  = Get-CMakeVersion
$need = [ordered]@{
    Git    = -not (Test-Cmd git)
    Dotnet = -not (Test-DotnetOk)
    CMake  = ($null -eq $cmv) -or ($cmv -lt [version]"4.2")
    VC     = ($null -eq (Get-VCInstallPath))
}
foreach ($k in $need.Keys) {
    if ($need[$k]) { Write-Note "$k : missing or too old" } else { Write-Ok "$k present" }
}

$installNeeded = ($need.Values -contains $true) -and (-not $SkipInstall)

# ---------------------------------------------------------------------------
# 2. Elevate once if we have to install anything
# ---------------------------------------------------------------------------
if ($installNeeded -and -not (Test-Admin)) {
    Write-Step "Administrator rights are needed to install prerequisites - relaunching (accept the UAC prompt)"
    $argList = @(
        "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "`"$PSCommandPath`"",
        "-Platform", $Platform, "-Target", $Target, "-Elevated"
    )
    Start-Process -FilePath "powershell.exe" -Verb RunAs -ArgumentList $argList
    return
}

Start-Transcript -Path (Join-Path $ProjectRoot "build.log") -Append | Out-Null
$ok = $false
try {

    # -----------------------------------------------------------------------
    # 3. Install missing prerequisites
    # -----------------------------------------------------------------------
    if ($installNeeded) {
        if (-not (Test-Cmd winget)) {
            throw "winget was not found. Install 'App Installer' from the Microsoft Store, or install Git, the .NET 8 SDK, CMake 4.2+, and Visual Studio 2026 with the 'Desktop development with C++' workload manually, then re-run."
        }

        if ($need.Git)    { Write-Step "Installing Git";          Install-Winget "Git.Git" @("--silent") }
        if ($need.Dotnet) { Write-Step "Installing .NET 8 SDK";   Install-Winget "Microsoft.DotNet.SDK.8" @("--silent") }
        if ($need.CMake)  { Write-Step "Installing CMake";        Install-Winget "Kitware.CMake" @("--silent") }
        Update-Path

        if ($need.VC) {
            Write-Step "Installing the Visual Studio C++ toolchain (large download, please wait)"
            $vsPath = Get-AnyVSInstallPath
            $setup  = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\setup.exe"
            if ($vsPath -and (Test-Path $setup)) {
                Write-Note "Adding 'Desktop development with C++' to $vsPath"
                & $setup modify --installPath "$vsPath" --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --quiet --norestart --wait
            }
            else {
                Write-Note "Installing Visual Studio 2026 Community with the C++ workload"
                try {
                    Install-Winget "Microsoft.VisualStudio.2026.Community" @("--override", "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended")
                }
                catch {
                    throw "Visual Studio could not be installed automatically. Install 'Visual Studio 2026 Community' with the 'Desktop development with C++' workload from https://visualstudio.microsoft.com/downloads/ and re-run."
                }
            }
        }

        Update-Path
        if ($null -eq (Get-VCInstallPath)) {
            throw "The Visual Studio C++ toolchain still isn't detected. Open the Visual Studio Installer, choose Modify, tick 'Desktop development with C++', then re-run this script."
        }
        Write-Ok "All prerequisites are installed"
    }

    # -----------------------------------------------------------------------
    # 4. Unblock files (Mark of the Web from the downloaded zip)
    # -----------------------------------------------------------------------
    Write-Step "Unblocking project files"
    Get-ChildItem -Path $ProjectRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -notmatch '\\vendor\\' -and $_.FullName -notmatch '\\build-' } |
        Unblock-File -ErrorAction SilentlyContinue
    Write-Ok "done"

    # -----------------------------------------------------------------------
    # 5. Fetch dependencies (idempotent - skips anything already cloned)
    # -----------------------------------------------------------------------
    Write-Step "Fetching dependencies (wxWidgets, ANTLR, Crashpad, vcpkg)"
    $global:LASTEXITCODE = 0
    & (Join-Path $ProjectRoot "scripts\bootstrap.ps1")
    if ($LASTEXITCODE -ne 0) { throw "Dependency bootstrap failed (exit $LASTEXITCODE)." }

    # -----------------------------------------------------------------------
    # 6. Drop a stale CMake cache that used a different generator
    # -----------------------------------------------------------------------
    $buildDir = Join-Path $ProjectRoot ("build-" + $Platform)
    $cache    = Join-Path $buildDir "CMakeCache.txt"
    if (Test-Path $cache) {
        $gen = Select-String -Path $cache -Pattern "CMAKE_GENERATOR:INTERNAL=" -ErrorAction SilentlyContinue
        if ($gen -and ($gen.Line -notmatch "Visual Studio 18 2026")) {
            Write-Note "Removing stale build cache (was: $($gen.Line.Split('=')[-1]))"
            Remove-Item -Recurse -Force $buildDir
        }
    }

    # -----------------------------------------------------------------------
    # 7. Restore Cake and build
    # -----------------------------------------------------------------------
    Write-Step "Restoring build tools"
    & dotnet tool restore
    if ($LASTEXITCODE -ne 0) { throw "dotnet tool restore failed (exit $LASTEXITCODE)." }

    Write-Step "Building NiiX Torrent ($Platform) - the first run compiles dependencies and can take a long time"
    & dotnet cake --target=$Target --platform=$Platform --vcpkg-triplet="$Platform-windows-static-md-rel"
    if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE). See build.log and the output above for the first error." }

    $exe = Join-Path $buildDir "Release\NiiXTorrent.exe"
    Write-Step "SUCCESS"
    if (Test-Path $exe) {
        Write-Host "NiiX Torrent built successfully:" -ForegroundColor Green
        Write-Host "    $exe" -ForegroundColor Green
    }
    else {
        Write-Note "Build reported success but NiiXTorrent.exe was not found at $exe - check build.log."
    }
    $ok = $true
}
catch {
    Write-Host "`n[FAILED] $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "A full log was written to build.log" -ForegroundColor Red
}
finally {
    Stop-Transcript | Out-Null
}

if ($Elevated) {
    Write-Host "`nPress Enter to close..."
    [void](Read-Host)
}

if (-not $ok) { exit 1 }
