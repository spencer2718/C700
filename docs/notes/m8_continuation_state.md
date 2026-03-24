# M8 Continuation State

Temporary implementation handoff for M8. Delete this file once M8 is complete and the relevant decisions are folded into the durable docs.

## Current branch state

- branch: `master`
- latest M8-related fix: `0eba936` (`fix mActiveEditor clearing — listen to all descendant mouse events`)
- latest build/install status: `scripts/build-install.sh` passed

## Completed M8 passes

- `8f0eb6f` Expand processor surface for M8 UI rebuild
- `7031583` Rebuild the fixed M8 editor shell
- `e8fe988` Restore bitmap selectors in M8 editor
- `95a945c` Rebuild per-slot edit controls in M8 editor
- `95d4c28` Fix slot text field commits in editor
- `e17c1b8` Keep text fields focused during UI refresh
- `1f90e8d` Read editor text fields from slot state
- `c9eb5ae` Stop mutating slot during field refresh
- `495bc0a` Attempt focus handoff on slot/channel change
- `64be85c` One-shot mForceFieldRefresh flag for slot changes
- `a7da77c` Mouse-click tracking (mActiveEditor) replaces focus-based gating
- `0eba936` Global addMouseListener(this, true) for descendant click clearing

## Verified working

- fixed-size editor shell and bundled bitmap assets
- track selectors, bank selectors, slot rocker
- load/unload flow from the reconstructed shell
- sustained playback after the processor-side note-reset fix
- mActiveEditor-based gating successfully protects text fields during typing (mouseDown on the TextEditor fires synchronously)
- mForceFieldRefresh successfully updates fields on slot change
- slot-local writes into the engine for Root Key and related editable fields

## Still broken

- mActiveEditor is not reliably cleared when the user clicks away from a text field onto another control (button, slider, knob). onFocusLost lags on X11, and parent mouseDown doesn't catch child component clicks reliably.
- Net result: typing works, slot-change refresh works, but clicking off a field can leave it "locked" as the active editor until onFocusLost eventually fires or the user presses Enter.
- This is a UX annoyance, not a data corruption bug — the underlying engine state remains correct.

## Rejected fix path

- Do not use `grabKeyboardFocus()` on the slot rocker or track buttons as the stale-field fix. Regresses Linux text editing.
- Do not go back to timer-side slot reads through `EditingProgram`. Mutates engine state and causes focus regression.
- Do not rely on parent `mouseDown` with `addMouseListener(this, true)` for clearing `mActiveEditor`. On Linux/X11, clicks on child components (buttons, sliders, knobs) do not reliably propagate `mouseDown` to the parent listener before the timer fires. The race condition persists.

## Next-pass fix target

The remaining problem is narrowly scoped: mActiveEditor needs to be cleared synchronously when the user interacts with any non-text-field control. Approaches to explore next session:

- Override mouseDown on individual buttons/sliders/knobs to clear mActiveEditor (tedious but deterministic)
- Use a short timeout: if mActiveEditor has been set for more than ~500ms without a keystroke, assume the user clicked away and clear it
- Use JUCE's ComponentListener::componentFocusChanged on the top-level component to detect any focus change
- Explicitly call commitPendingFieldEdits() at the start of every button/slider callback (already done for some — may need to be comprehensive)

## Remaining M8 work after the field bug

- waveform subsystem and BRR preview
- remaining echo/settings widgets
- recording settings overlay
- final pass on command/control parity with the original VSTGUI editor
