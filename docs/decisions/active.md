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
