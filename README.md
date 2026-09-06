# TotalRecruit

Universal, non-cheat recruitment for Kenshi, as a native RE_Kenshi C++ plugin.

Target: **Kenshi 1.0.65 x64 (Steam)**.

---

## Why

Every popular recruitment mod — Recruitable Prisoners, TameBeasties, Recruit
Anyone — attaches an FCS dialogue package to each NPC. That package only exists
on the NPCs that shipped with the mod, so **characters added by any other mod
cannot be recruited**, and the author ends up maintaining one compatibility
patch per mod, forever.

A plugin does not ask *"does this NPC have the dialogue installed?"*. It asks
*"is this a Character?"* — which covers mods that do not exist yet.

## What it does

Four ways to gain someone, each with its own logic.

### Rescue

You found them bleeding out and chose to save them. No cage, no price, no
negotiation. The engine remembers it as `LT_MY_LIFESAVER`, which persists in the
save, so the debt survives even if you walk away before they wake — and the
offer comes **when they get back on their feet**, not the moment the bandage
goes on.

This is the only route in Kenshi where the player is not exercising power, and
no existing recruitment mod has it. Animals live almost entirely here.

### Captivity

They are in your cage. What they want depends on who they are:

| Who | Wants |
|---|---|
| Starving or wounded | Food and healing |
| Bandits, traders | Cats |
| Slaves | To be freed |
| Soldiers, the proud | To be beaten first, then time |
| Unique characters | Submission, plus a longer wait |

Fearful people come cheaper. The brave and the cruel cost more.

### Surrender

You beat them, and they are bleeding out or running. Rather than be cut down,
they yield. No cage needed — the fight itself was the argument.

### Refusal, with a reason

Someone who watched you kill their friend says no — and you are told so. Every
verdict comes with its reason, on the button before you click and in the message
roller after. No other mod can do this, because dialogue data cannot explain
itself.

## Presets

- **Cheat** — anyone, instantly. The original behaviour.
- **Balanced** — the prices above. *(default)*
- **Vanilla+** — as Balanced, plus faction relation shifting the price.

Faction relation is deliberately never a gate. It is the one thing the player
cannot change, so using it as a door would make half the world unrecruitable —
which is the problem this mod exists to solve.

## Install

Copy the `TotalRecruit` folder into `[Kenshi]\mods\`, then enable it in the
launcher.

```
[Kenshi]\mods\TotalRecruit\
├── TotalRecruit.mod
├── RE_Kenshi.json
├── config.json
└── TotalRecruit.dll
```

Requires [RE_Kenshi](https://www.nexusmods.com/kenshi/mods/847). It coexists
with UnlimitedRecruitV2 and with the dialogue-based recruitment mods — all
widget names are distinct, and the overlay anchors beside the context menu
rather than below it, where other mods put theirs.

## Controls

| | |
|---|---|
| Right-click an NPC | The recruit button, labelled with what they want |
| **Ctrl+K** | Settings |
| **Shift+K** | Spawn panel |

## Optional: FCS dialogue

The plugin works as a code-only install. Supply an FCS dialogue and the offer
becomes a real conversation instead of a message, with branches and the chance
to decline.

The split is: **FCS owns the words, the plugin owns the timing.**

1. In FCS, create a `DIALOGUE_PACKAGE` and one or more `DIALOGUE` records.
2. On the line that means *"yes, join me"*, add a reference list named
   `totalrecruit accept` (the name is `dialogueAcceptKey` in config.json).
3. Put the string IDs in `config.json`:

```json
"dialoguePackage": "your-package-stringid",
"dialogueRescue":  "your-rescue-dialogue-stringid",
"dialogueCaptive": "your-captive-dialogue-stringid"
```

The plugin attaches the package **at runtime**, so it reaches NPCs from mods
that have never heard of it — the patch treadmill does not come back.

Useful FCS conditions for gating lines: `DC_IS_IMPRISONED`,
`DC_IMPRISONED_BY_TARGET`, `DC_PERSONALITY_TAG`, `DC_IS_SLAVE`, `DC_STARVING`,
`DC_MET_TARGET_BEFORE`, `DC_I_LOVE_THIS_GUY`.

Useful actions: `DA_JOIN_SQUAD_FAST`, `DA_TAKE_MONEY`, `DA_FREE_TARGET_SLAVE`,
`DA_REMEMBER_CHARACTER`.

## Configuration

Everything in the settings panel is also in `config.json`. `preset` is `0`
Cheat, `1` Balanced, `2` Vanilla+.

Set `probeOnClick` to `true` to log an NPC's personality, archetype and medical
state on right-click. That goes to `RE_Kenshi_log.txt` in the Kenshi root —
**not** `kenshi.log`.

## Building

Visual Studio with Platform Toolset **v100**, Boost 1.60, and the
`KENSHILIB_DIR` / `BOOST_INCLUDE_PATH` environment variables.

```bash
MSBUILD="/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"
"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo
```

Release | x64 only.

See **[DEVELOPING.md](DEVELOPING.md)** for the full build/deploy cycle, a map of
the source, and what to do when something breaks. [PLAN.md](PLAN.md) has the
design and every engine API it relies on; `../KENSHI-MODDING-NOTES.md` has the
SDK traps.

## Status

Compiles clean and exports correctly. **Not yet play-tested** — see the
verification section of [PLAN.md](PLAN.md) for exactly what remains unproven
against a running game.
