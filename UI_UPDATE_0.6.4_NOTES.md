# KyotoSpxrit 0.6.4 — Full UI update (free grid, hardware under parts, Discord usable)

Based on https://github.com/keganacummings-source/KyotoSpxrit.git

## Templates / pre-placed bays — SCRAPPED

- `BuilderCanvas` no longer draws allocated template slot plates (KNOB A, FADER B, etc.).
- Face is a **free 32×22 grid** with subtle peg dots only.
- Placement: arm a part → click the grid → part lands at that cell with a size based on type.
- Ghost outline follows the cursor while armed.

## Hardware reacts to part size

- When a part is placed or resized, a hardware plate is painted **under** it.
- Plate size = part footprint (`gx/gy/gw/gh`).
- Mini PCB traces + mounting pads scale with the part.
- Footprints refresh on drag/resize (`onGeometryChanged`).

## Free placement API

- New `placeAtGrid(gx, gy)` — no shell bay kind checks.
- Default sizes: dial 5×5, fader 4×8, wave/board 10×7, key/button 4×4, sound 6×5.
- Motherboard/screen created with free-grid footprint (12×8), not a template bay.
- Legacy builds without `gx` are migrated to grid cells on reflow.

## Discord section — more usable

- Channel picker taller (32px) with clearer hit target.
- Labels: `#name  [Server]  ·live` for always-live rooms.
- **Mainstreet** (DreamShare chat) auto-refreshes ~every 4s while on CHAT rail.
- **Selected Discord channel** (incl. #general) auto-refreshes ~every 6s on DISCORD rail.
- CHAT button jumps to Mainstreet; DISCORD prefers Kyoto #general.
- SEND routes to the selected Discord channel when on Discord rail.

## Scaling

- Default DreamShare text scale raised to **122%** (was 112%).
- A- / A+ still adjust 85%–160%.

## Drag (from 0.6.3)

- Parent-relative coords + only-apply-on-cell-change (no glitch).

## Files touched

- `Source/PluginShells.h` — BuilderCanvas free-grid paint + place
- `Source/PluginEditor.h` / `.cpp` — placeAtGrid, reflow, footprints, Discord live timer
- `Source/Themes.h` — default dsScale
- Worker multi-channel APIs from 0.6.3 retained

## Build

Rebuild VST3 via CI / `build-vst3.ps1`. Redeploy worker if Discord multi-channel not live yet.
