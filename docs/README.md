# vcvopxyz manual

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/guide/voices-dark.png"><img alt="A Sequencer playing two Drums, Digital, Analog, Organ and EP, with a Delay and a Reverb" src="assets/guide/voices-light.png"></picture>

## Start here

- [Your first sound](guide.md): from an empty rack to a bassline, a beat and an echo, (written for
  2.0, being rewritten for the plugs).
- [Quickstart](quickstart.md): the face, clicks and holds, the modes and the colours on one page, and the OP-Z's
  combos side by side with the clicks here.

## Modules

| | Module | Plays | What it is |
| --- | --- | --- | --- |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/Sequencer-label-dark.png"><img alt="sequencer" src="assets/modules/Sequencer-label-light.png" height="56"></picture> | [Sequencer](sequencer.md) | Every track | The OP-Z, face and voices: ten plug slots a track, tempo, transport, eight tracks of 16 steps, [step components](step-components.md), parameter locks, recording, the mixer, the master track, 16 patterns with chains, and project slots. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/Breakout-label-dark.png"><img alt="synth breakout" src="assets/modules/Breakout-label-light.png" height="56"></picture> | [Synth Breakout](breakout.md) | Tracks 5 to 8 | A synth track that sends its notes, envelope, LFO and dials out as CV and takes its voice back in. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/DrumBreakout-label-dark.png"><img alt="drum breakout" src="assets/modules/DrumBreakout-label-light.png" height="56"></picture> | [Drum Breakout](breakout.md#drum-breakout) | Tracks 1 to 4 | A drum track that sends its triggers, sounds, velocity, envelope, LFO and dials out and takes its sound back in. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/FxBreakout-label-dark.png"><img alt="send fx breakout" src="assets/modules/FxBreakout-label-light.png" height="56"></picture> | [Send FX Breakout](breakout.md#send-fx-breakout) | FX 1 or 2 | An FX track that sends its send, notes, time, LFO and dials out and takes an outside effect back in. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/Screen-label-dark.png"><img alt="screen" src="assets/modules/Screen-label-light.png" height="56"></picture> | [Screen](screen.md) | Nothing | The OP-Z's phone app for the selected track: every page at once, every dial a touch arc, and a drum track's kit editor. |

The engines and send effects are the tracks' **plugs**, inside the Sequencer: each track has ten slots, filled from the Sequencer's context menu (**Plug slots**), and TRACK with a black key picks one.

| Plug | Tracks | |
| --- | --- | --- |
| [Drum](drum-tracks.md) | Tracks 1 to 4 | A drum track: a 24-sound kit, built in or loaded from OP-1 and OP-Z kits, AIFF and WAV, with the drum note styles. |
| [Digital](synth-tracks.md#the-engines) | Tracks 5 to 8 | A sine driven past full scale so it wraps like an overflowing integer, with phase feedback. |
| [Analog](synth-tracks.md#the-engines) | Tracks 5 to 8 | Saw, sub and noise through an enveloped low-pass. |
| [Cluster](synth-tracks.md#the-engines) | Tracks 5 to 8 | Seven detuned saws per note. |
| [Organ](synth-tracks.md#the-engines) | Tracks 5 to 8 | Drawbar FM organ in eight algorithms. |
| [Bow](synth-tracks.md#the-engines) | Tracks 5 to 8 | A bowed string waveguide, with a second string for chorus. |
| [Electric](synth-tracks.md#the-engines) | Tracks 5 to 8 | Two sines modulating each other, at any ratio from 0.5 to 4. |
| [Saw](synth-tracks.md#the-engines) | Tracks 5 to 8 | Two saws through an enveloped resonant low-pass. |
| [Shade](synth-tracks.md#the-engines) | Tracks 5 to 8 | A soft FM piano with detune and drive. |
| [Uranus](synth-tracks.md#the-engines) | Tracks 5 to 8 | A clean sine bass with harmonics and feedback. |
| [Volt](synth-tracks.md#the-engines) | Tracks 5 to 8 | Saw, pulse and sub, with a width sweep and ring modulation. |
| [EP](synth-tracks.md#the-engines) | Tracks 5 to 8 | FM electric piano in eight algorithms, from tines to a toy. |
| [Delay](fx-tracks.md#delay) | FX 1 or 2 | A digital delay, its time set by notes in tempo. |
| [Reverb](fx-tracks.md#reverb) | FX 1 or 2 | A plate, its predelay set by notes. |
| [Rymd](fx-tracks.md#rymd) | FX 1 or 2 | The OP-Z's older reverb, a room. |
| [Chorus-80](fx-tracks.md#chorus) | FX 1 or 2 | A bucket-brigade stereo chorus. |
| [Crush](fx-tracks.md#crush) | FX 1 or 2 | A bitcrusher with a low-pass after it. |
| [Dist](fx-tracks.md#dist) | FX 1 or 2 | An oversampled overdrive with a tone control. |

The pages by kind: [Sequencer](sequencer.md), [step components](step-components.md), [synth tracks](synth-tracks.md),
[drum tracks](drum-tracks.md), [FX tracks](fx-tracks.md), [breakouts](breakout.md) and the [Screen](screen.md).

## Getting it running

- Installing: [Linux](../INSTALL.linux.md), [macOS](../INSTALL.macos.md) (Intel and Apple silicon),
  [Windows](../INSTALL.windows.md).
- [Development](development.md): building, releasing, the source tree, panels, the manual's pictures, and adding a
  module.

## Behind it

- [DESIGN.md](../DESIGN.md): how the modules fit together, and the choices made where the OP-Z guide is silent or
  where this plugin differs from it.
- [TODO.md](../TODO.md): what's next, and the loose ends.
