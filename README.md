# vcvopxyz

<picture><source media="(prefers-color-scheme: dark)" srcset="docs/assets/guide/band-dark.png"><img alt="A Sequencer playing kick, snare, bass and chords, with a Screen beside it" src="docs/assets/guide/band-light.png"></picture>

## What is it?

An ode to the [Teenage Engineering OP-Z](https://teenage.engineering/products/op-z), this is a [VCV Rack](http://vcvrack.com/) plugin which re-imagines the iconic pocket synthesizer as a virtual eurorack module.

## How do I use it?

You will need [VCV Rack](http://vcvrack.com/). Although the free version will do, *please* consider purchasing the full version to support its awesome creators.

Grab the `.vcvplugin` file for your OS and architecture from the newest version at the top of the project's [Releases](https://github.com/un1tz3r0/vcvopxyz/releases) page, and drop it into your VCV Rack plugins folder... If you're not sure how to find that, have a look at the **Install** instructions for your platform: [Linux](INSTALL.linux.md), [MacOS](INSTALL.macos.md) and [Windows](INSTALL.windows.md).

The main **Sequencer** module can be used stand-alone, just like the real OP-Z, or in combination with **Breakout** expander modules for patching individual sequencer tracks to external chains in place of the builtin sounds.

## What it is not

Rather than try and make a sample-perfect clone of each sound and FX engine, the goal was to build something similar *enough* to be instantly usable by anyone who is familiar with the original. What it lacks in sounding exactly like the hardware, it makes up for by allowing the sequencer tracks to be diverted to 1v/oct pitch and gate/trig outputs so that they can control chains of external modules whose output can then be routed back into the OP-Z's track-fx and mixer. 

#### This project is not affiliated with or endorsed by teenage engineering. OP-Z and OP-XY are their trademarks.

## Intros

These pages will be helpful for those with little or no prior experience with the OP-Z.

- [How it works](docs/how-it-works.md): the model behind the modules in one page, with pictures and a glossary. Read this first.
- [Beginner's Guide](docs/guide.md): step-by-step instructions starting with an empty rack to making a simple loop, with screenshots, for those without any experience with the OP-Z.
- [Quickstart](docs/quickstart.md): a brief rundown of the core features for those with some prior OP-Z experience who could use a refresher.

If you just want to see what it can do, have a look at the example patches in [patches/](patches).

The [manual](docs/README.md) has every module, page and key. To build the plugin or change it, start at
[Development](docs/development.md), [DESIGN.md](DESIGN.md) and [TODO.md](TODO.md).

## In a patch

<picture><source media="(prefers-color-scheme: dark)" srcset="docs/assets/diagrams/row-dark.svg"><img alt="A Sequencer with a row of modules to its right: two Synth Breakouts take tracks 5 and 6, a Screen shows the selected track, and a Drum Breakout takes track 1" src="docs/assets/diagrams/row-light.svg"></picture>

The **Sequencer** makes every sound, so a Sequencer and an audio module are all you need. To send a track to other
modules, put a **breakout** to the Sequencer's right. A **Screen** in the same row shows the selected track's dials.
The modules in the row need no cables between them. [How it works](docs/how-it-works.md) explains the rest.

## License

MIT, see [LICENSE](LICENSE). The Nunito font in `res-src/fonts` (SIL Open Font License) is used only to generate
panel artwork and isn't shipped with the plugin.
