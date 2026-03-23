# M8 UI Reconstruction Advisory

Temporary implementation note for M8. Delete this file once the JUCE UI reconstruction is complete and its decisions are captured elsewhere.

## Scope

This note is for reconstructing the original fixed-layout C700 UI in JUCE without changing the underlying DSP behavior already verified in M7.

Treat these files as the behavior and layout sources of truth:

- `ControlInstacnesDefs.h`
- `RecordingViewCntls.h`
- `GUIUtils.cpp`
- `C700GUI.cpp`
- `C700Edit.cpp`
- `RecordingSettingsGUI.cpp`

Do not treat `ControlInstacnesDefs.h` as a complete behavior spec by itself. It only gives control metadata and geometry.

## Current Implementation Snapshot (2026-03-22)

Completed so far:

- fixed-size `536x406` JUCE editor shell
- bundled bitmap assets for the original interface
- bitmap track selectors and bank selectors
- slot rocker and sample-management buttons
- main per-slot edit panel:
  - program name
  - root/low/high key
  - loop point
  - sample rate
  - loop / echo / PMOD / noise / mono / glide
  - ADSR sliders
  - volume and vibrato controls
  - ARAM/hardware/playercode status

Not built yet:

- waveform subsystem
- echo/settings sections beyond the currently exposed controls
- recording settings overlay
- final polish for all command controls and remaining bitmap widgets

Known open M8 bug:

- focused text fields can display stale values after a slot change even though the underlying engine state stays slot-local
- this is a UI-state/focus bug, not a DSP-state bug
- note: a naive `grabKeyboardFocus()` fix on the rocker/track buttons was tried and rejected because it reintroduced the Linux "cannot type into the field" regression
- likely fix path: decouple "actively being edited" from raw keyboard focus, then allow a one-shot field refresh after a committed slot/channel change

## Control Inventory

Live control count:

- Main editor: 180
- Recording popup: 34
- Total: 214

Distinct live control kinds:

- `stxt`
- `dtxt`
- `eutx`
- `bttn`
- `valp`
- `push`
- `slid`
- `cbtn`
- `knob`
- `sepa`
- `wave`
- `menu`
- `larr`
- `spls`
- `hzsw`

Grouped by role:

- 122 state-bearing controls
- 37 command controls
- 55 decorative controls

State-bearing split:

- 33 parameter-tagged controls
- 89 custom-property-tagged controls

Important note:

- There are commented-out legacy controls in `ControlInstacnesDefs.h`. Do not accidentally reconstruct dead UI.

## Widget Taxonomy

The original factory lives in `GUIUtils.cpp` and defines the real widget behavior:

- `wave` -> waveform view
- `spls` -> splash/help
- `menu` -> popup menu
- `push` -> text kick button or bitmap kick button depending on signature
- `slid` -> horizontal slider
- `knob` -> bitmap knob
- `cbtn` -> labeled on/off button
- `dtxt` -> numeric display
- `eutx` -> editable text/numeric field
- `valp` -> movie bitmap / indicator
- `bttn` -> on/off bitmap button
- `hzsw` -> horizontal switch
- `stxt` -> text label
- `larr` -> rocker switch
- `sepa` -> separator line

Do not flatten these into generic JUCE widgets too aggressively. Several of them have distinct runtime semantics despite looking similar.

## Hard Parts

### 1. Waveform subsystem

This is the hardest part.

The original UI has three coordinated waveform views driven from BRR data:

- overview: `kAudioUnitCustomProperty_BRRData`
- tail view: `kAudioUnitCustomProperty_BRRData + 1000`
- head/loop view: `kAudioUnitCustomProperty_BRRData + 2000`

`C700Edit::SetBRRData()` decodes BRR and updates all three views together. `C700Edit::SetLoopPoint()` updates the loop marker and the head view window.

Implications:

- This is not a static image area.
- It needs decoded preview data, not just raw BRR bytes.
- Loop point display is coupled to waveform display.
- The current JUCE wrapper does not expose a clean waveform snapshot API yet.

### 2. Editable fields

`eutx` controls are not one thing:

- some are plain numeric fields
- some are decimal-entry fields
- some are strings

The original behavior depends on `futureuse` in `GUIUtils.cpp`:

- `futureuse == 1`: decimal input with relaxed max handling
- `futureuse == 2`: special text mode

`dtxt` is separate from `eutx`. It is formatted display logic, not editable text.

### 3. Recording popup

The recording settings panel is its own subsystem:

- file path selection
- string metadata
- host beat capture
- save-format toggles
- time-base menu
- player-code loading and validation state

This is not just a child panel with APVTS attachments.

### 4. Channel indicators and selectors

The 16 track indicators are runtime state, not normal parameters.

There are at least three distinct channel-related UI layers:

- 16 voice activity indicators via `NoteOnTrack_1..16`
- 16 max-note displays via `MaxNoteTrack_1..16`
- track selection buttons that write `EditingChannel`

### 5. Loop point representation

Loop point is a trap.

The original UI converts between displayed sample units and stored BRR units:

- UI display path converts BRR units to sample units
- UI write path snaps sample input and converts back to BRR units

If JUCE rebuilds this as a plain integer field, it will be wrong.

## Hidden Runtime State Needed For UI

The current JUCE editor is too small a surface for the original UI. M8 needs explicit access to state beyond the current APVTS set.

Required nontrivial state:

- `kAudioUnitCustomProperty_BRRData`
- mirrored waveform views using `+1000` and `+2000`
- `kAudioUnitCustomProperty_NoteOnTrack_1..16`
- `kAudioUnitCustomProperty_MaxNoteTrack_1..16`
- `kAudioUnitCustomProperty_MaxNoteOnTotal`
- `kAudioUnitCustomProperty_IsHwConnected`
- `kAudioUnitCustomProperty_Bank`
- `kAudioUnitCustomProperty_EditingChannel`
- `kAudioUnitCustomProperty_SongPlayerCodeVer`

