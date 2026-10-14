# Test harnesses (saved 2026-10-14 from a session scratchpad)

Paths inside these scripts point at the old scratchpad (`/tmp/claude-0/.../scratchpad`); fix `S=` before reuse.
/mnt/project-files is noexec, so copy this folder and ../rack-headless to a scratchpad and run them from there.

- `robust.cpp`: AddressSanitizer + UBSan checks, one test per process: `./robust <test> <kits-dir>`.
  Tests: oldparams, component_label, component_play, organ_sine, kit_rate, kit_nan, kit_fuzz, opz_fuzz, slots_export,
  kit_tiny_rate, kit_names, kit_saved_speed, svf_nan, components_clamp, ppqn, switch_locked.
  Build: `g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I $V/src -I $V/src/opz
  -I $R/include -I $R/dep/include -DARCH_LIN robust.cpp $V/src/opz/Kit.cpp $V/src/opz/BuiltinKits.cpp -L$R -lRack
  -Wl,-rpath,$R -pthread -o robust` (V = repo, R = Rack source tree). Run with ASAN_OPTIONS=detect_leaks=0.
- `cpu.cpp`: µs per sample for a full row and each engine alone: `./cpu lights 10`. Build as above but with
  `-O3 -funsafe-math-optimizations -march=nehalem` and no sanitizers.
- `organhit.cpp`, `files.cpp`, `kits.cpp`: smaller one-off checks (Organ sine index, kit file loading, built-in kits).
- `kits/`: an OP-1 kit AIFF and a folder of sounds for the loaders.
- `patches/`: docs-hero.json (the docs' hero row), drums-hero.json, beat.json, breakout.json, and
  migrate-2.0.4.json / migrate-2.0.8.json (a Sequencer with zeroed mixer params, as an old and a new version).
- `actions/`: xdotool scripts for `../rack-headless/shot.sh ACTIONS=...` (doc screenshots: play.sh hero, comp.sh step
  components, drums.sh, bo.sh breakout; undo.sh project-switch undo). They source `opztest-lib.sh`, which needs
  `../rack-headless/lib.sh` and `ids-sequencer.py`. At ZOOM=2.0 they override at() with 5.906 px/mm.
