# Fetches heavy assets (Sponza model + textures + HDR) from the assets repo.
# Run once after cloning. Re-run with -Force to overwrite.
#
#   .\scripts\fetch_assets.ps1
#   .\scripts\fetch_assets.ps1 -Force

param([switch]$Force)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$baseUrl  = 'https://github.com/miguecrea/Assets/releases/latest/download'
$tmpDir   = Join-Path $env:TEMP 'vulkan-renderer-assets'

$keyFile = Join-Path $repoRoot 'Models\Sponza\NewSponza_Main_glTF_003.bin'
if ((Test-Path $keyFile) -and -not $Force) {
    Write-Host "Assets already present (found $keyFile). Use -Force to re-download." -ForegroundColor Green
    exit 0
}

New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null

$zips = @('mesh.zip', 'env.zip', 'textures_part1.zip', 'textures_part2.zip')

foreach ($zip in $zips) {
    $url  = "$baseUrl/$zip"
    $dest = Join-Path $tmpDir $zip
    Write-Host "Downloading $zip from $url"
    $ProgressPreference = 'Continue'
    Invoke-WebRequest -Uri $url -OutFile $dest

    Write-Host "Extracting $zip"
    Expand-Archive -Path $dest -DestinationPath $repoRoot -Force
    Remove-Item $dest -Force
}

Remove-Item -Recurse -Force $tmpDir -ErrorAction SilentlyContinue
Write-Host "Assets fetched successfully." -ForegroundColor Green
