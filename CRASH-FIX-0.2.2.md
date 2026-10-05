# KyotoSpxrit 0.2.2 crash/build fix

## Windows host crash
The previous parameter layout contained duplicate APVTS parameter IDs:
- FX effects use `mix0` ... `mix199`
- oscillator controls also used `mix1`, `mix2`, `mix3`

Those IDs are now unique. Oscillator mix parameters are:
- `oscMix1`
- `oscMix2`
- `oscMix3`

This is important because JUCE requires each APVTS parameter to have a unique parameter ID.

## macOS
The project uses apostrophe-free CMake target/output names and the GitHub Actions macOS job explicitly configures a universal `x86_64;arm64` Xcode build and builds the two VST3 targets directly.

## Branding
The project is fully branded `KyotoSpxrit` / `KyotoSpxrit FX`; no `Kyoto's Dream` or `KyotosDream` target names remain.
