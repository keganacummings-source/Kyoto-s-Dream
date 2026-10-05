# KyotoSpxrit 0.2.3

## Stability / audio engine
- Replaced the previous 200-effect `DspEngine` processing path with the native `FxChain` architecture developed and tested in the Claude implementation session.
- The FX engine is native C++17, has no WebView/WebAudio dependency, performs no audio-thread allocation, and sanitizes non-finite audio.
- 200 effect identities remain mapped to deterministic DSP recipes with per-effect voicing.
- Normal mode remains exactly 8 FX slots.
- Expert mode supports a 128-slot realtime chain with independent slot state and a scrollable editor UI.
- Expert chain state is published through fixed snapshots so editor/state mutations cannot race the audio thread.
- Added block-size, sample-rate, mono, rapid effect switching, 128-slot, finite-output, and CPU smoke testing.

## Crash/state compatibility
- APVTS parameter IDs are now unique.
- Legacy `mix1`/`mix2`/`mix3` duplicate IDs from pre-0.2.3 states are migrated to `oscMix1`/`oscMix2`/`oscMix3` before APVTS restoration.
- Existing defensive state validation remains in place.

## macOS build
- FX CMake `PRODUCT_NAME` is now apostrophe/space-free for Xcode's shared-code archive path.
- macOS workflow cleans the build directory and explicitly builds both VST3 targets as x86_64 + arm64.

## Cleanup
- Removed the unused legacy `DspEngine.cpp/.h` files.
- Kept the existing 42 themes, community/DreamShare functionality, module preset system, builder, and native UI architecture.


# KyotoSpxrit 0.2.4

FL Studio on Windows crashed while scanning 0.2.3. See CRASH-FIX-0.2.4.md.

- Unique host parameter names (`FX 1 Amount` ... `FX 200 Shape`).
- Optional stereo input, and bus layouts FL probes are accepted.
- VST2-compatibility CID disabled.
- Audio callback tolerates a process call before prepare, and a zero-channel buffer.
- Editor construction no longer starts a network request.
