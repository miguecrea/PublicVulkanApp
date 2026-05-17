
Miguel Angel Lozano Bedoya
https://github.com/Howest-DAE-GD/gp2-vulkan-renderer-miguecrea

# Setup (fetch assets)

The heavy assets (Sponza model + textures + HDR environment, ~2.8 GB) are hosted
separately at [miguecrea/Assets](https://github.com/miguecrea/Assets) and pulled
in by a script — they are NOT part of this repo.

After cloning, from PowerShell in the repo root:

```powershell
.\scripts\fetch_assets.ps1
```

This downloads and extracts the 4 release zips into `Models/Sponza/` and `Textures/`.
Re-run with `-Force` to overwrite an existing asset tree.

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

