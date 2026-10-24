# FX tracks and their effects

The OP-Z's send effects play on its two FX tracks, tracks 9 and 10: **Delay**, **Reverb**, **Rymd**, **Chorus-80**,
**Crush** and **Dist**, each a [plug](sequencer.md#plugs) in the FX tracks' slots. An FX track takes what every track
sends it with its **FX 1** or **FX 2** dial, and its sound, effect only, goes into the Sequencer's **L** and **R**
alongside the tracks' own. Select one with its key on the display, or with steps 9 and 10 in track mode. For an
effect made of other modules, use a [Send FX Breakout](breakout.md#send-fx-breakout).

## Sends and slots

- **Slots:** a new Sequencer puts the six effects in the first six slots of both FX tracks, FX 1 playing the Delay and
  FX 2 the Reverb. With an FX track selected, **TRACK** and a black key pick another.
- **Sends:** every track's **FX 1** and **FX 2** dials, on its mix page, set how much of it goes to each effect, after
  its level and before its pan. A track whose breakout has its **FX 1** or **FX 2** output patched sends that one to
  the jack instead, so an outside effect can take its place.
- **Mixer:** steps 9 and 10 mute the FX tracks, which silences the effect; its send keeps filling, so the tail is
  there when it comes back.
- **Steps:** an FX track has 16 steps, a step count, step length, note length and quantize like any other. A step's
  parameter locks hold the effect's dials while it plays, and its notes set the effect's time.

## Dials

| Page | Dials |
| --- | --- |
| Sound (green) | The effect's two dials, **Filter** (low-pass to the left, high-pass to the right, open in the middle), **Reso** |
| LFO (yellow) | **Amount**, **Speed**, **Target**, **Shape**, as on a [synth track](synth-tracks.md#pages) |

There are no envelope or mix pages, so **SHIFT** skips them. The filter is on the effect's return, so it cuts the
effect without touching the dry tracks. The return comes back at unity, as the OP-Z's does. The LFO can move
either effect dial, the filter or resonance, the **time** (up to an octave either way), the **send** (the level into
the effect, from none to twice), the **pan** or the **volume**.

## Time

The Delay's echo time and the Reverb's predelay are set by notes: two octaves of keys from F3, the shortest on the
lowest. The OP-Z's guide says this of its reverb's predelay; the Delay doing the same is this plugin's choice.
Play a key while the FX track is selected, or put notes on its steps, and the effect's time
follows; the Sequencer lights the key it's on. The other effects have no time and ignore their notes.

## The effects

### Delay

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Delay-dark.png"><img alt="Delay" src="assets/plugs/Delay-light.png" height="40"></picture>

A digital delay. The keys set the time in 64th notes of the tempo, from 1/64 on the lowest to 3/8 on the highest,
3/16 (a dotted eighth) to begin with. A new time bends the echoes on its way there, as a tape would, rather than
clicking. **Amount** is the feedback, how much of each echo comes round again, up to 98%; **Cutoff** is a low-pass
inside the loop, so each repeat is darker than the last.

### Reverb

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Reverb-dark.png"><img alt="Reverb" src="assets/plugs/Reverb-light.png" height="40"></picture>

A plate: Dattorro's figure-of-eight tank, two of its allpasses gently swaying, with different taps for each side.
**Decay** is how long it rings, **Tone** how much of the top it keeps. The keys set the predelay, 10 ms a key from
none on the lowest, 30 ms to begin with.

### Rymd

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Rymd-dark.png"><img alt="Rymd" src="assets/plugs/Rymd-light.png" height="40"></picture>

The OP-Z's older reverb, Swedish for space: a Freeverb-style room of eight damped combs and four allpasses, the right
side's slightly longer than the left's. **Amount** is the room's size, **Cutoff** how bright it stays.

### Chorus

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Chorus-dark.png"><img alt="Chorus" src="assets/plugs/Chorus-light.png" height="40"></picture>

Chorus-80, a bucket-brigade chorus of the early 80s: a short delay on each side, swept by one triangle, the left up as
the right comes down, darkened above 9 kHz as a bucket brigade would. Mixed with the dry tracks, it widens and
thickens them. **Speed** is the sweep's rate, 0.05 to 8 Hz, **Depth** how far it sweeps.

### Crush

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Crush-dark.png"><img alt="Crush" src="assets/plugs/Crush-light.png" height="40"></picture>

A bitcrusher. **Amount** holds each sample longer, down to a 64th of the sample rate, and rounds it to fewer levels,
12 bits down to 2, on a μ-law scale, so quiet tails crumble before loud hits do. **Cutoff** is a low-pass after it,
100 Hz to 20 kHz, to tame the aliasing or leave it.

### Dist

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Dist-dark.png"><img alt="Dist" src="assets/plugs/Dist-light.png" height="40"></picture>

An overdrive: up to 40 dB of drive into a soft, slightly lopsided clipper, whose even harmonics warm it as a valve's
would, run at four times the sample rate. Its output stays near the send's level as the drive rises. **Cutoff** is a
low-pass after it, 300 Hz to 18 kHz, the tone.

## Out of the Sequencer

A [Send FX Breakout](breakout.md#send-fx-breakout) on an FX track takes it out: its **L** and **R** carry the effect
on its own, and its **SEND** outputs hand what the tracks send it to an outside effect, which comes back in through
the track's filter and level. A [Screen](screen.md) shows an FX track's pages and its time.
