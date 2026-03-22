# Decision Log

## 2026-03-21 — M1 Initial Decisions

### JUCE integration method
**Decision:** Git submodule at `external/JUCE/`.
**Why:** Pinned version, no external fetch dependencies, simple `git submodule update --init`.

### Build system
**Decision:** Root-level CMakeLists.txt using `juce_add_plugin()`.
**Why:** Standard JUCE CMake workflow. No IDE project files as source of truth.

### Plugin format
**Decision:** VST3 only for now.
**Why:** Per PRD — VST3 first, other formats deferred.

### Source layout
**Decision:** New JUCE wrapper code lives in `src/plugin/`. Existing C700 source files remain untouched in the repo root for now.
**Why:** Avoids disrupting the legacy codebase during bring-up. DSP core integration (M2) will determine how sources get linked.

### Editor
**Decision:** Minimal placeholder editor for M1. Full GUI is Phase 2 / M6.
**Why:** Per PRD and TECH_PLAN — headless/functional first, GUI later.

---

## 2026-03-21 — M2 DSP Core Integration

### Core source subset
**Decision:** 17 core .cpp files from repo root + 7 snes_spc files + 3 file-generation files + 1 USB stub + 1 platform utility. Total: 29 source files added to the build.
**Why:** This is the minimum set needed for the audio engine to compile and initialize. File generation (SmcFileGenerate, SpcFileGenerate, PlayingFileGenerateBase) was included because C700DSP's register logger depends on them.

### Adapter boundary
**Decision:** Thin adapter at `src/plugin/C700Adapter.cpp/.h`. JUCE PluginProcessor owns lifecycle; adapter translates between JUCE MidiBuffer and C700Kernel's Handle* methods; engine renders via `C700Kernel::Render()` which already outputs float stereo.
**Why:** Per TECH_PLAN §6. Keeps JUCE concepts out of the engine.

### USB hardware (commusb/) — Linux stub
**Decision:** Added `#elif defined(__linux__)` stub class to `commusb/ControlUSB.h`. SpcControlDevice.cpp compiles against the stub. All USB methods are no-ops returning false/0.
**Why:** USB hardware communication (gimic SPC module) is not available on Linux. The DspController already handles the `mIsHwAvailable == false` path gracefully, falling back to software emulation.

### Platform-specific code found and fixed
- **ControlUSB.h** — macOS/Windows only. Added Linux no-op stub class.
- **MidiDriverBase.h** — Redefined `uint8_t`, `uint16_t`, `uint32_t` from scratch, conflicting with `<cstdint>`. Replaced with `#include <cstdint>` + kept only `sint16_t` typedef.
- **C700Kernel.cpp** — `#if MAC` / `#else <Shlobj.h>` for getPreferenceFolder/getDocumentsFolder. Added `#elif defined(__linux__)` using `$HOME/.config/C700/` and `$HOME/Documents/`.
- **DataBuffer.cpp** — File I/O used macOS CoreFoundation or Windows API. Added `#elif defined(__linux__)` using standard fopen/fwrite/fread.
- **DataBuffer.h** — Missing `<cstddef>` for NULL.
- **Multiple .cpp files** — Missing `<cstring>`, `<cmath>`, `<cstdio>` includes (previously provided implicitly by platform headers or precompiled headers).
- **PATH_LEN_MAX** — Was defined only in the Windows .vcxproj. Added as `target_compile_definitions(PATH_LEN_MAX=1024)` in CMakeLists.txt.
- **GUIUtils.cpp path functions** — `getFileNameParentPath()` etc. lived in GUI code with VSTGUI dependency. Created standalone `src/platform/PathUtils.cpp` with Linux implementations.

### Audio proof-of-life
**Decision:** Call `C700Kernel::SelectPreset(1)` ("Testtones") in `C700Adapter::init()` to load 5 built-in BRR waveforms (sine, square, 3 pulse widths) into instrument slots 0-4. Only loaded once via `mPresetsLoaded` flag so host re-calling `prepareToPlay` doesn't reset instrument state.
**Why:** Simplest path to audible output without implementing full sample loading (M4). The built-in waveforms are compiled into `samplebrr.h`.

