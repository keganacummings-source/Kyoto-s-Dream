# KYOTRIPPAH

Two VST3 plugins. One editor idea.

- **KYOTO** goes on an FL Studio channel. It is the instrument. Oscillator, sub, noise, filter, envelope, then up to 8 effects.
- **KYOTRIPPAH FX** goes on a mixer track or the master. It is the effect. No notes. Up to 12 effects. Empty chain passes audio through, so it can sit there as DreamShare only.

Building a plugin here does not compile a new binary. You pick an effect from the DreamMaster list, the host parameters for that slot turn on (each name is unique: `Slot 01 Amount`, never a repeated `Amount`), and the dials land on the panel. Save writes a `kyoteppah-module-1` JSON file on this machine. Upload sends that JSON to DreamShare KV so other logged-in people can load the same instrument or effect.

## DreamShare

Login, chat, and threads use `https://dreamshare-api.keganacummings.workers.dev`. The session token is stored in the user app-data folder, not inside the DAW project.

Catalog upload needs `worker/module-rules.js` spliced into the DreamShare worker, with the `DREAMSHARE_KV` binding. Until that action exists, Save still keeps the module locally.

## FL Studio

Push this folder to GitHub. The Actions workflow `Build KYOTRIPPAH` uploads a zip of the real bundles.

1. Quit FL Studio.
2. Remove any older `KYOTO.vst3` or `KYOTRIPPAH FX.vst3` from the VST3 folder and from FL's plugin database.
3. Unzip the artifact. Copy the `.vst3` folders, do not flatten them. Each one must contain `Contents/x86_64-win`.
4. Put them in `C:\Program Files\Common Files\VST3`.
5. Options, Manage plugins, Find more plugins.

Both plugins accept the disabled, mono, and stereo input layouts FL probes while scanning. The instrument has an input bus on purpose.

## Build on a machine

CMake 3.22+, a C++17 compiler, JUCE 8.0.6 fetched by CMake.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

JUCE is AGPLv3 unless you hold a commercial licence. Shipping the plugin means shipping this source or holding that licence.
