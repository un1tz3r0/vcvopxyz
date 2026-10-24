# Design

How vcvopxyz is put together, and why. For what the modules do, see the [manual](docs/README.md).

## Goals

- **Workflow and feel, not samples.** The modules copy the OP-Z's parameters, ranges, pages and quirks, and are
  tuned by ear. Nothing is null-tested against recordings of the hardware.
- **The OP-Z first, the OP-XY later.** The OP-Z's synth tracks and sequencer come first because each tests the
  other; the OP-XY's sequencer and engines follow once the OP-Z's are complete.
- **One box, no cables.** The hardware is one box, so the Sequencer plays every track itself, each with the plug
  picked for it. Breakouts to its right hand a track to other modules when it should be patched by hand.
- **Rack-native where the hardware can't translate.** Latched modes stand in for held keys, a CV input stands in for
  the motion sensor, and every dial is an ordinary Rack param with undo, MIDI mapping and typed values.

## Plugs and the hub

`src/opz/Plugs.hpp`, `src/opz/Host.hpp`, `src/opz/Hub.hpp`. Every sound a track can play is a plug: the eleven
synth engines, the drum tracks' kit player and the six effects, each registered from its own file by a static
`SynthPlug<Engine>`, `FxPlug<Effect>` or `PlugRegistration`, with its slug (its module's before 2.1.0), its kind and
its OP-Z id where one is known. Each kind's breakout registers a plug too, whose voice is other modules.

The Sequencer keeps a `HostedTrack` per track: a core of every plug of the track's kind and one of its breakout's,
all bound to the same 22 params of the Sequencer's (`trackParam`), the breakout's jacks and the track's lights. One
core plays. A switch of plug is a swap of pointers on the audio thread, nothing made or freed: the core left behind
takes no more notes and rings out under the new one until it's quiet (0.1 s, or 3 s for an effect, whose echoes have
gaps; 10 s at most), and the new one takes over the page, the clock and the LFO. FX units allocate their delay
lines when the sample rate changes, on the UI thread, never at a switch.

A track's two sound dials are stored as shares of their travel, 0 to 1, whichever plug plays, so steps, scenes and
the row record hold them alike. A `PlugDial` shows itself as the playing plug's own dial, configured once on a
module of its own (`Plug::dial`), and a core turns a share into its units through the plug's `SoundRange`.

Ten slots a track sit on the black keys in track mode: the synth and FX tracks' hold plugs, a drum track's hold kits
(`KitCore`, `src/opz/KitCore.hpp`), each an OP-Z kit, a built-in one or one from a file kept in the Sequencer's patch
storage. Each pattern remembers its slot.

Every frame the Sequencer writes each lane into one `Downstream`, runs the eight tracks' cores, sums their FX sends,
runs the two FX tracks' with this frame's sends, and mixes: no track is a sample behind any other.

The hub is how breakouts and Screens to its right reach the tracks. A breakout finds the first Sequencer to its left,
and its track: Auto takes the first of its kind that no breakout further left has, a fixed one its own if free. Each
frame it writes its inputs and which of its outputs are patched into the hub (`JacksIn`), and copies out what the
track's core wrote to its patched outputs (`JacksOut`). Both are double-buffered by Rack's frame count, so a
breakout's jacks are a sample behind the track whichever thread runs it. What a breakout has patched decides who
plays: its note outputs without its audio outputs hand the track to the breakout's core, which sends the notes out
and takes the sound back at its return; its audio outputs keep the plug playing, out of them instead of the mix.

Placement only changes when modules move, so breakouts walk left again only after Rack has given some module of
this plugin a new neighbour: every module's `onExpanderChange` bumps a plugin-wide count (`busLayout`). The UI thread
never keeps a hub: a Screen or a breakout's status screen walks left each frame (`findHub`). A row of a Sequencer
playing every track (four drums, four synths, two effects) takes 2.8 µs a sample on one thread, as the same tracks
did as eleven modules in 2.0.27. The master's dials carry no state of their own: each frame, a `MasterDial` widget
re-points itself at the param it turns now, so a turn is that param's own, with its tooltip, undo and MIDI mapping.

