# For OP-Z owners

This page is for readers who know the OP-Z. It gives each combination of the hardware and the clicks that do the same
thing here. Then it lists what is different, and what is not here. [Controls](controls.md) names each part of the
face.

## What is the same

The face, its icons and its colours are the OP-Z's. The tracks have the OP-Z's numbers and names, and the dials have
the OP-Z's four pages. The steps, step components, parameter locks, patterns, chains and project slots work as on the
hardware. The Sequencer can also read and write OP-Z project files: see [Project](project.md#project-files).

## Combinations

A mouse clicks one key at a time. Thus a key that you hold on the OP-Z latches here. **TRACK**, **REC** and the index
keys latch on a click. A step and **SHIFT** latch on a long press or a Ctrl+click (⌘+click on a Mac).

| On the OP-Z | Here |
| --- | --- |
| Hold track, press a step: select a track | Click the track's key on the display. Or latch **TRACK** and click the step. |
| Hold track, turn a dial | Latch **TRACK**, then turn the dial. |
| Hold track, press a value key: the plug in that slot | Latch **TRACK**, then click a black key. |
| Hold track and shift, press a value key: step length | Latch **TRACK** and **SHIFT**, then click a black key. |
| Step count (the OP-Z has no combination) | Latch **TRACK** and **SHIFT**, then click a step. |
| Hold track, press a white key: load a preset | Latch **TRACK**, then click a white key. |
| Store a preset | Latch **TRACK**, then long-press or Ctrl+click a white key. |
| Hold track, press rec: a random sound | Latch **TRACK**, then click **REC**. |
| Hold track, press − or +: move the pattern | Latch **TRACK**, then click **−** or **+**. |
| Hold track, hold stop: clear the track | Latch **TRACK**, then hold **STOP** for a second. |
| Press shift: the next page | Click **SHIFT**, or a page name on the display. |
| Hold shift, press steps, a white key and a value key: step components | Latch **SHIFT**, click steps, then a white key and a black key. |
| Hold shift, press stop: clear the step components | Latch **SHIFT**, then click **STOP**. |
| Hold a step, play keys: change its notes | Latch the step, then click keys. |
| Hold a step, turn a dial: a parameter lock | Latch the step, then turn the dial. Double-click the dial to remove the lock. |
| Hold a step, press another step: note length | Latch the step, then click the other step. |
| Hold a step, press − or +: micro timing | Latch the step, then click **−** or **+**. Or right-click the step. |
| Hold a step, hold stop: clear its locks | Latch the step, then hold **STOP**. |
| Hold a step, pitch bend: velocity | Right-click the step. |
| Hold rec while playing: record | Latch **REC** while the Sequencer plays. |
| Hold rec, turn a dial: record locks | Latch **REC** while the Sequencer plays, then turn the dial. |
| Hold rec, press −: erase notes | Latch **REC**, hold notes on the **GATE** input, then click **−**. |
| Hold project, press a value key: load a project | Latch **PROJECT**, then click a black key. |
| Hold project, hold a value key: save a project | Latch **PROJECT**, then long-press or Ctrl+click a black key. |
| Hold project, press a step: play a pattern | Latch **PROJECT**, then click a step. |
| Hold project, press play, then steps: a chain | Latch **PROJECT**, click **PLAY**, click steps, then click **PLAY**. |
| Hold project, press shift, then steps: copy a pattern | Latch **PROJECT**, click **SHIFT**, click steps, then click **SHIFT**. |
| Hold mixer, press a step: mute | Latch **MIXER**, then click the step. Latch **SHIFT** first to mute only the sound. |
| Hold tempo, turn the green dial, type or tap | Latch **TEMPO**, then do the same. |
| Triple-click an index key: index lock | Not necessary. The index keys always latch. |

## What is different

- **The display** beside the dials does the job of the phone app. It shows the plug, the tempo and the pages. A
  [Screen](screen.md) module shows every dial, as the app does.
- **Plugs:** each track has ten plug slots on the black keys in track mode. The context menu fills them. Each plug has
  14 presets on the white keys. See [Track](track.md#plugs).
- **Breakouts** give a track jacks, so that other modules can play its notes or change its sound. See
  [Breakouts](breakout.md).
- **Quantize** pulls notes toward the grid as they play. The OP-Z applies it to notes that you record.
- **A sound mute** sets a track's level to zero before its FX sends. Thus a track with a sound mute does not feed the
  FX tracks.
- **Tempo:** **−** and **+** in tempo mode move the tempo by one beat a minute.
- **The Delay's time** comes from notes, as the Reverb's predelay does.
- **Drum keys:** the octaves above and below F3 to E5 play the 24 sounds again. They are not silent.
- **The metronome** has six clicks of its own, in place of the OP-Z's counting voice.
- **The battery** on **SCREEN** is not real. It empties while the Sequencer plays, and fills while it is stopped.
- **The sounds** have the OP-Z's dials, ranges and behaviour, but they are not copies of its samples. They are tuned
  by ear.

[DESIGN.md](../DESIGN.md#where-it-differs-from-the-op-z-guide) gives each difference and the reason for it.

## What is not here

- Tracks 11 and 13 to 16: tape, perform, module, lights and motion.
- The Sample engine. The other engines are plugs.
- Punch-in effects, linked tracks, temporary tweaks with SHIFT, and record arm.
- Super panic, pitch bend, bounce, snapshots and tempo lock.
- Copying a pattern's settings or a track.
- The master track's effects page, sampling, and the MIDI settings.
