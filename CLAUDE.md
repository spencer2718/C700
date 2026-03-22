# CLAUDE.md — C700 Linux Fork

## What this is
Linux fork of osoumen/C700 — SPC700 (SNES sound chip) emulator VST plugin.
Original: macOS/Windows VST2 + VSTGUI. Fork: Linux VST3 + JUCE generic editor.

## Fork goal
Make C700 usable on Linux Ubuntu in REAPER. No engine modifications, no full GUI rewrite.
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
- GUI: JUCE GenericAudioProcessorEditor (auto-generated from parameters). No custom GUI.

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
