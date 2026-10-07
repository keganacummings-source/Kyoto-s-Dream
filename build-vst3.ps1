# KYOTRIPPAH 0.5.0 - build shareable Windows VST3 plugins with PowerShell.
# Requires: CMake, Visual Studio 2022 (Desktop development with C++), git, network.
# Usage (from this folder):
#   powershell -ExecutionPolicy Bypass -File .\build-vst3.ps1
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

Write-Host "KYOTRIPPAH VST3 build" -ForegroundColor Cyan
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake is not on PATH. Install CMake and reopen PowerShell."
}

$build = Join-Path $PSScriptRoot "build"
cmake -S $PSScriptRoot -B $build -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
cmake --build $build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

$out = Join-Path $PSScriptRoot "dist"
if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory -Path $out | Out-Null

$bundles = Get-ChildItem -Path $build -Recurse -Directory -Filter "*.vst3" |
    Where-Object { $_.FullName -match "Release" }
if (-not $bundles) { throw "No Release .vst3 bundles were produced." }

foreach ($bundle in $bundles) {
    Copy-Item $bundle.FullName -Destination (Join-Path $out $bundle.Name) -Recurse -Force
    Write-Host "Packed $($bundle.Name)"
}

$readme = @"
KYOTRIPPAH 0.5.0
Install: copy the .vst3 folders into
  C:\Program Files\Common Files\VST3
then rescan plugins in your DAW.

KYOTO.vst3 is the instrument. KYOTRIPPAH FX.vst3 is the effect.
First screen: LOG IN for DreamShare, or BUILD OFFLINE for Plugin Builder + Viewer.
Any empty peg accepts any part. The part resizes to that peg.
216 built-in FX, family-routed so a long chain stays one pass per stage.
"@
Set-Content -Path (Join-Path $out "INSTALL.txt") -Value $readme -Encoding UTF8

$zip = Join-Path $PSScriptRoot "KYOTRIPPAH-0.5.0-VST3-win64.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $out "*") -DestinationPath $zip
Write-Host "Share this zip: $zip" -ForegroundColor Green
