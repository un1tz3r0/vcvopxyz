# Design

How vcvopxyz is put together, and why. For what the modules do, see the [manual](docs/README.md).

## Goals

- **Workflow and feel, not samples.** The modules copy the OP-Z's parameters, ranges, pages and quirks, and are
  tuned by ear. Nothing is null-tested against recordings of the hardware.
- **The OP-Z first, the OP-XY later.** The OP-Z's synth tracks and sequencer come first because each tests the
  other; the OP-XY's sequencer and engines follow once the OP-Z's are complete.
- **One row, no cables.** The hardware is one box, so the modules are one row: a master on the left and engines to
  its right that find their tracks by position. Every connection the bus makes can still be patched by hand.
- **Rack-native where the hardware can't translate.** Latched modes stand in for held keys, a CV input stands in for
  the motion sensor, and every dial is an ordinary Rack param with undo, MIDI mapping and typed values.

## The bus

`src/opz/Bus.hpp`. The Sequencer sits at the left end of a row as the master. Every OP-Z module is a `BusNode`,
which owns the double buffers Rack's expander messages flip between, and passes on what it gets, a sample later:

- **Downstream** (rightward): each track's lane (poly pitch, velocity, gate, the sweep offsets, the parameter
  locks of the step playing, and its level from the mixer, 0 while its sound is muted), the tempo, the selected track, track mode, page requests, and counts of
  the drum and synth engines passed so far.
- **Upstream** (leftward): the L/R mix of every engine whose own outputs are unpatched, each engine's page, and the
  selected engine's four dial values for the master's dial LEDs.

An engine takes its track from its kind's count: the first drum engine plays track 1, the next track 2, and so on
to track 4, and the first synth engine track 5, and so on to track 8, so the two kinds can mix in any order. A track whose GATE output is patched sends an empty lane, which
takes its engine off the bus without moving the others. A module from another plugin ends the bus, since
`onBus` checks the plugin before casting. A Screen only relays.

Expander messages are the only safe way to share state between modules that Rack may run on different threads, and
the one-sample delay per module is inaudible for notes and dials. The master's dials carry no state of their own:
each frame, a `MasterDial` widget re-points itself at the param it turns now, mostly one of the selected engine's,
so a turn is that param's own, with its tooltip, undo and MIDI mapping.

## Tracks

`src/opz/Track.hpp`. `Track` is every track, drum or synth: the bus, the tempo, gate edges, the LFO, the filter,
the mix, the meters and the state published for the UI through a `TripleBuffer`, which the status screen and a
Screen read. A track supplies its voices through `noteOn`, `noteOff`, `releaseAll` and `envelope`, and may hide a
dial (`dial`) or name a key's sound (`soundName`). `SynthTrack<Engine>` (`src/opz/SynthTrack.hpp`) adds poly, mono
and legato voices with portamento around an engine; `Drum` (`src/Drum.cpp`) plays a kit.

- **Per sample:** the bus, the tempo, the notes, and each voice's envelope and sound. Voices are summed
  before the filter: the filter has one cutoff for the whole track and is linear, so filtering the mix sounds the
  same as filtering each voice, for a fraction of the work.
- **At control rate:** the LFO and the dials after it (`Controls`). `modulated()` adds the LFO's offset when it
  targets a dial, and the sweep component's offset, both as shares of the dial's travel, so the knob stays put while
  the sound moves.
- **After the voices:** the filter pair, level, FX sends, the soft limit (linear to ±5V, then bending towards ±10V)
  and the pan, then the outputs, the bus mix and the meters.

The panel has no dials, as the hardware's tracks have none of their own: the Sequencer's turn the selected track's,
and a Screen turns any. Its status screen (`TrackStatus`) reads the same published state as a Screen beside it,
both on the UI thread, which is the `TripleBuffer`'s one consumer.

### Synth engines

An engine supplies only its sound:

```cpp
struct Analog {
	static constexpr const char* SLUG = "Analog";            // module slug, and its panel in res/
	static void configSound(Module* m, int p1, int p2);     // configures its two sound dials
	struct Voice {
		// One sample, about ±1. `envelope` is the voice's amp envelope, 0 to 1, for an engine with a filter envelope.
		float process(float frequency, float sampleTime, float p1, float p2, float envelope);
	};
};
Model* modelAnalog = createModel<opz::SynthTrack<Analog>, opz::SynthTrackWidget<Analog>>(Analog::SLUG);
```

Breakout is the exception that proves the split: it derives from `SynthTrack<BreakoutSound>` and overrides
`process` to send each stage out as CV and skip the stages whose outputs are patched, with `lfoInside` turning the
LFO's inside targets off.

### Drum kits

`src/opz/Kit.hpp` is a kit as the OP-1 and OP-Z keep one: one buffer of mono samples and 24 slices into it, each
with its tuning, gain, direction and play mode. `Kit.cpp` reads OP-1 and OP-Z kit AIFFs (the slices are JSON in
an "op-1" application chunk), plain AIFF, AIFC and WAV, and folders of sounds, all at 44.1 kHz mono. A loaded kit
is saved in the patch's storage folder as a float WAV, with its slices in the module's JSON.

The built-in kits (`BuiltinKits.cpp`) are synthesized from short recipes, a tone with a pitch sweep, FM and a
partial, noise and 808-style metal through a filter, a click and drive, so the plugin ships no samples. All four
are made once, on the UI thread, when the first Drum is created (about 200 ms, 10 MB).

A kit from a file reaches the audio thread through a mutex the audio thread only `try_lock`s, and the kit it
replaces goes back to the UI thread to be freed, so the audio thread never blocks or frees memory.

## The sequencer

`src/opz/Sequence.hpp` holds the pattern, the track settings and the `Player`, with no Rack in it, so it can be
tested on its own. `src/opz/Sequencer.cpp` is the module: the face, the modes, recording, the clock and the bus.

- **Time** is counted in sixteenths, the length of a step at step length ×1. A step is 24 ticks, for micro timing
  and note lengths, so both stretch with the step length.
- **Two clocks:** the player's clock always runs, for notes played live. Song time stands still while stopped, so
  CONT carries on where it stopped and sweeps stay in phase with the pattern.
- **One slot ahead:** the player schedules each step slot a slot early, which covers micro timing from −23 to +24
  ticks and still starts every note exactly on time. A slot is one step's turn under the playhead; pulse and pulse
  hold give a step several.
- **Channels:** each track's poly outputs have a fixed number of channels. A new note takes the channel silent
  longest, or failing that the one playing longest, and drops its gate for a sample so the engine retriggers.
- **Undo:** the pattern changes in the audio thread, so the widget compares it with the last recorded copy every
  frame and pushes a `PatternChange` when they differ. Every edit, from any source, is one undo step.
- **Components** are a bitmask and a value per step. Most act in the player when it schedules a slot; sweeps go out
  over the bus as offsets on the lane; the gate step jump advances the tracks that wait for triggers, as **ADV** does.
- **Parameter locks** are a mask and a value per step over a track's 18 params, each a share of the
  dial's travel, as the OP-Z's files keep them. The player marks each slot with whether the step's parameter spark
  let its locks through, and the lane carries the locks of the slot under the playhead; the engine reads every dial
  through `value()`, which takes a lock over the param, so the knob stays put. While a step is held, the dials
  turn four `LockQuantity` params on the Sequencer that read and write the held step's locks on the UI thread, as
  the step menu does, and the pattern's undo records the change.
- **Modes:** TRACK and the three latching index keys (PROJECT, MIXER, TEMPO) are one mode at a time, each taking
  over the dials, steps and value keys. The dials re-point at the mode's own params, so the mixer's levels and the
  metronome are ordinary params with undo and MIDI mapping too.

### Projects

`src/opz/Project.hpp` is a project slot as plain data, and the reader and writer for the OP-Z's own project files,
with no Rack in either. The slots live in the Sequencer and save with the patch as JSON. A slot holds the engines'
dials as shares of each dial's range, so a project sets the same dial positions whichever engine plays a track, as
the OP-Z's files do.

