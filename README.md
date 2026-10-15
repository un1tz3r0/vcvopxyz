# vcvopxyz

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/assets/rack-dark.png">
  <img alt="A Sequencer playing four Drums, Digital, Analog, Cluster and Organ, with a Screen beside Digital" src="docs/assets/rack-light.png">
</picture>

VCV Rack 2 plugin sequencer module and a set of expanders that work with it, acting as the drum and synth engines played by the eight sequencer tracks. It is an unashamed imitation of the [Teenage Engineering OP-Z](https://teenage.engineering/products/op-z), as fans will immediately recognize from the identical button layout and markings and familiar behavior as far as editing patterns and playing them back is concerned.

The goal of this project was to recreate the magic and expressiveness of the OP-Z's sequencing workflow, bringing to the software modular world something similar enough to be instantly usable by anyone who is proficient at playing the original. This project is likewise less about faithfully or exactly replicating the sound of the synth engines, filters and effects, as it's most interesting configuration revolves around using the `drum breakout` and `synth breakout` expanders to patch arbitrary third party module chains in to be driven by the drum and synth parts of the sequence, while routing their audio output back into the track's amplitude-envelope VCA, filter, send-effects buses and mixer. Used in this way, you can combine the ultimate flexibility, customizability and infinite possibilities of eurorack modular synthesis with the OP-Z's uniqely fluid, beautifully rich and deeply complex compositional and live performance features. Doesn't that sound pretty neat? I sure think so.

For those unfamiliar, this will likely be, as is the real thing upon unboxing, slightly confusing and frustrating, until one has learned TE's screenless, lightly modal and mostly easy-to-remember multi-button-combination-based interface. This may sound daunting, but be patient and trust that once you're comfortable with how it works, this little guy is so much fun to use that it can be hard to put down even after hours of playing.

Not affiliated with or endorsed by teenage engineering. OP-Z and OP-XY are their trademarks.

## Modules

The Sequencer module is the master, and controls up to eight expanders to its right (up to four drum kits and four synth engines), which pick up the tracks in the order they appear, showing the assigned track number, symbol and name on each attached engine's status panel. The main sequencer module has V/OCT, GATE and velocity inputs for recording notes, and clock and transport control inputs and outputs with pass-through, and clocks itself internally if no clock signal is patched. The tracks reach other modules through a Breakout, not through cables from the Sequencer. The tempo of the internal clock can be adjusted exactly as on the real OP-Z, using the tempo-mode button along the top of the device.

## Button Combos/Holding Buttons

One caveat of the virtual rack vs real hardware is that it is not possible to press and hold multiple buttons using multiple fingers and both hands. Therefore, a workaround was needed since much of the OP-Z's advanced features are accessible only while holding certain button combinations to temporarily switch the rest of the buttons (or some subset of them at least) into an alternate mode where the indicators and short- and long-presses have different meanings. The workaround takes several forms, depending on the button in question's other behaviors:
1. buttons which have no short-press or momentary behavior but that do something when held are always toggle-on-click instead of normally-off momentary-push
2. buttons which have a short-press action but no special long-press action will toggle on and stay on when long-pressed, with an additional short-press toggling them off again
3. buttons which have both a short-press and a long-press action that also do something when held can be toggled by control-clicking, which will also work for the other two types of button which can be held, described above. 

| Module | |
| --- | --- |
| [Sequencer](docs/sequencer.md) | The OP-Z's front panel: tempo, transport, eight tracks of 16 steps with micro timing, a master track that transposes them in the key it detects, an arpeggiator on the arp track, recording, [step components](docs/step-components.md), a mixer, and ten project slots that import and export the OP-Z's project files. |
| [Drum](docs/drum-tracks.md) | The four drum tracks: 24 sounds a kit, synthesized or loaded from OP-1 and OP-Z kits, AIFF and WAV, with the drum note styles. |
| [Digital](docs/synth-tracks.md#the-engines) | Two binary-counter oscillators reading a 4-bit sine, one frequency-modulating the other. |
| [Analog](docs/synth-tracks.md#the-engines) | Saw, sub and noise through an enveloped low-pass. |
| [Cluster](docs/synth-tracks.md#the-engines) | Seven detuned saws per note. |
| [Organ](docs/synth-tracks.md#the-engines) | Drawbar FM organ in eight algorithms. |
| [Breakout](docs/breakout.md) | A synth track that sends its notes, envelope, LFO and dials out as CV and takes its voice back in. |
| [Drum Breakout](docs/breakout.md#drum-breakout) | A drum track that sends its triggers, sounds, velocity, envelope, LFO and dials out as CV and takes its sound back in. |
| [Delay, Reverb, Rymd, Chorus, Crush, Dist](docs/fx-tracks.md) | The OP-Z's send effects, on FX tracks 9 and 10: every engine's FX 1 and FX 2 sends reach them over the bus. |
| [Screen](docs/screen.md) | The OP-Z's phone app: a picture of the page, and every dial as a touch arc. |

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/assets/sequencer-dark.png">
  <img alt="The Sequencer" src="docs/assets/sequencer-light.png" width="49%">
</picture>
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/assets/engines-dark.png">
  <img alt="Analog, Cluster and Organ" src="docs/assets/engines-light.png" width="49%">
</picture>

## Documentation

- [The manual](docs/README.md): every module, page and key.
- Installing: [Linux](INSTALL.linux.md), [macOS](INSTALL.macos.md) (Intel and Apple silicon),
  [Windows](INSTALL.windows.md).
- [Development](docs/development.md): building, releasing, the source tree and the panel generator.
- [DESIGN.md](DESIGN.md): how it fits together. [TODO.md](TODO.md): what's next.

## License

MIT, see [LICENSE](LICENSE). The Nunito font in `res-src/fonts` (SIL Open Font License) is used only to generate
panel artwork and isn't shipped with the plugin.
