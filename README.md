# vcvopxyz

<picture><source media="(prefers-color-scheme: dark)" srcset="docs/assets/guide/band-dark.png"><img alt="A Sequencer playing kick, snare, bass and chords, with a Screen beside it" src="docs/assets/guide/band-light.png"></picture>

## What is it?

An ode to the [Teenage Engineering OP-Z](https://teenage.engineering/products/op-z), this is a [VCV Rack](http://vcvrack.com/) plugin which re-imagines the iconic pocket synthesizer as a virtual eurorack module.

## How do I use it?

You will need [VCV Rack](http://vcvrack.com/). Although the free version will do, *please* consider purchasing the full version to support it's awesome creators.

Grab the `.vcvplugin` file for your OS and architecture from the newest version at the top of the project's [Releases](https://github.com/un1tz3r0/vcvopxyz/releases) page, and drop it into your VCV Rack plugins folder... If you're not sure how to find that, have a look at the **Install** instructions for your platform: [Linux](INSTALL.linux.md), [MacOS](INSTALL.macos.md) and [Windows](INSTALL.windows.md).

The main **Sequencer** module can be used stand-alone, just like the real OP-Z, or in combination with **Breakout** expander modules for patching individual sequencer tracks to external chains in place of the builtin sounds.

## What it is not

Rather than try and make a sample-perfect clone of each sound and FX engine, the goal was to build something similar *enough* to be instantly usable by anyone who is familiar with the original. What it lacks in sounding exactly like the hardware, it makes up for by allowing the sequencer tracks to be diverted to 1v/oct pitch and gate/trig outputs so that they can control chains of external modules whose output can then be routed back into the OP-Z's track-fx and mixer. 

#### This project is not affiliated with or endorsed by teenage engineering. OP-Z and OP-XY are their trademarks.

## Intros

These pages will be helpful for those with little or no prior experience with the OP-Z.

- [Beginner's Guide](docs/guide.md): step-by-step instructions starting with an empty rack to making a simple loop, with screenshots, for those without any experience with the OP-Z.
- [Quickstart](docs/quickstart.md): a brief rundown of the core features for those with some prior OP-Z experience who could use a refresher.

If you just want to see what it can do, have a look at the example patches in [patches/](patches). 

## Modules

The **Sequencer** module is the whole OP-Z: its face, its sequencer, and every track's voice. There are four drum tracks ("Kick", "Snare", "Perc" and "Sample"), four synth tracks ("Bass", "Lead", "Arp" and "Chord"), and two send effect tracks, plus a master track which transposes the synth tracks in the song's detected scale. Each track plays one of its ten **plugs**, the drum kits and synth and effects engines that the OP-Z's phone app puts in slots, picked with TRACK and a black key, so a Sequencer and an audio output are all it takes to make music.

The rest of the modules sit to the Sequencer's right. A **breakout** takes a track and gives it jacks: patch its audio outputs and the track's plug plays out of them instead of the mix, or patch only its note CVs and the track's notes, envelopes and dials go out to other modules while their sound comes back into the track's FX sends and mixer. Breakouts take the tracks of their kind in the order they sit in the row, and show on their screens which one they took. The **Screen** shows the selected track's pages and edits its kit.

The Sequencer has V/OCT, GATE and velocity inputs for recording notes, clock and transport inputs and outputs, and clocks itself when
no clock is patched, at a tempo set in its tempo mode as on the OP-Z. The tracks reach other modules through the
breakouts, not through cables from the Sequencer. New to it? Start with [your first sound](docs/guide.md).


| | Module | |
| --- | --- | --- |
| <picture><source media="(prefers-color-scheme: dark)" srcset="docs/assets/modules/Sequencer-label-dark.png"><img alt="Sequencer" src="docs/assets/modules/Sequencer-label-light.png" height="56"></picture> | [Sequencer](docs/sequencer.md) | The OP-Z's front panel: tempo, transport, eight tracks of 16 steps with micro timing, a master track that transposes them in the key it detects, an arpeggiator on the arp track, recording, [step components](docs/step-components.md), a mixer, 16 patterns per project with chains, and ten project slots that import and export the OP-Z's project files. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="docs/assets/modules/Breakout-label-dark.png"><img alt="Synth Breakout" src="docs/assets/modules/Breakout-label-light.png" height="56"></picture> | [Synth Breakout](docs/breakout.md) | A synth track that sends its notes, envelope, LFO and dials out as CV and takes its voice back in. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="docs/assets/modules/DrumBreakout-label-dark.png"><img alt="Drum Breakout" src="docs/assets/modules/DrumBreakout-label-light.png" height="56"></picture> | [Drum Breakout](docs/breakout.md#drum-breakout) | A drum track that sends its triggers, sounds, velocity, envelope, LFO and dials out as CV and takes its sound back in. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="docs/assets/modules/FxBreakout-label-dark.png"><img alt="Send FX Breakout" src="docs/assets/modules/FxBreakout-label-light.png" height="56"></picture> | [Send FX Breakout](docs/breakout.md#send-fx-breakout) | An FX track that sends its send, notes, time, LFO and dials out and takes an outside effect back in. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="docs/assets/modules/Screen-label-dark.png"><img alt="Screen" src="docs/assets/modules/Screen-label-light.png" height="56"></picture> | [Screen](docs/screen.md) | Place this extender anywhere to the right of a Sequencer to see the selected track's waveform scope/envelope shape, voice allocation, and all of its knob parameter pages at once. **Also allows editing drum kits** when a drum track is selected (click the "Kit" button in the top right corner). |

The engines and send effects are the tracks' **plugs**, inside the Sequencer: each track has ten slots, filled from the Sequencer's context menu, and TRACK with a black key picks one.

| Plug | |
| --- | --- |
| [Drum](docs/drum-tracks.md) | The four drum tracks: 24 sounds a [kit](docs/kits.md): the OP-Z's own, sampled, synthesized ones, or loaded from OP-1 and OP-Z kits, AIFF and WAV, with the drum note styles. |
| [Digital](docs/synth-tracks.md#the-engines) | A sine driven past full scale so it wraps like an overflowing integer, with phase feedback. |
| [Analog](docs/synth-tracks.md#the-engines) | Saw, sub and noise through an enveloped low-pass. |
| [Cluster](docs/synth-tracks.md#the-engines) | Seven detuned saws per note. |
| [Organ](docs/synth-tracks.md#the-engines) | Drawbar FM organ in eight algorithms. |
| [Bow](docs/synth-tracks.md#the-engines) | A bowed string waveguide, with a second string for chorus. |
| [Electric](docs/synth-tracks.md#the-engines) | Two sines modulating each other, at any ratio from 0.5 to 4. |
| [Saw](docs/synth-tracks.md#the-engines) | Two saws through an enveloped resonant low-pass. |
| [Shade](docs/synth-tracks.md#the-engines) | A soft FM piano with detune and drive. |
| [Uranus](docs/synth-tracks.md#the-engines) | A clean sine bass with harmonics and feedback. |
| [Volt](docs/synth-tracks.md#the-engines) | Saw, pulse and sub, with a width sweep and ring modulation. |
| [EP](docs/synth-tracks.md#the-engines) | FM electric piano in eight algorithms, from tines to a toy. |
| [Delay, Reverb, Rymd, Chorus, Crush, Dist](docs/fx-tracks.md) | The OP-Z's send effects, on FX tracks 9 and 10: fed by every track's FX 1 and FX 2 sends. |

## Holding keys

Much of the OP-Z's features are accessed by holding a modifier key to temporarily switch the behavior of the other keys, or, in other words, key combinations. Since you can't click on more than one thing at a time (at least, not without a second mouse plugged in), we needed a workaround, so keys that can be held on the real OP-Z can be *latched* here, instead.

- **TRACK**, **REC** and the index keys switch on and off with a click
- a step or **SHIFT**, which also
do something on a click, latch with a long press or a Ctrl+click (⌘+click on a Mac)
- Either behavior can be turned off in the context menu.

The [quickstart](docs/quickstart.md) has complete descriptions of the recognized combinations and their equivalents on the real OP-Z.

## Further Reading

- [Your first sound](docs/guide.md): a walk from an empty rack to a beat and a voice of your own, with screenshots.
- [Quickstart](docs/quickstart.md): the face, clicks and holds, modes and colours.
- [The manual](docs/README.md): every module, page and key.
- Installing: [Linux](INSTALL.linux.md), [macOS](INSTALL.macos.md) (Intel and Apple silicon),
  [Windows](INSTALL.windows.md).
- [Development](docs/development.md): building, releasing, the source tree and the panel generator.
- [DESIGN.md](DESIGN.md): how it fits together. [TODO.md](TODO.md): what's next.

## License

MIT, see [LICENSE](LICENSE). The Nunito font in `res-src/fonts` (SIL Open Font License) is used only to generate
panel artwork and isn't shipped with the plugin.
