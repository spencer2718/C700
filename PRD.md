# PRD — C700 Linux Port

## Status
Draft v0.1

## 1. Purpose

Port **C700** to **Linux Ubuntu** for use in **REAPER**, preserving the existing SPC700/SNES DSP core while replacing the legacy plugin shell with a minimal JUCE VST3 wrapper using the generic parameter editor.

The implementation path is:

- **JUCE + CMake**
- **VST3 first**
- **headless/minimal-editor milestone before full GUI parity**

The DSP core should remain the source of truth for synthesis/emulation behavior. The wrapper, parameter layer, build system, and GUI are the parts intended to change.

---

## 2. Background / Problem

The current C700 codebase consists of:

- a portable C++ DSP/emulation core,
- a legacy **VST 2.4** plugin wrapper,
- an old vendored **VSTGUI** frontend,
- Windows/macOS-specific glue code and build assumptions.

This creates three porting problems:

1. the plugin shell is legacy and not suitable as a new Linux target,
2. the GUI stack is not a practical Linux path in its current form,
3. the build system is not modernized for cross-platform Linux development.

The core emulator logic is the main reusable asset. The shell and GUI should be treated as replaceable.

---

## 3. Product Goal

Deliver a Linux-native C700 build that:

1. loads in REAPER on Ubuntu,
2. produces correct audio from the existing DSP core,
3. accepts MIDI correctly,
4. exposes essential parameters via JUCE's generic editor on Linux,
5. preserves project/preset behavior as closely as practical,
6. results in a maintainable codebase for future cross-platform work.

---

## 4. Product Decision

### Chosen approach

**Port the plugin shell to JUCE, keep the DSP core intact, target VST3 first, and ship in phases.**

### Why this approach

- It isolates risk: wrapper first, GUI later.
- It preserves the most valuable part of the codebase: the DSP core.
- It creates a modern Linux plugin target without being trapped in VST2/VSTGUI legacy code.
- It gives a clean path to a minimal Linux build before committing to full GUI parity.

---

## 5. Non-Goals

For the initial Linux port, the project will **not**:

- preserve the original VST2 wrapper on Linux,
- port the old VSTGUI code in place,
- target all plugin formats on day one,
- guarantee full visual parity before functional parity,
- redesign the DSP engine,
- add new synthesis features unrelated to porting,
- optimize for every Linux DAW before REAPER works,
- build a custom GUI for this fork (the generic JUCE parameter editor is sufficient for the target user).

---

## 6. Primary User

**Primary user:** a Linux-based REAPER user who wants real C700 functionality on Ubuntu, not a rough approximation or a separate emulator workflow.

Secondary users may include developers who want a maintainable modern codebase for future cross-platform plugin work.

---

## 7. Scope

## Phase 0 — Feasibility / Wrapper Bring-Up

Goal: prove that the DSP core can run inside a JUCE plugin on Linux.

Deliverables:

- JUCE/CMake project builds on Ubuntu
- VST3 plugin loads in REAPER
- plugin instantiates without crashing
- engine initializes and renders audio
- basic MIDI note input reaches the engine
- minimal parameter exposure works
- state save/load stub exists

This phase may use a generic or minimal editor.

## Phase 1 — Minimal Functional Linux Build

Goal: produce a usable early Linux build without full GUI parity.

Deliverables:

- stable VST3 in REAPER on Ubuntu
- essential parameters exposed
- preset/state serialization working
- sample or instrument loading path functional
- regression-tested audio parity where feasible

This is the first externally usable milestone.

## Phase 2 — Full Parameter Exposure + SPC Export

Goal: Expose all essential C700 parameters and enable SPC/SMC recording.

Deliverables:

- Instrument parameters: ADSR, volume L/R, echo, base key, loop
- Global parameters: echo delay/feedback/FIR, ARAM budget display
- Interactive sample loading via file dialog
- SPC/SMC recording and export
- Cleanup: remove test scaffolding, production-ready build

## Phase 3 — Optional: Custom GUI (deferred indefinitely)

Only pursue if this fork is distributed publicly and users need a visual editor.
Not planned for the current target user (single Linux REAPER composer).

---

## 8. Requirements

### 8.1 Audio engine integration

