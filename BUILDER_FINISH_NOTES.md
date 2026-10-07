# Builder finish — wire, grid, locked screen

Pinned repo: 27eb76be33833a941f5fc73738ce2a0d7e72a26b

## Bugs fixed in Source/PluginEditor.cpp
- Cut wire / Plug into used menu ids 6001 and 6002, the same range as modular parts (6000 + piece index). Choosing Cut wire armed a new part. Those commands are now 7101 / 7102 and are handled before the part range.
- repairParents() treated a missing parent as 0 and wrote every cable back onto the motherboard. Invalid parents are now unwired (-1).
- The first effect plugs into the motherboard. Later effects relay from the selected part, otherwise the chain tail.
- Keys and sound sources feed the motherboard. A dial dropped on an effect stays a control ribbon (existing bind path).
- Cut wire opens that cable and removes anything relaying through it. It does not place a part.
- Plug into rejects a loop.
- Template combo and Randomize Peg are hidden. Placement stays on the grid cells (gx/gy/gw/gh).
- The motherboard owns a locked DISPLAY. Dragging or corner-resizing the board moves and resizes that display with it. Tap screens are not locked.

## Geek
PluginViewScreen traces follow the part's grid footprint. A cut is a short stub, not a wire home to the motherboard.

## Runnable bench
Open builder/index.html. Demo chain is DRIVE → DELAY → HALL.
Select DELAY, Cut wire: HALL leaves, footer stays on wireOrder 3.
Arm CHORUS, click an empty cell: it relays from DRIVE, not the board.
