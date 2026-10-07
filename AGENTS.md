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

**Object compile is not enough.** A declared-but-undefined member (e.g. `swapPartInBay`, `rollNewInstanceTemplate`, `SocialDirectory::setData`/`paint`/`mouseDown`, `rebuildDirectory`) compiles fine and only fails at link. Before pushing, also link the real targets so `undefined reference` surfaces:
```bash
docker exec kyoto-build ninja -C /tmp/build -j"$(nproc)" KYOTO_VST3 KYOTRIPPAHFX_VST3
```
That builds all JUCE modules too (~10 min cold, ~1 min warm) and writes both `.vst3/Contents/x86_64-linux/*.so` bundles. Success = `FULL_EXIT=0` and 0 `error:`/`undefined reference` lines.

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

The `docker run -d ... bash -c '... cmake ...'` recipe above exits as soon as cmake configure finishes, so the follow-up `docker exec ninja` fails with "container is not running". Start the container with `sleep infinity` and run apt/cmake/ninja as separate `docker exec` steps instead.

## Discord bridge (`discord/`)
- `discord/worker.js` + `discord/wrangler.toml` are a **separate** Cloudflare Worker (one Durable Object gateway) that relays Discord <-> DreamShare chat and publishes guild presence. The user deploys it on their own Cloudflare account and registers `/dream`; nothing in it runs in this sandbox.
- DreamShare side is gated on `env.DREAMSHARE_BRIDGE_KEY`. A request carrying a matching `x-dreamshare-bridge` header skips session auth for the actions the bot uses, `chat_send` accepts `dis:true` plus a `DreamUser:<name>` author (the VST draws that as a DIS tag), and `POST /?op=discord_presence` stores the guild online list, which `readPresence` merges into `onlineUsers` with `dis:true`.
- With the key unset every bridge route stays closed and the API behaves exactly as before — verified by diffing the 401/400 responses with and without the header, and by checking that a bridge-authed write never reaches the shared extendsclass bin (empty-text `chat_send` is rejected before persisting).
- The local key lives in `.dev.vars` (gitignored). Wrangler reads it only at startup: after creating or changing `.dev.vars`, run `docker compose -f docker-compose.base44.yml restart worker` — live reload does not pick up a newly created file.
- `worker/worker.js` and `WORKER_DREAMSHARE.js` are byte copies of `worker.js` and were re-synced after the bridge change. `JV_WORKER.js` had already drifted before this work and was left alone.

## Known-benign log noise (do not chase)
- `Build failed ... Unexpected end of file` at `worker.js:<line>`: wrangler rebuilds on every save, so it can catch a large file mid-write. It is transient — the file parses once the write finishes (`node --check worker.js`). Only treat it as a real defect if the error persists after the edit lands; confirm against the container start time in `docker compose logs --timestamps worker` before "fixing" anything.
- `Broken pipe (os error 32)` from workerd: a client (browser/preview) closed the connection early. Not an app error.
- `npm error signal SIGTERM` on the worker: that is the container being stopped/restarted, not a crash.

## Verification status (branch `ui-refactor-system`, commit f618ecb)
- Full link of both plugin targets passed: `ninja KYOTO_VST3 KYOTRIPPAHFX_VST3` → exit 0, 0 `error:`, 0 `undefined reference`, both `.vst3/Contents/x86_64-linux/*.so` written. `SocialDirectory`/`SocialRail` symbols (incl. `Row`, `Bubble`) are present in the linked binary, and the `dis` wiring exists at `Source/PluginEditor.cpp` chat bubbles + directory rows.
- DreamShare bridge verified over HTTP against the local stack: `?op=discord_presence` returns 401 without `x-dreamshare-bridge` and 200 with it; `chat_send` with the bridge key + `dis:true` returns a message carrying `dis:true`; a wrong key is rejected 401. `discord/worker.js` bundles with `npx wrangler deploy --dry-run` (20.6 KiB, GATEWAY Durable Object binding resolved).
- Still not verifiable here: the rendered DIS chip (native UI) and the deployed bot on Cloudflare — both need the user's DAW / Cloudflare account.

## Discord DIS rendering (native UI)
- `SocialRail::Bubble::dis` and `SocialDirectory::Row::dis` drive a small DIS chip beside the author name in the chat feed and in the Socials online list; the editor reads `dis` from chat messages and from `onlineUsers`.
- `PluginEditor.cpp` compiles clean for both targets (object compile; only a pre-existing class of JUCE deprecation warning). The rendered chip itself still needs a DAW check — the browser preview cannot show native UI.