## Tracks

`src/opz/TrackCore.hpp`. `TrackCore` is every track, drum, synth or FX: the tempo, gate edges, the LFO, the
filter, the mix, the meters and the state published for the UI through a `TripleBuffer`, which the status screen and
a Screen read. A core supplies its voices through `noteOn`, `noteOff`, `releaseAll` and `envelope`, and may hide a
dial (`dial`) or name a key's sound (`soundName`). `SynthCore<Engine>` (`src/opz/SynthTrack.hpp`) adds poly, mono
and legato voices with portamento around an engine; `KitCore` (`src/opz/KitCore.hpp`) plays a kit.

A core holds no `Module`. It reads and writes params, jacks and lights through pointers (`params`, `inputs`,
`outputs`, `lights`, `paramQuantities`), by the ids in `TrackIds`, so its code reads as a module's would. Each frame
the Sequencer hands it the lanes (`bus`) and the slot its sound goes into (`slot`); its track (`busTrack`) is set
once. Patch data goes through `dataToJson`/`dataFromJson` on the core, saved by its plug's slug.

- **Per sample:** the bus, the tempo, the notes, and each voice's envelope and sound. Voices are summed
  before the filter: the filter has one cutoff for the whole track and is linear, so filtering the mix sounds the
  same as filtering each voice, for a fraction of the work.
- **At control rate:** the LFO and the dials after it (`Controls`). `modulated()` adds the LFO's offset when it
  targets a dial, and the sweep component's offset, both as shares of the dial's travel, so the knob stays put while
  the sound moves.
- **After the voices:** the filter pair, level, FX sends, the soft limit (linear to ±5V, then bending towards ±10V)
  and the pan, then the outputs, the bus mix and the meters. A filter whose state goes NaN or infinite, from a bad
  input on Breakout's RET say, clears it, so the track recovers on the next sample.

A breakout's panel has no dials, as the hardware's tracks have none of their own: the Sequencer's turn the selected
track's, and a Screen turns them too. Its status screen (`TrackStatus`) reads the same published state as a Screen,
both on the UI thread, which is the `TripleBuffer`'s one consumer.

### Synth engines

An engine supplies only its sound:

```cpp
struct Analog {
	static constexpr const char* SLUG = "Analog";            // its plug's slug, its module's before 2.1.0
	static void configSound(Module* m, int p1, int p2);     // configures its two sound dials
	struct Voice {
		// One sample, about ±1. `envelope` is the voice's amp envelope, 0 to 1, for an engine with a filter envelope.
		float process(float frequency, float sampleTime, float p1, float p2, float envelope);
	};
};
static opz::SynthPlug<Analog> analogPlug;                 // and its OP-Z plug id, where it has one
```

Synth Breakout (slug `Breakout`, kept for old patches) is the exception that proves the split: its core derives
from `SynthCore<BreakoutSound>` and overrides `process` to send each stage out as CV and skip the stages whose
outputs are patched, with `lfoInside` turning the LFO's inside targets off. The kit player and Drum Breakout share `DrumCore`
(`src/opz/DrumTrack.hpp`): the drum dials, note styles and keys. The synth and drum breakouts share
`TrackCore::emitReturn`, the return through whatever isn't patched out. Send FX Breakout's core derives from
`FxCore` and overrides `process` to send the bus's send out and take the effect back through the filter; it shares
Delay's time keys through `opz::TempoTime`.

The FM engines (Organ, Electric, Shade, Uranus, Volt, EP) share `xyz::sine` and `xyz::softClip` in
`src/dsp/Sine.hpp`: a 4096-point table, linearly interpolated, and a rational tanh. The saws use `src/dsp/Blep.hpp`.
Bow is the one physical model, a bowed waveguide after STK's: its bridge low-pass never sits below the note's
eighth harmonic and its friction curve stays shallow, since past either the string jumps to an upper mode, and the
delay is shortened by the low-pass's phase delay to keep it in tune.

