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

### Audio output status
**Status:** Plugin builds and initializes without crashing. No audible output yet because the engine requires BRR sample data to be loaded into instrument slots before it can produce sound. This is expected — sample loading is M4 scope. The engine has built-in preset waveforms (samplebrr.h: sine, square, pulse) that could be wired up in a future step to verify audio path.

### Files excluded (per task spec)
- All GUI files: C700GUI.*, C700Edit.*, RecordingSettingsGUI.*, VSTGUI controls, GUIUtils.*
- Platform-specific: macOSUtils.mm, C700_CocoaViewFactory.*, CoreAudio/*
- Old plugin wrappers: C700VST.cpp, C700.cpp
- File format loaders: AudioFile.*, XIFile.cpp, SPCFile.*, RawBRRFile.*, PlistBRRFile.*
- commusb/ControlUSB.cpp (Linux stub in header instead)
- Utilities not needed by core: EfxAccess.*, czt.*, WaveView.*, noveclib/*
