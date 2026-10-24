# Quickstart

How to work the Sequencer's face with a mouse: what's where, how a click differs from a hold, the modes, and what
the colours mean. For a first patch that makes sound, see the [guide](guide.md). Every key is covered in full in
[the Sequencer's manual](sequencer.md).

If you've used an OP-Z, most of this will be familiar: the face, its icons and its colours are the hardware's, and
so is the idea that holding one key changes what the others do. What's new is how a mouse holds a key, and the
[table at the end](#for-op-z-owners) maps the hardware's combos to the clicks here.

## The face

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/face-off-dark.png"><img alt="The Sequencer's face, with no notes on any track" src="assets/keys/face-off-light.png"></picture>

| | Where | What it does |
| --- | --- | --- |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/project-off-dark.png"><img alt="PROJECT" src="assets/keys/project-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/mixer-off-dark.png"><img alt="MIXER" src="assets/keys/mixer-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/metronome-off-dark.png"><img alt="TEMPO" src="assets/keys/metronome-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/screen-off-dark.png"><img alt="SCREEN" src="assets/keys/screen-off-light.png" height="36"></picture> | Top left corner | The four index keys: modes that change what the other keys do. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/page0-off-dark.png"><img alt="Page 1" src="assets/keys/page0-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/page1-off-dark.png"><img alt="Page 2" src="assets/keys/page1-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/page2-off-dark.png"><img alt="Page 3" src="assets/keys/page2-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/page3-off-dark.png"><img alt="Page 4" src="assets/keys/page3-off-light.png" height="36"></picture> | Beside the display | The dials' four pages: sound, envelope, LFO, mix. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/select4-off-dark.png"><img alt="Bass" src="assets/keys/select4-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/fx_select0-off-dark.png"><img alt="FX 1" src="assets/keys/fx_select0-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/master-off-dark.png"><img alt="MASTER" src="assets/keys/master-off-light.png" height="36"></picture> | The row under the display | A key per track: the eight tracks, the two FX tracks, then the master track. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/dial0-off-dark.png"><img alt="Green dial" src="assets/keys/dial0-off-light.png" height="60"></picture> | Top right | Four dials, green, blue, yellow and red, for the selected track's sound. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/track-off-dark.png"><img alt="TRACK" src="assets/keys/track-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/rec-off-dark.png"><img alt="REC" src="assets/keys/rec-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/octave_down-off-dark.png"><img alt="−" src="assets/keys/octave_down-off-light.png" height="36"></picture> | The left column | Track mode, recording, and an octave down. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/play-off-dark.png"><img alt="PLAY" src="assets/keys/play-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/stop-off-dark.png"><img alt="STOP" src="assets/keys/stop-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/octave_up-off-dark.png"><img alt="+" src="assets/keys/octave_up-off-light.png" height="36"></picture> <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/shift-off-dark.png"><img alt="SHIFT" src="assets/keys/shift-off-light.png" height="36"></picture> | The second column | The transport, an octave up, and SHIFT. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/step0-off-dark.png"><img alt="Step 1" src="assets/keys/step0-off-light.png" height="36"></picture> | Along the top | The 16 steps of the selected track, each under the icon of the track it selects in track mode. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key1-off-dark.png"><img alt="Black key 1" src="assets/keys/key1-off-light.png" height="36"></picture> | Numbered 1 to 0 | The keyboard's black keys, and the ten values of everything that takes a number. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/key0-off-dark.png"><img alt="White key" src="assets/keys/key0-off-light.png" height="36"></picture> | Along the bottom | The keyboard's white keys, each under the icon of a step component. |

The keyboard runs F3 to E5 across the black and white keys; the steps sit over it, not in it.

## Clicks and holds

Most of the OP-Z's tricks are two keys at once: hold one, press another. A mouse has one button, so a key that the
OP-Z holds **latches** here: it stays on until you click it again, and blinks or lights white to say so.

| Gesture | Does | On |
| --- | --- | --- |
| **Click** | Presses the key. | Every key. |
| **Click to switch on, click to switch off** | Latches a mode, since the key does nothing else on its own. Switching one of these on switches the others off; REC stays as it is. | **TRACK**, **PROJECT**, **MIXER**, **TEMPO**, and **REC** |
| **Long press** (0.4 s) or **Ctrl+click** (⌘+click on a Mac) | Latches a key that also does something on a click. | A step (picks it up), **SHIFT**, a black key in project mode (saves to the slot), a white key in project mode (saves the chain) |
| **Hold the mouse button down** | Works only while held, as on the hardware. | **SCREEN** (battery), **STOP** held a second (clears) |
| **Double-click a dial** | Lets go of a parameter lock, or resets the dial. | The four dials |
| **Right-click** | The key's own menu: a step's velocity, micro timing, length and locks. | Steps, and every key's tooltip names it |

The context menu's **Long-press toggles hold** and **Ctrl-click toggles hold** turn either way of latching off, if
you'd rather a long press did nothing, or need Ctrl+click for something else.

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/shift-shift-dark.png"><img alt="SHIFT latched" src="assets/keys/shift-shift-light.png" height="44"></picture> SHIFT is the one key with both: a click turns the dials' page, as on the OP-Z,
and a long press or Ctrl+click latches it for the things SHIFT is held for: the step count in track mode, the step
components, and sound mutes in the mixer.

## Modes

TRACK and the index keys are one mode at a time; SHIFT, a held step and REC work on their own or alongside. With
none on, the steps are the selected track's pattern and the keys play it.

| Mode | Lit | The steps | The keys | The dials |
| --- | --- | --- | --- | --- |
| None | | The pattern: click an empty step to paste the last notes played, a lit one to clear it | Play the track | The track's engine, on the page shown |
| **TRACK** | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/track-track-dark.png"><img alt="TRACK on" src="assets/keys/track-track-light.png" height="36"></picture> | Select a track: 1 to 8, 9 and 10 for FX, 12 for master | Black: step length | Note length, note style, quantize, portamento |
| **TRACK** + SHIFT | | The step count | | |
| **PROJECT** | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/project-project-dark.png"><img alt="PROJECT on" src="assets/keys/project-project-light.png" height="36"></picture> | The 16 patterns; PLAY builds a chain | Black: the ten project slots; white: saved chains | |
| **MIXER** | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/mixer-mixer-dark.png"><img alt="MIXER on" src="assets/keys/mixer-mixer-light.png" height="36"></picture> | Mute a track (with SHIFT, only its sound) | Black: the ten mute groups | Drum level, synth level, punch, master level |
| **TEMPO** | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/metronome-tempo-dark.png"><img alt="TEMPO on" src="assets/keys/metronome-tempo-light.png" height="36"></picture> | | Black: type a tempo; white: tap it | Tempo, swing, metronome sound, metronome level |
| **SHIFT** latched | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/shift-shift-dark.png"><img alt="SHIFT on" src="assets/keys/shift-shift-light.png" height="36"></picture> | Pick steps for components | White: a component; black: its value | |
| A step held | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/step0-held-dark.png"><img alt="A held step" src="assets/keys/step0-held-light.png" height="36"></picture> | Click another to stretch the held step's notes to it | Add and remove the step's notes | The step's parameter locks |
| **REC** | <picture><source media="(prefers-color-scheme: dark)" srcset="assets/keys/rec-rec-dark.png"><img alt="REC on" src="assets/keys/rec-rec-light.png" height="36"></picture> | Stopped: the step to record into | Notes recorded as you play | Playing: turns recorded as locks |

**SCREEN** shows the battery while held. **STOP**, held for a second in track or project mode, clears the track or
the pattern, with the steps filling red to show how long is left; let go early to keep them.

## Colours

The keys' LEDs keep the OP-Z's colours. On a step:

| Colour | Means |
| --- | --- |
| Orange | Notes on the step; dim past the step count, where the track doesn't play |
| White | The playhead; in track mode the selected track; in project mode the pattern playing |
| Red | The playhead while recording; a sound mute; a clear filling up |
| Blue | How long the held step's notes last; on the black keys in track mode with SHIFT, the step length |
| Green | Steps with step components; the rest of a chain; the battery |
| Blinking | A held step, or one picked for components |

The track keys are white for the one selected and orange while one plays. The page keys light in their page's colour
and the dials' small LEDs follow; a dial's LED turns white while the held step locks it.

## For OP-Z owners

The hardware's combos, and the clicks that do the same here. "Latch" means a click for TRACK, REC and the index
keys, and a long press or Ctrl+click for SHIFT and the steps.

| On the OP-Z | Here |
| --- | --- |
| Hold track, press a step to select a track | Click the track's key beside the dials, or latch TRACK and click the step |
| Hold track, turn a dial | Latch TRACK, turn the dial |
| Hold track, press a value key: pick the plug in that slot | Latch TRACK, click a black key |
| Hold track and shift, press a value key: step length | Latch TRACK and SHIFT, click a black key |
| Hold track, press a white key: load a preset | Latch TRACK, click a white key |
| Store a preset | Latch TRACK, long-press or Ctrl+click a white key |
| Hold track, press rec: randomize the sound | Latch TRACK, click REC |
| Step count (no combo on the OP-Z) | Latch TRACK and SHIFT, click a step |
| Hold track, press −/+: shift the pattern | Latch TRACK, click − or + |
| Hold track, hold stop: clear the track | Latch TRACK, hold STOP a second |
| Press shift: next page | Click SHIFT, or a page key |
| Hold shift, press steps, then a white key and a value key: step components | Latch SHIFT, click steps, then a white key and a black key |
| Hold shift, press stop: clear the components | Latch SHIFT, click STOP |
| Hold a step, play keys: edit its notes | Latch the step, play keys |
| Hold a step, turn a dial: parameter lock | Latch the step, turn the dial; double-click the dial to remove it |
| Hold a step, press another: note length | Latch the step, click the other |
| Hold a step, press −/+: micro timing | Latch the step, click − or +; or right-click the step |
| Hold a step, hold stop: clear its locks | Latch the step, hold STOP |
| Hold a step, pitch bend: velocity | Right-click the step |
| Hold rec while playing: record | Latch REC while playing |
| Hold rec, turn a dial: record locks | Latch REC while playing, turn the dial |
| Hold rec, press −: subtractive recording | Latch REC, hold notes on the inputs, click − |
| Hold project, press a value key: load a project | Latch PROJECT, click a black key |
| Hold project, hold a value key: save | Latch PROJECT, long-press or Ctrl+click a black key |
| Hold project, press a step: select a pattern | Latch PROJECT, click a step |
| Hold project, press play, then steps: chain | Latch PROJECT, click PLAY, click steps, click PLAY |
| Hold project, press shift, then steps: copy a pattern | Latch PROJECT, click SHIFT, click steps, click SHIFT |
| Hold mixer, press a step: mute | Latch MIXER, click the step (with SHIFT latched, the sound only) |
| Hold tempo, turn the green dial / type / tap | Latch TEMPO, the same |
| Triple-click an index key: index lock | Not needed: the index keys always latch |

Not here: the tape, perform, module, lights and motion tracks (11, 13 to 16); punch-in effects; plugs and presets
(each engine is a module you place instead); randomize; linked tracks; shift + dial temporary tweaks; record arm;
super panic; pitch bend; copying a pattern's settings or a track; bounce; snapshots; tempo lock; the master track's
effects page; sampling; MIDI settings and channels. The engines aren't the OP-Z's sounds sample for sample: they
have its dials, ranges and quirks, tuned by ear.
