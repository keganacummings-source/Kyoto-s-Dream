# KyotoSpxrit 0.2.4 — FL Studio will not open

FL Studio 20/21/2024 kills the host process while scanning a VST3 when either of these is true:

1. Two parameters report the same name. 0.2.3 had unique IDs (`amt0`..`amt199`) but the displayed names were duplicated 200 times (`Amount`, `Tone`, `Motion`, `Mix`, `Shape`). Image-Line's wrapper asserts and takes FL down before the editor exists. 0.2.4 names them `FX 1 Amount` ... `FX 200 Shape`.
2. `isBusesLayoutSupported` returns false for a layout FL probes. The instrument declared output only. FL still asks for a disabled or stereo input during scan. Both plugins now expose an optional stereo input and accept disabled, mono, and stereo.

Also in this build:

- `JUCE_VST3_CAN_REPLACE_VST2=0`, so FL does not look for a VST2 compatibility CID.
- `processBlock` no longer renders if the host calls it with zero channels, and it prepares itself if FL processes before `prepareToPlay`.
- Parameter atomics are null-checked.
- Expert mode is an atomic flag.
- The editor does not start a network request while it is being constructed (FL creates the editor during verify).
- The Windows workflow packs the `.vst3` directories with tar so the bundle is not flattened.

After installing, remove the old plugin-database entries and rescan. FL will keep launching the binary that already crashed if you only overwrite the file.