The Linux port must:

- preserve the existing DSP core with minimal logic changes,
- connect engine initialization/reset to plugin lifecycle methods,
- connect audio rendering to the plugin audio callback,
- connect MIDI input to the engine,
- support deterministic reset/reinitialization behavior.

### 8.2 Parameter system

The Linux port must:

- expose a stable initial parameter set,
- separate host-visible automation state from internal engine state,
- support automation for the MVP control surface,
- support state save/load through the new wrapper.

### 8.3 Plugin format

Initial required format:

- **VST3 on Linux**

Deferred:

- LV2
- CLAP
- standalone app

### 8.4 GUI

Initial milestone may use:

- no custom GUI,
- a minimal JUCE editor,
- or a generic host-facing parameter editor.

Full editor parity is deferred to Phase 2.

### 8.5 Build system

The project should use:

- **CMake**
- a modern JUCE layout
- reproducible Ubuntu build instructions
- repo-local instructions that Claude Code can follow deterministically

---

## 9. Alternatives Considered

### A. Modernize VSTGUI in place

Rejected as the primary path. Too much migration risk for too little long-term benefit.

### B. DPF

Reasonable fallback if JUCE becomes too heavy or too GUI-opinionated, but not the first choice.

### C. CLAP-first

Deferred. Interesting, but not the simplest first path for a Linux bring-up.

### D. Headless-only long term

Rejected. Good first milestone, insufficient final product.

---

## 10. Success Criteria

### Phase 0 success

- plugin builds on Ubuntu
- plugin scans in REAPER
- plugin can be instantiated
- MIDI note-in produces audible output
- no immediate crash on load/unload

### Phase 1 success

- stable VST3 Linux build usable in REAPER
- basic preset/state load-save works
- parameter automation works for the MVP surface
- basic sample/instrument workflow works
- reproducible build instructions exist

### Phase 2 success

- All essential instrument parameters automatable from REAPER
- Echo/reverb configurable
- ARAM budget visible
- Samples loadable via file dialog (no hardcoded paths)
- SPC file export works
- User can compose SNES-legal loops entirely on Linux

---

## 11. Risks / Known Hard Problems

### 11.1 GUI rewrite cost

The DSP is easier than the editor. Recreating the existing frontend behavior in JUCE is real work.

### 11.2 Linux GUI edge cases

Linux plugin GUIs can behave differently across host/windowing environments. REAPER on Ubuntu is the target environment, not universal Linux perfection.

### 11.3 State/preset migration

Legacy projects or presets may depend on assumptions embedded in the old wrapper or GUI.

### 11.4 File/dialog workflows

Sample loading, path handling, drag/drop, and browser behavior may need explicit redesign.

### 11.5 Scope drift

This can easily become “rewrite C700.” It must remain a **port**, not a speculative redesign.

---

## 12. Build / Environment Assumptions

Target environment:

- Ubuntu desktop
- REAPER for Linux
- modern CMake toolchain
- modern GCC or Clang
- JUCE integrated as a dependency/submodule/vendor path per implementation choice

The implementation must document exact dependencies and commands in a reproducible way.

---

## 13. Open Questions

- What is the minimum parameter surface required for a useful headless build?
- Which preset/state behaviors are mandatory for parity?
- Which parts of the current GUI are essential vs nice-to-have?
- What is the least risky sample-loading path for Phase 1?
- Should LV2 be a formal later target or stay optional?
- What compatibility promises do we want to make for legacy sessions/presets?

---

## 14. Proposed First Milestone

**Milestone M1: Linux Headless VST3 Bring-Up**

Deliver:

- JUCE/CMake skeleton
- DSP core linked
- basic MIDI in
- stereo audio out
- minimal parameter/state support
- verified load and playback in REAPER on Ubuntu

This is the correct first milestone because it retires the main architectural question first:

**Can the existing C700 engine run cleanly inside a modern Linux plugin shell?**

---

## 15. Source-of-Truth Policy

Priority order:

1. `PRD.md`
2. `TECH_PLAN.md`
3. implementation decisions log
4. code/tests
5. incidental chat history

If implementation reveals that the PRD is wrong, update the PRD before normalizing drift in code.
