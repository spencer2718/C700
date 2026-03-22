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

## Current JUCE Surface Gap

Current APVTS coverage is only the small set created in `src/plugin/PluginProcessor.cpp`.

### Missing editable scalar/menu state from the original UI

Parameters missing from the current JUCE wrapper:

- `kParam_poly`
- `kParam_velocity`
- `kParam_bendrange`
- `kParam_engine`
- `kParam_vibrate`
- `kParam_vibdepth2`
- `kParam_voiceAllocMode`
- `kParam_bankAmulti`
- `kParam_bankBmulti`
- `kParam_bankCmulti`
- `kParam_bankDmulti`
- `kParam_fir0`
- `kParam_fir1`
- `kParam_fir2`
- `kParam_fir3`
- `kParam_fir4`
- `kParam_fir5`
- `kParam_fir6`
- `kParam_fir7`

Properties missing from the current JUCE wrapper:

- `kAudioUnitCustomProperty_Band1`
- `kAudioUnitCustomProperty_Band2`
- `kAudioUnitCustomProperty_Band3`
- `kAudioUnitCustomProperty_Band4`
- `kAudioUnitCustomProperty_Band5`
- `kAudioUnitCustomProperty_LowKey`
- `kAudioUnitCustomProperty_HighKey`
- `kAudioUnitCustomProperty_LoopPoint`
- `kAudioUnitCustomProperty_ProgramName`
- `kAudioUnitCustomProperty_MonoMode`
- `kAudioUnitCustomProperty_PitchModulationOn`
- `kAudioUnitCustomProperty_NoiseOn`
- `kAudioUnitCustomProperty_PortamentoOn`
- `kAudioUnitCustomProperty_PortamentoRate`
- `kAudioUnitCustomProperty_NoteOnPriority`
- `kAudioUnitCustomProperty_ReleasePriority`

### Missing runtime/read-only main-UI surface

- `kAudioUnitCustomProperty_BRRData`
- `kAudioUnitCustomProperty_NoteOnTrack_1..16`
- `kAudioUnitCustomProperty_MaxNoteTrack_1..16`
- `kAudioUnitCustomProperty_MaxNoteOnTotal`
- `kAudioUnitCustomProperty_IsHwConnected`
- bank-selection state
- editing-channel state

### Missing recording-popup surface

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

### Missing commands and one-shot actions

- sample load
- sample unload
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
