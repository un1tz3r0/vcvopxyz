# vcvopxyz

VCV Rack 2 modules that recreate the synth engines and sequencers of teenage engineering's OP-Z and OP-XY. The aim
is their workflow and feel: the same parameters, ranges and quirks, tuned by ear rather than matched sample for
sample.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/assets/rack-dark.png">
  <img alt="A Sequencer playing four Drums, Digital, Analog, Cluster and Organ, with a Screen beside Digital" src="docs/assets/rack-light.png">
</picture>

Not affiliated with or endorsed by teenage engineering. OP-Z and OP-XY are their trademarks.

## Modules

A Sequencer on the left is the master; engines to its right pick up its tracks with no cables, and its dials turn
theirs.

| Module | |
| --- | --- |
| [Sequencer](docs/sequencer.md) | The OP-Z's front panel: tempo, transport, eight tracks of 16 steps with micro timing, recording, [step components](docs/step-components.md), a mixer, and ten project slots that import and export the OP-Z's project files. |
| [Drum](docs/drum-tracks.md) | The four drum tracks: 24 sounds a kit, synthesized or loaded from OP-1 and OP-Z kits, AIFF and WAV, with the drum note styles. |
| [Digital](docs/synth-tracks.md#the-engines) | Two binary-counter oscillators reading a 4-bit sine, one frequency-modulating the other. |
| [Analog](docs/synth-tracks.md#the-engines) | Saw, sub and noise through an enveloped low-pass. |
| [Cluster](docs/synth-tracks.md#the-engines) | Seven detuned saws per note. |
| [Organ](docs/synth-tracks.md#the-engines) | Drawbar FM organ in eight algorithms. |
| [Breakout](docs/breakout.md) | A synth track that sends its notes, envelope, LFO and dials out as CV and takes its voice back in. |
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
