# Fetches heavy assets (Sponza model + textures + HDR) from the assets repo.
# Single ~60 MB zip download with 3 retries; verified by checking that the
# marker file Models/Sponza/Sponza.bin exists after extraction.
#
#   .\scripts\fetch_assets.ps1
#   .\scripts\fetch_assets.ps1 -Force
#   .\scripts\fetch_assets.ps1 -Verify    # only check, don't download

param(
    [switch]$Force,
    [switch]$Verify
)

$ErrorActionPreference = 'Stop'

$repoRoot   = Split-Path -Parent $PSScriptRoot
$assetsUrl  = 'https://github.com/miguecrea/Assets/releases/latest/download/assets.zip'
$marker     = 'Models\Sponza\Sponza.bin'
$markerPath = Join-Path $repoRoot $marker
$tmpDir     = Join-Path $env:TEMP 'vulkan-renderer-assets'
$dlPath     = Join-Path $tmpDir 'assets.zip'

if ($Verify) {
    if (Test-Path $markerPath) {
        Write-Host "OK     marker $marker present" -ForegroundColor Green
    } else {
        Write-Host "MISSING marker $marker not found" -ForegroundColor Yellow
    }
    exit 0
}

if ((Test-Path $markerPath) -and -not $Force) {
    Write-Host "Assets already present (marker $marker). Use -Force to re-download." -ForegroundColor Green
    exit 0
}

New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null

$success = $false
for ($attempt = 1; $attempt -le 3; $attempt++) {
    try {
        Write-Host "Downloading assets.zip (attempt $attempt/3)..." -ForegroundColor Cyan
        $ProgressPreference = 'Continue'
        Invoke-WebRequest -Uri $assetsUrl -OutFile $dlPath -UseBasicParsing -ErrorAction Stop
        $success = $true
        break
    } catch {
        Write-Warning ("  attempt {0} failed: {1}" -f $attempt, $_.Exception.Message)
        if (Test-Path $dlPath) { Remove-Item $dlPath -Force }
    }
}

if (-not $success) {
    throw "Failed to download assets.zip after 3 attempts. Check network connection."
}

Write-Host "Extracting assets.zip..." -ForegroundColor Cyan
Expand-Archive -Path $dlPath -DestinationPath $repoRoot -Force
Remove-Item $dlPath -Force

if (-not (Test-Path $markerPath)) {
    throw "Extraction of assets.zip did not produce $marker. Zip may be corrupt."
}

if (Test-Path $tmpDir) { Remove-Item -Recurse -Force $tmpDir -ErrorAction SilentlyContinue }
Write-Host "Assets ready." -ForegroundColor Green
