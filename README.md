# KYOTRIPPAH 0.4.0

Native VST3 instrument (**KYOTO**) and effect (**KYOTRIPPAH FX**).

## This revision

### Login
First open asks for DreamShare username and password. The session token and username are saved in the app-data folder (`KYOTRIPPAH/session.json`). Password is never stored and the login fields stay hidden until Log Out.

### DreamShare
Chat uses the live worker actions `chat_list` and `chat_send` on `https://dreamshare-api.keganacummings.workers.dev`. Catalog is a card browser (name, face, author, load), not a combo box. Upload uses `module_publish` / `module_list` / `module_get`.

Those catalog actions are in `worker/module-rules.js`. Paste that handler into the worker's authenticated action switch and bind `DREAMSHARE_KV`. Until that splice is deployed, Save still writes locally and Upload reports the worker error.

### FX Builder
Stacks effects into **one** saved effect (`kyoteppah-effect-1`), not a chain. Saved effects can be dropped into the Chain builder as a single linked block. Upload catalogues them on the API as face `effect`.

### Chain builder
Renamed from the free-peg builder. Each ADD NEXT links to the previous step. There is no drag-and-drop. The grid combo has ten placement styles and reflows the series:

1. Series Row
2. Series Column
3. Performance Deck
4. Keys Wall
5. Diagonal Cascade
6. Twin Columns
7. Console Faders
8. Hero Wave
9. Arc Satellites
10. Split Bay

Widget kinds: Dial, Fader, Key, WAV view, Saved effect. LOAD WAV decodes a real file, draws its peaks, and plays it from a Key. Live WAV views read the output scope, not a fake sine.

## Build

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Push this folder to GitHub. Actions workflow `Build KYOTRIPPAH` uploads the VST3 zips.

JUCE is AGPLv3 unless you hold a commercial licence.
