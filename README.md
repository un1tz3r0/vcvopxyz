# vcvopxyz

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/assets/rack-dark.png">
  <img alt="A Sequencer playing four Drums, Digital, Analog, Cluster and Organ, with a Screen beside Digital" src="docs/assets/rack-light.png">
</picture>

An ode to the [Teenage Engineering OP-Z](https://teenage.engineering/products/op-z), this is a [VCV Rack](http://vcvrack.com/) plugin which adds a module and a set of expanders that work together to bring the magic, fun and deeply expressive and versatile sequencing experience of the OP-Z to the eurorack modular form factor.

Rather than try and make a sample-perfect copy of each sound and FX module, the goal is to bring something similar enough to be instantly usable by anyone who is proficient at playing the original to the modular rack, where it can be combined with third-party module chains to add just about any sound you can imagine as a voice for one of the OP-Z's eight (four percussive and four melodic) musical part tracks.

Doesn't that sound pretty neat? I sure think so.

For those unfamiliar, this will likely be, as is the real thing upon unboxing, slightly confusing and frustrating, until one has learned TE's screenless, lightly modal and mostly easy-to-remember multi-button-combination-based interface. This may sound daunting, but be patient and trust that once you're comfortable with how it works, this little guy is so much fun to use that it can be hard to put down even after hours of playing.

#### Not affiliated with or endorsed by teenage engineering. OP-Z and OP-XY are their trademarks.

## Modules

The Sequencer module is the master, and controls up to eight expanders to its right (up to four drum kits and four synth engines), which pick up the tracks in the order they appear, showing the assigned track number, symbol and name on each attached engine's status panel. The main sequencer module has V/OCT, GATE and velocity inputs for recording notes, and clock and transport control inputs and outputs with pass-through, and clocks itself internally if no clock signal is patched. The tracks reach other modules through a Breakout, not through cables from the Sequencer. The tempo of the internal clock can be adjusted exactly as on the real OP-Z, using the tempo-mode button along the top of the device.


| Module | |
| --- | --- |
| [Sequencer](docs/sequencer.md) | The OP-Z's front panel: tempo, transport, eight tracks of 16 steps with micro timing, a master track that transposes them in the key it detects, an arpeggiator on the arp track, recording, [step components](docs/step-components.md), a mixer, 16 patterns per project with chains, and ten project slots that import and export the OP-Z's project files. |
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

## Button Combos/Holding Buttons

One caveat of the virtual rack vs real hardware is that it is not possible to press and hold multiple buttons using multiple fingers and both hands. Therefore, a workaround was needed since much of the OP-Z's advanced features are accessible only while holding certain button combinations to temporarily switch the rest of the buttons (or some subset of them at least) into an alternate mode where the indicators and short- and long-presses have different meanings. The workaround takes several forms, depending on the button in question's other behaviors:

1. <key>control-click</key> will always toggle holding any button which it makes sense to hold

2. buttons which have no short-press or momentary behavior but that do something when held always toggle-on-click, even without <key>control</key> pressed

2. buttons which have a short-press action but no special long-press action will toggle holding when long-pressed, in addition to <key>control-click</key>-

## Documentation

- [The manual](docs/README.md): every module, page and key.
- Installing: [Linux](INSTALL.linux.md), [macOS](INSTALL.macos.md) (Intel and Apple silicon),
  [Windows](INSTALL.windows.md).
- [Development](docs/development.md): building, releasing, the source tree and the panel generator.
- [DESIGN.md](DESIGN.md): how it fits together. [TODO.md](TODO.md): what's next.

## License

MIT, see [LICENSE](LICENSE). The Nunito font in `res-src/fonts` (SIL Open Font License) is used only to generate
panel artwork and isn't shipped with the plugin.
