# KyotoSpxrit (Revised Visual Build)

Native VST3 instrument (**KYOTO**) and FX (**KYOTRIPPAH FX**) for FL Studio / Ableton.

This revision focuses on the visual and Builder / DreamShare experience requested:

## What changed

### Builder
- **Proper FX browser** – scrollable searchable list instead of a plain ComboBox.
- **Hover preview** – moving the mouse over an effect in the browser applies it at half intensity so you can audition while browsing.
- **Single placement** – PLACE button (and browser selection) adds one effect at a time; no multi-add.
- **Peg grid** – 12×8 peg layout. Free pegs light up when you are about to place something. Collision detection prevents dials, sliders, keys and waveform screens from overlapping.
- **Widget kinds** – Dial, Slider, Key/Note, lightweight Waveform display. Instrument-style producers can drop a Key; effects drop the usual AMT/TONE/MOT/MIX/SHP set on free pegs only.
- **Theme travels with the module** – saved instruments/effects store the chosen theme id so the layout looks the same when loaded by anyone else.

### Themes
- All themes live in a **single file** `Source/Themes.h` (no more one-HTML-file-per-theme).
- 22 distinctive palettes taken from the original Site/Themes set (Trippah, Goonr, Abyss, Amber, Bloodmoon, Cobalt, Ember, Fog, Graphite, Honey, Ice, Ink, Lagoon, Lilac, Mint, Neon, Pine, Plum, Rust, Steel, Void, Wine + Default).
- Theme selector on the DreamShare home screen; choice is remembered in the local session and written into every saved module.

### DreamShare home
- Cleaner two-column layout.
- Login, live feed, send, catalog, theme switcher.
- Same Cloudflare Worker endpoint (`https://dreamshare-api.keganacummings.workers.dev`).

### Still native
- No WebView in the audio path. DSP, parameters and state remain pure JUCE / C++.
- Real-time engine is unchanged from the previous stable core.

## Build

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64   # Windows
cmake --build build --config Release --parallel
```

macOS:

```bash
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build --config Release --parallel
```

Produces `KYOTO.vst3` and `KYOTRIPPAH FX.vst3`.

## Install (Windows / FL Studio)

1. Quit FL.
2. Remove any older Kyoto / KYOTRIPPAH bundles.
3. Copy the two `.vst3` folders into `C:\Program Files\Common Files\VST3`.
4. Rescan plugins.

## Worker

Use the included `worker/module-rules.js` (or the project `WORKER_DREAMSHARE.js` if present) and bind `DREAMSHARE_KV`.

Default API: `https://dreamshare-api.keganacummings.workers.dev`
