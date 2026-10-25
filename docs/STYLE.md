# How the manual is written

The manual uses plain English, after the rules of ASD-STE100 (Simplified Technical English). The rules for sentences
are strict. The rules for words come from the list of terms below. `tools/docs-lint.py` checks the rules that a script
can check.

## Who reads it

- **VCV users new to the OP-Z.** They know modules and cables, but not tracks, steps, pages or latched keys.
- **OP-Z owners.** They know the combos and the colours, but maybe not Rack, and not what is different here.
- **Newcomers to both.** They need the install, a short Rack primer, and a tutorial that assumes nothing.
- **Contributors.** They read [development.md](development.md) and [DESIGN.md](../DESIGN.md), which this file does
  not cover.

## One job for each page

Each page does one of four jobs. It does not do the others.

| Job | Pages | The reader |
| --- | --- | --- |
| Learn | The tutorials | Follows numbered steps and hears the result. |
| Do | The recipes | Has a goal and wants the shortest way there. |
| Look up | The reference pages | Wants one fact about one key, jack or dial. |
| Understand | [How it works](how-it-works.md) | Wants the model behind the modules. |

Each fact lives on one page. Other pages link to it and do not tell it again.

## Sentences

- A step has at most 20 words. A description has at most 25.
- A step does one action. Write steps as a numbered list, in the imperative: "Click **TRACK**."
- Write a goal before a procedure and a result after it: "You hear the bass." "The step lights orange."
- Use the active voice and simple tenses.
- Do not use semicolons.
- Do not use phrasal verbs, such as "pick up", "turn up" or "come back". Use one verb: "latch", "raise", "return".
- Do not stack four or more nouns.
- A paragraph has one topic and four sentences or fewer.
- Use a list for three or more steps or conditions.

## Terms

Use one word for one thing. [How it works](how-it-works.md#glossary) defines each term. Link a term to the glossary
the first time a page uses it.

| Use | For | Do not use |
| --- | --- | --- |
| **latch** | A key that stays on after you release the mouse button. TRACK, REC and the index keys latch on a click. A step and SHIFT latch on a long press or a Ctrl+click. | "pick up", and "hold" for a latched key |
| **hold** | Only a real held mouse button: SCREEN, and STOP for a second. | |
| **plug** | The sound a track plays. Its kinds are **engine** (synth tracks), **kit** (drum tracks) and **effect** (FX tracks). | "sound" or "voice" for a plug |
| **voice** | One note that plays now. Polyphony counts voices. | "channel", except for a jack's channels |
| **plug slot**, **project slot** | The ten places on the black keys in track mode and in project mode. | "slot" with no qualifier |
| **dial**, **page** | The four coloured controls, and their four sets of jobs: sound, envelope, LFO, mix. | "knob" |
| **the row** | The modules to the right of a Sequencer that touch it and each other, with no module from another plugin between. | "bus" (DESIGN.md keeps it) |
| **track 5 (bass)** | A track: its number, then its name in brackets, the first time a page uses it. | The name alone |
| **TRACK** | A key or jack, in bold, spelled as on the panel. | Other spellings |

## Pictures

- Every picture has alt text, and a light and a dark version in a `<picture>` element.
- A picture in a procedure shows the result of the step above it.
- Diagrams come from `tools/docs-images/diagrams.py`. Panel pictures come from the other scripts in that folder,
  which run a headless Rack.

## Facts

Check each fact against the source code, not against the old text. Compare names and behaviour with the matching
chapter of teenage engineering's OP-Z guide. Do not describe a feature that the plugin does not have.
