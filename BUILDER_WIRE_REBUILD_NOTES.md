# Builder wire rebuild 0.5.3

## Wire graph
- `Source/WireGraph.h` rebuilds DSP order from UI widgets (chain parent + slot index).
- Order is stored on `uiState.wireOrder` and consumed by `KyotoAudioProcessor::rebuildActiveSlots`.
- Screen / wave widgets get `tapAfter` = last DSP slot before them so later effects are not implied on earlier screens.

## Dial on effect
1. Place an effect on a bay (PLACE / armed FX).
2. Arm a dial or fader.
3. Click the **same** effect bay (or any occupied effect bay).
4. Popup asks what to control: Mix %, Amount, Decay, Time, Hz, Strength, Feedback, etc. (by effect family).
5. A satellite dial is created bound to that parameter (`bindFx` + `bindSuffix`).

## Geek / Plugin View
- Signal path polyline with stage badges and animated packet.
- Layout detail: satellite dial rings, screen TAP@N labels, bay outlines.
- Existing internals / x-ray retained.

## Builder placement
- Occupied bay + dial → bind flow (no longer hard-rejects).
- Status text guides chaining and dial binding.
