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
no existing recruitment mod has it. Animals never take this route: they are
tamed with food from your hand and nothing else (see Captivity).

### Captivity

They are in your cage. What they want depends on who they are:

| Who | Wants |
|---|---|
| Starving or wounded | Food and healing |
| Bandits, traders | Cats |
| Slaves | To be freed |
| Soldiers, the proud | To be beaten first, then time |
| Unique characters | Submission, plus a longer wait |
| Animals | Food from **your** hand, until the stomach reaches their bar |

Fearful people come cheaper. The brave and the cruel cost more.

**Animals.** A wild animal sits at a full stomach on its own, so a full stomach
proves nothing: an animal joins only once you have fed it at least once (the
meals are saved with the character) and it is full enough. Caged or loose, the
same rule; time in the cage, intimidation and having saved its life do not tame
it.

**Golpear and Noquear.** Right-click a prisoner awake in your cage and two
buttons stand under Hablar. They are Kenshi's own orders (beat the cage
occupant, stealth knockout) given to the character you have selected, so the
wounds, the stress and whether the knockout lands are the game's; Noquear shows
the game's own odds. A knockout ordered this way counts as your defeat.

**Easy mode.** A healthy, fed prisoner who has been in your cage long enough
simply joins. The wait is **6 game hours, flat for everyone** (`easyHoursCaged`;
it used to be 12). The clock starts when the prisoner is first seen in your cage
and is dropped when they leave it.

**Price.** Quotes follow one calibrated curve by the recruit's own level, at
vanilla-like prices: roughly 380 at level 5, 1k at 10, 1.6k at 13, 3.4k at 20,
11.5k at 40, 20k at 60 and 27k at 75, for an ordinary character. Talent (up to
x1.5), uniqueness (x2) and personality (coward x0.6, brave/cruel x1.6) multiply it
together, capped at `premiumMaxMultiplier` (x1.6; a coward's discount still
applies), so a level-13 named, talented NPC costs ~2,600. Faction standing
multiplies after that cap. The result is rounded to the
nearest 10, floored at `catsMinimum` and capped at `catsHardCap` (150,000). Your own
level is not part of the price: it already drives the join chance. A charge is
all-or-nothing, and if the engine refuses the recruit afterwards the cats are
refunded.

### Surrender

You beat them, and they are bleeding out or running. Rather than be cut down,
they yield. No cage needed — the fight itself was the argument.

**They come to you.** A beaten NPC that yields (beaten enough times, or
running from you) walks up on its own, calm — a hostile one stops attacking for
the walk (`submissionApproachSeconds`, 60 s) — kneels, and asks to join:
**Aceptar** (they join), **Perdonar** (they run off and stay calm for
`submissionForgiveSeconds`, 5 minutes, so you can still talk to them; then back
to what they were) or **Rechazar** (your defeats over them are wiped, no route
recruits them for `submissionRejectCooldownHours`, 72 game hours, saved with
them, and they are hostile again). Closing the window with the X is no answer:
they stay calm where they are for the same 5 minutes. One NPC at a time, once
per beating. Calm is per character: no faction relation is changed. Turn it off
with `submissionEventEnabled`.

### Allies, by level (Balanced)

An ally (faction standing at or above `friendlyRelation`) no longer gets one
daily roll whatever their level. In Balanced their level and their personality
decide the deal. A good-natured ally under level 20 joins free on today's roll;
everyone else pays a friend's price, with friendship taking up to 20% off
(10% from level 50) in proportion to the standing. From level 80 the good and
the ordinary only come along as temporary members, for 3 to 10 days, and only
behind a leader no more than 15 levels below them; the cruel refuse, and from
level 100 so do the greedy. The popup says which: "Aliado (nivel 34): te cobra
1.840 cats, con 14% de descuento por amistad", with the discount beside the
price. Every number is an `allyTier*` key in `config.json` and in Mod Hub;
Vanilla+ and Cheat keep their old ally behaviour.

