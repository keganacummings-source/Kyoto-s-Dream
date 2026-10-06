# KYOTRIPPAH update log

Append new entries at the top. Older revision notes were folded in here so the loose `*_NOTES.txt` files could be removed. Do not add a new notes file for the next change — add a section to this log.

## 0.4.2 — Plugin Builder: display root, series wires, lego template

The motherboard is no longer an effect and it is no longer seated for you.

- Step 1 is the template. Hidden chassis modules are buttons you lego-click together (`Source/TemplateModules.h`). Each snapped module opens placement bays. Every module except the mainboard adds a subtle hardware colour (warmth, glue, width, air, tape, exciter, soft clip, vinyl, Haas). The mainboard only holds the display bay and inserts no effect.
- Step 2 puts a display on that bay (Scope, Peaks, CRT, or Meter). The display widget has no DSP slot.
- The first wire leaves the display. After that, click a part that is already placed and the next part's wire connects to it. Wires are parent → child, not a star from one bus.
- Several controls can hang on one effect: click the effect, then place dials, faders, or keys bound to Amount, Tone, Motion, Mix %, or Shape. They share that effect's DSP slot.
- Place a Mix % dial, then hang an effect on that dial. The dial is rebound to the new effect's mix, and the new effect is given a later DSP slot so its mix blends it onto the part the dial hangs from.
- Pro mode and SKIP TO BUILDER are removed. PLUGIN BUILDER always opens the guide. MODULES returns to the template without clearing the build. Workshop keeps the same click-then-place rule.
- Saves and catalog publishes keep `bricks`, `slotCursor`, `nextUid`, and each widget's `uid` / `link`. `module-rules.js`, `worker/module-rules.js`, and `JV_MODULE_RULES.js` all store those fields.
- Old board widgets migrate to a display and their dedicated hidden slot is turned off if nothing else uses it.

## 0.4.1 — UI refinement

- Theme registry is 42 stable presets. Theme IDs are serialization-safe and shared by the editor, machine builder, wave surfaces, and modular parts.
- Theme selection drives fonts as well as colours, popup menus, text editors, labels, sliders, FX catalog surfaces, and plugin-view text.
- Text layout was loosened to cut overdraw. Chat uses theme fonts and extra line spacing.
- PLUGIN VIEW no longer overlaps the account/status controls.
- DreamShare home is a dashboard: hero, catalog/threads split, side chat, responsive catalog cards.
- Chain builder controls sit in two compact action rows.
- DreamShare utility dropdown uses human-readable labels. The API action names are unchanged.

## Build fixes (CI)

- `ThemePalette` has no `accent2`. Theme fields use `pegHot` as the second accent.
- Range-for over string literals compared with `juce::String(key)`, not a bare `const char*`.
- Slot prefixes are `juce::String` (`"s01" + "type"` is not pointer arithmetic).
- A local named `cursor` shadowed `juce::Component::cursor` and was renamed.
- `PluginEditor.h` declares the catalog/thread/rail buttons.
- `PluginViewScreen.h` can see `kt::kFx` because the editor includes `FxCatalog.h` before it.
- Machine playground menu uses `PopupMenu::showMenuAsync` (JUCE 8 has no `PopupMenu::show()`).

## CPU and custom-effect bounds

- Slot parameters are snapshotted once per audio block. Stereo FX is one pass per slot.
- `rebuildActiveSlots()` walks only slots that are on. Empty sessions exit early.
- Active-slot count cannot exceed `kMaxSlots`.
- Delay memory is capped around 0.75 s per slot. Output safety is a soft bound, not a hard clip.
- A placed custom effect expands into primitive stages only (no nested custom-in-custom). Placement refuses when there are not enough free slots. FX Builder stays at 16 stages.

## FX Builder, chain utilities, catalog

- Each effect step stores Amount, Tone, Motion, Mix, and Shape. Labels change with the effect family.
- CHAIN BREAK and MASTER MIX are special stage types. RANDOMIZE and CLEAR sit in FX Builder.
- REMOVE and UNDO exist on both builders. Undo keeps up to 24 snapshots of processor state plus the FX stack.
- Built-in FX are searchable. The CUSTOM tab lists local saves and DreamShare effects. Clicking a remote effect loads it into FX Builder.
- Published effects share as `[KYOTRIPPAH_EFFECT:<id>]`. Chat and threads turn that token into a LOAD FX button.
- Network callbacks hold the editor with `juce::Component::SafePointer`.

## DreamShare home and worker

- Center surface is the catalog. Threads are a separate tab. Chat is the right rail. Friends/Online is a rail mode, not a thread view.
- Chat, threads, comments, DMs, and presence store the sender theme.
- Canonical worker is `worker.js`. Kyoto catalog rules live in `module-rules.js` and receive their dependencies as arguments.
- One router covers module list/get/publish/delete plus catalog and community aliases. `list_threads` loads its own feed.
- Publish accepts `kyoteppah-module-1` and `kyoteppah-effect-1`, keeps up to 16 steps, 32 slots, chain levels, and `machineDesign`.
- `module_get` bumps the download count. `module_delete` is admin-only and clears the index, the owner's list, and any community-instrument row.
- Compatibility copies (`WORKER_DREAMSHARE.js`, `JV_WORKER.js`, `JV_MODULE_RULES.js`, `worker/`) stay aligned with the canonical pair. Deploy `worker.js`.

## Machine design and plugin view

- Plugin View is the built machine full-screen, with BACK and GEEK only.
- New Machine picks 4:5, 1:1, 5:4, or freeform. RANDOMIZE MACHINE is collision-checked.
- `MachineDesign` (playground, body, parts, sockets, connections, macros) is stored on the module payload.
- Geek view shows the snapped lego modules when a template exists, otherwise the old shell internals.
