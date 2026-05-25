
Miguel Angel Lozano Bedoya
https://github.com/Howest-DAE-GD/gp2-vulkan-renderer-miguecrea

# Setup

The heavy assets (Sponza model + textures + HDR environment, ~60 MB) are hosted
separately at [miguecrea/Assets](https://github.com/miguecrea/Assets) as a Release
attachment — they are NOT part of this repo.

**Standard setup:** just configure CMake and the assets are auto-downloaded on
the first run (one-time ~60 MB download):

```bash
cmake -S . -B out/build/x64-Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug
```

To skip auto-fetch (e.g. on CI or if you have a local mirror):
`cmake ... -DFETCH_ASSETS=OFF`

**Manual fetch** (alternative, if you want assets without configuring CMake):

```powershell
.\scripts\fetch_assets.ps1           # downloads + extracts if missing
.\scripts\fetch_assets.ps1 -Force    # re-download even if present
.\scripts\fetch_assets.ps1 -Verify   # check if assets are present, no download
```

Both the CMake auto-fetch and the script retry the download 3 times and verify
the marker file `Models/Sponza/Sponza.bin` after extraction, so a partial
failure self-heals on the next run.

You must have the Vulkan SDK installed and on PATH before building.

# Camera Controls

## Movement
| Key | Action |
|-----|--------|
| `W` | Move forward |
| `S` | Move backward |
| `A` | Move left |
| `D` | Move right |
| `E` | Move up |
| `Q` | Move down |
| `Left Shift` | Sprint (3x speed) |

## Look Around
| Input | Action |
|-------|--------|
| `Mouse` | Look around |
| `Scroll Wheel` | Zoom in/out |

## Cursor
| Key | Action |
|-----|--------|
| `Escape` | Toggle cursor lock/unlock |

