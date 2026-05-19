# Fetches heavy assets (Sponza model + textures + HDR) from the assets repo.
# Each zip has its own marker file; missing markers trigger re-fetch + retry
# so a partial failure self-heals on next run.
#
#   .\scripts\fetch_assets.ps1
#   .\scripts\fetch_assets.ps1 -Force
#   .\scripts\fetch_assets.ps1 -Verify    # only check, don't download

param(
    [switch]$Force,
    [switch]$Verify
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$baseUrl  = 'https://github.com/miguecrea/Assets/releases/latest/download'
$tmpDir   = Join-Path $env:TEMP 'vulkan-renderer-assets'

# zip name -> representative file that proves the zip was extracted
$packs = @(
    @{ Zip = 'mesh.zip';             Marker = 'Models\Sponza\NewSponza_Main_glTF_003.bin' }
    @{ Zip = 'env.zip';              Marker = 'Textures\environment.hdr' }
    @{ Zip = 'textures_part1.zip';   Marker = 'Models\Sponza\textures\arch_stone_wall_01_BaseColor.png' }
    @{ Zip = 'textures_part2.zip';   Marker = 'Models\Sponza\textures\wood_tile_01_Roughnesswood_tile_01_Metalness.png' }
)

$anyFetched = $false

foreach ($pack in $packs) {
    $markerPath = Join-Path $repoRoot $pack.Marker
    $haveIt = Test-Path $markerPath

    if ($Verify) {
        if ($haveIt) { Write-Host "OK     $($pack.Zip) (marker $($pack.Marker) present)" -ForegroundColor Green }
        else         { Write-Host "MISSING $($pack.Zip) (marker $($pack.Marker) not found)" -ForegroundColor Yellow }
        continue
    }

    if ($haveIt -and -not $Force) {
        Write-Host "Skipping $($pack.Zip) (marker present, use -Force to re-fetch)" -ForegroundColor DarkGray
        continue
    }

    if (-not $anyFetched) {
        New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null
        $anyFetched = $true
    }

    $url     = "$baseUrl/$($pack.Zip)"
    $dlPath  = Join-Path $tmpDir $pack.Zip
    $success = $false

    for ($attempt = 1; $attempt -le 3; $attempt++) {
        try {
            Write-Host "Downloading $($pack.Zip) (attempt $attempt/3)..." -ForegroundColor Cyan
            $ProgressPreference = 'Continue'
            Invoke-WebRequest -Uri $url -OutFile $dlPath -UseBasicParsing -ErrorAction Stop
            $success = $true
            break
        } catch {
            Write-Warning ("  attempt {0} failed: {1}" -f $attempt, $_.Exception.Message)
            if (Test-Path $dlPath) { Remove-Item $dlPath -Force }
        }
    }

    if (-not $success) {
        throw "Failed to download $($pack.Zip) after 3 attempts. Check network connection."
    }

    Write-Host "Extracting $($pack.Zip)..." -ForegroundColor Cyan
    Expand-Archive -Path $dlPath -DestinationPath $repoRoot -Force
    Remove-Item $dlPath -Force

    if (-not (Test-Path $markerPath)) {
        throw "Extraction of $($pack.Zip) did not produce expected file $($pack.Marker). Zip may be corrupt."
    }
}

if ($Verify) {
    Write-Host "(verify only - no downloads attempted)" -ForegroundColor DarkGray
    exit 0
}

if (Test-Path $tmpDir) { Remove-Item -Recurse -Force $tmpDir -ErrorAction SilentlyContinue }
Write-Host "Assets ready." -ForegroundColor Green
