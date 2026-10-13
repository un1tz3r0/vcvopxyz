# To do

## Next

- [ ] **FX modules** for the **FX 1** and **FX 2** sends, the OP-Z's tracks 13 and 14.
- [ ] **The arp track's page:** the OP-Z swaps track 7's LFO page for arp speed, pattern, style and range.

## Later

- [ ] **A master module** that owns the tempo, 16 patterns per track and pattern chaining, and tonality. The
      Sequencer's tempo display, INT/EXT and transport move there.
- [ ] **Tonality:** a chord progression and key from the master, for the **tonality** component and for ramps and
      random, which use C major until then.
- [ ] **A ninth track,** the master track, beside the four drum and four synth tracks.
- [ ] **More OP-Z synth engines,** each a `Voice` and two dials on the shared synth track.
- [ ] **The OP-XY:** its sequencer and engines, once the OP-Z's are complete.
- [ ] **VCV Library** submission.

## Loose ends

- [ ] Tune by ear: each engine's voicing, the built-in drum kits and the drums' level against the synths, the sweep
      component's depth (half a dial's travel now) and portamento component's glide times (up to a beat now).
- [ ] **Drum note styles** follow a reading of the guide (retrig rings two sounds or more, mono chokes, gate and
      loop follow the note). Check them against the hardware.
- [ ] **OP-1 kit files:** the slices' pitch and volume encodings are guesses. Check them with a kit whose sounds
      are retuned or turned down on an OP-1 or OP-Z.
- [ ] **Kit editing:** a kit's slice starts and ends, and each slice's tuning and play mode, can't be changed in
      Rack yet, and kits can't be saved back out as OP-1 kit files.
- [ ] Loading a kit from the context menu isn't undoable.
- [ ] A preset keeps a loaded kit's slices but not its sounds, which live in the patch's storage folder.
- [ ] Jack LEDs on unpatched outputs stay dark, since an unpatched output has no channels to meter.
- [ ] The RUN, RESET and CONT inputs aren't tested in a running Rack yet.
- [ ] **OP-Z project files** are untested on a real OP-Z. Check an export loads, and work out the track settings'
      bytes (step count, step length, note length, quantize) and swing, which are guesses.
- [ ] Write the mixer's mutes into exported project files.
- [ ] **Recording dial moves as parameter locks** while recording live, if the OP-Z does: the dials only lock a
      held step now.
- [ ] A Screen shows the dials' own values, not the locks of the step playing.
- [ ] **16 patterns and chains** per project, read and written in project files, once the master module has them.
      Only the pattern playing is read and written now.

## Waiting on Vic

- [ ] Rename the Sequencer, now that it is the master and not just a sequencer?
- [ ] What do Digital's **octave** and **feedback** dials do on the hardware? Assumed: the modulator's octave, and
      FM depth.