State with extra UI behavior:

- `kAudioUnitCustomProperty_TotalRAM`
- ARAM display turns red when exceeding hardware capacity

Potentially derived or command-adjacent state:

- current sample/program name
- current source-file path
- preemphasis toggle used at import time

## Remaining JUCE Surface Gap

The big processor-surface gap from the initial advisory is now partly closed. The main remaining gaps are runtime UI surfaces and non-APVTS overlay state.

### Remaining runtime/read-only main-UI surface

- `kAudioUnitCustomProperty_BRRData`
- `kAudioUnitCustomProperty_NoteOnTrack_1..16`
- `kAudioUnitCustomProperty_MaxNoteTrack_1..16`
- `kAudioUnitCustomProperty_MaxNoteOnTotal`
- `kAudioUnitCustomProperty_IsHwConnected`
- bank-selection state
- editing-channel state

### Remaining recording-popup surface

- `kAudioUnitCustomProperty_SongRecordPath`
- `kAudioUnitCustomProperty_RecSaveAsSpc`
- `kAudioUnitCustomProperty_RecSaveAsSmc`
- `kAudioUnitCustomProperty_TimeBaseForSmc`
- `kAudioUnitCustomProperty_GameTitle`
- `kAudioUnitCustomProperty_SongTitle`
- `kAudioUnitCustomProperty_NameOfDumper`
- `kAudioUnitCustomProperty_ArtistOfSong`
- `kAudioUnitCustomProperty_SongComments`
- `kAudioUnitCustomProperty_RepeatNumForSpc`
- `kAudioUnitCustomProperty_FadeMsTimeForSpc`
- `kAudioUnitCustomProperty_SongPlayerCodeVer`

### Remaining commands and one-shot actions

- sample save
- sample save XI
- preemphasis toggle
- auto sample rate
- auto base key
- change loop point
- track select `1..16`
- bank select `A..D`
- recording settings open
- choose record path
- set record start from host beat
- set record loop start from host beat
- set record end from host beat
- player-code load
- Khaos load

The processor already covers most of the main scalar/menu controls listed in the original advisory:

- per-slot keys, loop, sample rate, mono, PMOD, noise, portamento, note priorities
- vibrato depth/rate
- engine and voice allocation
- echo FIR taps and band controls
- recording region / SPC export core parameters

The missing work is now mostly widget behavior, waveform/runtime views, and overlay state rather than raw parameter exposure.

## APVTS Guidance

Do not wire all controls to APVTS attachments.

Use APVTS for:

- stable scalar parameters
- bools
- small menus
- values that need host automation

Do not force APVTS for:

- waveform blobs
- runtime indicators
- string/filepath metadata
- one-shot commands
- host-derived state

For M8, the processor/adapter likely needs both:

- a larger APVTS surface
- a separate snapshot/query API for runtime UI state

## Recommended Pass Order

### Pass 1: Processor and adapter surface expansion

Before rebuilding widgets, add the missing state surface.

Goals:

- add missing scalar/menu/bool state where APVTS makes sense
- expose non-APVTS snapshot APIs for waveform and indicators
- expose bank/editing-channel/runtime state cleanly
- expose recording popup string and status accessors

### Pass 2: Fixed shell reconstruction

Goals:

- fixed size `536 x 406`
- non-resizable editor
- original background image
- exact integer bounds
- asset loading path that works on Linux

Do this before rebuilding advanced widgets so geometry problems are solved early.

### Pass 3: Main interactive controls

Goals:

- sample/program area
- ADSR and instrument controls
- global echo/main controls
- track and bank selectors
- load/unload/save actions
- editable fields with original value semantics

### Pass 4: Waveform and indicators

Goals:

- three-view waveform system
- loop marker behavior
- channel activity indicators
- max-note readouts
- ARAM visual warning behavior

Important:

- decode or prepare preview data outside `paint()`
- cache snapshot data

### Pass 5: Recording popup and Linux polish

Goals:

- recording overlay
- path chooser
- player-code loader status
- text metadata
- modal/focus behavior on Linux/X11
- final pixel polish

## JUCE/Linux Traps

- Do not rely on stock Linux fonts matching `Monaco`, `Arial`, or `Arial Black`.
- Do not make the editor resizable.
- Do not use relative runtime asset paths inside the VST3 bundle and assume they work on Linux.
- Prefer embedded resources or a deterministic asset-loading strategy.
- Prefer an in-editor child overlay to a separate modal window when possible.
- Use async file choosers where practical.
- Bitmap-based controls can blur under scale factors if the editor is not kept on exact pixel bounds.
- Repaint cost can get bad if waveform or indicator polling is naive.

## Practical Recommendation

Do not start M8 by writing a generic parser that blindly maps every `ControlInstances` row to a JUCE attachment.

Do this instead:

1. Use the layout headers for geometry and IDs.
2. Use `C700GUI.cpp`, `C700Edit.cpp`, and `RecordingSettingsGUI.cpp` as the behavior spec.
3. Expand processor and adapter state first.
4. Rebuild the UI in functional groups rather than as one giant generic factory.

## Bottom Line

The reconstruction plan is viable, but the risky assumption is that the original UI is mostly a bitmap skin over a small parameter set.

It is not.

The original UI mixes:

- engine parameters
- custom properties
- read-only runtime indicators
- derived display state
- string/filepath metadata
- one-shot commands

M8 will go faster and more cleanly if the processor surface is widened before the visual reconstruction starts.
