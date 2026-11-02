# Development status — Studio Bol Vocal Manager v0.8-dev

## Branding update
- Product name, plugin identifier, CMake target, editor title and CI artifact renamed to Studio Bol Vocal Manager.
- Windows build workflow now targets StudioBolVocalManager_VST3.

## Current prototype features
- Import WAV/AIFF/MP3 reference audio (mono/stereo, up to 20 minutes).
- Analyse broad spectral balance and RMS/peak/crest factor.
- Estimate six-band EQ corrections and compressor starting values.
- Adjustable Match Amount, Compression, De-Esser, Saturation and Output controls.
- Save/restore calculated EQ and compressor values in plugin state.

## Not implemented / not validated
- Neural source separation is not implemented. A full stereo master is analysed as a full mix; instruments affect the reference profile. Use an isolated vocal stem.
- This does not recover the reference mix engineer's original plugin parameters; matching is approximate.
- No compiled Windows VST3 is included. No Windows build or Cubase 13 validation has occurred.
- DSP remains experimental; do not use on irreplaceable sessions without rendering and checking results.
- Reference audio is not embedded in the Cubase project. Reimport the reference to calculate a new match after reopening.

## Build blocker
A configure/build attempt in this environment could not fetch JUCE 8.0.6 because github.com was not resolvable. No binary has been produced.


## v0.8 engineering update
- Corrected the saturation transfer curve to preserve unity small-signal gain. The previous normalisation by tanh(drive) could create a substantial level increase on quieter vocal material.
- CMake now accepts an optional local JUCE source checkout via `STUDIO_BOL_JUCE_SOURCE_DIR`, while retaining the pinned JUCE 8.0.6 FetchContent fallback. This makes offline/reproducible builds possible when JUCE is supplied locally.
- No VST3 binary has been produced or validated in Cubase. Full-mix vocal isolation remains unimplemented.


## v0.9 engineering update
- Hardened the asynchronous reference-file chooser callback with `juce::Component::SafePointer`. Closing the editor while the native file dialog is open no longer leaves a callback that dereferences a destroyed editor.
- Documented that reference decoding/analysis is currently synchronous and may temporarily block the UI for long files.
- No compiled VST3 was produced; JUCE is not present locally and network access to fetch it is unavailable in this build environment. No Cubase validation has occurred.

## v0.10 engineering update
- Changed reference-file analysis to stream the audio reader in 2048-sample FFT windows with 50% overlap. This avoids allocating a full 20-minute stereo reference in RAM (which could exceed 1 GB) during import.
- The streaming reader uses a bounded two-channel window buffer and computes RMS, peak, and six-band spectral energy incrementally.
- Re-ran CMake configure with the available GNU C/C++ compiler. Configuration still stops at JUCE FetchContent because this environment cannot resolve github.com. No plugin binary has been produced and no VST3/Cubase validation has occurred.
