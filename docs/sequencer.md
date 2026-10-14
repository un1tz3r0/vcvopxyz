# Sequencer

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/sequencer-dark.png">
  <img alt="The Sequencer, playing" src="assets/sequencer-light.png" width="550">
</picture>

The OP-Z's front panel, and the master of the OP-Z modules to its right. It keeps the tempo and the transport and
sequences the OP-Z's first eight tracks: kick, snare, perc and sample, then bass, lead, arp and chord. Each track has
16 steps of up to four notes, with micro timing, note lengths, step count and step length, and ten project slots
that save it all.

## The chain

Place engines to its right, Screens beside them if you like, and they need no cables. The first synth engine plays
track 5 (bass), the next track 6, and so on, and the first [Drum](drum-tracks.md) track 1 (kick), the next track 2,
and so on to track 4, wherever they sit in the row. Each track's notes, velocity and the tempo go to its engine over
the bus, and engines with their own outputs unpatched are mixed into **L** and **R**. To play other modules from a
synth track, use a [Breakout](breakout.md) for its engine: it sends the track's notes out as CV. A module from another
plugin ends the chain. The engines have no dials of their own: the Sequencer's four turn them, and a Screen beside
one shows every page.

## The face

The face copies the OP-Z's, icons and all, scaled up to fill the panel above the jacks: four dials over 17 columns
of round keys. **TRACK**, **REC** and **−**, **PLAY** and **+**, **STOP** and **SHIFT** on the left, the 16 steps along
the top under the track icons, the black keys numbered 1 to 0 on the column lines, and the white keys along the
bottom under the step component icons. Each key has an RGB LED that lights up its symbol. Top left are the OP-Z's
four index keys, **PROJECT**, **MIXER**, **TEMPO** and **SCREEN**, and under them, where the OP-Z's speaker is, the
display and the selected track's channel dots. Under the face, the four page keys over the jacks, and a key per
track, named.

## Tracks and dials

- **Selecting a track:** click its key under the face (white while selected, orange while it plays), or switch
  **TRACK** on and click one of steps 1 to 8. The keys, steps, dials and recording all work on the selected track.
- **Dials:** they are the selected track's engine's dials, so turning one turns that parameter on the engine, with
  its own tooltip, undo and MIDI mapping. Click **SHIFT**, or one of the four page keys, to change the page; the
  page keys and the dials' LEDs show it in its colour. In track, tempo and mixer mode they set those modes' own
  values instead.

## Steps

- **Pasting and clearing:** click an empty step to paste the last notes played (a chord, if they were down
  together), or a lit step to clear it. Steps past the step count glow dimly and don't play. Right-click a step to
  set its notes' velocity, micro timing and length.
- **Holding a step:** Rack can't hold one button while clicking another, so a long press or a Ctrl+click (⌘+click
  on a Mac) picks a step up, and it blinks until clicked again. Either can be switched off in the context menu. While
  a step is held, the keys add and remove its notes, clicking another step makes its notes last to the end of that
  one (blue LEDs show how far; the same click again restores the default length), and **−** and **+** nudge it a
  tick, 1/24 of a step. Picking a step up while stopped plays it, and copies it for pasting.
- **Parameter locks:** while a step is held, the dials are that step's. Turn one to lock its parameter on the step,
  and its LED turns white; double-click it to let go of the lock. A lock holds the engine's dial where you set it
  for as long as the step is under the playhead, then lets it go back to where the dial is, so the knob itself
  never moves. Any dial on any of the four pages can be locked, and a held step plays with its locks while stopped,
  so you can hear them. A step's tooltip counts its locks, and its context menu clears them.
- **Step components:** with **SHIFT** latched, steps take components that bend how they play. See
  [Step components](step-components.md).

## Modes

- **Track mode:** click **TRACK** to switch it on, and again to switch it off. Steps 1 to 8 select a track. The dials
  set its **note length** (1/64 to a bar with the dotted values between, then drone), the engine's **note style**,
  **quantize** (pulls micro timing towards the grid as it plays) and the engine's **portamento**. The black keys set
  the step length in blue (1 to 8 for ×1 to ×8, 9 for ×16, 0 for one step per trigger on **ADV**), and **−** and
  **+** shift the pattern a step. With SHIFT latched too, the steps set the step count. **STOP** lets go of the
  notes, and held for a second clears the track.
- **SHIFT:** a click turns the page. A long press or a Ctrl+click latches it, for track mode's step count, the
  step components and the mixer's sound mutes.

The index keys stand in for the OP-Z's held keys the same way: click one to switch its mode on, again to switch it
off. Switching one on switches the others, and track mode, off.

