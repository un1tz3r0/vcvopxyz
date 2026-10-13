# Breakout

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/breakout-dark.png">
  <img alt="Breakout, with Analog patched into RET L" src="assets/breakout-light.png" width="180">
</picture>

A [synth track](synth-tracks.md) whose voice is made of other modules. It is 12 HP and has everything an engine has,
plus a dark box of outputs at the top that send the track out, and **RET L** and **RET R** to take the sound back in. It takes
its track from a Sequencer to its left like any engine.

- **Out:** **V/OCT**, **GATE** and **ENV** are polyphonic, a channel per voice (one in mono and legato), with the
  glide and the LFO's pitch in V/OCT. **LFO** is the LFO after its amount, ±5V. **CV 1** and **CV 2** are the sound
  page's dials, and **FILTER**, **RESO**, **FX 1** and **FX 2** the dials of those names, all 0 to 10V, with the LFO.
- **Return:** whatever goes out, the track stops doing itself. With **ENV** patched the return isn't enveloped; with
  **FILTER** or **RESO** patched it isn't filtered; with an **FX** amount patched, that **FX** output carries the
  full signal for an outside VCA; with **LFO** patched, the LFO moves nothing inside. Level and pan always apply.
- **Envelope:** a polyphonic return is enveloped channel by channel, one voice each; a mono one by the loudest
  voice. **RET R** is normalled to **RET L**.

## A voice from outside

1. Patch **V/OCT** to a VCO's V/OCT, and the VCO's output to **RET L**. The track's envelope, filter and mix now
   shape the VCO.
2. Patch **FILTER** and **RESO** to an outside filter's cutoff and resonance CV, and put that filter between the VCO
   and **RET L**. The inside filter switches off.
3. Patch **ENV** to an outside VCA's CV and put it in the chain too. The inside envelope switches off, and the
   track is now only level and pan around your own voice.
