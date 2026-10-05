# KyotoSpxrit setup

## 1. GitHub

Create a repository named `KyotoSpxrit` and upload the entire project folder, including `.github`, `Source`, `Resources`, `updates` and `WORKER_DREAMSHARE.js`.

## 2. Deploy the DreamShare Worker

Deploy `WORKER_DREAMSHARE.js` to the same Cloudflare Worker used by the site:

`https://dreamshare-api.keganacummings.workers.dev`

Bind:

- `DREAMSHARE_KV` — required for durable accounts and **Community Instruments**.
- `DREAMSHARE_R2` — recommended for large WAV/image media.

The worker accepts the existing DreamShare login/thread/presence API and adds the community instrument API.

## 3. Build

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

Or use GitHub Actions. The workflow builds both VST3 variants.

## 4. Install both plugins

Install both bundles:

- **KyotoSpxrit** — instrument. Use this in FL Studio's Channel Rack / MIDI tracks and Ableton MIDI tracks.
- **KyotoSpxrit FX** — effect. Use this on the FL Studio Mixer Rack or Ableton audio/effect tracks.

Rescan VST3 plugins in the DAW.

## 5. First launch

The Home screen is DreamShare. Log in with the same Dream account used by the site.

The plugin selector is locked until at least one module exists. BUILD remains the native creation area after login.

## 6. Make an instrument

1. Open BUILD.
2. Normal mode is active by default.
3. Use the five macros to quickly shape a sound.
4. Toggle EXPERT MODE for oscillator, envelope, filter, LFO and per-effect precision controls.
5. Save the instrument.
6. The selector unlocks and the saved module becomes selectable.

## 7. Publish an instrument

From BUILD:

1. Save the instrument.
2. Click **UPLOAD COMMUNITY**.
3. Enter a description.
4. Other users can open **COMMUNITY**, browse the list, select an instrument and load it.

Only parameter/state JSON is shared. No executable plugin code is accepted from community uploads.

## 8. Themes

The native VST contains all **42 legacy site themes**. THEMES also provides four UI variants:

- Classic
- Compact
- Glass
- Terminal

The variant changes the shared UI treatment while the selected theme supplies colors/scene identity. This avoids maintaining a different UI codebase for every theme.

Users can create a theme by entering a name and accent color. Custom themes are saved with the DAW project.

## 9. Test focus/minimize behavior

For the instrument:

- Play MIDI notes.
- Minimize the plugin editor.
- Unfocus the window.
- Continue playback.

For the FX plugin:

- Put audio through the mixer.
- Close/minimize the editor.
- Confirm audio processing continues.

The realtime engine is native and no longer depends on WebAudio/WebView focus.

## 10. Cloudflare R2 + KV beginner guide

For a click-by-click setup, use `Docs/CLOUDFLARE-R2-KV-DUMMYS.md`. It covers creating the namespace/bucket, adding the exact `DREAMSHARE_KV` and `DREAMSHARE_R2` bindings, deploying the Worker, and testing Community Instruments/WAV/image storage.
