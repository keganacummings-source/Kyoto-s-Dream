# AGENTS.md — Base44 dev environment notes

## What this project actually is
- **Native VST3 plugins** (KYOTO synth + KYOTRIPPAH FX), C++17/JUCE 8.0.6 via CMake. Built by the GitHub Actions workflow (`.github/workflows/main.yml`) on Windows + macOS runners only. They cannot be built or shown in the Base44 browser preview.
- **DreamShare API** — a Cloudflare Worker (`worker.js` + `module-rules.js` at repo root; `worker/` holds identical canonical copies). This is the only piece that runs in the sandbox preview, served on port 3000.

## Running locally (Base44)
- `docker compose -f docker-compose.base44.yml up -d` starts `npx wrangler dev` (wrangler 4, local mode) from `node:22` with the repo bind-mounted at `/app`.
- `wrangler.toml` binds `DREAMSHARE_KV` with a fake ID — wrangler local mode simulates KV in `.wrangler/state` (gitignored). **No Cloudflare account or credentials are needed for local dev.**
- Wrangler live-reloads on `worker.js` / `module-rules.js` edits — no container restart needed for worker changes; restart only for dependency/compose changes.
- Dependencies: `package.json` pins `wrangler` and is installed at container startup; `node_modules/` is gitignored.

## Data storage quirks
- The worker's source of truth is the **public extendsclass.com JSON bins** (`abffdbc` feed, `ffedede` users) — shared with the production deployment. Logins/chats created in the local preview hit that live shared store; accounts are created on first login (no separate register). Super-admins: Trippah, Goonr.
- KV-backed features (module catalog) run against the local simulation and are NOT shared with production.
- The production worker lives at `https://dreamshare-api.keganacummings.workers.dev`; the VST hardcodes that URL.

## Compile-checking the VST3 plugins locally (Linux container)
CI (Windows/macOS) builds the plugins; to verify C++ changes before pushing:
```bash
docker run -d --name kyoto-build -v "$PWD":/src:ro ubuntu:24.04 bash -c \
  'apt-get update -qq && apt-get install -y -qq cmake ninja-build git g++ libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev pkg-config && cmake -S /src -B /tmp/build -G Ninja -DCMAKE_BUILD_TYPE=Debug'
docker exec kyoto-build ninja -C /tmp/build -j"$(nproc)" \
  CMakeFiles/KYOTO.dir/Source/PluginEditor.cpp.o CMakeFiles/KYOTO.dir/Source/PluginProcessor.cpp.o CMakeFiles/KYOTO.dir/Source/DreamApi.cpp.o \
  CMakeFiles/KYOTRIPPAHFX.dir/Source/PluginEditor.cpp.o CMakeFiles/KYOTRIPPAHFX.dir/Source/PluginProcessor.cpp.o CMakeFiles/KYOTRIPPAHFX.dir/Source/DreamApi.cpp.o
```
Compiling just those objects needs JUCE headers only (Fetched at configure) — no full JUCE build. ~3 min. Past CI failures were undeclared `kt::kFx` usage in PluginViewScreen.h (fixed by including FxCatalog.h) and rail buttons missing from PluginEditor.h.

## Auto-adjusting builder toolbars
- Plugin Builder and FX Builder toolbars use `flx::row()` (Source/FlexLayout.h) to flex-distribute buttons and dropdowns across any window width. Fixed-width items (buttons) get their natural size; flex items (dropdowns) share the remaining space. No hardcoded pixel widths that overflow on narrow windows.
- The sidebar (effects list) and FX inspector now scale proportionally via `juce::jlimit(min, max, width/fraction)` instead of fixed pixel widths.
- The builder wizard step 3 ("YOUR FIRST EFFECT") now allows manual NEXT advance if at least one widget has been placed, in addition to the auto-advance in `placeInSlot()`.

## Per-chain mixer compatibility
- Plugin Builder's mixer uses chain ordinals separated by active BREAK slots, not individual effect wet/dry parameters. New sessions sum these gains without automatic normalization; older DAW states and module files keep legacy balanced summing until the user enables PER-CHAIN LEVELS.
- Mixer parameters are appended to the APVTS layout to preserve existing parameter order/type ranges. DAW state and undo retain them; module JSON carries `chainLevels: { enabled, levels }`. Keep `module-rules.js` and `worker/module-rules.js` in sync so catalog publishing does not strip the gains.
- Native controls cannot be interaction-tested in the browser preview (which serves only the Worker API). Compile-check both plugin variants and test the pure `Source/ChainMix.h` accumulator for gain/mute/legacy balance.

## Native UI guide and chat
- The builder guide now has four states: shell, playground theme, built-in FX selection/placement, then control practice. Advance with `showTab(1)`, not just `resized()`, so control visibility updates on every transition. Step 3 unlocks only after a successful bay placement.
- JUCE ComboBox owns a child Label: never draw its text again in `drawComboBox`. Key/board/cosmetic canvas widgets paint their captions themselves.
- Chat feed order is oldest-to-newest. Opening the chat follows the bottom; refreshing preserves history browsing unless already near the bottom. These native behaviors require a DAW check; Worker HTTP checks do not verify them.

## Verify it works
```bash
curl -s http://localhost:3000/ | head -c 300          # feed JSON, "storage":"durable-v4"
curl -s -X POST localhost:3000/ -H 'Content-Type: application/json' \
  -d '{"action":"login","user":"dev_test","pass":"x"}'  # returns token
```
Container healthcheck: `node -e "fetch('http://127.0.0.1:3000/')..."` (no curl/wget in node image).

## Discord Lite (Threads → Discord relay)
- The DreamShare home "THREADS" tab now has a **DISCORD** toggle button. When on, the
  board shows the Discord channels the bot can see (Discord Lite); tapping a channel
  loads its recent 50 messages, with a `< CHANNELS` back button.
- The worker proxies Discord REST (`discord.com/api/v10`) with the bot token so the
  token never reaches the VST. Actions: `discord_channels` (lists text/announcement
  channels of the guild) and `discord_messages` (`{ channelId }` → recent messages).
- Secrets `DISCORD_BOT_TOKEN` + `DISCORD_GUILD_ID` are delivered via `/run/base44/app.env`
  (compose `env_file:`). The compose startup writes them into `.dev.vars` (gitignored)
  so wrangler local dev exposes them as worker `env` bindings. Without them the
  `discord_*` actions return a clear "not configured" error and the rest of the app is
  unaffected.
- A 403 from `discord_channels` means the token is valid but the bot is not in the
  guild (or lacks View Channels / Read Message History). Invite the bot with those
  permissions; enable the Message Content Intent in the Developer Portal.
- C++ compile-check note: `randomizeTemplate` used `StringArray::removeAndReturn` (no
  such method) and `editEffectPopup`'s modal lambda needed `mutable` to call
  `ValueTree::setProperty` on a by-value capture. Both were pre-existing and are now
  fixed so the plugin objects compile on Linux GCC.
