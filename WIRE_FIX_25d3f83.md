# Wire fix on 25d3f83 (0.6.4 free grid)

The previous pass edited an older commit. This pass edits
`25d3f83df63ca17923e3271ff332b0ba7f908967`.

## Why Cut wire placed a part
Menu ids 6001 and 6002 are inside the modular-part range (`6000` + piece, 16 pieces).
The result handler armed a modular part before it ever reached the wire commands.
Plug into / Cut wire are now 7101 / 7102 and are handled before that range.

## Why every cable went to the motherboard
`repairParents()` read a missing parent as 0 and wrote 0 back whenever the feeder
was not an occupied bay. `placeAtGrid` (the 0.6.4 path) also stored `parent -1`
and `wiredInto -1`, so the repair pass snapped each new part onto bay 0.
Invalid parents now stay unwired. The first effect plugs into the motherboard.
The next effect relays from the selected live stage, otherwise the chain tail.
Keys and sound sources feed the motherboard. A dial dropped on an effect is a
control ribbon (`satellite`), not a DSP stage. Plug into rejects a loop.

## Cut wire
Opens that cable and deletes anything whose parent or `wiredInto` leads through
the cut part. The cut part stays, unwired. No new part is created.
`WireGraph::rebuildOrder` no longer appends unwired stages, so `wireOrder` does
not grow.

## Grid, not templates or pegs
Randomize Peg is gone from the menu. Builder wires walk shell slots past the old
template `slotCount`, so a free-grid part still draws its real feeder.

## Board and display
Creating the motherboard also creates a DISPLAY with `lockToBoard`. Dragging or
corner-resizing the board moves and resizes that display. Tap screens are not
locked.

## Geek
Signal path and layout detail use each part's `gx/gy/gw/gh` footprint instead of
the retired template bay. A cut is a short stub, not a trace home to the board.