### Effects

`src/opz/FxTrack.hpp`. `FxCore<Effect>` is a `TrackCore` with `effect` set, around an effect's `Unit` (`src/Delay.cpp`
and the rest), which takes a stereo sample and returns the effect alone. It has no voices and only the sound and LFO
pages, so `dial` hides the rest and `nextPage` skips them. Its notes set `timeKey`, which the effect turns into
seconds against the tempo; the LFO's attack and pitch target slots stand in for time and send. The return goes
through the track's filter pair and level, so mutes, locks and the mix work as for any track. The delay lines
(`src/dsp/Delay.hpp`) are powers of two in size, read with Hermite interpolation where a time moves.

### Drum kits

`src/opz/Kit.hpp` is a kit as the OP-1 and OP-Z keep one: one buffer of mono samples and 24 slices into it, each
with its tuning, gain, direction and play mode. `Kit.cpp` reads OP-1 and OP-Z kit AIFFs (the slices are JSON in
an "op-1" application chunk), plain AIFF, AIFC and WAV, and folders of sounds, all at 44.1 kHz mono. A loaded kit
is saved in the Sequencer's patch storage as a float WAV (`kit-<track>-<slot>.wav`), with its slices in the
Sequencer's JSON. Files are taken as
untrusted: rates outside 1 to 384 kHz are refused, samples that aren't finite become 0 and the rest are held to
±4, slice speeds to four octaves either way, and names that aren't UTF-8, which JSON can't hold, lose their other
bytes to '?'.

The built-in kits (`BuiltinKits.cpp`) are synthesized from short recipes, a tone with a pitch sweep, FM and a
partial, noise and 808-style metal through a filter, a click and drive. All four are made once, on the UI thread,
when the first Drum is created (about 200 ms, 10 MB).

The OP-Z kits are samples of the device's own (Vic owns three OP-Zs and is fine with shipping them; teenage
engineering gets asked before any wider release). `tools/kits/split.py` cuts Vic's bounces in `samples/` (each
kit's 24 sounds played one after another on a grid) into `res/kits/<kind>-<n>.wav`, the sounds back to back in mono
16-bit, and `.json`, their slices: the same pair a patch keeps a loaded kit in, so `Kit::fromSaved()` reads them. The
OP-Z outputs digital silence between hits, so a sound runs from the first sample after a silence to its last above
-72 dBFS, with 5 ms faded out; the grid finds sounds that start under the last one's tail, and pads with nothing on
their beat stay empty. `FactoryKits.cpp` lists the kits once and reads each the first time the UI thread asks for it,
then keeps it, so the audio thread can play it through a pointer the Drum stores. A patch saves the kit's file
name.

A kit from a file reaches the audio thread through a mutex the audio thread only `try_lock`s, and the kit it
replaces goes back to the UI thread to be freed, so the audio thread never blocks or frees memory.

The Screen edits the playing slot's kit through three `TrackCore` hooks (`kit()`, `editSlice()`, `lastSlice()`) that
only the kit player fills in.
Slices change in place while they play: the audio thread reads a slice's ends once a sample and stops a voice that
finds them less than two samples apart, so a half-made edit costs at most a cut-off sound. Editing a built-in kit
first copies it in as a loaded kit. `Kit::toOp1()` writes the OP-1 kit format the loader reads, resampled to 44.1
kHz, with the unknown fields (envelope, FX, LFO) at the values OP-1 kits commonly carry.

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
  frame and pushes a `PatternChange` when they differ. Every edit, from any source, is one undo step, and so is a
  project switch (below).
- **Components** are a bitmask and a value per step. Most act in the player when it schedules a slot; sweeps go out
  on the lane as offsets; the gate step jump advances the tracks that wait for triggers, as **ADV** does.
