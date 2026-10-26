# Sequencer

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/Sequencer-large-dark.png"><img alt="The Sequencer, playing" src="assets/modules/Sequencer-large-light.png"></picture>

The Sequencer is the OP-Z: its face, its tracks and every sound. It plays 11 tracks: four drum tracks, four synth
tracks, two FX tracks and the master track. It keeps the tempo, and it mixes every track into its **L** and **R**
outputs. [How it works](how-it-works.md) explains the model. New to the Sequencer? Start with
[Your first song](guide.md).

## The chapters

The manual tells about the Sequencer in chapters, as teenage engineering's OP-Z guide does.

| Chapter | Tells how to |
| --- | --- |
| [Controls](controls.md) | Find each key and jack, and read the colours. |
| [Steps](steps.md) | Play the keys, and write, latch, lock, record and clear steps. |
| [Step components](step-components.md) | Change how a step plays, for example with ratchets or ramps. |
| [Track](track.md) | Select a track, choose its plug and presets, and set its step count and step length. |
| [Project](project.md) | Play and chain patterns, and save and load projects. |
| [Mixer](mixer.md) | Set the levels, and mute tracks. |
| [Tempo](tempo.md) | Set the tempo, the swing and the metronome, or follow an outside clock. |
| [Master track](master.md) | Move the synth tracks in the song's key. |

The plugs have their own pages: [synth tracks](synth-tracks.md), [drum tracks](drum-tracks.md), [kits](kits.md) and
[FX tracks](fx-tracks.md).

## Transport

- **PLAY** starts every track from step 1. While the Sequencer plays, PLAY starts again from step 1.
- **STOP** stops. If the Sequencer is stopped already, STOP releases every note, also the notes on the inputs.

Outside project mode, **PLAY** starts the [chain](project.md#chains) from its first pattern.

## Jacks

The jacks are in one row under the face.

| Jack | Direction | Does |
| --- | --- | --- |
| **CLOCK** | In | An outside clock, at 4 pulses for each beat. It sets the tempo: see [Tempo](tempo.md#an-outside-clock). |
| **RUN** | In | Plays from step 1 when it goes high, and stops when it goes low. |
| **RESET** | In | Starts again from step 1, while the Sequencer plays. |
| **CONT** | In | Continues from the step where the Sequencer stopped. |
| **V/OCT**, **GATE**, **VEL** | In | Polyphonic notes for the selected track, for example from MIDI-CV. They play and [record](steps.md#record) as the keys do. **VEL** is 0 to 10 V. |
| **ADV** | In | Moves each track with step length **Per ADV trigger** one step. |
| **CLOCK** | Out | Pulses while the Sequencer plays, at the same rate as the **CLOCK** input. |
| **RUN** | Out | High while the Sequencer plays. |
| **RESET**, **CONT** | Out | A pulse when the Sequencer starts, or continues. |
| **L**, **R** | Out | The mix of every track. |

Each jack has a small light that shows its level.

## The row

The other modules of this plugin work in the Sequencer's [row](how-it-works.md#the-row): the modules to its right that
touch it and each other.

- A [breakout](breakout.md) gives one track jacks. The track's key on the display shows a small jack while a breakout
  has the track.
- A [Screen](screen.md) shows the selected track's pages and dials, and edits a drum track's kit.

## The context menu

Right-click the Sequencer to open its context menu.

| Item | Does |
| --- | --- |
| **Clock pulses per beat** | Sets the rate of the **CLOCK** jacks, from 1 to 96. |
| **polyphony** | Sets the selected track's [polyphony](track.md#polyphony). The item starts with the track's name. |
| **Plug slots** | Puts a plug in each track's [plug slots](track.md#fill-the-plug-slots). |
| **Long-press toggles hold** | Lets a long press latch a step or **SHIFT**. On at the start. |
| **Ctrl-click toggles hold** | Lets a Ctrl+click (⌘+click on a Mac) latch a step or **SHIFT**. On at the start. |
| **Projects** | Saves the project on a switch, and moves the project slots in and out of [files](project.md#project-files). |
