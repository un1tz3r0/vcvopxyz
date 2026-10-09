# vcvopxyz

VCV Rack 2 modules that recreate the synth engines and sequencers of teenage engineering's OP-Z and OP-XY. The aim
is their workflow and feel: the same parameters, ranges and quirks, tuned by ear rather than matched sample for
sample.

Not affiliated with or endorsed by teenage engineering. OP-Z and OP-XY are their trademarks.

## Status

No modules yet. Planned, in order:

1. **Digital:** the OP-Z's Digital synth engine, with its filter, envelope, LFO, note styles and portamento.
2. **Synth track sequencer:** an OP-Z synth track with step count, step length, note length and step components.
3. **Parameter locks:** the sequencer docks beside an engine, like an expander, to lock its parameters per step.

## Building

Install the toolchain for your platform as described in the
[plugin development tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial):

- **Linux:** `build-essential`, `jq`, `zstd`
- **macOS:** Xcode command line tools, plus `jq` and `zstd` from Homebrew
- **Windows:** [MSYS2](https://www.msys2.org), then in its MINGW64 shell
  `pacman -S make tar unzip zstd jq mingw-w64-x86_64-gcc`

Then download the [Rack SDK](https://vcvrack.com/downloads/) for your platform (`lin-x64`, `win-x64`, `mac-x64` or
`mac-arm64`) and point `RACK_DIR` at it:

```sh
unzip Rack-SDK-2.6.6-lin-x64.zip
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
git tag v2.0.1
git push origin v2.0.1
```

The release job checks that the tag matches `plugin.json`, then attaches all four packages to a GitHub release.
Linux builds run in an Ubuntu 20.04 container so the plugin also loads on older distributions. The SDK version is
set once, as `RACK_SDK_VERSION` at the top of the workflow.

## What's where

| Path | Contents |
| --- | --- |
| `plugin.json` | The manifest: plugin and module metadata, and the version. |
| `src/plugin.*` | The plugin's entry point, which registers each module's `Model`. |
| `src/Screen.*` | A reusable display widget whose readouts line up with the controls around it. |
| `src/ui.hpp` | Panel layout lookup, plus the custom knobs and button. |
| `src/TripleBuffer.hpp` | Lock-free hand-off of data from the audio thread to the UI thread. |
| `res/` | Panels and component graphics, shipped with the plugin. Generated, so don't edit by hand. |
| `res-src/panelkit.py` | Fonts as outlines, colour themes, the `Panel` class and the shared component graphics. |
| `res-src/panels/<slug>.py` | Each module's panel, built with `panelkit`. |
| `res-src/panel.py` | Writes every panel and component graphic into `res/`. |

## Panels

`make panels` runs `res-src/panel.py`, which needs Python 3 and fontTools (`python3 -m pip install fonttools`) but
not the SDK. It writes each module's light and dark panel, plus the custom knob and button graphics, into `res/`.
Labels are converted to paths, since Rack's SVG renderer ignores `<text>`.

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

## Adding a module

1. Write `src/MyModule.cpp` with the module, its widget and `Model* modelMyModule = createModel<...>("MyModule")`.
   The [template's Example module](https://github.com/un1tz3r0/vcv-rack2-plugin/blob/main/src/Example.cpp) shows
   every common panel idiom in working code.
2. Declare `extern Model* modelMyModule;` in `src/plugin.hpp`, call `p->addModel(modelMyModule);` in
   `src/plugin.cpp`, and add an entry with the slug `MyModule` to `modules` in `plugin.json`.
3. Write `res-src/panels/MyModule.py` with a `panel()` function returning a `panelkit.Panel`, and run `make panels`
   to get `res/MyModule.svg` and `res/MyModule-dark.svg`.

## License

MIT, see [LICENSE](LICENSE). The Nunito font in `res-src/fonts` (SIL Open Font License) is used only to generate
panel artwork and isn't shipped with the plugin.
