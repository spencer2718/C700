> *This is an English translation of the original Japanese README by osoumen. Fork maintained at github.com/spencer2718/C700.*

# How to Use the C700

![gui_image](images/gui_image.png)

* A software sampler that emulates the SNES (Super Famicom) built-in sound chip.
* Supports loading AIFF (Mac only) and WAV files with loop points set.
* Supports reading and writing raw BRR data in AddmusicM format.
* Can synchronize with a [G.I.M.I.C](http://gimic.jp) unit with an SPC module attached to play on real hardware.
* When USB hardware is detected, an icon appears in the lower-right corner of the UI.

![hw_conn_ind](images/hw_conn_ind.png)

## System Requirements
### macOS
* macOS 10.11 or later (Intel, Apple Silicon)
* A host application supporting Audio Units or VST2.4

### Windows
* A host application supporting VST2.4 on Windows Vista or later (32/64-bit)

## Verified Hosts
### macOS

* Logic Pro 10.7 (Intel, Apple Silicon) (macOS Big Sur 11.6.1)
* Digital Performer 8 (32/64-bit) (10.8.5)
* Digital Performer 9.12 (32/64-bit) (10.10.5)

### Windows

* Cubase 10 (32/64-bit) (Windows 10)

## Differences Between Mac and Windows Versions
* The Mac version can load AIFF, WAV, and SD2 waveform data, while the Windows version only supports WAV (with known issues).
* The Mac version detects USB hardware insertion/removal while running, whereas the Windows version only checks at plugin startup.
* The Mac version is likely more stable overall.

## Features
* Various waveform data formats can be loaded directly.
* Supports AIFF (Mac only), WAV, SPC, and AddmusicM format BRR files.
* Up to 128 waveforms can be registered.
* Supports program change, pitch bend, and modulation wheel.
	* These are configured per MIDI channel.
* Maximum sample rate for playback waveforms is 120kHz.
* Normally, waveform numbers correspond to program change numbers.
	* Alternatively, multiple waveforms can be grouped into a single bank.
* Up to 4 banks are available, and each bank can be set to multi-sample mode.
	* When a bank is set to multi-sample mode, selecting any waveform number within that bank via program change selects the entire bank.
	* The HighKey and LowKey settings for waveforms are only effective in multi-sample mode.

## Global Settings

![general_settings](images/general_settings.png)

Settings common to all MIDI channels.

<dl>
  <dt>Track 1-16</dt>
  <dd>Shows the note activity and selection state of MIDI channels 1-16.</dd>

  <dt>Engine</dt>
  <dd>Old: Compatibility mode for older versions.</dd>
  <dd>Relaxed: Uses waveform memory without capacity restrictions. Polyphony can be increased up to 16 voices. Some behaviors differ from real hardware.</dd>
  <dd>Accurate: Uses Blargg's Audio Engine for more precise emulation. In this mode, the "Poly" setting is ignored and fixed at 8.</dd>

  <dt>VoiceAlloc</dt>
  <dd>Oldest: On note-on, prioritizes cutting the oldest sounding voice. Tends to produce more natural-sounding results.</dd>
  <dd>SameCh: On note-on, prioritizes reusing voices from the same channel. May result in smaller recorded performance data.</dd>

  <dt>Poly</dt>
  <dd>Sets the total polyphony (1-16).</dd>

  <dt>Bend Range</dt>
  <dd>Sets the pitch bend range.</dd>

  <dt>Velocity Curve</dt>
  <dd>Sets the velocity curve to one of: fixed value, quadratic, or linear.</dd>

  <dt>Multi Bank A-D</dt>
  <dd>C700 has 4 banks that can treat multiple waveforms as a single instrument.</dd>
  <dd>Banks set to On are configured as multi-sample mode,</dd>
  <dd>enabling LowKey/HighKey settings and sample key mapping.</dd>
  <dd>Useful for building drum kits, etc.</dd>
  <dd>When key ranges overlap, the higher-numbered sample's range takes priority.</dd>
  <dd>Banks not set to multi-sample mode map waveform numbers directly to program change numbers.</dd>

  <dt>Vibrato Depth, Rate</dt>
  <dd>Adjusts the modulation wheel (CC:1) intensity.</dd>
  <dd>Currently shared across all MIDI channels.</dd>
</dl>

## Per-Waveform Settings

![wave_settings](images/wave_settings.png)

Edit the current instrument settings for the selected track.

<dl>
	<dt>Bank</dt>
	<dd>Sets which bank the waveform belongs to.</dd>
	<dd>When a multi-sample mode bank is selected, all waveforms in that bank are treated as a single instrument.</dd>
	<dd>When a non-multi-sample bank is selected, waveform number = program change number.</dd>
	<dt>Waveform Number / Label</dt>
	<dd>Displays the currently selected waveform number (via program change) and its name.</dd>
	<dt>Low Key, High Key</dt>
	<dd>Sets the lower and upper pitch limits.</dd>
	<dd>Ignored when not in multi-sample mode.</dd>
	<dt>Root Key</dt>
	<dd>Sets the base pitch of the waveform.</dd>
	<dd>C4 = 60.</dd>
	<dd>Includes auto-detection.</dd>
	<dt>Loop Point / Loop</dt>
	<dd>Sets the waveform loop on/off and loop point.</dd>
	<dd>Due to BRR format constraints, limited to 16-sample boundaries.</dd>
	<dt>Sample Rate</dt>
	<dd>Sets the sample rate when playing at the root key pitch.</dd>
	<dd>Includes auto-detection.</dd>
	<dt>Priority</dt>
	<dd>Sets the voice priority. On note-on, if maximum polyphony is exceeded, the voice with the lowest priority and oldest timestamp is cut.</dd>
	<dd>NteOn (NoteOn)</dd>
	<dd>Priority assigned to the voice on note-on.</dd>
	<dd>Rel (Release)</dd>
	<dd>Priority assigned to the voice on note-off.</dd>
	<dt>Waveform Display</dt>
	<dd>Displays the selected waveform and the loop end-to-start region.</dd>
	<dt>PreEmphasis</dt>
	<dd>When enabled, applies a high-frequency emphasis filter when loading WAV or AIFF files.</dd>
	<dd>This compensates for the high-frequency roll-off caused by DSP processing during playback, bringing the sound closer to the original waveform.</dd>
	<dd>If this processing causes clipping, normalization is applied automatically.</dd>
	<dt>Load</dt>
	<dd>Loads waveform data into the currently displayed number.</dd>
	<dd>Files can also be loaded via drag and drop.</dd>
	<dd>Supports AddmusicM format (.brr), AIFF (Mac only), WAV, and SPC.</dd>
	<dd>Stereo data is automatically converted to mono.</dd>
	<dd>Loop point and key information from WAV/AIFF files is preserved if present.</dd>
	<dd>If the loop length is not a multiple of 16 samples, automatic sample rate conversion is performed to align to 16-sample boundaries.</dd>
	<dd>Legacy proprietary format (.brr) waveform files saved with older versions can only be loaded in the Mac AU version.</dd>
	<dd>Waveform files are limited to a maximum of 116,480 samples.</dd>
	<dd>Sample rate is auto-detected when loading .brr files (without .smpl file) and .spc files.</dd>
	<dt>Save Smpl...</dt>
	<dd>Saves the displayed waveform data in raw BRR format.</dd>
	<dd>A .smpl file with the same name is created in the same location.</dd>
	<dd>This file contains instrument settings — do not move or delete it.</dd>
	<dt>Export...</dt>
	<dd>Saves the displayed waveform or bank in FastTracker II instrument format (XI format).</dd>
	<dd>For instruments set to multi-sample mode, the export includes multiple waveforms as a single instrument.</dd>
	<dt>Unload</dt>
	<dd>Discards the displayed waveform.</dd>
	<dt>Echo</dt>
	<dd>Enables echo for this waveform when playing.</dd>
	<dt>PM</dt>
	<dd>Enables the pitch modulation register for this waveform when playing.</dd>
	<dd>Pitch modulation uses the output of the previous voice channel for frequency modulation.</dd>
	<dd>To control the modulation source channel, enable "Mono" to fix the voice channel assignment.</dd>
	<dt>Noise</dt>
	<dd>Enables noise for this waveform when playing.</dd>
	<dd>The noise frequency is shared across all channels, so when multiple noise-enabled instruments play, the last triggered frequency is used.</dd>
	<dt>Mono</dt>
	<dd>When enabled, the waveform always plays monophonically.</dd>
	<dd>The instrument is assigned a fixed voice channel. MIDI channels 1-8 map to voice channels 1-8; MIDI channels 9-16 map to voice channels 1-8.</dd>
	<dd>Additionally, subsequent notes do not trigger a key-on (legato).</dd>
	<dt>Glide</dt>
	<dd>When enabled, portamento effect is applied.</dd>
	<dt>Rate</dt>
	<dd>Sets the portamento speed.</dd>
	<dt>Volume</dt>
	<dd>Sets the playback volume for the waveform.</dd>
	<dd>Setting to a negative value inverts the phase.</dd>
	<dt>AR, DR, SL, SR1, SR2</dt>
	<dd>Configures the hardware envelope.</dd>
	<dd>During key-on, the SR1 sustain rate is used. After key-off, it switches to the SR2 value.</dd>
	<dt>Enable Release</dt>
	<dd>When set to off, note-off does not transition to SR2 and instead immediately triggers key-off.</dd>
	<dt>Khaos!</dt>
	<dd>Generates a random waveform.</dd>
</dl>

## Echo Settings

![echo_settings](images/echo_settings.png)

Settings for the built-in echo effect. Shared across all channels.

<dl>
	<dt>Main</dt>
	<dd>Sets the main volume.</dd>
	<dd>Setting to a negative value inverts the phase.</dd>
	<dt>Echo</dt>
	<dd>Adjusts the echo component volume.</dd>
	<dd>Setting to a negative value inverts the phase.</dd>
	<dt>Delay Time</dt>
	<dd>Sets the delay time.</dd>
	<dt>Feedback</dt>
	<dd>Sets the feedback amount.</dd>
	<dd>Setting to a negative value inverts the phase.</dd>
	<dd>Setting too high may cause oscillation.</dd>
	<dt>Filter</dt>
	<dd>Configures the filter applied to the wet signal.</dd>
	<dd>Values can be entered directly (-128 to 127),</dd>
	<dd>or set visually using the EQ sliders.</dd>
	<dt>RAM Simulation</dt>
	<dd>Displays the total memory consumed by all loaded waveforms and echo usage.</dd>
	<dd>The real SNES hardware has 64kB of memory available, including the driver and sequence data.</dd>
	<dd>If the capacity exceeds what is possible on real hardware, the display turns red.</dd>
	<dd>If behavior becomes erratic while the display is red, reload the plugin.</dd>
</dl>

## Recording Performance

Set a region and play it back to record the performance. The recording can be saved in SPC or ROM format.
Click the "Set Recorder..." button at the bottom of the main screen to open the settings panel.
On first use, a message will prompt you to load the playback code. Obtain "playercode.bin" from the same page as the distribution site ([http://picopicose.com/software.html](http://picopicose.com/software.html)) and drag and drop it into the window. If loaded successfully, the following screen will appear:

![recorder_settings](images/recorder_settings.png)

<dl>
	<dt>Save Path</dt>
	<dd>When recording is finished, files are saved to the folder set here.</dd>
	<dt>Save as *.spc</dt>
	<dd>Check this to export an SPC file.</dd>
	<dd>Playback uses the APU's internal timer set to a 1/16000 second period, so recording resolution is 62.5us.</dd>
	<dd>Due to the 64KB limit of SPC files, long songs or large waveform memory usage may prevent the entire region from being exported.</dd>
	<dt>Save as *.smc</dt>
	<dd>Check this to export a ROM format file.</dd>
	<dd>Playback uses horizontal sync interrupts, so the recording resolution is 1/15734 sec for NTSC and 1/15625 sec for PAL.</dd>
	<dd>Recording can fill up to a 32Mbit ROM.</dd>
	<dt>smc Format</dt>
	<dd>Select whether the output ROM file should be NTSC or PAL format.</dd>
	<dd>NTSC and PAL have different recording resolutions.</dd>
	<dt>PlayerCode</dt>
	<dd>If playercode.bin has been loaded successfully, "Valid" is displayed.</dd>
	<dd>Use the Load button to update playercode.bin.</dd>
</dl>

The above settings are saved to a preferences file and persist across plugin launches.
The preferences file is located at "~/Library/Application Support/C700/C700.settings" on macOS, and "[Home Folder]/AppData/Roaming/C700/C700.settings" on Windows.

<dl>
	<dt>Record Start Pos [ppq]</dt>
	<dd>Sets the position to start recording.</dd>
	<dd>Click Set to use the current song pointer position.</dd>
	<dt>Loop Start Pos [ppq]</dt>
	<dd>Sets the loop point position.</dd>
	<dd>Click Set to use the current song pointer position.</dd>
	<dt>Record End Pos [ppq]</dt>
	<dd>Sets the position to end recording.</dd>
	<dd>Click Set to use the current song pointer position.</dd>
	<dt>Game Title</dt>
	<dd>Sets the game title embedded in SPC or SMC files.</dd>
	<dd>Maximum 32 characters for SPC files, 21 characters for SMC files.</dd>
	<dt>Song Title for spc</dt>
	<dd>Sets the song title embedded in the SPC file. Maximum 32 characters.</dd>
	<dt>Name of dumper for spc</dt>
	<dd>Sets the dumper information embedded in the SPC file. Maximum 16 characters.</dd>
	<dt>Artist of Song for spc</dt>
	<dd>Sets the artist information embedded in the SPC file. Maximum 32 characters.</dd>
	<dt>Comments for spc</dt>
	<dd>Sets the comment embedded in the SPC file. Maximum 32 characters.</dd>
	<dt>Repeat num for spc</dt>
	<dd>Used to determine the SPC playback duration. Sets how many times the loop-start-to-loop-end section repeats.</dd>
	<dt>Fade milliseconds for spc</dt>
	<dd>Sets the fade-out duration in milliseconds after SPC playback ends.</dd>
</dl>

## Tips for Getting Good Sound

* Keep the total across all parts to 8 voices or fewer.
* Keep the combined waveform + echo memory usage to around 40kB.
* It is better to trim waveforms shorter rather than lowering the sample rate.
* Use key splits for instruments with a wide pitch range.
* Configure echo settings carefully.
* Sample rates can be low for all waveforms except the highest-pitched one or two.
* Recording at the note A produces pitches that are multiples of 440Hz, so one cycle will be an integer number of samples.
* Use a tuner to match pitch accurately for cleaner loops.

## Changelog
* 2021/10/31
	* Fixed FIR filter settings being replaced by slider values after save/restore
	* Fixed BRR files with loop points near the end of the file not loading correctly
	* Fixed incorrect looping in SPC/SMC output in some cases

* 2021/10/23
	* Added script700 file output when SPC performance data exceeds capacity
	* Fixed note-on at the very start not sounding during SPC export
	* Added support for loading BRR files with loop points at the file end position
	* Provided alternative binary for environments with UI issues

* 2021/05/01
	* Added Apple Silicon support
	* Changed minimum macOS version to 10.11
	* Fixed voice allocation for programs with no waveform data
	* Fixed Loop checkbox not toggling correctly in some cases
	* Fixed VST version overwriting ch1 program number with selected channel's number on save

* 2020/03/21
	* Changed internal voice management processing

* 2017/04/22
	* Separate SR values can now be set for key-on and key-off states

* 2017/03/19
	* Added SPC/SMC format performance recording export
	* Added voice allocation method setting
	* Added pitch modulation and noise features

* 2016/01/31
	* Added G.I.M.I.C SPC module support
	* Official 64-bit VST support
	* Integrated [Blargg's Audio Engine](http://www.slack.net/~ant/libs/audio.html)
	* Fixed portamento being disabled on reset in some hosts
	* Fixed notes not being cut when multiple note-ons occur simultaneously in mono mode
	* RAM display now turns red when capacity is exceeded
	* Improved key-on noise
	* Fixed UI display issues on Windows

* 2014/10/19
	* Fixed mono mode not producing sound correctly

* 2014/09/20
	* Implemented help feature
	* Added control change support for various parameter changes
	* Added SustainMode
	* Added random waveform generation
	* Added feature to mute 8ms before note-on to prevent noise
	* Added independent PitchBendRange per MIDI channel
	* Added portamento support
	* Added mono mode
	* Added per-MIDI-channel voice priority settings
	* Dropped PowerPC support
	* Changed Mac minimum requirement to 10.6
	* Improved stability

* 2013/11/10
	* Fixed crash on startup in some cases

* 2013/03/26
	* Created VST, Windows, and 64-bit versions
	* Implemented linear velocity mode
	* XI export now uses original waveform files when available
	* Changed save format to raw BRR (AddmusicM format)
	* SR setting now used for release envelope
	* Added restriction preventing loading of waveforms larger than 64kB

* 2012/06/17
	* Fixed BRR encoding bug

* 2012/06/03
	* Added per-track maximum polyphony counting
	* Improved BRR encoding method

* 2012/05/23
	* Added multi-channel support
	* Added bank feature
	* Integrated echo feature
	* Added XI format export

* 2011/11/10
	* Changed velocity curve

* 2011/11/08
	* Initial provisional release

## Known Issues

* The VibDepth1-16 plugin parameters may not work in some hosts. In that case, use Control Change 1 instead.
* In Engine: Accurate mode, behavior may become erratic if memory is exceeded. If this happens, restart the plugin.
* Unexpected issues may occur in host environments that have not been verified.

## Distribution
[http://picopicose.com](http://picopicose.com)
