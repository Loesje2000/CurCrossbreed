# CurCrossbreed status

## Current release

- Version: **1.0.1**
- Formats: AU, VST3, Standalone
- Last updated: 2026-10-23

## 2026-10-23 transport update

The TapeBox-lite transport no longer applies an independent random delay offset
to every sample. Its flutter irregularity now uses a centered, correlated
one-pole state with a 35 Hz characteristic, preserving the existing TapeBox-lite
wow/flutter structure while avoiding sample-rate broadband phase noise and the
associated misleading low-frequency FFT energy.

Release validation and installation completed on 2026-10-23:

- Release AU, VST3, and Standalone build succeeded.
- AU and VST3 bundles were installed to the user-local plugin folders.
- Build and installed bundle signatures verified successfully.
- AU and VST3 installed metadata report version 1.0.1.
- `auval -v aufx CRCB Manu` passed.
- Strictness-5 `pluginval` passed. The optional nested VST3 validator was
  skipped because no validator path is configured.
