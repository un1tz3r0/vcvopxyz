# Synth tracks

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/guide/slots-dark.png"><img alt="The bass track in track mode, its plug slots on the black keys and Digital playing" src="assets/guide/slots-light.png"></picture>

Tracks 5 (bass), 6 (lead), 7 (arp) and 8 (chord) are the synth tracks. Each plays an **engine**, its
[plug](track.md#plugs). This page gives the dials, note styles and arp of a synth track, and then the eleven engines.
To play a synth track on other modules, use a [Synth Breakout](breakout.md#synth-breakout).

## Pages

| Page | Green | Blue | Yellow | Red |
| --- | --- | --- | --- | --- |
| Sound | The engine's first dial | The engine's second dial | **Filter** | **Reso** |
| Envelope | **Attack** | **Decay** | **Sustain** | **Release** |
| LFO | **Amount** | **Speed** | **Target** | **Shape** |
| Mix | **FX 1** | **FX 2** | **Pan** | **Level** |
| Track mode | Note length | **Style** | Quantize | **Glide** |

- **Filter:** the left half sweeps a low-pass up, and the right half sweeps a high-pass up. The middle is open.
- **Envelope:** each stage goes from 1 ms to 10 s.
- **LFO amount** is off in the middle. **LFO speed** is synced to the tempo on the left half, from 1/64 to 2/1. The
  right half is free, from 8 s to 20 Hz.
- **FX 1** and **FX 2** send the track to the [FX tracks](fx-tracks.md). A breakout can send them to its jacks.
- On track 7 (arp), the LFO page controls the [arp](#arp), and the track has no LFO.

[Track](track.md#pages) tells how to change the page.

## Playing

- **Note styles:** **poly** plays each note on its own voice. **Mono** plays the newest note, and starts the envelope
  again on each change. **Legato** is as mono, but does not start the envelope again.
- **Glide** works in mono and legato only.
- **LFO targets:** the two sound dials, filter, resonance, attack, pitch (up to two octaves each way), pan and level.
- **LFO shapes:** sine, triangle, square, saw, random and external run free, and swing above and below zero. Bell,
  triangle, square, saw, random and single saw start again with each note, and stay above zero. Use them as a second
  envelope.
- **External** follows the **EXT** input of a [breakout](breakout.md), ±5 V, in place of the OP-Z's motion sensor.
- **Synced LFO speeds** follow the tempo, or the **CLOCK** input of a breakout while it is patched.

## Arp

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/guide/arp-screen-dark.png"><img alt="A Screen showing the arp track's arp page" src="assets/guide/arp-screen-light.png"></picture>

Track 7 (arp) has an arpeggiator. Hold notes or a chord, from the steps, the keys or the inputs. The arp plays them one
at a time, on the tempo's grid. The [master track](master.md) moves its notes, as it moves any track's notes.

| Dial | Setting |
| --- | --- |
| Green: **Speed** | Fully left is off: the track plays its notes as they are. Then 1/4, 1/8, 1/8 triplets, 1/16 (the start), 1/16 triplets, 1/32, 1/32 triplets and 1/64. |
| Blue: **Pattern** | As played, up, down, up and down, down and up, or random. |
| Yellow: **Style** | A rhythm of accents: as set, LHLLHLLH, HLLHLLHL, LLHLLHLL, LMH, or LL- (two notes, then a rest). |
| Red: **Range** | 1 to 4 octaves of the notes, from low to high. |

In a style, H plays at the note's velocity, M at three quarters, L at half, and - is a rest.

- Each arp note lasts half the time to the next note. Swing moves the arp's notes as it moves the steps.
- The first note plays as soon as you hold the notes. While the Sequencer is stopped, the grid starts with it.
- In mono with glide, the arp slides from note to note.
- A step can lock the arp's dials, as it locks other dials. The arp's dials belong to the pattern, not to presets.
- A patch saved before the arp existed opens with the arp off.

## The engines

| | Engine | Sound | First dial | Second dial |
| --- | --- | --- | --- | --- |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Digital-dark.png"><img alt="Digital" src="assets/plugs/Digital-light.png" height="40"></picture> | Digital | A sine, driven past full scale so that it wraps as an overflowing integer does. | **Octave**: how many times each peak wraps, 0 to 7. | **Feedback**: how much the sine moves its own phase. It leans the sine toward a saw. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Analog-dark.png"><img alt="Analog" src="assets/plugs/Analog-light.png" height="40"></picture> | Analog | A saw, a square an octave below, and noise, through a low-pass that the envelope opens. | **Mix**: saw, then saw and sub, then sub and noise. | **Envelope**: how far the filter closes between notes. The envelope opens it up to eight octaves. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Cluster-dark.png"><img alt="Cluster" src="assets/plugs/Cluster-light.png" height="40"></picture> | Cluster | Seven detuned saws for each note. | **Tone**: a low-pass that follows the note, from almost a sine to fully bright. | **Gravity**: pulls the saws together, from almost two semitones each way to one saw. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Organ-dark.png"><img alt="Organ" src="assets/plugs/Organ-light.png" height="40"></picture> | Organ | Up to four sine carriers and one modulator, in eight algorithms. | **Algorithm**: flute, jazz, full and gospel drawbars, reed, pipe, combo, bell. | **Tweak**: the FM depth, from pure drawbars to grit. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Bow-dark.png"><img alt="Bow" src="assets/plugs/Bow-light.png" height="40"></picture> | Bow | A bowed string. The bow's speed follows the envelope, so a note swells and then rings. | **Tension**: the bow's pressure and the string's brightness, from airy to biting. | **Chorus**: a second string on each side of the note, up to 9 cents away. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Electric-dark.png"><img alt="Electric" src="assets/plugs/Electric-light.png" height="40"></picture> | Electric | Two sines that move each other's phase. The envelope opens the sound. | **Cross mod**: how much, from two pure tones to a snarl. | **X mod**: the second sine's ratio to the note, 0.5 to 4. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Saw-dark.png"><img alt="Saw" src="assets/plugs/Saw-light.png" height="40"></picture> | Saw | Two saws, a few cents apart, through a resonant low-pass. | **Envelope**: how far the envelope opens the filter, from plucks to swells. | **Tone**: where the filter sits above the note, and how much it rings. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Shade-dark.png"><img alt="Shade" src="assets/plugs/Shade-light.png" height="40"></picture> | Shade | A soft FM piano: a sine that a second sine moves, at the same pitch. | **Detune**: a second piano against the first, from unison to a wide chorus. | **Drive**: a soft clip, from clean to growling. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Uranus-dark.png"><img alt="Uranus" src="assets/plugs/Uranus-light.png" height="40"></picture> | Uranus | A clean bass: a sine that modulates itself, as a DX7 operator does. | **Tone**: adds the second and third harmonics. | **Feedback**: from a sine toward a saw. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/Volt-dark.png"><img alt="Volt" src="assets/plugs/Volt-light.png" height="40"></picture> | Volt | A saw, a pulse above it and a triangle sub. | **Variation**: the pulse's interval above the saw, from unison to two octaves. It also adds the sub. | **Modulation**: a slow sweep of the pulse's width, and ring modulation of the saw and the pulse. |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/plugs/EP-dark.png"><img alt="EP" src="assets/plugs/EP-light.png" height="40"></picture> | EP | An FM electric piano in eight algorithms. It starts bright and becomes mellow as it fades. | **Algorithm**: tine, bright tine, dyno, reed, soft, bell, glass, toy. | **Tone**: how hard the hammer strikes. |

A new Sequencer puts every engine in each synth track's plug slots. Each track starts with the OP-Z's own engine:
Saw on the bass, Volt on the lead, Saw on the arp and Shade on the chord. The OP-Z's twelfth engine, Sample, is not
here yet.