### Null pointer crash fix
**Found:** `C700Kernel::Reset()` and `Render()` had unguarded calls to `propertyNotifyFunc` which is NULL when no callback is registered (our JUCE wrapper doesn't set one). Added null checks at lines 137 and 1327.
**Why:** In the legacy VST2/AU wrappers, the callback was always set before Reset() was called. The JUCE adapter doesn't need property change notifications yet.

### Files excluded (per task spec)
- All GUI files: C700GUI.*, C700Edit.*, RecordingSettingsGUI.*, VSTGUI controls, GUIUtils.*
- Platform-specific: macOSUtils.mm, C700_CocoaViewFactory.*, CoreAudio/*
- Old plugin wrappers: C700VST.cpp, C700.cpp
- File format loaders: AudioFile.*, XIFile.cpp, SPCFile.*, RawBRRFile.*, PlistBRRFile.*
- commusb/ControlUSB.cpp (Linux stub in header instead)
- Utilities not needed by core: EfxAccess.*, czt.*, WaveView.*, noveclib/*

---

## 2026-03-21 — M3 MIDI + Minimal Parameters (start)

### MIDI routing verification (code analysis)
- **Note-on/off**: JUCE MidiBuffer → `C700Adapter::process()` → `C700Kernel::HandleNoteOn/Off()` → enqueues `MIDIEvt` → `MidiDriverBase::ProcessMidiEvents()` (called per output sample in `C700Driver::Process()`) → voice allocation + DSP key-on/off.
- **Sample-accurate timing**: `toWaitCycles` = JUCE `samplePosition`; decremented once per output sample in `ProcessMidiEvents()`. Events fire at the correct sample offset.
- **Pitch**: Computed in `handleNoteOnDelayed()` as `pow(2., (note - vp.basekey) / 12.) / 32000 * rate * 4096`. Different MIDI notes produce different pitches.
- **Polyphony**: `DynamicVoiceAllocator` manages up to 16 voices (`kMaximumVoices`); default limit configurable via `SetVoiceLimit()` (default 8). Voice stealing uses oldest-first or same-channel modes.
- **Program change**: `handleProgramChange()` updates `mChStat[ch].prog` → subsequent notes use `mVPset[prog]` instrument. With Testtones preset: program 0=sine, 1=square, 2-4=pulse widths.
- **Note-on gating**: `isPatchLoaded()` checks `mVPset[prog].hasBrrData()` before allowing key-on. Only slots with loaded BRR data produce sound.

### MIDI features confirmed working (by code path)
- Note-on/off with velocity
- Pitch bend (2-semitone default range via `DEFAULT_PBRANGE`)
- CC: volume (CC7), expression (CC11), pan (CC10), modulation (CC1), portamento (CC5/65)
- Program change
- All notes off / all sound off / reset all controllers
- Sustain/damper (CC64)

### Note release fix — uniqueID matching
**Bug:** Notes sustained indefinitely — note-off events never released voices.
**Root cause:** `C700Adapter::process()` passed `uniqueID = 0` for all note-on and note-off events. The voice allocator (`DynamicVoiceAllocator::ReleaseVoice`) matches note-off to voices by uniqueID. With all IDs = 0, the allocator couldn't reliably find the right voice to release after multiple notes were played.
**Fix:** Use `uniqueID = noteNumber + channel * 256` (matching the legacy C700VST.cpp behavior at line 520/527). This gives each note/channel combination a unique ID that matches between note-on and note-off.
**ADSR analysis:** Default ADSR values are AR=15, DR=7, SL=7, SR1=0. SR1=0 means infinite sustain (no decay during note-on) which is correct — release happens via SPC700 KOF (key-off) register at ~8ms. The `sustainMode=true` + `sr2=31` + `mFastReleaseAsKeyOff=true` defaults correctly trigger a DSP key-off on note-off.

### Program selection parameter (VST3 workaround)
**Problem:** VST3 does not forward raw MIDI program change messages to the plugin's processBlock MIDI buffer (unlike VST2). REAPER's Bank/Program Select lane sends the event, but JUCE's VST3 wrapper swallows it. `HandleProgramChange()` was never called.
**Fix:** Exposed "Program" (int 0-127) as a JUCE automatable parameter via `AudioProcessorValueTreeState`. In `processBlock()`, the parameter value is compared against the last-seen value; on change, `mAdapter.setProgramForAllChannels(newValue)` calls `HandleProgramChange` on all 16 channels. The MIDI `isProgramChange()` handler remains as a belt-and-suspenders path for hosts that do pass it.
**UX benefit:** Program is now automatable in REAPER's envelope lanes and visible in the generic editor. Users can switch between test tones (0=sine, 1=square, 2-4=pulse) via the parameter control.

### Volume parameter
Added "Volume" (float 0.0-1.0, default 1.0) as a JUCE parameter. Applied as a gain multiplier on the output buffer after engine rendering. Proves the parameter system works and is useful for mixing.

### Editor switch to GenericAudioProcessorEditor
Replaced the M1 placeholder editor with JUCE's `GenericAudioProcessorEditor`, which auto-generates UI controls for all exposed parameters. Custom GUI is deferred to M6.

### State save/load
Implemented `getStateInformation` / `setStateInformation` using APVTS XML serialization plus kernel chunk serialization. Parameters (Program, Volume) and all loaded instrument data survive project save/load in REAPER.

### Install path
`scripts/build-install.sh` — builds Release and copies .vst3 to `~/.vst3/`.

---

## 2026-03-22 — M4 Sample Loading

### Sample loading path analysis
- **AudioFile.cpp**: Entirely platform-specific (macOS CoreAudio / Windows mmio). Not portable without a full rewrite.
- **RawBRRFile.cpp**: Mostly portable. File I/O had MAC/Windows split — added `#elif defined(__linux__)` using `fopen`/`fread`. The `.smpl` sidecar file parsing is already portable (standard C `fscanf`).
- **Decision**: Write a portable WAV loader directly in `C700Adapter.cpp` instead of porting `AudioFile.cpp`. The WAV format is simple enough to parse with standard C++ I/O.

### File formats supported
| Format | Status | Notes |
|--------|--------|-------|
| `.brr` | Working | Raw BRR with optional 2-byte loop point header + `.smpl` sidecar for instrument params |
| `.wav` | Working | PCM 8/16/24-bit, mono/stereo. Reads `smpl` chunk for loop points and base key. BRR-encoded via `brrencode()` |
| `.spc` | Deferred | SPCFile.cpp not yet ported (lower priority) |
| `.xi` | Deferred | XIFile.cpp not ported (niche format) |

### WAV loading implementation
Portable WAV loader in `C700Adapter::loadWAV()`:
1. Parses RIFF/WAVE chunks (fmt, data, smpl)
2. Supports PCM 8/16/24-bit, mono and stereo (mixed to mono)
3. Reads WAV `smpl` chunk for MIDI base key and loop points
4. Calls `brrencode()` to convert PCM→BRR
5. Sets instrument params (rate=source sample rate, basekey from smpl or default 60, ADSR defaults)
6. Registers via `C700Kernel::SetBRRData()` + `RefreshKeyMap()`

### State save/load
Extended `getStateInformation` / `setStateInformation`:
- Saves APVTS parameters (XML binary) + kernel instrument data (chunk format) in a combined blob
- On load, restores parameters first, then deserializes instrument slots via `RestorePGDataFromChunk()`
- Loaded samples survive REAPER project save/reload

### Test sample
Auto-loads `~/.config/C700/test_sample.wav` into slot 5 on init (temporary for M4 verification). Set Program=5 in the generic editor to play it.

### Platform issues fixed
- **RawBRRFile.cpp**: Added `#elif defined(__linux__)` file I/O using `fopen`/`fread`. Added `<cstring>` include.
- **AudioFile.cpp**: Not ported — too platform-entangled. Replaced with portable WAV loader in adapter.

### Bug fix: state restore produced no audio
**Root cause**: Two issues:
1. **Chunk format mismatch**: `getStateData` wrote raw property chunks directly, but the kernel's `RestorePGDataFromChunk` expects property data nested inside a sub-reader. Fixed to match the legacy C700VST.cpp format: each program's properties are wrapped in an outer `CKID_PROGRAM_DATA + pgnum` chunk, and restore creates a sub-`ChunkReader` for the inner data before calling `RestorePGDataFromChunk`.
2. **Call ordering**: `setStateInformation` may be called before `prepareToPlay`. If engine state is restored before init, then `prepareToPlay` → `init()` → `Reset()` would reset the DSP after instruments were loaded. Fixed by deferring engine state restore: `setStateInformation` stores the data as pending, and it's applied either immediately (if sample rate is already set) or in `prepareToPlay` (after init completes).

### Bug fix: sine wave corrupted after loading sample into slot 5
**Root cause**: `C700Kernel::SetBRRData()` internally calls `SetPropertyValue(kAudioUnitCustomProperty_Loop, ...)` which uses `mEditProg` to target the instrument slot. `mEditProg` defaults to 0, so loading a sample into any slot via `SetBRRData(data, size, slot=5, ...)` would set the loop flag on slot 0 (the sine wave) instead of slot 5. This changed the sine wave's loop behavior, producing a short pop instead of a sustained tone.
**Fix**: Save/restore `mEditProg` via `GetPropertyValue/SetPropertyValue(kAudioUnitCustomProperty_EditingProgram)` around `SetBRRData` calls in both `loadBRR()` and `loadWAV()`.

---

## 2026-03-22 — Scope clarification

### No custom GUI
**Decision:** This fork will NOT include a custom GUI rewrite. JUCE's GenericAudioProcessorEditor is sufficient for the target user (single Linux REAPER composer). Custom GUI is deferred indefinitely.
**Why:** The full VSTGUI->JUCE GUI port would take as long as everything else combined, and the target user doesn't need it. Parameters exposed via the generic editor are automatable and functional.

### Remaining milestones
**Decision:** M5 (parameters + echo + ARAM) then M6 (sample loading + SPC export). No M7.
**Why:** After M6, the fork provides the complete C700 experience on Linux without the bitmap GUI.

---

## 2026-03-22 — M5 Essential Parameters

### Parameter architecture
**Decision:** Per-instrument parameters (ADSR, volume, echo, base key, loop, rate) are "soft" — they reflect whichever program slot is selected. On program change, values sync FROM the engine to JUCE parameters. On user edits, values push TO the engine. A `mSyncingFromEngine` flag prevents feedback loops.
**Why:** The C700 kernel uses `mEditProg` to route property get/set to the correct slot. JUCE parameters are global singletons. The sync approach bridges the gap without duplicating 128 slots worth of parameters.

### Global parameters exposed
Main Volume L/R (-128..127), Echo Delay (0..15), Echo Feedback (-128..127), Echo Volume L/R (-128..127). These push to the kernel every processBlock via `SetParameter()` — cheap and avoids tracking individual changes.

### ARAM budget
Exposed as a read-only float parameter "ARAM Used (bytes)" (0..65536). Updated every processBlock from `GetPropertyValue(kAudioUnitCustomProperty_TotalRAM)`. Shows in REAPER's generic editor.

### Cleanup
- Removed `~/.config/C700/test_sample.wav` auto-load from init
- Removed `mPresetsLoaded` flag logic tied to test sample (kept for preset-vs-state guard only)
- SelectPreset(1) test tones remain as default initial state

---

## 2026-03-22 — M6 Interactive Sample Loading

### Custom editor with file dialog
**Decision:** Replaced `GenericAudioProcessorEditor` return with a custom `C700AudioProcessorEditor` that has a "Load Sample" button at the top and embeds the generic editor below for all parameters.
**Why:** JUCE's generic editor can't trigger file dialogs. A minimal custom editor provides the file loading UX while preserving all parameter controls.

### File dialog approach
Uses `juce::FileChooser::launchAsync()` with `*.wav;*.brr` filters. Remembers last browse directory per session. On file selection, calls `mAdapter.loadSampleToSlot(currentProgram, path)` and syncs parameters from engine so the editor reflects the loaded sample's settings (base key, ADSR, etc.).

### SPC recording/export
**Implementation:** Wired the kernel's existing recording infrastructure to JUCE. The path is:
1. User loads `playercode.bin` via "Load Player Code" button (required — contains SPC700 playback code)
2. User sets Record Start/Loop/End beat positions via JUCE parameters
3. User clicks "Export SPC..." to choose output directory and arm recording
4. User plays through the song region in REAPER
5. When playback crosses the end beat, the engine's `RegisterLogger` captures all DSP writes and `SpcFileGenerate` writes the .spc file

**Transport feeding:** `processBlock` reads JUCE `AudioPlayHead` position (tempo, PPQ, isPlaying) and feeds it to the kernel via `SetTempo/SetCurrentSampleInTimeLine/SetIsPlaying`. This is how the engine knows when to trigger recording start/end.

**Blocker — playercode.bin:** The SPC player code binary is not included in the repository. It must be obtained from the original C700 distribution site (picopicose.com). Without it, SPC export is disabled (button grayed out with status message). The player code contains the SPC700 assembly that plays back the register log inside the .spc file.

**Parameters added:** Record Start Beat, Record Loop Beat, Record End Beat (float, 0-10000, step 0.01).
