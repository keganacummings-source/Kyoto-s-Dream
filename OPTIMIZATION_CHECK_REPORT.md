# KyotoSpxrit Homepage Optimization Check

## Fixed build error
- `Source/Emoji.h`: replaced the removed JUCE `Graphics::drawArc()` call with `Path::addCentredArc()` + `Graphics::strokePath()`.
- This directly addresses the Windows/MSVC and macOS/clang CI error reported in `logs_101753708232.zip`.

## Homepage performance changes
- DreamShare editor timer: **30 Hz -> 12 Hz**. The homepage animation remains active but no longer runs at audio-like UI frequency.
- Timer animation/repaint is now limited to the logged-in homepage (`tab == 0`). Builder/FX/plugin screens no longer receive the homepage animation repaint workload.
- Hidden waveform/stack widgets are no longer repainted by the homepage timer on non-home screens.
- Removed a duplicate `widgets -> setTheme()` pass in `applyTheme()`.
- Updated the DS scale font construction to the current `juce::FontOptions` API.
- Cleaned up floating-point geometry passed to integer JUCE draw APIs in the FX builder cards.

## Validation
- C++ brace balance checked for modified files.
- Confirmed no `Graphics::drawArc()` remains in the modified source.
- JavaScript syntax checks passed for the worker scripts.
- CMake configuration was attempted locally, but this environment cannot resolve GitHub, so JUCE 8.0.6 could not be fetched for a full native compile. The supplied CI logs independently identify the original JUCE compile blocker as `Emoji.h:93`.
