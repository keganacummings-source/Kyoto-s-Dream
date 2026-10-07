# 0.5.6 FL Studio close/minimize crash fix

## Root causes addressed
1. **Editor Timer still running** during destruction (30Hz painting widgets / chainLevels).
2. **PluginViewScreen Timer (was 60Hz)** kept calling into the editor after teardown (private Timer base made stopTimer inaccessible from outside).
3. **Viewport** still referenced `socialRail` / `catalogHolder` while they died.
4. **PopupMenu async callbacks** used raw `this` after the host destroyed the editor UI.
5. **parentHierarchyChanged** could construct a new PluginViewScreen while the editor was unparenting.

## Fixes
- `editorClosing` flag set first in destructor.
- `stopTimer()` on editor; `viewScreen->shutdownViewer()` then delete.
- Clear viewport viewed components; clear widgets (drops SliderAttachments); reset overlays.
- PluginViewScreen: public `shutdownViewer()`, 20Hz timer, no work when hidden / closing.
- Menu/modal callbacks capture `SafePointer` and bail if editor is gone or closing.
- `parentHierarchyChanged` skips work when closing or fully unparented.

## Test in FL
1. Open KYOTO / KYOTRIPPAH FX UI.
2. Close the plugin window (X) without removing the plugin.
3. Minimize FL / switch projects.
4. Re-open the UI — should not crash the host.
