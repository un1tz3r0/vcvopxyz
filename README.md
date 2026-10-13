# vcvopxyz

VCV Rack 2 modules that recreate the synth engines and sequencers of teenage engineering's OP-Z and OP-XY. The aim
is their workflow and feel: the same parameters, ranges and quirks, tuned by ear rather than matched sample for
sample.

Not affiliated with or endorsed by teenage engineering. OP-Z and OP-XY are their trademarks.

## Modules

### Sequencer

The OP-Z's front panel, and the master of the OP-Z modules to its right. It keeps the tempo and the transport and
sequences the OP-Z's first eight tracks: kick, snare, perc and sample, then bass, lead, arp and chord. Each track has
16 steps of up to four notes, with micro timing, note lengths, step count and step length.

Place engines to its right, Screens beside them if you like, and they need no cables. The first synth engine plays
track 5 (bass), the next track 6, and so on; drum engines will take tracks 1 to 4 the same way. Each track has poly
**V/OCT** and **GATE** outputs. While a track's GATE is unpatched, its notes, velocity and the tempo go to its engine
over the bus instead. Engines with their own outputs unpatched are mixed into **L** and **R**. A module from another
plugin ends the chain.

The face copies the OP-Z's, icons and all, scaled up to fill the panel above the jacks: four dials over 17 columns
of round keys. **TRACK**, **REC** and **−**, **PLAY** and **+**, **STOP** and **SHIFT** on the left, the 16 steps along
the top under the track icons, the black keys numbered 1 to 0 on the column lines, and the white keys along the
bottom under the step component icons. Each key has an RGB LED that lights up its symbol. The tempo display and the
selected track's channel dots sit where the OP-Z's speaker is. Under the face, a row of keys: **TEMPO**, the four
pages, and a key per track over its jacks.

- **Selecting a track:** click its key over its jacks (white while selected, orange while it plays), or switch
  **TRACK** on and click one of steps 1 to 8. The keys, steps, dials and recording all work on the selected track.
- **Dials:** they are the selected track's engine's dials, so turning one turns that parameter on the engine, with
  its own tooltip, undo and MIDI mapping. Click **SHIFT**, or one of the four page keys, to change the page; the
  page keys and the dials' LEDs show it in its colour.
- **Steps:** click an empty step to paste the last notes played (a chord, if they were down together), or a lit step
  to clear it. Steps past the step count glow dimly and don't play. Right-click a step to set its notes' velocity,
  micro timing and length.
- **Holding a step:** Rack can't hold one button while clicking another, so a long press or a Ctrl+click (⌘+click
  on a Mac) picks a step up, and it blinks until clicked again. Either can be switched off in the context menu. While
  a step is held, the keys add and remove its notes, clicking another step makes its notes last to the end of that
  one (blue LEDs show how far; the same click again restores the default length), and **−** and **+** nudge it a
  tick, 1/24 of a step. Picking a step up while stopped plays it, and copies it for pasting.
- **Track mode:** click **TRACK** to switch it on, and again to switch it off. Steps 1 to 8 select a track. The dials
  set its **note length** (1/64 to a bar with the dotted values between, then drone), the engine's **note style**,
  **quantize** (pulls micro timing towards the grid as it plays) and the engine's **portamento**. The black keys set
  the step length in blue (1 to 8 for ×1 to ×8, 9 for ×16, 0 for one step per trigger on **ADV**), and **−** and
  **+** shift the pattern a step. With SHIFT latched too, the steps set the step count. **STOP** lets go of the
  notes, and held for a second clears the track.
- **Tempo mode:** click **TEMPO** to switch it on. The dials set the **tempo** (40 to 200 BPM) and **swing** (50%,
  straight, to 75%, which delays every second sixteenth). Type a tempo on the black keys, three digits as on the
  OP-Z, or tap one on the white keys. Its LED flashes on the beat.
