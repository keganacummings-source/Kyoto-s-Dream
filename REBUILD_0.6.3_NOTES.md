# KyotoSpxrit 0.6.3 — Discord-style home + peg grid rebuild

## 1. DreamShare / Threads → Discord-style multi-channel home

- **Channel picker** (`discordChannelBox`) lists every text channel the bot can see across Kyoto + TurboNerdos.
- Each channel is labeled `#name  [Server]` with a `·live` marker for always-live rooms.
- **Mainstreet** is a virtual channel → DreamShare live chat (always live).
- **#general** (Kyoto) is always live.
- Clicking a channel live-updates that channel’s messages into the chat rail.
- Worker `discord_channels` now fetches real guild channels via the bot API (both guilds).
- Worker `discord_messages` / `discord_send` accept an optional `channel` id.
- SEND routes to the selected Discord channel when the Discord rail is active; Mainstreet still uses DreamShare chat.

Files: `worker.js` (+ copies), `Source/DreamApi.h`, `Source/DreamApi.cpp`, `Source/PluginEditor.h`, `Source/PluginEditor.cpp`

## 2. Grid placing / movement — peg snap (no more glitch)

**Root cause of glitch:** drag used local component coordinates. When `setBounds` moved the part under the cursor, the local mouse position jumped and the part thrashing.

**Fix:**
- Drag delta is computed in **parent-relative** coordinates (`dragStartParent`).
- Grid cells only update when `gx/gy/gw/gh` actually change (skips redundant `applyGrid`).
- Parts still snap to integer peg cells on the 32×22 face grid.

## 3. Templates retired → Randomize Peg

- Button / menu item renamed **Randomize Peg**.
- No longer tears down and re-rolls part kinds from shell templates.
- **Shuffles every existing part** (including motherboard) onto a fresh random free peg cell.
- Keeps kind, size, and wiring; only `gx`/`gy` change. Collision-aware placement.

## 4. Clear button with “you sure?”

- **Clear…** in the right-click playground menu (item 5207).
- Triggers `clearCanvasBtn` which shows:
  > “You sure? This removes every part except the motherboard and starts again.”
- FX Builder **CLEAR** also asks for confirmation before wiping the effect stack.

## Build / deploy

1. Rebuild VST3 via CI / `build-vst3.ps1`.
2. Redeploy DreamShare worker (`worker.js`) so multi-channel Discord APIs are live.
3. Bot needs `View Channel` + `Read Message History` on the guilds listed in `KYOTO_DISCORD_GUILDS`.
