# Remotes

Remotes plays the [Sequencer](sequencer.md)'s patterns and saved chains from jacks. Put it directly to the Sequencer's
left. **LINK** lights when Remotes finds the Sequencer. Remotes makes no sound, and it is 8 HP wide.

Each input works only while it has a cable. With no cables, the Sequencer works as before.

## Inputs

| Jack | Kind | What it does |
| --- | --- | --- |
| **SELECT** | Polyphonic trigger | A trigger on channel 1 to 16 plays [pattern](project.md#patterns) 1 to 16, as a step does in project mode. |
| **NEXT** | Trigger | Plays the next pattern of the [chain](project.md#chains). With one pattern, it plays the next pattern with notes. |
| **PREV** | Trigger | Plays the previous pattern, in the same way as **NEXT**. After 16 comes 1, and before 1 comes 16. |
| **BAR** | Trigger | Makes the other inputs wait for the next bar. See [Bar sync](#bar-sync). |
| **SONG** | CV, 0 to 10 V | Chooses one of the 14 [saved chains](project.md#saved-chains). |
| **LOAD** | Trigger | Plays the saved chain that **SONG** chooses. With no cable here, the chain plays when **SONG** changes. |

If two **SELECT** channels trigger together, the lower channel wins. While you build a chain (**PROJECT** latched,
**PLAY** blinking), each trigger adds its pattern to the chain, as a step does.

A saved chain that is empty does nothing.

## Outputs

| Jack | Kind | What it does |
| --- | --- | --- |
| **START** | Trigger | A pattern starts: from the top, at a change of pattern, or when it loops. |
| **END** | Trigger | A pattern's [cycle](#the-cycle) ends and the pattern loops. |
| **SONG END** | Trigger | A chain returns to its first pattern. |
| **PATTERN** | Polyphonic gate | 16 channels. The channel of the pattern that plays is high. |

**PATTERN** into the **SELECT** of a second Remotes makes the second Sequencer follow the first.

## When a change happens

While the Sequencer is stopped, a change happens at once.

While it plays, a change happens at once if **BAR** has no cable. Each track continues from its step, as when you
click a step in project mode. The next step comes from the new pattern.

### Bar sync

With a cable in **BAR**, a change waits for the next **BAR** trigger. On that trigger, every track starts the new
pattern from step 1. Several changes before the bar add up: two **NEXT** triggers move two patterns.

### Late triggers

A trigger that comes a little late still counts as on time. The default is 5 ms. The context menu sets 1, 2, 5, 10 or
20 ms, or **Off**.

- A late **SELECT**, **NEXT**, **PREV** or **LOAD**: the step that started in that time plays again from the new
  pattern.
- A late **BAR**: the bar starts on the sixteenth that started in that time.
- A change that comes just after a **BAR**: it counts for that bar, not the next.

Thus **END** into **NEXT** works, also through other modules. Each cable adds one sample, so the trigger is always a
little late.

### The cycle

**END** comes at the end of the pattern's cycle: the time for every track to return to step 1 together. Each track's
length is its step count times its step length. The cycle is the lowest length that all the lengths go into.

- Only tracks with notes, [step components](step-components.md) or [parameter locks](steps.md) count. Empty tracks
  and tracks that wait for **ADV** do not.
- Lengths of 16, 12 and 10 sixteenths give a cycle of 240 sixteenths, which is 15 bars.
- In a chain, each pattern ends at its longest track, before its cycle ends. **END** does not come in a chain, but
  **START** does.
- **END** follows the time, not the steps. A [jump](step-components.md) can put a track on another step when **END**
  comes.