- **Parameter locks** are a mask and a value per step over a track's 22 params (the arp's four last), each a share of
  the dial's travel, as the OP-Z's files keep them. The player marks each slot with whether the step's parameter spark
  let its locks through, and the lane carries the locks of the slot under the playhead; the engine reads every dial
  through `value()`, which takes a lock over the param, so the knob stays put. While a step is held, the dials
  turn four `LockQuantity` params on the Sequencer that read and write the held step's locks on the UI thread, as
  the step menu does, and the pattern's undo records the change. Recording live, the audio thread watches the
  engine params under the four dials at each control step, and a change locks the param on the step under the
  playhead; a recalled scene moves them without counting as a turn. The undo history waits until the dials have been
  still half a second, so a turn is one edit (the knob's own move is another).
- **Arp:** the arp lives in the arp track's player, so every plug, the breakout's included, gets its notes as
  ordinary ones on the lane. With it on, the player's channels hold the notes (all 16, whatever the polyphony) and a second
  set of channels, as many as the polyphony, carries what it plays. Its dials are the arp track's four extra params,
  which its core shows on the LFO page and the Sequencer reads as turned; the player applies the
  step's locks to them itself, so a locked step's first note already follows them. A held sequenced note keeps its
  pitch and tonality component, and each arp note moves it as the master track says then. In OP-Z files the arp
  track's LFO bytes hold its arp dials.
- **Modes:** TRACK and the three latching index keys (PROJECT, MIXER, TEMPO) are one mode at a time, each taking
  over the dials, steps and value keys. The dials re-point at the mode's own params, so the mixer's levels and the
  metronome are ordinary params with undo and MIDI mapping too.

### Projects

`src/opz/Project.hpp` is a project slot as plain data, and the reader and writer for the OP-Z's own project files,
with no Rack in either. The slots live in the Sequencer and save with the patch as JSON. A slot holds each pattern's
plug slot and dials, the dials as shares of each one's range, as the OP-Z's files do.

Switching projects happens in the audio thread, from the keys; files are read and written in the UI thread. A mutex
guards the slots between the two, which the audio thread only `try_lock`s: a key's switch or save, or a slot the UI
thread hands back by number, waits for a control step when the lock is free. A switch keeps the live project either
side of it, and the widget pushes them as one `ProjectSwitch` undo step, which puts back the patterns, settings,
dials and mixer together; it takes the same lock, so it never sees a switch half done.

Patches from 2.0.5 and earlier had keys where the mixer's and the metronome's params now are. Rack keeps each
module's plugin version in the patch, so a Sequencer from before 2.0.6 resets those six to their defaults on loading.
Param and port ids are append-only from 2.0.6 on.

2.1.0 moved every engine into the Sequencer. A Sequencer from 2.0.25 on recorded the engine modules on its row
(`row`: each one's slug, its dials as shares and its settings), so an older patch's tracks pick those plugs up from
their slots, or the last slot where a track's defaults lack it; a kit loaded from a file was the Drum module's and is
lost. Rack reports the engine modules themselves as missing. A breakout's first param was a sound dial, and is now
its track selector, so one from before 2.1.0 goes back to Auto.

The .opz layout comes from the OP-Z community's reverse engineering (see the comment at the top of `Project.hpp`).
Each imported file is kept, in the patch's storage folder, so an export writes the slot over the file it came from
and keeps everything the Sequencer doesn't model, such as the drum kits and any pattern never played. The synth and
FX tracks' plugs go by their OP-Z ids: on import a pattern plays the slot holding the file's plug, if any does, and
on export it writes its slot's plug where that has an id (Analog, Organ, EP and Rymd have none, and leave the file's).

A project's 16 patterns are `Project::Scene`s. The one playing lives where a single pattern always did, in the lanes,
the params, the tracks' dials and their plugs, and the other 15 wait in the Sequencer's `scenes`; a switch stores the one
playing into its scene and recalls the next, in the audio thread under the slots' lock. A chain moves on in
`followChain`, every sample: since each `Player` schedules a slot one ahead, it sets `Player::hold` to the
pattern's end, so no track schedules a slot from there on, and once the end is reached it switches and calls
`restartPattern` on every lane, which starts them all on their first step at that moment, whatever their step
lengths. If the UI thread has the lock, the switch waits for it rather than the audio thread, a few samples late at
worst. A slot is about 400 KB, too big for an audio thread's stack, so a blank one is kept rather than built when an
empty slot loads.

## Panels

Panels are generated, never drawn by hand. `res-src/panel.py` builds every panel from Python (`panelkit.py` for
the kit, `synth_track.py` for the shared synth track layout, `opz.py` for the OP-Z's icons and keys) and writes
light and dark SVGs into `res/`, with text converted to paths. A hidden `components` layer records where each widget
goes, by its C++ enum name, and `PanelLayout` reads it back at runtime, so the panel is the only place the layout is
written down. Every track's panel carries its kind as a chip between the top screws, so the three kinds tell apart at
a glance in the module browser: SYNTH in cyan with a wave, DRUM in orange with a drum, FX in magenta with a send.
The colours come from the OP-Z's LEDs, away from the four dial colours that mean pages.

The OP-Z's artwork, name and logo stay off the panels: the icons are redrawn, and the modules are named by what they
do, so the plugin can go to the VCV Library some day.

## Where it differs from the OP-Z guide

Checked against teenage engineering's OP-Z guide. Some differences are forced by a mouse, and the rest are choices
Vic made to keep (2026-10-21).

- **Held keys latch:** TRACK, REC and the index keys switch on and off with a click; a step and SHIFT latch with a long
  press or Ctrl+click. Index lock (triple-click) has nothing to do.
- **Step length** is TRACK, SHIFT and a black key, and TRACK and a black key pick the plug, as in the guide; on the
  master track, which has no plugs, TRACK and a black key set it too. **Step count**, which the guide gives no
  combo for, is TRACK and SHIFT and a step.
- **Plugs** come from ten slots per track, filled from the context menu; presets aren't stored or randomized yet.
- **Quantize** pulls notes towards the grid as they play; the guide applies it to notes recorded live.
- **Sound mutes** zero a track's level before its sends, so a sound-muted track stops feeding the FX tracks; the
  guide says muted tracks still send to the effects.
- **Tempo nudge:** TEMPO with − and + moves the tempo a BPM; the guide's nudge, for keeping time by ear with others,
  may be a momentary push instead.
- **Delay time** is set by notes, as the guide has the reverb's predelay set; the guide doesn't say this of the
  delay.
- **Not modelled:** tracks 11 and 13 to 16 (tape, perform, module, lights, motion), punch-in effects, linked tracks,
  temporary tweaks with SHIFT, record arm, super panic, pitch bend, copying a pattern's settings or a track, bounce,
  snapshots, tempo lock, the master track's effects page, sampling and MIDI settings.

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
- **Digital's dials** follow Vic's reading of the hardware: the sine wraps around as it's driven, octave setting how
  many times each peak wraps (0 to 7, continuously), and feedback is the sine's phase modulating itself, up to three
  tenths of a cycle, before the wrap. The wrap runs at twice the sample rate with antiderivative anti-aliasing. A
  patch from before 2.0.17, when these dials were the modulator's octave and FM depth, resets them on loading.
- **LFO:** synced speeds run 1/64 at the far left to 2/1 just left of centre, in the order the guide lists them, so
  the dial is slowest in the middle. Depth is squared, for fine vibrato; at full depth the LFO sweeps a target's
  whole range, or ±2 octaves of pitch. Free shapes are bipolar; note-triggered shapes are unipolar.
- **Filter:** two 12 dB state-variable filters, Q from 0.5 to 10. The dial sweeps a low-pass up across its left half
  and a high-pass up across its right.
- **Step components:** values follow the guide's chart where its text survives; where it doesn't (multiply's broken
  chord and quantize, sweep's long versions, the sparks' modes), the closest reading of the hardware's behaviour.
- **Key detection:** the guide says only that the OP-Z finds the key from the synth tracks. Here each candidate key
  and mode scores by how much of the notes' weight it holds (the bass counted twice), then by how much of each step's
  lowest note falls on its root, then by mode, most common first: Ionian, Aeolian, Mixolydian, Dorian, Lydian,
  Phrygian, Locrian. It's found every 256 samples and when the sequencer starts.
- **Master track:** home is the key's root nearest middle C. A note in the key moves the tracks by scale degrees (so
  the mode follows), and one outside it by semitones (so the key changes). Latch lets go at the pattern's start
  rather than the bar's. The tonality component's values follow the guide's names; the three quantize values snap
  to the moved scale, triad, and root and fifth, the lower note winning a tie.
- **Arp:** the guide names the dials and the six patterns; a community wiki (hyphz/opzdoc) gives the styles as
  accent strings, which are read here as velocities: H as played, M three quarters, L half, and rests. It lists
  style 3 the same as style 2, so style 3 here is the same rhythm moved to start on the accent (HLLHLLHL). The eight
  speeds (1/4 to 1/64 with the triplets between), range as 1 to 4 octaves, notes half the time to the next, the arp following
  swing, and its grid starting with the first key while stopped are guesses. A patch saved before the arp has it off
  on load, so it still plays the arp track as before.
- **FX:** the guide names the effects and their two dials and little else. Delay's keys run 1/64 to 3/8 in 64ths
  (3/16 by default) and its feedback tops out at 98%; Reverb is Dattorro's plate with the keys as 10 ms of predelay
  each; Rymd is Freeverb's room; Chorus-80 is a 3.5 ms BBD-style chorus; Crush holds and μ-law quantizes; Dist is a
  4× oversampled tanh with a slight bias. The FX tracks' LFO targets are the synth track's, with attack moving the
  time and pitch the send. The returns come back at unity, with no dial for their level.
- **Untuned:** engine voicing, the built-in kits and the drums' level against the synths, sweep depth (half a dial's travel) and glide times (up to a beat) are first guesses,
  waiting to be tuned by ear.
