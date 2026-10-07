# 0.5.5 Plastic parts + Geek visual upgrade (CPU-aware)

## Plastic part graphics (`Source/PlasticParts.h`)
- Cartoonish **plastic** bodies: soft shell, gloss strip, accent rim.
- Colours always follow the active **theme** (+ optional part skin).
- **Size tiers** (Tiny / Small / Medium / Large): larger bays pick up more accent plastic and specular detail; tiny bays stay muted and readable.
- **Unique dial styles** (5 variants hashed from part identity): pointer, arc window, dual ring, flat top, gem cap.
- Keys, faders, buttons, vents/rails/badges, wave frames each have dedicated plastic silhouettes.

## Builder
- CanvasWidget paints plastic art under a transparent JUCE slider (interaction only).
- Waveform drawing reduced to 24–32 segments (was 64) for lower CPU.

## Geek / Plugin View
- Same plastic parts in the live viewer.
- Signal packet animation is O(points), no path flattening.
- Part-hardware underlay skips when geekReveal is near zero.
- Scope copies use 64 samples in viewer board path where padded.

## CPU notes
- No blur / no per-frame allocations in the hot paint path beyond stack arrays.
- Geek heavy layers gated by `geekReveal`.
