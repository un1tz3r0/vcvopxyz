# vcvopxyz

VCV Rack 2 modules that recreate the synth engines and sequencers of teenage engineering's OP-Z and OP-XY. The aim
is their workflow and feel: the same parameters, ranges and quirks, tuned by ear rather than matched sample for
sample.

Not affiliated with or endorsed by teenage engineering. OP-Z and OP-XY are their trademarks.

## Modules

### Digital

The OP-Z's raw digital engine on a full OP-Z synth track. Two binary-counter oscillators read a 32-step, 4-bit sine
with no interpolation, and one frequency-modulates the other.

Like the hardware, it has four dials whose job changes with the page. **PAGE** steps through the pages, and the LED
above each dial flashes the new page's colour, then glows in it as bright as the dial is turned. The legend under
the dials says what each one does on each page, and clicking a page's name there jumps straight to it.

| Page | Dials |
| --- | --- |
| Sound (green) | **Octave** (the modulator's pitch, in octaves from the note), **Feedback** (FM depth), **Filter** (low-pass to the left, high-pass to the right, open in the middle), **Reso** |
| Envelope (blue) | **Attack**, **Decay**, **Sustain**, **Release**, 1 ms to 10 s |
| LFO (yellow) | **Amount** (off in the middle), **Speed** (synced 1/64 to 2/1 to the left, free 8 s to 20 Hz to the right), **Target**, **Shape** |
| Mix (red) | **FX 1** and **FX 2** (sends to the outputs of the same names), **Pan**, **Level** |
| Track (white) | **Style** (poly, mono, legato) on the second dial, **Glide** (mono and legato) on the fourth |

- **Track mode:** click **TRACK** in the legend to switch it on, and again to switch it off. It stands in for holding
  the hardware's track key. The dials set the track settings, and the white keys become 14 quick slots: click one to
  load it, or hold it for a second to save the sound to it. Filled slots glow dimly, and the last one used glows
  brightly. Slots are ordinary presets in a `Slots` folder, so every Digital shares them and they also appear in the
  module's Preset menu. The context menu opens the folder.
- **Keyboard:** two octaves, C3 to B4 until **−** and **+** move it. The keys play while **GATE** is unpatched, and
  light up under every held note, whether it comes from the keys or the inputs. The 16 dots above them are the voices.
- **Note styles:** poly plays each gate channel on its own voice. Mono plays the newest held note and retriggers
  the envelope on every change, including falling back to an older note. Legato does the same without retriggering.
- **LFO shapes:** sine, triangle, square, saw, random and external are free-running and bipolar. Bell, triangle,
  square, saw, random and single saw are unipolar and restart with every note (but not legato slides), so they work
  as a second envelope. External follows the **EXT** input, ±5V, in place of the OP-Z's motion sensor.
- **Clock:** the synced speeds follow the **CLOCK** input, at 4 pulses per beat unless you change it in the context
  menu, and 120 BPM with nothing patched. **RESET** restarts the LFO.
- **Outputs:** **L** carries a mono mix while **R** is unpatched. **FX 1** and **FX 2** carry the track after its
  level, before the pan, for patching to effects.
- **Jack LEDs:** green follows each jack's level. Red means clipping: above 10V on an input, or into the soft
  limiter that keeps the outputs within ±10V.

Coming next: a sequencer modelled on an OP-Z synth track, then parameter locks between the two.

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
| `src/opz/` | The OP-Z synth track every engine shares (`SynthTrack.hpp`), its panel (`SynthTrackWidget.hpp`), its LFO and note handling. |
| `src/dsp/` | Building blocks that know nothing of Rack: filter, envelope. |
| `src/Digital.cpp` | The Digital engine. |
| `src/Screen.*` | A reusable display widget whose readouts line up with the controls around it. |
| `src/ui.hpp` | Panel layout lookup, plus the custom dials, keys, buttons and lights. |
| `src/TripleBuffer.hpp` | Lock-free hand-off of data from the audio thread to the UI thread. |
| `res/` | Panels and component graphics, shipped with the plugin. Generated, so don't edit by hand. |
| `res-src/panelkit.py` | Fonts as outlines, colour themes, the `Panel` class and the shared component graphics. |
| `res-src/synth_track.py` | The panel layout every synth track shares. |
| `res-src/panels/<slug>.py` | Each module's panel, built with `panelkit`. |
| `res-src/panel.py` | Writes every panel and component graphic into `res/`. |

## Panels

`make panels` runs `res-src/panel.py`, which needs Python 3 and fontTools (`python3 -m pip install fonttools`) but
not the SDK. It writes each module's light and dark panel, plus the custom dial, knob, key and button graphics, into `res/`.
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
