# TECH_PLAN — C700 Linux Port

## Status
Active as of 2026-03-22

## 1. Purpose

This document translates `PRD.md` into an implementation plan.

It is not a broad product document. It is an execution document for the PM + Claude Code workflow.

---

## 2. Technical Strategy

### Core decision

Keep the existing **DSP/emulation core** intact and wrap it in a new **JUCE VST3** shell.

### Explicit boundaries

**Keep / preserve:**
- SPC700 emulation
- BRR decode and sample logic
- echo/filter/voice logic
- internal synthesis/audio engine behavior
- existing portable C++ code where possible

**Replace / rewrite:**
- VST2 wrapper
- VSTGUI frontend
- platform-specific shell glue
- legacy project/build setup for plugin packaging

### Phase order

1. **headless or minimal-editor Linux VST3**
2. **state/preset and sample workflow stabilization**
3. **full parameter exposure + SPC export**
4. optional: custom GUI or format expansion (deferred indefinitely)

---

## 3. Repository Layout (target)

Recommended target layout:

```text
/PRD.md
/TECH_PLAN.md
/CLAUDE.md
/docs/
  decisions/
    active.md
  notes/
  parity/
/external/
  JUCE/
  libpng/
  zlib/
  snes_spc/
/src/
  core/
  plugin/
  gui/
  platform/
/tests/
  smoke/
  parity/
  fixtures/
/scripts/
  build/
  dev/

```

Notes:

- `src/core/` should contain preserved DSP/emulation logic or thin wrappers around existing source locations.
- `src/plugin/` should contain JUCE processor/state/parameter integration.
- `src/gui/` should contain the new JUCE editor.
- `src/platform/` should be minimized and only hold unavoidable platform glue.
- `docs/decisions/active.md` is the durable running decision log.

---

## 4. Milestones

### M1 — Build Skeleton + Plugin Bring-Up ✓

**Goal**

Produce a Linux VST3 that builds and loads in REAPER.

**Tasks**
- add JUCE to repo strategy (submodule/vendor/fetch)
- create CMake root build
- create JUCE plugin target
- compile a trivial plugin on Ubuntu
- confirm REAPER scans and loads it
- document build steps

**Exit criteria**
- cmake configure succeeds
- build succeeds on Ubuntu
- .vst3 artifact exists
- REAPER scans it
- plugin instantiates without crashing

### M2 — DSP Core Integration ✓

**Goal**

Connect the real C700 engine to the plugin shell.

**Tasks**
- identify portable DSP source subset
- isolate any platform-specific assumptions
- create adapter layer between JUCE processor and engine
- initialize/reset engine in plugin lifecycle
- render stereo output through plugin callback
- confirm note-driven sound generation

**Exit criteria**
- plugin produces audible output from the existing DSP core
- repeated load/unload does not crash
- sample rate and block size changes do not immediately break processing

### M3 — MIDI + Minimal Parameters ✓

**Goal**

Make the plugin musically usable in a minimal sense.

**Tasks**
- map JUCE MIDI input to engine note events
- expose minimum required parameters
- define parameter/state ownership boundary
- verify automation for exposed controls
- test basic note-on/note-off behavior in REAPER MIDI editor

**Exit criteria**
- MIDI note input produces expected pitch triggering
- note-off/release behavior works
- minimal parameters can be changed from host
- automation does not crash or corrupt state

### M4 — State / Preset / Sample Loading ✓

**Goal**

Make the headless build actually usable, not just audible.

**Tasks**
- implement getStateInformation / setStateInformation
- define MVP preset/state schema
- restore enough sample/instrument loading behavior for meaningful testing
- verify save/load in REAPER project sessions
- document known incompatibilities with legacy state if any

**Exit criteria**
- REAPER project save/reopen preserves working state
- essential sample/instrument configuration survives reload
- known limitations are documented explicitly

### M5 — Essential Parameters + Echo + ARAM

**Goal**

Expose the parameters needed for real sound design.

**Tasks**
- Expose per-instrument: AR, DR, SL, SR1, SR2, sustain mode, base key, volume L/R, echo enable, loop/loop point
- Expose global: echo delay (EDL), echo feedback (EFB), echo FIR coefficients, main volume L/R
- Expose read-only: total ARAM usage
- Remove auto-load test sample path from init()
- Remove hardcoded ADSR in WAV loader (use sensible defaults but allow override via params)

**Exit criteria**
- User can shape instrument envelopes from REAPER's generic editor
- Echo/reverb is audible and configurable
- ARAM usage is visible as a parameter

### M6 — Interactive Sample Loading + SPC Export ✓

**Goal**