- **SHIFT:** a click turns the page. A long press or a Ctrl+click latches it, for track mode's step count now and
  the step components later.
- **Recording:** click **REC** to switch it on. While playing, notes from the keys and inputs go into the nearest
  step with their micro timing and length. Hold notes on the inputs and click **−** to erase them from the steps
  they pass instead, until they're released. While stopped, REC records a step at a time: each note or chord goes
  into the blinking step, which moves on when they're released. Click a step to move it there.
- **Transport:** **PLAY** starts every track from the first step, or again from it while playing. **STOP** stops,
  and when stopped already, lets go of every note, the live ones too. Pattern edits can be undone like any other
  change.
- **Transport jacks:** **RUN** plays from the start while high and stops when it falls. **CONT** carries on from
  where it stopped. **RESET** starts over while playing. The outputs of the same names follow the sequencer: RUN is
  high while playing, and RESET and CONT pulse as it starts and carries on.
- **Clock:** the display shows the tempo, with **INT** lit. A patched **CLOCK**, at 4 pulses per beat unless you
  change it in the context menu, takes over and lights **EXT**, and the display shows dashes until it has timed two
  pulses. PLAY then waits for the next pulse, and the sequencer follows the pulses as they speed up, slow down or
  stop. The **CLOCK** output pulses while it plays.
- **Keys and inputs:** the keys, F3 to E5 as on the OP-Z, play the selected track while they're down, and so do notes
  on the poly **V/OCT**, **GATE** and **VEL** inputs (from MIDI-CV, say), which follow their V/OCT while held. **−**
  and **+** move the keyboard an octave.
- **Polyphony:** as the OP-Z's, one channel for every track but the lead (3) and chords (4). Change the selected
  track's in the context menu. A new note takes the channel silent longest, or failing that the one playing longest,
  and its gate drops for a sample so the engine retriggers.

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
  the hardware's track key, and the dials set the track settings.
- **Notes:** Digital has no keys of its own. Placed to the right of a Sequencer, it plays its track from there
  (see the Sequencer), follows its tempo, and its dials are the Sequencer's while its track is selected. A patched
  **GATE** plays the poly **V/OCT**, **GATE** and **VEL** inputs instead, and a patched **CLOCK** sets the LFO's
  tempo. The 16 dots under **VOICES** light with each voice's envelope.
- **Note styles:** poly plays each gate channel on its own voice. Mono plays the newest held note and retriggers
  the envelope on every change, including falling back to an older note. Legato does the same without retriggering.
- **LFO shapes:** sine, triangle, square, saw, random and external are free-running and bipolar. Bell, triangle,
  square, saw, random and single saw are unipolar and restart with every note (but not legato slides), so they work
  as a second envelope. External follows the **EXT** input, ±5V, in place of the OP-Z's motion sensor.
- **Clock:** the synced speeds follow the **CLOCK** input, at 4 pulses per beat unless you change it in the context
  menu; unpatched, the Sequencer's tempo, or 120 BPM without one. **RESET** restarts the LFO.
- **Outputs:** **L** carries a mono mix while **R** is unpatched. With both unpatched, the track also goes, in stereo,
  to the Sequencer's **L** and **R**. **FX 1** and **FX 2** carry the track after its
  level, before the pan, for patching to effects.
- **Jack LEDs:** green follows each jack's level. Red means clipping: above 10V on an input, or into the soft
  limiter that keeps the outputs within ±10V.

### Analog, Cluster, Organ

Three more OP-Z engines on the same synth track as Digital: the same pages, legend, jacks, note styles and bus. Only
the two sound dials differ.