A temporary member tells you, over their head, how long they will stay ("Te
acompanare 6 dias"), and their card counts the days left. Starve them, hit them
or knock them out and they complain ("No me trates asi"); one strike before the
last they warn you ("Si sigues asi me voy"), and at three strikes they leave.
Otherwise they say "Me ire pronto" half a day before their time is up and then
go: back to their own faction, walking away in a squad of their own, and they
will not join you again for a week. The numbers are `tempMember*` keys.

Sometimes an ally far stronger than your leader, of good or ordinary character,
walks up and offers to come along for a few days, for free ("Quiere ofrecerte
ayuda"; the popup says how many days and that you cannot give them orders).
Accept and they stay in their own faction but follow you and fight whoever your
leader fights, until their days run out ("Hasta aqui llego"); hit them and they
leave at once. Their card counts the days left, and they pick up again after a
reload. A 15% chance per ally per day, `helper*` keys; `helperDebugKey` (a
Windows key code, 119 = F8) makes the selected NPC offer at once for testing.

With TotalSparring installed, an ally of level 80 or more
can also be challenged to a duel. Their popup keeps its usual answer and adds
**RETAR A DUELO**, with your selected fighter's head-to-head record against
them ("DUELOS 2-1", also on their card as "Duelos: 2-1"). The button opens
TotalSparring's own Dueling window with the character you have selected;
TotalSparring decides whether it is friendly or to the death and fights it out.
Win and they join you for good ("Me ganaste. Me uno a ti"); lose or draw and
they tell you to come back stronger, and you cannot challenge them again for a
day. A spar never counts, and a duel that was cancelled or interrupted leaves
the challenge open for six in-game hours. Without TotalSparring nothing
changes. `duelChallenge*` keys in `config.json` and in Mod Hub.

### Refusal, with a reason

Someone who watched you kill their friend says no — and you are told so. Every
verdict comes with its reason, on the button before you click and in the message
roller after. No other mod can do this, because dialogue data cannot explain
itself.

## Presets

- **Cheat** — the cage and a one-minute wait, nothing else: the character must be
  in your cage and held for `cheatHoursCaged` (0.02 game hours); no cats, food,
  submission, level or standing. `cheatNeedsCage` turns the cage requirement off.
- **Balanced** — the prices above, and allies by level. *(default)*
- **Vanilla+** — as Balanced, plus faction relation shifting the price; allies
  keep the single daily roll.

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
├── messages.json         <- every line the NPCs and the UI say
├── TotalRecruit.dll
└── gui/images/tr_white.png   <- fills of the readout card
```

`gui/images` is needed for the readout bars to look right with a skin mod such as
DarkUI or PowerModUI; without it the mod still works and falls back to `WhiteSkin`.

Requires [RE_Kenshi](https://www.nexusmods.com/kenshi/mods/847). Settings live in
Emkej's Mod Hub (optional: without it the mod
runs on `config.json`). It coexists with UnlimitedRecruitV2 and with the
dialogue-based recruitment mods — all widget names are distinct. The Hablar
button sits beside the context menu in the first free slot (left, right, then
below whatever is already there), so it never covers the buttons TotalSparring,
TotalLoot, TotalPredator or Loot-Scoot-Execute put there, and it has the same
size and look as theirs (160 x 44, Kenshi_Button1).

It also coexists with **TotalSparring** and **Kill Counter**. TotalRecruit gives
each character its own id, stored in the save under `total_recruit.character_id`
(TotalSparring uses `total_sparring.fighter_id`), so nothing is shared or
overwritten and a character keeps their join roll and state across a relaunch.
A save from before this has no ids: each character gets one on first contact and
their stable roll is recast once.

## Controls

| | |
|---|---|
| Right-click an NPC | The recruit button, labelled with what they want |
| Press the recruit button (Hablar): click it, or hold right click on the NPC, slide onto it and release, as with the game's own menu options | The recruit popup, with the stress and join-chance card beside it (see below) |
| **Shift+K** | Spawn panel |

Settings are in the Mod Hub panel, not in a window of this mod.

### The recruit popup

Every Hablar opens the same window, whatever the answer: the NPC's portrait,
name and route, one line on what is happening, and a small box with the price,
what you carry and the odds when they apply. A price you cannot pay turns red,
says how many cats you are short, and hides Accept. A refusal shows its reason
in red, plus the one thing you can do about it when there is one (feed them,
beat them). Money only moves when you press Accept. The text is the `popup`
block of `messages.json`.

**Haggling.** A price in cats comes with a slider, from half the ask
(`haggleMinFraction`) up to the ask, and the live odds that they take your
offer. At the ask it is Accept as before; below it the button reads Offer and
you get one try, on the same daily seed as the join roll. They take it and you
pay what you offered, or they are offended: no more haggling with them that
in-game day and the ask goes up 20% (`haggleOffenceMarkup`). Your purse is
checked against the offer. Turn it off with `haggleEnabled`.

### The readout card

Select an NPC with a left click (or press Hablar on them) and a small card appears
over their head: the route that answers them (Captivity, Surrender, Animal, Free,
Rescue, Mercy), a three-colour stress bar (hunger, wounds, tiredness), the join
chance, the defeats counted against the threshold, the stomach bar for animals,
one short line saying what stands in the way, and the hours still to wait when
easy mode applies. Every number is one the decision itself computed. The card
shows the odds, never whether the roll will pass, and asking has no side effects.
It follows the selection: deselect the NPC and it goes, select another and it
switches. It is worked out when the selection changes and refreshed every 2
seconds, so it costs nothing per frame. Pressing Hablar places it beside the offer
window; when the offer closes it stays only if that NPC is still selected. It never
takes a click and hides while a right-click menu is open. Hub switches: card
on/off, compact, and "on selection" (on by default; off, it only shows on Hablar).

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

Everything in the Mod Hub is also in `config.json`. `preset` is `0` Cheat, `1`
Balanced, `2` Vanilla+. The Hub writes by merging into the file: keys you edited
by hand, comments and unknown keys are kept.

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

Built (Release|x64) and unit-tested: `tests/rules_tests` covers the engine-free
rules (route order, chance, price curve, money transfers, defeats, hold clock,
identity registry, config merge, readout view-model). **In-game QA is pending**:
the cases to run, in priority order, are in [QA.md](QA.md) section 12.
[ESTADO.md](ESTADO.md) says what is proven in play and what only compiles.
