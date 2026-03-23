# M8 Continuation State

Temporary implementation handoff for M8. Delete this file once M8 is complete and the relevant decisions are folded into the durable docs.

## Current branch state

- branch: `master`
- latest M8-related fix: `c9eb5ae` (`Stop mutating slot during field refresh`)
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

## Verified working

- fixed-size editor shell and bundled bitmap assets
- track selectors, bank selectors, slot rocker
- load/unload flow from the reconstructed shell
- sustained playback after the processor-side note-reset fix
- Linux text-field typing/focus stability in general
- slot-local writes into the engine for Root Key and related editable fields

## Still broken

- focused text fields can display stale values after a slot change
- this is display/UI-state only; the underlying engine state remains slot-local
- user-confirmed symptom:
  - focus Root Key on slot 0
  - switch to slot 1
  - the field keeps showing slot 0's value
  - changing Root Key audibly affects only the intended slot, not every slot

## Likely root cause

`syncEditorFields()` intentionally refuses to overwrite a text editor while it still has keyboard focus.

That behavior is correct during active typing, but `adjustProgram()` currently does this:

1. `commitPendingFieldEdits()`
2. change the `program` APVTS parameter
3. `forceParamSync()`

It does not explicitly take focus away from the active text editor after committing. If the editor still reports keyboard focus, the timer skips refreshing that field for the new slot and the old slot's text remains visible.

This also explains why the bug is visual while the underlying slot data is still correct.

## Next-pass fix target

Change slot-switch behavior so that any active text edit is fully ended before the slot change is presented.

Concrete target areas:

- `C700AudioProcessorEditor::adjustProgram()`
- any other path that changes edit slot or channel while text editors may still be focused
- potentially `selectEditingChannel()` if the same stale-display symptom appears there

Likely safe approaches:

- explicitly move keyboard focus to the slot rocker/button after `commitPendingFieldEdits()`
- or clear editor focus before changing the slot
- or track the last displayed slot and force a field refresh when the slot changes after a commit

Do not revert to using `getProgramPropertyValue()` / `getProgramPropertyDoubleValue()` in the timer for these fields. That path mutates `EditingProgram` internally and caused the Linux focus regression fixed by `c9eb5ae`.

## Remaining M8 work after the field bug

- waveform subsystem and BRR preview
- remaining echo/settings widgets
- recording settings overlay
- final pass on command/control parity with the original VSTGUI editor
