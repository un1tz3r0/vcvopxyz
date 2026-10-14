# Temporary handoff branch

Not part of the plugin's source tree; delete this branch once the new thread has pulled it down.

- `handoff/HANDOFF.md`: state of play after v2.0.10.
- `handoff/thread-export.md`: the whole "Synth track sequencer" thread.
- `handoff/session-transcript.jsonl.gz`, `handoff/subagent-transcripts.tar.gz`: raw transcripts.
- `tools/harness/`: sanitizer tests, CPU benchmark, test kits and patches, screenshot click scripts (see its README).
- `tools/rack-headless/`: headless VCV Rack screenshot and click-test scripts.

Fetch with `git fetch origin handoff-2026-10-14 && git worktree add ../handoff origin/handoff-2026-10-14`.