- **Index keys:** the OP-Z's are held while turning dials or pressing keys, so here three latch, like TRACK, and
  SCREEN, which only shows the battery, works while held. The battery is pretend, running down while playing.
- **Mixer:** levels are squared, as the dials' are; a mute stops a
  track's gates, while a sound mute (SHIFT) only zeroes its level. Punch is a compressor with
  make-up gain, and the master level comes after it.
- **Parameter locks** hold for the step's slots, from its slot's start until the next step's, rather than for its
  notes' lengths, so a lock on an empty step changes the notes still sounding from earlier ones. A held step plays
  its locks only while stopped; while playing, the playhead's step does.
- **Metronome:** the OP-Z counts in a voice that can't be copied, so its sounds are six synthesized clicks.
- **Patterns and chains:** a pattern's end, for a chain, is its longest track's (step count times step length),
  or a bar if every track waits for triggers. Each pattern has its own sound, as the OP-Z's files store it, but a
  pattern never played leaves the tracks' dials and plugs as they are. The mute group is per pattern; the groups themselves
  are the project's. Chains are picked and saved on the keys of project mode rather than with PROJECT held, since
  the index keys latch: PLAY builds one, the white keys keep 14 of the file's 15 saved chains, and SHIFT copies. A
  note with negative micro timing on the first step of a chain's next pattern plays on the step, since that pattern
  isn't in until its start.
- **Project files:** what the track settings' bytes mean is unknown, so the step count, step length, note length,
  quantize, swing and the length of notes are written in the most likely reading and read back the same way.
  Mutes and the mixer's mute groups aren't written.
