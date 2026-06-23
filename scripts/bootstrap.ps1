#Requires -Version 5.1
[CmdletBinding()]
param(
    [switch] $Force
)

$ErrorActionPreference = "Stop"

$Root      = Split-Path -Parent $PSScriptRoot
$VendorDir = Join-Path $Root "vendor"

function Get-Dependency {
    param(
        [Parameter(Mandatory)] [string] $Name,
        [Parameter(Mandatory)] [string] $Url,
        [Parameter(Mandatory)] [string] $Ref,
        [switch] $Recursive
    )

    $target = Join-Path $VendorDir $Name

    if ((Test-Path (Join-Path $target ".git")) -and -not $Force) {
        Write-Host "[skip] vendor/$Name already present" -ForegroundColor DarkGray
        return
    }

    if ((Test-Path $target) -and $Force) {
        Remove-Item -Recurse -Force $target
    }

    Write-Host "[clone] vendor/$Name ($Ref)" -ForegroundColor Cyan
    git clone --quiet $Url $target
    git -C $target checkout --quiet $Ref

    if ($Recursive) {
        git -C $target submodule update --init --recursive --depth 1
    }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is required but was not found on PATH."
}

New-Item -ItemType Directory -Force -Path $VendorDir | Out-Null

# Known-good pinned versions. Bump these to update dependencies.
Get-Dependency -Name "wx"             -Url "https://github.com/wxWidgets/wxWidgets"        -Ref "v3.3.2"  -Recursive
Get-Dependency -Name "vcpkg"          -Url "https://github.com/microsoft/vcpkg"            -Ref "master"
Get-Dependency -Name "antlr4"         -Url "https://github.com/antlr/antlr4"               -Ref "4.13.2"
Get-Dependency -Name "crashpad"       -Url "https://github.com/picotorrent/crashpad"       -Ref "master"  -Recursive
Get-Dependency -Name "sentry-crashpad" -Url "https://github.com/picotorrent/sentry-crashpad" -Ref "master" -Recursive

# ---- Patches for vendored sources -------------------------------------------
# ANTLR 4.13.2's ProfilingATNSimulator.cpp uses std::chrono::high_resolution_clock
# but does not #include <chrono>. Older MSVC pulled it in transitively; MSVC v145
# with C++20 does not, so the file fails to compile. Add the include (idempotent).
$antlrProfiler = Join-Path $VendorDir "antlr4/runtime/Cpp/runtime/src/atn/ProfilingATNSimulator.cpp"
if (Test-Path $antlrProfiler) {
    $c = Get-Content -Raw $antlrProfiler
    if ($c -notmatch '#include\s*<chrono>') {
        Write-Host "[patch] adding <chrono> to ANTLR ProfilingATNSimulator.cpp" -ForegroundColor Cyan
        Set-Content -Path $antlrProfiler -Value ("#include <chrono>`r`n" + $c) -NoNewline
    }
}
# -----------------------------------------------------------------------------

# Bootstrap vcpkg
$vcpkgExe = Join-Path $VendorDir "vcpkg/vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    Write-Host "[vcpkg] bootstrapping" -ForegroundColor Cyan
    & (Join-Path $VendorDir "vcpkg/bootstrap-vcpkg.bat") -disableMetrics
}

Write-Host ""
Write-Host "All dependencies are ready. Next:" -ForegroundColor Green
Write-Host "    dotnet tool restore"
Write-Host "    dotnet cake --target=Build --platform=x64 --vcpkg-triplet=x64-windows-static-md-rel"
