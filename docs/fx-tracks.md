# FX tracks

Tracks 9 (FX 1) and 10 (FX 2) are the send effects. Each plays an **effect**, its [plug](track.md#plugs): Delay,
Reverb, Rymd, Chorus-80, Crush or Dist. Every other track sends part of its sound to the FX tracks, with its **FX 1**
and **FX 2** dials. An FX track mixes only the effect into **L** and **R**, beside the tracks' own sound.

To select an FX track, click its key on the display, **FX 1** or **FX 2**. For an effect made of other modules, use a
[Send FX Breakout](breakout.md#send-fx-breakout).

## Sends and plug slots

- **Plug slots:** a new Sequencer puts the six effects in the first six plug slots of both FX tracks. FX 1 starts
  with the Delay, and FX 2 with the Reverb.
- **Sends:** each track's **FX 1** and **FX 2** dials, on its mix page, set how much of the track goes to each
  effect. The send comes after the track's level and before its pan.
- **A breakout's FX outputs:** a track's breakout can have its **FX 1** or **FX 2** output patched. That send then
  goes to the jack, not to the FX track.
- **Mute:** in mixer mode, steps 9 and 10 mute the FX tracks. A muted effect is silent, but its send continues to
  fill. Thus the effect's tail is there when you unmute it.
- **Steps:** an FX track has 16 steps, a step count, a step length, a note length and quantize, as other tracks do.
  Its notes set the effect's [time](#time), and its parameter locks set its dials.

## Dials

| Page | Green | Blue | Yellow | Red |
| --- | --- | --- | --- | --- |
| Sound | The effect's first dial | The effect's second dial | **Filter** | **Reso** |
| LFO | **Amount** | **Speed** | **Target** | **Shape** |

An FX track has no envelope page and no mix page, so **SHIFT** goes from the sound page to the LFO page and back.

- The **filter** is on the effect's output. It changes the effect, but not the dry tracks.
- The effect returns at full level, as on the OP-Z.
- The LFO can move either effect dial, the filter, the resonance, the **time**, the **send**, the pan or the volume.
  The time moves up to an octave each way. The send goes from none to twice its level.

## Time

Notes set the Delay's echo time and the Reverb's predelay. The keys from F3 up give two octaves of times, the
shortest on the lowest key. Play a key while the FX track is selected, or put notes on its steps. The Sequencer
lights the key of the time now. The other effects have no time, and do not use their notes.

The OP-Z's guide gives notes this job for the Reverb's predelay. For the Delay, it is this plugin's choice.

## The effects

### Delay

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Delay-dark.png"><img alt="Delay" src="assets/plugs/Delay-light.png" height="40"></picture>

A digital delay. The keys set the time in 64th notes of the tempo, from 1/64 on the lowest key to 3/8 on the highest.
It starts at 3/16, a dotted eighth. A new time bends the echoes, as a tape does, and does not click.

- **Amount:** the feedback, which is how much of each echo plays again. It goes up to 98%.
- **Cutoff:** a low-pass in the feedback, so each echo is darker than the last.

### Reverb

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Reverb-dark.png"><img alt="Reverb" src="assets/plugs/Reverb-light.png" height="40"></picture>

A plate reverb, after Dattorro's design, with a slow sway inside it and different taps for each side. The keys set
the predelay: 10 ms for each key, from none on the lowest. It starts at 30 ms.

- **Decay:** how long it rings.
- **Tone:** how much of the top it keeps.

### Rymd

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Rymd-dark.png"><img alt="Rymd" src="assets/plugs/Rymd-light.png" height="40"></picture>

The OP-Z's older reverb. Its name is Swedish for space. It is a room after the Freeverb design, a little longer on
the right than on the left.

- **Amount:** the size of the room.
- **Cutoff:** how bright the room stays.

### Chorus

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Chorus-dark.png"><img alt="Chorus" src="assets/plugs/Chorus-light.png" height="40"></picture>

Chorus-80 is a bucket-brigade chorus of the early 1980s. A short delay on each side moves up on the left as it moves
down on the right. A low-pass at 9 kHz darkens it, as a bucket brigade does. With the dry tracks, it makes them wider
and thicker.

- **Speed:** the rate of the sweep, 0.05 to 8 Hz.
- **Depth:** how far it sweeps.

### Crush

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Crush-dark.png"><img alt="Crush" src="assets/plugs/Crush-light.png" height="40"></picture>

A bitcrusher. Quiet tails crumble before loud hits do.

- **Amount:** holds each sample longer, down to 1/64 of the sample rate. It also cuts the bits, from 12 down to 2.
- **Cutoff:** a low-pass after the crusher, 100 Hz to 20 kHz.

### Dist

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Dist-dark.png"><img alt="Dist" src="assets/plugs/Dist-light.png" height="40"></picture>

An overdrive into a soft clipper. The clipper is a little uneven, so it adds warm, even harmonics as a valve does. The
output stays near the send's level as the drive increases.

- **Amount:** the drive, up to 40 dB.
- **Cutoff:** a low-pass after the clipper, 300 Hz to 18 kHz.

## A Screen and a breakout

A [Screen](screen.md) shows an FX track's pages and its time. A
[Send FX Breakout](breakout.md#send-fx-breakout) gives an FX track jacks. Its **L** and **R** carry the effect alone.
Its **SEND** outputs give an outside effect what the tracks send.
