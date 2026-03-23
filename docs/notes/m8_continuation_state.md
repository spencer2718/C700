# M8 Continuation State

Temporary implementation handoff for M8. Delete this file once M8 is complete and the relevant decisions are folded into the durable docs.

## Current branch state

- branch: `master`
- latest M8-related fix attempt: `495bc0a` (`fix stale text field display on slot/channel change`)
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
- the latest attempted fix for that bug reintroduced the Linux text-entry regression
- user-confirmed symptom:
  - focus Root Key on slot 0
  - switch to slot 1
  - the field keeps showing slot 0's value
  - changing Root Key audibly affects only the intended slot, not every slot
  - after the `grabKeyboardFocus()` change, Root Key can become non-editable again, repeating the earlier Linux focus failure

## Updated diagnosis

`syncEditorFields()` intentionally refuses to overwrite a text editor while it still has keyboard focus.

That behavior is correct during active typing, but `adjustProgram()` currently does this:

1. `commitPendingFieldEdits()`
2. change the `program` APVTS parameter
3. `forceParamSync()`

It does not explicitly take focus away from the active text editor after committing. If the editor still reports keyboard focus, the timer skips refreshing that field for the new slot and the old slot's text remains visible.

This also explains why the bug is visual while the underlying slot data is still correct.

However, the most recent attempted fix was to force focus onto the rocker / track button with `grabKeyboardFocus()` after `commitPendingFieldEdits()`. User testing showed that this is not a stable fix on Linux in this widget stack. It reintroduced the old failure mode where the text editors stop accepting input reliably.

So the real constraint is now clearer:

- stale display must be fixed without using synthetic focus transfer to another control
- typing must remain stable on Linux
- the solution must distinguish "actively being edited" from mere keyboard focus

## Rejected fix path

- Do not use `grabKeyboardFocus()` on the slot rocker or track buttons as the stale-field fix.
- Do not go back to timer-side slot reads through `EditingProgram`.

Both approaches were tested and both regress Linux text editing.

## Next-pass fix target

Change stale-field handling so that slot/channel changes can refresh the displayed values without forcibly moving focus to another widget.

Concrete target areas:

- `C700AudioProcessorEditor::adjustProgram()`
- any other path that changes edit slot or channel while text editors may still be focused
- potentially `selectEditingChannel()` if the same stale-display symptom appears there

Likely safer approaches:

- track whether a field is actively dirty/editing rather than using `hasKeyboardFocus()` as the only gate
- after `commitPendingFieldEdits()`, clear the editor's "dirty edit" state but do not force focus onto another widget
- track the last displayed slot/channel and allow a one-shot field refresh when that identity changes
- if needed, add an explicit "force refresh once after committed slot change" path that updates field text even if the editor still nominally has focus

Do not revert to using `getProgramPropertyValue()` / `getProgramPropertyDoubleValue()` in the timer for these fields. That path mutates `EditingProgram` internally and caused the Linux focus regression fixed by `c9eb5ae`.

## Remaining M8 work after the field bug

- waveform subsystem and BRR preview
- remaining echo/settings widgets
- recording settings overlay
- final pass on command/control parity with the original VSTGUI editor
