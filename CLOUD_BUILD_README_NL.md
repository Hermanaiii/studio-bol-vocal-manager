# Windows 11 / Cubase 13 build — Studio Bol Vocal Manager

This repository contains source code only. No compiled Windows plugin has been validated.

## Automated build with GitHub Actions
1. Upload this complete folder to a GitHub repository.
2. Open Actions → Build Windows VST3 → Run workflow (or push to main/master).
3. Confirm the workflow completes successfully; a failed JUCE download means no build.
4. Download the `StudioBolVocalManager-Windows-x64-VST3` artifact.
5. Extract the VST3 bundle directory to `C:\Program Files\Common Files\VST3\`.
6. Restart Cubase and rescan plugins.

A successful compile is not proof of audio quality or Cubase compatibility. Test before using on important sessions.

## Known limitations
- Full mixed songs are analysed as a whole; vocal separation is not implemented.
- Matching is approximate, not reverse engineering of original settings.
- Reference audio is not embedded in the Cubase project.
- DSP has not passed formal real-time, audio-quality or Cubase validation.
