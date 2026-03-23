# CLAUDE.md — C700 Linux Fork

## What this is
Linux fork of osoumen/C700 — SPC700 (SNES sound chip) emulator VST plugin.
Original: macOS/Windows VST2 + VSTGUI. Fork: Linux VST3 + JUCE wrapper/editor.

## Fork goal
Make C700 usable on Linux Ubuntu in REAPER while preserving the original DSP/emulation behavior. The current phase includes a JUCE reconstruction of the original bitmap UI.
The DSP core is the source of truth for all audio/emulation behavior.

## Planning docs
- `PRD.md` — product requirements and phasing
- `TECH_PLAN.md` — implementation milestones and adapter design
- `docs/decisions/active.md` — running decision log
- Source-of-truth priority: PRD > TECH_PLAN > decisions > code > chat

## Architecture
- DSP core: C700Kernel, C700DSP, C700Driver, snes_spc/ (UNTOUCHED — portable C++)
- Plugin wrapper: src/plugin/PluginProcessor.cpp (JUCE VST3)
- Adapter: src/plugin/C700Adapter.cpp (thin bridge: JUCE <-> C700Kernel)
- Platform stubs: commusb/ControlUSB.h (Linux no-op), DataBuffer.cpp, C700Kernel.cpp (#ifdef __linux__)
- GUI: JUCE custom editor. M8 bitmap-based reconstruction is in progress; shell, selectors, and the main per-slot edit panel exist, while waveform/recording overlays and one slot-switch text-field bug remain.

## Build
```bash
scripts/build-install.sh    # build + install to ~/.vst3/
```
Full instructions: docs/notes/build_ubuntu.md

## Key files
- src/plugin/PluginProcessor.cpp — JUCE entry point, parameter layout, state save/load
- src/plugin/C700Adapter.cpp — sample loading (WAV/BRR), MIDI routing, engine lifecycle
- C700Kernel.cpp — main engine (DO NOT MODIFY unless platform porting requires it)
- C700DSP.cpp — SPC700 DSP emulation (DO NOT MODIFY)
- C700Properties.h — 91 custom properties (reference for parameter exposure)

## Conventions
- Commit and push at end of every pass
- Do not modify DSP core behavior — adapter/wrapper only
- Log decisions in docs/decisions/active.md
- Keep the adapter thin — no JUCE concepts in the engine

## Testing
Automated smoke tests (pluginval + ReaScript) live in the companion repo [snes_music](https://github.com/spencer2718/snes_music).
Full test harness is defined in `snes_music/docs/planning/TEST_HARNESS_PRD.md` — deferred until UI is stable for daily use.
