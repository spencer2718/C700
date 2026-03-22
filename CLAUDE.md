# CLAUDE.md — C700 Fork

## What this is
Fork of osoumen/C700 — an SPC700 (SNES sound chip) emulator VST plugin.
Goal: English translation + Linux build (for use in REAPER on Ubuntu).

## Fork goals
1. Translate Japanese comments and documentation to English
2. Build on Linux (original supports macOS + Windows only)
3. Minimal functional changes — this is a port, not a rewrite

## Planning docs
- `PRD.md` — product requirements, phasing, success criteria
- `TECH_PLAN.md` — implementation plan, milestones, adapter design
- Source-of-truth priority: PRD > TECH_PLAN > decisions > code > chat

## Architecture
- DSP core: C700Kernel, C700DSP, C700Driver, snes_spc/ (portable C++, no platform deps)
- Plugin wrapper: C700VST.cpp (VST 2.4 SDK)
- GUI: C700GUI.cpp + VSTGUI (vendored, old version, no Linux support)
- Platform-specific: macOSUtils.mm, C700_CocoaViewFactory.mm, cocoasupport.mm
- Vendored libs: vstgui/, libpng/, zlib/, CoreAudio/, snes_spc/

## Key files
- C700Kernel.cpp/.h — main audio engine, voice management, MIDI processing
- C700DSP.cpp/.h — SPC700 DSP emulation (BRR decode, envelope, echo)
- C700Driver.cpp/.h — high-level driver (register writes, voice allocation)
- C700GUI.cpp/.h — full UI layout using VSTGUI
- C700VST.cpp/.h — VST2.4 plugin entry point
- brrcodec.cpp — BRR encode/decode
- snes_spc/ — Blargg's SPC emulation (standalone library)

## Build status
- macOS: Xcode project (C700.xcodeproj) — untested on fork
- Windows: VS2015 (vc2015/C700.sln) — untested on fork
- Linux: not yet supported — primary fork goal

## Conventions
- Commit and push at end of every pass
- Keep Japanese originals as comments during translation (delete after review)
- Don't modify DSP/audio core behavior — port only
