# 0.5.7 Builder Guide

## Open
Right-click playground → last item **Guide**

## Files
- `Resources/BuilderGuide.html` — full HTML guide
- `Source/BuilderGuide.h` — embedded fallback (always available in the VST)
- Overlay: existing `HtmlOverlay` with improved tag→text stripping for readability

## Guide structure
1. Big picture / signal path
2. Quick start (7 steps)
3. Systems by importance (Placement → Wiring → Controls → Plugin View → Cosmetics → DreamShare)
4. Effect families table (8 tags, not 200 names)
5. Part types
6. Themes / plastic
7. Save / randomize / undo
8. Pitfalls

## Logic alignment (already true in code)
- Manual Put wire Into only (no auto-chain on place)
- Screens are visual taps (tapAfter)
- Cosmetics do not touch DSP
- Dial-on-effect binds family-relevant controls
