# KyotoSpxrit

Native VST3 instrument and FX. Version 0.2.4 is the FL Studio Windows fix.

## Open this in FL Studio (Windows)

The plugin was crashing FL on load. 0.2.4 fixes the three host-killers:

- Every automatable parameter now has a unique name. FL aborts if two parameters share a name, and 0.2.3 still named 200 controls "Amount", "Tone", "Motion", "Mix" and "Shape".
- Both plugins accept the disabled / mono / stereo input layouts FL probes while scanning. The instrument previously had no input bus, so that probe failed inside the host.
- The Windows artifact is a real `.vst3` bundle. Do not flatten the folder.

Build it by pushing this folder to GitHub. The Actions workflow `Build KyotoSpxrit` uploads `KyotoSpxrit-Windows-VST3.zip`.

Install:

1. Quit FL Studio.
2. Delete any older `KyotoSpxrit.vst3` and `KyotoSpxritFX.vst3` from `C:\Program Files\Common Files\VST3` and from FL's extra search paths.
3. Unzip the Actions artifact. You must end up with folders named `KyotoSpxrit.vst3` and `KyotoSpxritFX.vst3`, each containing `Contents\x86_64-win\`.
4. Copy those two folders into `C:\Program Files\Common Files\VST3`.
5. In FL: Options > Manage plugins. Remove the old KyotoSpxrit entries (a failed scan stays cached). Then Find more plugins, or rescan.
6. Load **KyotoSpxrit** as a generator. Load **KyotoSpxrit FX** on a mixer slot.

If FL still opens the old binary, the database entry is stale. Remove it and rescan. Do not put the zip itself on the plugin path.



KyotoSpxrit is a native, modular VST3 platform for **FL Studio and Ableton on Windows and macOS**. It is intentionally split into a stable native audio core and updateable/community content.

## What ships in this build

- **KyotoSpxrit** — VST3 MIDI instrument for channel/MIDI tracks.
- **KyotoSpxrit FX** — VST3 audio effect for mixer racks / pedal-style use.
- Shared native DSP, state format, UI, DreamAPI client, themes and community instrument format.
- 42 legacy Dream themes imported from the existing site.
- 30 legacy Dream instrument HTML files and DreamSynth audio retained as source/reference material.
- 200 Master/INXOMNIA effect identities; Normal mode exposes 8 FX slots, while Expert mode supports a long ordered FX chain (128 realtime slots).
- DreamShare Home screen with login, thread feed, WAV posting and image posting.
- Community Instruments: publish saved instruments, browse public instruments and load them directly into the native engine.
- Normal mode by default for simple controls; Expert mode exposes precision controls and removes the FX-chain slot limit.
- Four reusable UI variants (Classic, Compact, Glass, Terminal) applied across all themes.
- Local custom theme creation; every saved instrument/effect is forced to carry a custom UI/theme definition.
- Builder canvas with 10 placement-grid styles, drag/drop positioning, fixed-size dials/sliders, text elements, and 5 waveform/screen types.

## Architecture

The audio path is native:

`DAW -> VST3 -> MIDI/Audio -> native instrument/effect engine -> native 200-FX DSP -> DAW`

DreamAPI is only used for account/community/online services. The realtime audio engine does not depend on a browser, WebView, WebAudio `AudioContext`, page focus, or the plugin editor remaining open.

## Theme system

The old site's 42 theme packs are embedded in `Resources/themes/themes.json` and the original HTML theme files remain under `Resources/themes/` for reference. Native KyotoSpxrit renders their palettes/scenes through one shared UI renderer rather than maintaining 42 independent layouts.

Users can also create a custom theme from the THEMES screen. The BUILD screen extends the theme builder into the actual module UI: add dials, sliders, wave/screen displays, and text, choose one of 10 grid styles, and drag elements into place. Controls use fixed designed dimensions with no resize handles. Every saved module stores this UI layout and custom theme with the module state.

## Normal vs Expert

**Normal** is the default:
- Macro controls instead of engineering-style parameter grids.
- Fast instrument shaping for beginners.
- Exactly eight visible FX slots.

**Expert**:
- Three oscillator mix/detune controls.
- Full envelope/filter/LFO controls.
- Long ordered FX chain; use **+ ADD FX** and keep adding rows. The native realtime engine safely supports 128 slots per instance.
- Scrollable FX area for long chains.
- Per-effect Amount/Tone/Motion/Mix/Shape controls.
- All 200 Master/INXOMNIA effects remain selectable and effects can be repeated in the chain.

## Community Instruments

Saving an instrument creates a native preset/module. The BUILD screen provides an **UPLOAD COMMUNITY** action. Published instruments are stored as JSON module state in `DREAMSHARE_KV` and can be browsed from the COMMUNITY screen by other logged-in Dream users.

Community upload intentionally stores **parameters/state, not executable code**, so a community instrument cannot replace or inject the VST's native audio engine.

### Worker requirement

Community Instruments require a Cloudflare Worker KV binding named:

`DREAMSHARE_KV`

The existing worker can continue using the same account/presence/feed storage in that namespace. R2 remains recommended for WAV/image media.

## DreamAPI endpoint

Default:

`https://dreamshare-api.keganacummings.workers.dev`

Change it in `Source/DreamAPI.h` if the worker moves.

## GitHub updates

The stable VST binary should not need to change when only module/theme/community content changes. `updates/manifest.json` remains the update manifest for module/binary release information.

A real self-updater should use a signed external helper for replacing VST3 files because DAWs can keep plugin bundles locked while they are loaded. The project deliberately does not overwrite a live VST3 in-place.

## Build requirements

- CMake 3.22+
- C++17 compiler
- JUCE 8.0.6 (fetched automatically)
- Windows: Visual Studio 2022
- macOS: Xcode + command-line tools

### Windows

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

### macOS universal

```bash
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build --config Release --parallel
```

The build produces two VST3 bundles: the instrument and the FX version.

## GitHub Actions

`.github/workflows/build.yml` builds Windows x64 and macOS universal VST3 artifacts. After a successful run, download both artifacts from the Actions run and install/rescan them in the DAW.

## Worker deployment

Use the included `WORKER_DREAMSHARE.js` as the source of truth for the DreamShare Worker. It keeps the existing DreamAPI actions and adds:

- all 42 themes in the theme API
- `community=instruments` GET listing
- `community=<id>` GET instrument state
- `community_publish` authenticated POST
- existing WAV/image support

Deploy the worker, then bind `DREAMSHARE_KV` and, for larger media, `DREAMSHARE_R2`.
