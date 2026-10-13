# To do

## Next

- [ ] **Parameter locks over the bus:** per-step dial values for the engine playing each track, held like a step
      and set by turning a dial. Brings the **parameter spark** component to life.
- [ ] **Drum engines** for tracks 1 to 4 (kick, snare, perc, sample), with the drum note styles: retrig, mono, gate,
      loop.

## Later

- [ ] **A master module** that owns the tempo, 16 patterns per track and pattern chaining, and tonality. The
      Sequencer's tempo display, INT/EXT and transport move there.
- [ ] **Tonality:** a chord progression and key from the master, for the **tonality** component and for ramps and
      random, which use C major until then.
- [ ] **A ninth track,** the master track, beside the four drum and four synth tracks.
- [ ] **The arp track's page:** the OP-Z swaps track 7's LFO page for arp speed, pattern, style and range.
- [ ] **FX modules** for the **FX 1** and **FX 2** sends.
- [ ] **More OP-Z synth engines,** each a `Voice` and two dials on the shared synth track.
- [ ] **The OP-XY:** its sequencer and engines, once the OP-Z's are complete.
- [ ] **VCV Library** submission.

## Loose ends

- [ ] Tune by ear: each engine's voicing, the sweep component's depth (half a dial's travel now) and portamento
      component's glide times (up to a beat now).
- [ ] Jack LEDs on unpatched outputs stay dark, since an unpatched output has no channels to meter.
- [ ] The RUN, RESET and CONT inputs aren't tested in a running Rack yet.
- [ ] **OP-Z project files** are untested on a real OP-Z. Check an export loads, and work out the track settings'
      bytes (step count, step length, note length, quantize) and swing, which are guesses.
- [ ] Write the mixer's mutes into exported project files.
- [ ] **16 patterns and chains** per project, read and written in project files, once the master module has them.
      Only the pattern playing is read and written now.

## Waiting on Vic

- [ ] Rename the Sequencer, now that it is the master and not just a sequencer?
- [ ] What do Digital's **octave** and **feedback** dials do on the hardware? Assumed: the modulator's octave, and
      FM depth.
