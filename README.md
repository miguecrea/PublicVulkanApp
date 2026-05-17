
Miguel Angel Lozano Bedoya
https://github.com/Howest-DAE-GD/gp2-vulkan-renderer-miguecrea

# Setup

The heavy assets (Sponza model + textures + HDR environment, ~2.8 GB) are hosted
separately at [miguecrea/Assets](https://github.com/miguecrea/Assets) as Release
attachments — they are NOT part of this repo.

**Standard setup:** just configure CMake and the assets are auto-downloaded on
the first run (one-time ~2.8 GB download):

```bash
cmake -S . -B out/build/x64-Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug
```

To skip auto-fetch (e.g. on CI or if you have a local mirror):
`cmake ... -DFETCH_ASSETS=OFF`

**Manual fetch** (alternative, if you want assets without configuring CMake):

```powershell
.\scripts\fetch_assets.ps1          # downloads + extracts
.\scripts\fetch_assets.ps1 -Force   # re-download even if present
```

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

