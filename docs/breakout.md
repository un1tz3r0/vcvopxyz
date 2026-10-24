# Breakouts

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/guide/breakout-dark.png"><img alt="A Synth Breakout taking the bass track, its V/OCT into a VCO and the VCO back into its return" src="assets/guide/breakout-light.png"></picture>

A breakout hands one of the [Sequencer](sequencer.md)'s tracks to other modules. It makes no sound of its own: place
it anywhere to the right of a Sequencer and it reaches its track over the bus along the row, with no cables. There is
one for each kind of track:

| | Breakout | Takes | Width |
| --- | --- | --- | --- |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/Breakout-label-dark.png"><img alt="synth breakout" src="assets/modules/Breakout-label-light.png" height="56"></picture> | [Synth Breakout](#synth-breakout) | One of tracks 5 to 8 | 12 HP |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/DrumBreakout-label-dark.png"><img alt="drum breakout" src="assets/modules/DrumBreakout-label-light.png" height="56"></picture> | [Drum Breakout](#drum-breakout) | One of tracks 1 to 4 | 14 HP |
| <picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/FxBreakout-label-dark.png"><img alt="send fx breakout" src="assets/modules/FxBreakout-label-light.png" height="56"></picture> | [Send FX Breakout](#send-fx-breakout) | FX 1 or FX 2 | 12 HP |

## Two ways out

What a breakout does depends only on what's patched:

- **Audio out:** patch its **L** and **R** (or **L** alone, for mono), and the track plays its own plug as ever, but
  its sound leaves there instead of going into the Sequencer's mix: one track on its own, for your own effects,
  mixer or recorder.
- **A sound from outside:** patch its note outputs instead (**V/OCT** or **GATE** on a Synth Breakout, **TRIG** or
  **GATE** on a Drum Breakout, **SEND** on a Send FX Breakout), and the track's plug stops. The breakout sends out
  what the plug would have played with, and outside modules make the sound. Whatever comes back into **RET L** and
  **RET R** goes through the track as its plug's sound would: its envelope, filter, level and pan, its FX sends and
  the mixer. The display shows **Breakout** in place of the plug.

Pull the cables and the track's plug comes back. Patched audio outputs win, so with both patched the plug plays out of
**L** and **R**.

## Common to all three

- **Which track:** the context menu's **Track**. **Auto**, the default, takes the first track of its kind that no
  breakout to its left has, so two Synth Breakouts take the bass and the lead. A fixed track is the breakout's own,
  unless a breakout to its left has it already. The track's key on the Sequencer's display shows a small jack in its
  corner while a breakout has it.
- **The screen:** the track it has as a big digit beside the track's icon, which lights white while the Sequencer
  has the track selected; a dash for none. Under it, a scope of the track's sound, a dot for each voice as bright as
  its envelope, and the level in twelve segments, green, then yellow, then red into the soft limit.
- **Lights:** **SEL** lights while the Sequencer has the track selected, and **NOTE** (**OUT** on a Send FX Breakout)
  glows while it plays. Each jack's LED follows its level, red above 10V.
- **Dials:** a breakout has none: the Sequencer's four turn the track, and a [Screen](screen.md) shows them all.
  Parameter locks, step components, presets and patterns work on a broken-out track as on any other.
- **The rest:** whatever the breakout sends out, the track stops doing itself. With **ENV** patched the return
  isn't enveloped; with **FILTER** or **RESO** patched it isn't filtered; with an **FX** amount patched, that **FX**
  output carries the full signal for an outside VCA; with **LFO** patched, the LFO moves nothing inside. Level and pan
  always apply. **RET R** is normalled to **RET L**.
- **Inputs:** a patched **GATE** input plays the track from the poly **V/OCT**, **GATE** and **VEL** inputs in place
  of its steps and keys. **CLOCK** sets the LFO's synced speeds (4 pulses per beat), **RESET** restarts the LFO, and
  **EXT** is what the External LFO shape follows, ±5V; on a track with no breakout that shape stays still.
- **FX 1** and **FX 2** outputs (synth and drum): patched, the track's send goes there instead of to its FX track.

## Synth Breakout

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/Breakout-large-dark.png"><img alt="Synth Breakout" src="assets/modules/Breakout-large-light.png"></picture>

A [synth track](synth-tracks.md) for a voice made of other modules. Its slug is still `Breakout`, the name it had
before 2.0.21, so old patches find it.

- **Out:** **V/OCT**, **GATE** and **ENV** are polyphonic, a channel per voice (one in mono and legato), with the
  glide and the LFO's pitch in V/OCT. **LFO** is the LFO after its amount, ±5V. **CV 1** and **CV 2** are the sound
  page's dials, and **FILTER**, **RESO**, **FX 1** and **FX 2** the dials of those names, all 0 to 10V, with the LFO.
- **Envelope:** a polyphonic return is enveloped channel by channel, one voice each; a mono one by the loudest
  voice.

### A voice from outside

1. Patch **V/OCT** to a VCO's V/OCT, and the VCO's output to **RET L**. The track's envelope, filter and mix now
   shape the VCO.
2. Patch **FILTER** and **RESO** to an outside filter's cutoff and resonance CV, and put that filter between the VCO
   and **RET L**. The inside filter switches off.
3. Patch **ENV** to an outside VCA's CV and put it in the chain too. The inside envelope switches off, and the
   track is now only level and pan around your own voice.

## Drum Breakout

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/DrumBreakout-large-dark.png"><img alt="Drum Breakout" src="assets/modules/DrumBreakout-large-light.png"></picture>

A [drum track](drum-tracks.md) whose sounds are made of other modules, with the same note styles and keys as a kit.

- **Out, a channel per note:** **TRIG** is a 1 ms trigger as each note starts, and **GATE** is high while it's held.
  **SOUND** is which of the 24 sounds its key picks, 1/12V a sound from 0V for the first (F3), the way a V/OCT
  input steps through samples. **VEL** is the velocity and **ENV** the envelope, times the velocity, 0 to 10V. Each
  of the notes a step plays keeps its own channel; **mono** puts them all on the first.
- **Out, one channel:** **PITCH** is the pitch dial as V/OCT, ±1V, with the LFO's pitch target added. **REV** is
  10V while reverse is on. **LFO**, **FILTER**, **RESO**, **FX 1** and **FX 2** are as on Synth Breakout.
- **Envelope:** retrig and mono leave it open at the sustain level (all the way, by default), for the sound outside
  to end itself; gate and loop release it with the note.

### Drums from outside

1. Patch **TRIG** to a drum module's trigger input, or **GATE** to a gate input, and its output to **RET L**. The
   track's level, pan, filter and FX sends now carry it, and its steps, components and parameter locks play it.
2. For a sampler that picks its sample by V/OCT, patch **SOUND** to that input: each key plays the next sample.
3. Patch **VEL** to an accent or velocity input, and **PITCH** to its tuning, so the Sequencer's pitch dial and LFO
   reach it.

## Send FX Breakout

<picture><source media="(prefers-color-scheme: dark)" srcset="assets/modules/FxBreakout-large-dark.png"><img alt="Send FX Breakout" src="assets/modules/FxBreakout-large-light.png"></picture>

An [FX track](fx-tracks.md) whose effect is made of other modules. Its **L** and **R** carry the track's own effect
on its own; its **SEND** outputs replace it.

- **Out:** **SEND L** and **SEND R** carry what the tracks send the FX track, ±5V, after the LFO's send target.
  **TIME** is the time its keys set, as on Delay (1/64 of a bar on the lowest key to 3/8 on the highest, in tempo),
  at 1V a second, after the LFO's time target. **V/OCT** and **GATE** are its notes, a channel each. **LFO** is ±5V;
  **CV 1**, **CV 2**, **FILTER** and **RESO** are the dials, 0 to 10V.
- **Return:** **RET L** and **RET R** come back through the track's filter, unless **FILTER** or **RESO** is patched,
  and at its level into the mix, as the built-in effects do.
- **In:** with **GATE** patched, the **V/OCT** and **GATE** inputs set the time in place of the track's notes.

### An effect from outside

1. Patch **SEND L** and **SEND R** into a delay or reverb, and its outputs to **RET L** and **RET R**. Turn up a
   track's **FX 1** (or **FX 2**) send, and the effect returns to the mix like the built-in ones.
2. Patch **TIME** to the effect's time CV, scaled to suit, so the track's keys and steps set it in tempo.
3. Patch **CV 1** and **CV 2** to two of its controls: the Sequencer's dials, a Screen and parameter locks now turn
   them.