Complete the workflow — load samples interactively, export hardware-compatible files.

**Tasks**
- File dialog for loading WAV/BRR into any slot
- SPC recording path: set record start/end, trigger recording, write .spc file
- SMC export if feasible
- Clean build, remove all debug scaffolding

**Exit criteria**
- User can load samples from REAPER's generic editor or a JUCE file dialog
- User can export .spc files for hardware playback
- Build is clean and documented

### M7 — Verification and Stability (Codex agent)

**Goal**

Adversarial audit and bug fixing by a separate agent. Verify SPC700 accuracy, fix crashes, test edge cases.

**Tasks**
- Replicate and fix the sample loading crash (occurs when replacing built-in test tones with WAV samples, ~3rd-4th load)
- Verify multi-bank / drum kit workflow (multiple samples on one channel via high/low key mapping)
- Verify SPC output is hardware-legal and ROM-insertable
- Verify state save/load across all parameter combinations
- Stress test: load/unload plugin repeatedly, rapid program changes, edge case MIDI
- Audit adapter code for memory safety (const_cast, buffer sizes, BRR encoder bounds)
- Document all bugs found and fixed

**Exit criteria**
- No known crash on sample loading
- Multi-bank drum kit works
- SPC output verified compatible with real hardware (or emulator parity confirmed)
- All findings documented in docs/decisions/

### M8 — UI Reconstruction (Claude Code)

**Goal**

Recreate the original C700 GUI in JUCE, matching the existing VSTGUI bitmap-based interface.

**Reference material:**
- images/ directory (gui_image.png, wave_settings.png, echo_settings.png, recorder_settings.png, general_settings.png)
- Original VSTGUI source: C700GUI.cpp, C700Edit.cpp, ControlInstacnesDefs.h, RecordingSettingsGUI.cpp
- graphics/ directory (PNG button/knob bitmaps, background images)

**Tasks**
- Recreate main panel layout from gui_image.png
- Waveform display view
- Channel activity indicators (1-16)
- Knob/slider controls matching original bitmap style
- Sample loading area (Load/Save/Export/Unload buttons)
- Echo settings panel
- Recording settings panel
- ARAM usage display

**Approach:**
- Work from screenshots as primary reference
- Read original VSTGUI source for control layout coordinates and parameter mapping
- Use JUCE LookAndFeel for bitmap-based knobs/buttons where practical
- Iterate with visual feedback (screenshot → edit → screenshot cycle)

**Exit criteria**
- Plugin editor visually resembles the original C700
- All essential controls are functional
- Workflow parity with original for core use cases

**Current implementation status (2026-03-22)**
- In progress.
- Completed so far:
  - processor/APVTS surface expansion for the original UI's main controls
  - fixed-size `536x406` editor shell
  - bitmap track selectors, bank selectors, slot rocker, and sample-management buttons
  - main per-slot edit panel for program name, key range, loop point, sample rate, toggles, ADSR, volume, and vibrato
  - hardware/playercode/ARAM status display
- Verified during M8:
  - a processor-layer note-reset regression was fixed; sustained playback now works correctly again
  - the remaining text-field issue is UI-only: slot-local writes affect only the intended voice, but focused text fields can display stale values after a slot change
- Remaining work:
  - waveform subsystem
  - echo settings / remaining advanced controls
  - recording settings overlay
  - final slot-switch text-field behavior fix and general UI hardening

---

## 5. First Implementation Slice

The first Claude Code slice should be intentionally narrow.

**Slice 1**

Create a trivial JUCE VST3 that builds and loads in REAPER on Ubuntu.

No DSP integration yet beyond placeholder audio pass-through or silence.

**Slice 2**

Link the C700 DSP core and produce audible output from one hardcoded/simple test path.

**Slice 3**

Accept MIDI note events and route them into the engine.

Do not start with GUI reconstruction.

---

## 6. Adapter Boundary

Define a thin adapter between JUCE and the existing engine.

**JUCE side owns:**
- plugin lifecycle
- audio callback entrypoint
- MIDI buffer ingestion
- parameters
- state blob serialization
- host-facing metadata
- GUI/editor lifecycle

**Engine side owns:**
- synthesis/emulation behavior
- sample decode/playback logic
- voice allocation/emulation internals
- echo/filter/audio generation
- internal timing as applicable

**Adapter layer responsibilities:**
- translate JUCE sample rate / block size init into engine init
- translate MIDI note/control events into engine commands
- translate engine audio output into JUCE buffers
- map host-visible parameters onto engine setters/getters
- provide reset/suspend safety

The adapter must be thin and testable. Avoid mixing JUCE concepts deep into the engine.