Switching projects happens in the audio thread, from the keys; files are read and written in the UI thread. A mutex
guards the slots between the two, and the UI thread hands a changed slot back by number, for the audio thread to
load at its next control step.

The .opz layout comes from the OP-Z community's reverse engineering (see the comment at the top of `Project.hpp`).
Each imported file is kept, in the patch's storage folder, so an export writes the slot over the file it came from
and keeps everything the Sequencer doesn't model: the other 15 patterns, chains, the drum and FX tracks.

## Panels

Panels are generated, never drawn by hand. `res-src/panel.py` builds every panel from Python (`panelkit.py` for
the kit, `synth_track.py` for the shared synth track layout, `opz.py` for the OP-Z's icons and keys) and writes
light and dark SVGs into `res/`, with text converted to paths. A hidden `components` layer records where each widget
goes, by its C++ enum name, and `PanelLayout` reads it back at runtime, so the panel is the only place the layout is
written down.

The OP-Z's artwork, name and logo stay off the panels: the icons are redrawn, and the modules are named by what they
do, so the plugin can go to the VCV Library some day.

## Choices made where the OP-Z guide is silent

- **Note styles** use the OP-Z's names: mono retriggers on every change, legato doesn't, and neither restarts the
  note-triggered LFO shapes on a slide. Portamento glides in mono and legato only.
- **Drum note styles:** retrig rings each sound to its end, two or more at once (as many as the track's channels);
  mono cuts the last sound with a 3 ms fade; gate releases the envelope with the note; loop loops the slice while
  held, then plays it out. A kit slice set to gate or loop keeps its mode in any style. Unconfirmed against the
  hardware.
- **Drum keys:** F3 to E5 play the 24 sounds, and other octaves wrap around to them rather than going silent.
- **OP-1 kit encodings:** markers at 4058 to a sample and the play mode and reverse values are well established;
  pitch as 512ths of a semitone and volume with 8192 as unity are guesses.
- **Digital's dials:** octave steps the modulator from −2 to +3 octaves from the note, and feedback is FM depth, up
  to two cycles of phase deviation. Unconfirmed against the hardware.
- **LFO:** synced speeds run 1/64 at the far left to 2/1 just left of centre, in the order the guide lists them, so
  the dial is slowest in the middle. Depth is squared, for fine vibrato; at full depth the LFO sweeps a target's
  whole range, or ±2 octaves of pitch. Free shapes are bipolar; note-triggered shapes are unipolar.
- **Filter:** two 12 dB state-variable filters, Q from 0.5 to 10. The dial sweeps a low-pass up across its left half
  and a high-pass up across its right.
- **Step components:** values follow the guide's chart where its text survives; where it doesn't (multiply's broken
  chord and quantize, sweep's long versions, the sparks' modes), the closest reading of the hardware's behaviour.
  Ramps use C major until a tonality master exists.
- **Untuned:** engine voicing, the built-in kits and the drums' level against the synths, sweep depth (half a dial's travel) and glide times (up to a beat) are first guesses,
  waiting to be tuned by ear.
- **Index keys:** the OP-Z's are held while turning dials or pressing keys, so here three latch, like TRACK, and
  SCREEN, which only shows the battery, works while held. The battery is pretend, running down while playing.
- **Mixer:** levels are squared, as the engines' are, and only reach the engines over the bus; a mute stops a
  track's gates, outputs included, while a sound mute (SHIFT) only zeroes its level. Punch is a compressor with
  make-up gain, and the master level comes after it.
- **Parameter locks** hold for the step's slots, from its slot's start until the next step's, rather than for its
  notes' lengths, so a lock on an empty step changes the notes still sounding from earlier ones. A held step plays
  its locks only while stopped; while playing, the playhead's step does.
- **Metronome:** the OP-Z counts in a voice that can't be copied, so its sounds are six synthesized clicks.
- **Project files:** what the track settings' bytes mean is unknown, so the step count, step length, note length,
  quantize, swing and the length of notes are written in the most likely reading and read back the same way.
  Mutes and the mixer's mute groups aren't written.