- **Tempo:** the dials set the **tempo** (40 to 200 BPM), **swing** (50%, straight, to 75%, which delays every
  second sixteenth), the **metronome sound** (click, wood, beep, cowbell, hat or blip, higher on each bar's first beat) and the
  **metronome level** (off fully left; it plays into **L** and **R** while the sequencer plays). Type a tempo on the
  black keys, three digits as on the OP-Z, tap one on the white keys, or nudge it a BPM with **−** and **+**. The
  key's LED flashes on the beat.
- **Mixer:** the dials set the **drum level** (tracks 1 to 4), the **synth level** (tracks 5 to 8), **punch** (a
  compressor on **L** and **R** that gets louder as it squashes) and the **master level** of **L** and **R**. Steps
  1 to 8 light for the tracks that play; click one to mute its track, which stops its gates. With SHIFT latched, a
  click mutes only its sound, and the step lights red. The black keys pick one of ten mute groups, each its own set
  of mutes, and the display shows which.
- **Project:** the black keys are ten project slots, lit dimly for the ones in use and brightly for the one
  playing. Click one to load it; the project left is saved to its own slot first, as on the OP-Z, unless
  **Save the project on switching** is off in the context menu's **Projects**. A long press or a Ctrl+click saves
  the project to that slot instead. An empty slot loads as a new, blank project. Holding **STOP** for a second clears
  every track. The display shows the project's number.
- **Screen:** hold it to see the battery, on the display and on the steps in green. There is no battery, so it runs
  down slowly while playing and charges while stopped.

## Projects

A slot keeps everything that makes a song: each track's pattern, step components, parameter locks, step count and
length, note length, quantize and polyphony, every dial of the engine playing it (the note style and portamento too),
the tempo, swing and metronome, and the mixer's levels and ten mute groups. Loading one sets the engines' dials too, so each
engine plays the sound it had. The slots save with the patch, and a switch is one step of Rack's undo, which puts
the project left back as it was.

The context menu's **Projects** moves them in and out of files:

- **Save the slots to a file** and **Load the slots from a file** keep all ten in one JSON file, to carry them from
  one patch to another.
- **Export the slots to a projects folder** writes each slot in use as `project01.opz` to `project10.opz`, the
  files the OP-Z shows in its disk mode's `projects` folder. **Import a projects folder** reads them back into the
  slots of the same numbers, and **Import a project** reads one file into the current slot.

The OP-Z's project files hold far more than a slot does: 16 patterns per track, chains, the drum tracks' samples and
the FX, master and module tracks. An imported file is kept with the patch, and its export writes the slot over it, so
whatever the Sequencer doesn't model goes back unchanged. Only the first pattern is read and written, and micro
timing is written from −12 to +11 ticks, as far as the OP-Z's files go. The file
format comes from the OP-Z community's reverse engineering, not from teenage engineering, and has not been tried on
a real OP-Z yet, so copy your OP-Z's projects somewhere safe before you put exported files on it.

## Recording

Click **REC** to switch it on. While playing, notes from the keys and inputs go into the nearest step with their
micro timing and length. Hold notes on the inputs and click **−** to erase them from the steps they pass instead,
until they're released. While stopped, REC records a step at a time: each note or chord goes into the blinking step,
which moves on when they're released. Click a step to move it there.

## Transport and clock

- **Transport:** **PLAY** starts every track from the first step, or again from it while playing. **STOP** stops,
  and when stopped already, lets go of every note, the live ones too. Pattern edits can be undone like any other
  change.
- **Transport jacks:** **RUN** plays from the start while high and stops when it falls. **CONT** carries on from
  where it stopped. **RESET** starts over while playing. The outputs of the same names follow the sequencer: RUN is
  high while playing, and RESET and CONT pulse as it starts and carries on.
- **Clock:** the display shows the tempo, with **INT** lit. A patched **CLOCK**, at 4 pulses per beat unless you
  change it in the context menu, takes over and lights **EXT**, and the display shows dashes until it has timed two
  pulses. PLAY then waits for the next pulse, and the sequencer follows the pulses as they speed up, slow down or
  stop. The **CLOCK** output pulses while it plays.

## Notes in

- **Keys and inputs:** the keys, F3 to E5 as on the OP-Z, play the selected track while they're down, and so do notes
  on the poly **V/OCT**, **GATE** and **VEL** inputs (from MIDI-CV, say), which follow their V/OCT while held. **−**
  and **+** move the keyboard an octave.
- **Polyphony:** as the OP-Z's, two channels for each drum track, so a step can hold two sounds, three for the
  lead, four for the chords, and one for bass and arp. Change the selected
  track's in the context menu. A new note takes the channel silent longest, or failing that the one playing longest,
  and its gate drops for a sample so the engine retriggers.