---

## 7. Parameter Strategy

Start with the smallest useful surface.

**Candidate MVP parameter groups:**
- output volume
- program/sample selection placeholder if practical
- pitch-related global control if already central
- panic/reset if useful for debugging

Do not try to expose every old UI control immediately.

The rule is:

> if a control is needed to prove audio + state + workflow, expose it now;
> otherwise defer it until GUI phase.

---

## 8. State Strategy

### Immediate goal

Get stable save/load behavior for the Linux wrapper.

### Recommendation

Use a JUCE-owned state object for host-visible parameters and wrap engine-specific serialized state inside it as needed.

Questions to resolve early:

- what legacy state format exists, if any?
- what must remain compatible?
- what can be Linux-port-only initially?

This should be logged in `docs/decisions/active.md` as soon as understood.

---

## 9. GUI Strategy

### Current approach

JUCE GenericAudioProcessorEditor — auto-generated from exposed parameters.
No custom GUI planned for this fork. All controls available via REAPER's generic editor and automation lanes.

### Future (deferred indefinitely)

Custom JUCE GUI only if the fork is distributed publicly and users need a visual editor.
Not planned for the current target user (single Linux REAPER composer).

---

## 10. Testing Strategy

### 10.1 Smoke tests
- plugin scans in REAPER
- plugin instantiates
- MIDI note in -> audible output
- no crash on play/stop
- no crash on plugin removal
- no crash on project reopen

### 10.2 Parity tests

Use small known test cases comparing:

- note triggering behavior
- output presence
- rough level consistency
- release/tail behavior
- state restore behavior

Do not over-promise bit-perfect parity too early.

### 10.3 Manual Linux GUI tests

Once GUI exists:

- open/close editor repeatedly
- resize if supported
- load samples
- adjust controls during playback
- reopen saved projects
- verify behavior across Ubuntu/REAPER sessions

---

## 11. Build System Plan

### Recommendation

Use root-level CMake.

**High-level tasks:**
- define JUCE dependency strategy
- create top-level CMakeLists.txt
- create plugin target
- add core sources incrementally
- separate third-party includes cleanly
- keep build instructions documented in repo

**Avoid:**
- IDE-generated project files as the primary source of truth
- parallel competing build systems if avoidable

---

## 12. Ubuntu Environment Notes

Document exact package dependencies once confirmed.

Expected categories:

- compiler toolchain
- CMake/Ninja or Make
- audio dev headers as required by JUCE
- graphics/windowing/font dev packages as required by JUCE

This belongs in a future `docs/notes/build_ubuntu.md`.

---

## 13. Decision Log Policy

Every material implementation decision should be written to:

`docs/decisions/active.md`

At minimum, log:

- JUCE integration method
- repo layout decisions
- parameter MVP choices
- state serialization choices
- legacy compatibility decisions
- GUI scope boundaries
- known Linux-specific host issues

If a decision is not in the repo, assume it is not durable.

---

## 14. Claude Code Operating Guidance

Claude Code should:

- read PRD.md, TECH_PLAN.md, and CLAUDE.md first
- work in small slices
- prefer headless bring-up over speculative GUI work
- update docs/decisions/active.md when tradeoffs are resolved
- avoid large architectural rewrites unless explicitly instructed
- preserve the DSP core unless a concrete incompatibility forces change

The PM should keep CC tightly scoped:

- one milestone slice at a time
- clear touched files
- explicit tests to run
- explicit non-goals for the slice

---

## 15. Suggested First Claude Code Prompt

> Read PRD.md, TECH_PLAN.md, and CLAUDE.md.
>
> Goal: complete Milestone M1 only.
>
> Tasks:
> 1. Set up a minimal JUCE + CMake VST3 plugin target for Linux Ubuntu.
> 2. Ensure it builds locally with clear documented commands.
> 3. Do not integrate the C700 DSP core yet beyond placeholder code.
> 4. Document the exact build steps in docs/notes/build_ubuntu.md.
> 5. Update docs/decisions/active.md with any dependency/layout decisions made.
>
> Constraints:
> - No GUI reconstruction work yet.
> - No VST2 work.
> - No speculative multi-format expansion.
> - Keep changes minimal and reversible.
>
> Deliverables:
> - CMake build skeleton
> - plugin target source files
> - successful build instructions
> - decision log updates

---

## 16. Source-of-Truth Policy

Priority order:

1. `PRD.md`
2. `TECH_PLAN.md`
3. `docs/decisions/active.md`
4. code/tests
5. chat history

If implementation pressure conflicts with the plan, update the plan or explicitly log the deviation.