| Engine | Sound | First dial | Second dial |
| --- | --- | --- | --- |
| Analog | A saw, a square an octave below and noise, through a low-pass the amp envelope opens | **Mix**: saw, then saw and sub, then sub and noise | **Envelope**: how far the filter closes between notes, for the envelope to sweep it up to eight octaves |
| Cluster | Seven detuned saws per note | **Tone**: a low-pass that follows the note, from a near sine to fully bright | **Gravity**: pulls the saws together, from almost two semitones each way to one saw |
| Organ | Up to four sine carriers and one modulator, in eight algorithms | **Algorithm**: flute, jazz, full and gospel drawbars, reed, pipe, combo, bell | **Tweak**: FM depth, from pure drawbars to grit |

### Breakout

A synth track whose voice is made of other modules. It is 20 HP and has everything an engine has, plus a dark box of
outputs that send the track out, and **RET L** and **RET R** to take the sound back in.

- **Out:** **V/OCT**, **GATE** and **ENV** are polyphonic, a channel per voice (one in mono and legato), with the
  glide and the LFO's pitch in V/OCT. **LFO** is the LFO after its amount, ±5V. **CV 1** and **CV 2** are the sound
  page's dials, and **FILTER**, **RESO**, **FX 1** and **FX 2** the dials of those names, all 0 to 10V, with the LFO.
- **Return:** whatever goes out, the track stops doing itself. With **ENV** patched the return isn't enveloped; with
  **FILTER** or **RESO** patched it isn't filtered; with an **FX** amount patched, that **FX** output carries the
  full signal for an outside VCA; with **LFO** patched, the LFO moves nothing inside. Level and pan always apply.
- **Envelope:** a polyphonic return is enveloped channel by channel, one voice each; a mono one by the loudest
  voice. **RET R** is normalled to **RET L**.

### Screen

The OP-Z's phone app, as an expander: the OP-Z famously has no screen, since you already carry one. Place it
directly to the right of an OP-Z engine. Its top line names the engine, the page the dials are on and the tempo, and
under it is the dial last turned, in its colour, with its value.

The middle shows a picture of the page:

| Page | Picture |
| --- | --- |
| Sound | A scope of the output, with the filter pair's response over it |
| Envelope | The envelope, a stage per dial colour, with a dot riding it for each sounding voice |
| LFO | One cycle of the shape, as deep as the amount, with the speed, the target and a dot at the current phase |
| Mix | The two sends, the pan, the output meters and the level |
| Track | The note style, and the glide as a slide between two notes |

Below that, every dial of every page is an arc in its colour, with the page the dials are on lit up. Click one to
select it and jump to its page (the track row switches track mode on), then drag up or down anywhere on the screen to
turn it, finely with Ctrl, like the app's touch pad. Each drag is one undo step.

Coming next: the sequencer's step components, then parameter locks over the bus, and drum engines.

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
| `src/opz/Sequence.hpp` | The sequencer's steps and the player that schedules them, knowing nothing of Rack. |
| `src/opz/Sequencer.cpp` | The Sequencer module. |
| `src/opz/Bus.hpp` | The bus between the Sequencer and the engines and Screens to its right. |
| `src/dsp/` | Building blocks that know nothing of Rack: filter, envelope, band-limited saw. |
| `src/Digital.cpp`, `Analog.cpp`, `Cluster.cpp`, `Organ.cpp` | The engines. |
| `src/Breakout.cpp` | Breakout, the synth track for a voice made of other modules. |
| `src/opz/Screen.cpp` | The Screen expander. |
| `src/Screen.*` | A reusable display widget whose readouts line up with the controls around it. |
| `src/ui.hpp` | Panel layout lookup, plus the custom dials, keys, buttons and lights. |
| `src/TripleBuffer.hpp` | Lock-free hand-off of data from the audio thread to the UI thread. |
| `res/` | Panels and component graphics, shipped with the plugin. Generated, so don't edit by hand. |
| `res-src/panelkit.py` | Fonts as outlines, colour themes, the `Panel` class and the shared component graphics. |
| `res-src/synth_track.py` | The panel layout every synth track shares. |
| `res-src/opz.py` | The OP-Z's track, step component and key icons, and its round keys. |
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
