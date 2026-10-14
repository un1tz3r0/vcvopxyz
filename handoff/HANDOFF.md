# Handoff: vcvopxyz, after v2.0.10 (2026-10-14)

This is a temporary note so the "Synth track sequencer" thread can start fresh. The full thread is in
`thread-export.md` beside this file. It has every message from Vic, every reply, both decision cards, and the
nine context summaries. The raw session transcript is `session-transcript.jsonl.gz`, and the subagents' transcripts
are in `subagent-transcripts.tar.gz`. Project memory (`MEMORY.md` and its linked files) holds the durable facts.
This note is the state of play.

## Where things stand

- **Repo:** github.com/un1tz3r0/vcvopxyz. On main, everything through **PR #11 (v2.0.10)** is merged.
- **First release:** Vic pushed the **v2.0.9** tag on 114bea4, and CI publishes all four platforms on a tag. Sessions
  here can't push tags, so ask Vic for the next one. That would be v2.0.10 if he wants it.
- **Working branch:** `claude/synth-track-sequencer-n9025v`. Its PR is merged, so restart it from main for new work
  (`git fetch origin main && git checkout -B claude/synth-track-sequencer-n9025v origin/main`).
- **Linux test builds:** they are in `/mnt/project-files/builds/` and go up to 2.0.10. Builds made in a session
  need glibc 2.38+. CI artifacts run on older systems.

## Modules (v2.0.10)

- **Sequencer** (40 HP) is the OP-Z front panel and the master of the row. It has 8 tracks (4 drum, 4 synth), 16
  steps, step components, parameter locks, 10 project slots with .opz import and export, a mixer with mute groups,
  tempo/INT/EXT and a metronome. It no longer has per-track outputs: their 16 output ids stay in the enum as
  `OLD_TRACK_OUTPUT`, with no jacks.
- **Engines** sit to its right as expanders over the bus (`src/opz/Bus.hpp`). Drum takes tracks 1 to 4 and synth
  engines take 5 to 8, in order.
  - Synth engines: Digital, Analog, Cluster and Organ, all 6 HP.
  - Breakout (12 HP) sends a synth track out as CV and takes its voice back in at RET.
  - Drum plays a kit: built-in synthesized kits, OP-1 kit AIFF, AIFF or WAV, or a folder.
- **Screen** shows any track as the OP-Z app does.

## Decisions Vic made on 2026-10-14

1. **Releases:** GitHub only for now. Asking teenage engineering for permission is pinned, and a redesign under
   another name is the last resort, for later.
2. **Old patches** are migrated. `Sequencer::fromJson` reads the plugin version Rack stores per module. Below 2.0.6,
   or with no version, it resets the mixer and metronome params to their defaults. From here on, **port and param
   ids are append-only.**
3. v2.0.9 is the first release. That is done.
4. **There is no separate master module.** Patterns, chains, tonality and the tempo all go into the Sequencer. TODO
   and the docs now say so.
5. The robustness pass, Vic's pick on the decision card, is done in PR #11. It covered:
   - bus CPU, which halved;
   - kit file sanitising;
   - NaN-safe filters;
   - clamped patch JSON;
   - the projects lock, which the audio thread now only tries;
   - one-step project undo;
   - Organ, Cluster and track-mode fixes.

## Open questions for Vic

- What should be built next? The candidates, in the order of the 2026-10-14 review:
  - **Drum Breakout:** his README already describes it. Tracks 1 to 4 have no way out since the outputs went.
  - **FX sends and FX modules:** the OP-Z's tracks 13 and 14.
  - **The arp track's page.**
  - **16 patterns and chaining** in the Sequencer.
  - **Tonality.**
- Rename the Sequencer?
- What do Digital's octave and feedback dials do on the hardware? The current guess is the modulator's octave and
  the FM depth.

## Standing rules (Vic)

- Bump plugin.json's patch version on every code change, and bump again if Vic has already tried a build.
- Keep code concise and complete, with sparse comments and no excess tests.
- Keep `docs/`, `DESIGN.md` and `TODO.md` current with each feature.
- Write PR bodies as Before/After/How.
- Don't copy libopz code: its licence is non-commercial.
- Push only to the designated branch.

## Tooling to rebuild in a new session

- **No Rack SDK:** vcvrack.com is blocked. Build Rack 2.6.6 from source in the scratchpad. The steps are in memory:
  `headless-rack-screenshots`, and `local-build-without-sdk` for a compile-only build.
- **Headless Rack scripts** are in `/mnt/project-files/tools/rack-headless/`. That folder is mounted noexec, so copy
  it to the scratchpad first.
- **Test harnesses** are in `/mnt/project-files/tools/harness/`: the sanitizer tests, the CPU benchmark, test kits,
  test patches and the doc-screenshot click scripts. Its README covers the build lines and the paths to fix.
- **Project notes** are in `/mnt/project-files/notes/`. They include the OP-Z reference, the sequencer research, the
  .opz format, and the 2026-10-14 project review with its CPU table.

## Known loose ends (also in TODO.md)

- **Untuned:** engine voicing, the built-in kits, the drums' level against the synths, sweep depth and glide times.
- **Unverified on hardware:** drum note styles, the OP-1 kit pitch and volume encodings, and the .opz track-settings
  bytes and swing.
- **.opz export** holds micro timing only from −12 to +11 ticks.
- **Housekeeping:**
  - CI warns about Node 20, and the Linux job runs in an end-of-life ubuntu:20.04 container.
  - Dead code: `src/Screen.{hpp,cpp}` and the unused widgets in `src/ui.hpp`.
