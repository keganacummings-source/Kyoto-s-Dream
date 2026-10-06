# KYOTRIPPAH 0.4.2

Native VST3 instrument (**KYOTO**) and effect (**KYOTRIPPAH FX**).

## This revision

### Plugin Builder
Opening PLUGIN BUILDER always starts the guide. Pro mode is gone.

1. **Build the template.** Lego-click the chassis modules (mainboard, power, caps, ribbon, fan, port, expansion, RAM, PSU, drive, antenna). Each one snaps onto the highlighted piece and opens placement bays. Every module except the mainboard adds a small hardware colour. The mainboard is only the display bay.
2. **Put on a display.** Pick Scope, Peaks, CRT, or Meter and click the glowing bay. The display loads no effect. It is the start of the chain.
3. **Wire the chain.** Click a part that is already placed, then place the next part. The wire runs from the clicked part to the new one. Later parts attach the same way, so the chain is a series instead of a star.
4. **Several controls, one effect.** Click an effect and hang more than one dial, fader, or key on it. Or place a Mix % dial first, then hang an effect on that dial: the dial becomes that effect's mix, and the mix blends the new effect onto the part the dial hangs from.

Workshop (after WORKSHOP >) keeps the same click-then-place rule. MODULES returns to the template without wiping the build.

### Login
First open asks for DreamShare username and password. The session token and username are saved in the app-data folder (`KYOTRIPPAH/session.json`). Password is never stored and the login fields stay hidden until Log Out.

### DreamShare
Chat uses the live worker actions `chat_list` and `chat_send` on `https://dreamshare-api.keganacummings.workers.dev`. Catalog is a card browser (name, face, author, load), not a combo box. Upload uses `module_publish` / `module_list` / `module_get`.

The Cloudflare Worker entry point is `worker.js`. It imports the single Kyoto module/catalog implementation from `module-rules.js`. Bind `DREAMSHARE_KV` for durable module/catalog storage. The old `WORKER_DREAMSHARE.js` names are retained only as compatibility copies; deploy `worker.js` so there is one canonical entry point.

Published modules keep the lego template (`bricks`, `slotCursor`, `nextUid`) and the wire (`link`, `uid`) on each widget.

### FX Builder
Stacks effects into **one** saved effect (`kyoteppah-effect-1`), not a chain. Saved effects can be dropped into the Chain builder as a single linked block. Upload catalogues them on the API as face `effect`.

### Worker / API routing
The authenticated Kyoto route is centralized in one block and covers `module_list`, `module_get`, `module_publish`, `module_delete`, `catalog`, `catalog_delete`, `community`, `community_get`, and `community_publish`. `list_threads` loads its feed independently so it does not depend on a later feed variable. Kyoto effects accept `kyoteppah-effect-1` and preserve up to 16 FX Builder steps.

History of earlier revisions is in [UPDATE_LOG.md](UPDATE_LOG.md). Add new notes there.

## Build

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Push this folder to GitHub. Actions workflow `Build KYOTRIPPAH` uploads the VST3 zips.

JUCE is AGPLv3 unless you hold a commercial licence.
