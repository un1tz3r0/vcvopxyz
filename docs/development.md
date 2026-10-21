# Development

## Building

Install the toolchain for your platform (see [Linux](../INSTALL.linux.md#building-from-source),
[macOS](../INSTALL.macos.md#building-from-source) or [Windows](../INSTALL.windows.md#building-from-source)), download
the [Rack SDK](https://vcvrack.com/downloads/) for your platform (`lin-x64`, `win-x64`, `mac-x64` or `mac-arm64`) and
point `RACK_DIR` at it:

```sh
make RACK_DIR=Rack-SDK            # builds plugin.so / plugin.dylib / plugin.dll
make install RACK_DIR=Rack-SDK    # packages it and copies it into your Rack user folder
make dist RACK_DIR=Rack-SDK       # just the package: dist/vcvopxyz-<version>-<platform>.vcvplugin
```

Restart Rack after `make install` to load the new build. The code is C++17; see the note in the `Makefile` about
the parts of its standard library that macOS builds can't use.

## Releasing

CI (`.github/workflows/build.yml`) builds Linux, Windows, macOS x64 and macOS arm64 packages on every push and pull
request, and keeps them as artifacts of the workflow run. To publish a release, bump `version` in `plugin.json`
(Rack 2 plugins are versioned `2.x.y`), commit, and push a matching tag:

```sh
git tag v2.0.5
git push origin v2.0.5
```

The release job checks that the tag matches `plugin.json`, then attaches all four packages to a GitHub release.
Two ways need no tag pushed, and each tags the commit `v<version>` from `plugin.json` itself, refusing a version
that is already tagged:
- Label a pull request **release** before merging it (`.github/workflows/release.yml`): the merge is published.
- Run the **Build** workflow by hand on `main` (Actions, Build, Run workflow) with **release** ticked.
Linux builds run in an Ubuntu 20.04 container so the plugin also loads on older distributions. The SDK version is
set once, as `RACK_SDK_VERSION` at the top of the workflow.

## What's where

| Path | Contents |
| --- | --- |
| `plugin.json` | The manifest: plugin and module metadata, and the version. |
| `src/plugin.*` | The plugin's entry point, which registers each module's `Model`. |
| `src/opz/` | The OP-Z track every engine shares (`Track.hpp`), its panel and status screen (`TrackWidget.hpp`), the synth track's voices around an engine (`SynthTrack.hpp`), the drum track's dials and keys (`DrumTrack.hpp`), and the FX track around an effect (`FxTrack.hpp`). |
| `src/opz/Kit.*`, `BuiltinKits.cpp` | Drum kits: the OP-1/OP-Z kit, AIFF and WAV readers, and the synthesized built-in kits. |
| `src/opz/Sequence.hpp` | The sequencer's steps, components and the player that schedules them, knowing nothing of Rack. |
| `src/opz/Sequencer.cpp` | The Sequencer module. |
| `src/opz/Project.hpp` | A project slot, and the reader and writer for the OP-Z's `.opz` project files, knowing nothing of Rack. |
| `src/opz/Bus.hpp` | The bus between the Sequencer and the engines and Screens to its right. |
| `src/dsp/` | Building blocks that know nothing of Rack: filter, envelope, band-limited saw, delay line. |
| `src/Digital.cpp`, `Analog.cpp`, `Cluster.cpp`, `Organ.cpp` | The engines. |
| `src/Drum.cpp` | Drum, the drum track. |
| `src/Delay.cpp`, `Reverb.cpp`, `Rymd.cpp`, `Chorus.cpp`, `Crush.cpp`, `Dist.cpp` | The FX modules. |
| `src/Breakout.cpp`, `DrumBreakout.cpp`, `FxBreakout.cpp` | Synth, Drum and Send FX Breakout, the synth, drum and FX tracks for sounds made of other modules. |
| `src/opz/Screen.cpp` | The Screen expander. |
| `src/Screen.*` | A reusable display widget whose readouts line up with the controls around it. |
| `src/ui.hpp` | Panel layout lookup, plus the custom dials, keys, buttons and lights. |
| `src/TripleBuffer.hpp` | Lock-free hand-off of data from the audio thread to the UI thread. |
| `res/` | Panels and component graphics, shipped with the plugin. Generated, so don't edit by hand. |
| `res-src/panelkit.py` | Fonts as outlines, colour themes, the `Panel` class and the shared component graphics. |
| `res-src/synth_track.py` | The panel layout every track shares, drum or synth. |
| `res-src/opz.py` | The OP-Z's track, step component and key icons, and its round keys. |
| `res-src/panels/<slug>.py` | Each module's panel, built with `panelkit`. |
| `res-src/panel.py` | Writes every panel and component graphic into `res/`. |
| `docs/` | This manual; `docs/assets/` holds its pictures. |
| `tools/rack-headless/` | `shot.sh`: screenshots of a patch in a headless Rack (Xvfb, software GL), with scripted clicks. |
| `tools/docs-images/` | The manual's pictures: the modules, the Sequencer's keys in each state, and the guide's scenes. |

## Panels

`make panels` runs `res-src/panel.py`, which needs Python 3 and fontTools (`python3 -m pip install fonttools`) but
not the SDK. It writes each module's light and dark panel, plus the custom dial, knob, key and button graphics, into
`res/`. Labels are converted to paths, since Rack's SVG renderer ignores `<text>`.

Each panel also gets a hidden `components` layer that marks where every widget goes, using the SDK's `helper.py`
convention: a red circle for a param, green for an input, blue for an output, magenta for a light, and a yellow
rectangle for a custom widget. Each shape's id is the C++ enum name (`CUTOFF_PARAM`, `OUT_LIGHT`, `SCREEN_WIDGET`).
At runtime, `PanelLayout` reads those positions back, so the panel is the only place the layout is written down:

```cpp
addParam(createParamCentered<PastelKnob<Pastel::Salmon>>(AT(CUTOFF_PARAM)));
// AT(CUTOFF_PARAM) expands to: layout.center("CUTOFF_PARAM"), module, Digital::CUTOFF_PARAM
```

To add a control, add a label and a `p.place('NAME_PARAM', x, y)` to the module's panel function, run
`make panels`, and add one line like the one above. Moving a control needs no C++ change at all.

## The manual's pictures

Every picture in `docs/assets/{modules,keys,guide}` comes from a running Rack, in light and dark, so they can be
made again whenever a panel changes. They need a Rack 2 build (`RACK_DIR`, or `tools/rack-headless/Rack`), with
Fundamental in `tools/rack-headless/Fundamental`, plus `xvfb-run`, `xdotool`, ImageMagick's `import`, `zstd`, `jq`,
`pngquant` and Pillow, and the plugin built in place:

```sh
tools/docs-images/render_modules.py tools/rack-headless . [Slug ...]   # docs/assets/modules: large, small, label
tools/docs-images/render_keys.py tools/rack-headless . [state ...]     # docs/assets/keys: each key, and the face
tools/docs-images/render_guide.py tools/rack-headless . [scene ...]    # docs/assets/guide: the guide's scenes
```

Rack draws each at 300% and the scripts scale down: modules at 125% (large), 50% (small) and a square of the panel's
top at 175% (label); keys at 125%; the face and the guide at 100%. To add one to a page, write a shorthand and run
`tools/docs-images/expand.py . docs/*.md README.md`, which turns it into a `<picture>` with both themes; `--prune`
deletes the pictures no page uses. `[[key:track-track|TRACK|32]]` is the TRACK key in the `track` state, 32 px high;
`[[module:Digital-label|digital|56]]`, `[[face:project|…]]` and `[[guide:bass|…]]` work the same way.

## Adding a module

1. Write `src/MyModule.cpp` with the module, its widget and `Model* modelMyModule = createModel<...>("MyModule")`.
   The [template's Example module](https://github.com/un1tz3r0/vcv-rack2-plugin/blob/main/src/Example.cpp) shows
   every common panel idiom in working code.
2. Declare `extern Model* modelMyModule;` in `src/plugin.hpp`, call `p->addModel(modelMyModule);` in
   `src/plugin.cpp`, and add an entry with the slug `MyModule` to `modules` in `plugin.json`.
3. Write `res-src/panels/MyModule.py` with a `panel()` function returning a `panelkit.Panel`, and run `make panels`
   to get `res/MyModule.svg` and `res/MyModule-dark.svg`.

A new synth engine is less work: it is a struct with a slug, its two sound dials and a `Voice`, handed to
`SynthTrack` (see [DESIGN.md](../DESIGN.md#synth-engines)), with a panel from `res-src/synth_track.py`.
`src/Analog.cpp` is a complete example in about 50 lines. A new effect is the same: a struct with a slug, its two
dials, its time keys and a `Unit`, handed to `FxTrack` (`src/opz/FxTrack.hpp`), with a panel from `fx_track_panel`;
`src/Crush.cpp` is the shortest.
