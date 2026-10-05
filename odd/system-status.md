# TotalRecruit: system status, 2026-10-04

One document for the whole mod: what it does today, what is still missing, and the ideas not built yet. Built from every feature document in `odd/tasks/`, `REQUISITOS.md`, `QA.md`, `DEVELOPING.md`, `README.md`, `ESTADO.md`, `TODO.md` and the session handoff. There is no TODO, FIXME or HACK in `src/`.

Engram project: `totalrecruit` (`.engram/config.json`). This document is mirrored under the topic `odd/system-status`; each feature document under `odd/<feature>/tasks`.

## Where things stand
- **`main` @ `3554d19`, deployed.** `feat/native-ai-packages` (native helper, native leave, KenshiLib 0.5.0, duel bridge, play-test fixes, duel join chance, daily notice) was fast-forwarded into `main`. Every merged local branch was deleted; `main` is the only local branch.
- **TotalSparring:** `feat/public-api` was merged into TotalSparring `master` (`be52149`); its branch and the `TotalSparring-worktrees/public-api` worktree were removed. The duel bridge talks to that API (v1, `TS_GetApi`).
- **Awaiting play-test:** the duel join chance and the daily days-left notice (deployed in `3554d19`; QA.md 12.29, 12.25 T12-T14).
- **Not pushed.** `origin/main` is an unrelated squash commit (`6257bd6`), so a plain push is rejected. Delivery is the user's call.

## Features
Status tags:
- `verified in game`: seen working in a real game with the log line that proves it (ESTADO "Jugado"). It predates the later rounds and is not a formal QA pass.
- `play-tested informally`: the user played it and reported it working. No QA row was marked.
- `not verified`: built and unit-tested (`tests/rules_tests`, 47 suites), not yet seen in game. The QA.md section to run is given.

Nothing has a formal QA pass mark.

### 1. Recruitment routes and gating (11)
| Feature | Status |
|---|---|
| Works on any `Character`, including NPCs from other mods (no per-NPC dialogue package) | `verified in game` |
| One chance engine: relation, level gap, personality, defeats and situation as separate terms, logged as `[chance]` (R0.1, R2.1) | `verified in game` (§2) |
| Route order: unbeatable grudge before field yield; cage defeats feed the chance (`[route] <name>`) | `not verified` (12.1) |
| Captivity: needs by who they are (food and healing, cats, freedom, beating then time) | `not verified` (§2, 12.4) |
| Balanced cage without shortcuts: the easy wait (`easyHoursCaged`) and the "healthy thanks to you" route were removed (v1-presets-close P4); a cared-for prisoner gets a stable daily bonus on the roll (5..20, `submissionCaredForBonusMin/Max`) | `needs in-game QA` (12.33 C1-C6, 12.3 E3, 12.8 U3) |
| Insistence cooldown, cumulative and configurable (R2.4); only a real refusal counts (failed roll, personality, honour, loyalty, grudge, ally tier), never a state, an instruction or a yes, and animals have no insistence (`insistence_rules`, animal-insistence-cheat-feed A1) | `needs in-game QA` (§5 I1, I6-I9, 12.7 N1) |
| Rescue: `LT_MY_LIFESAVER` debt, offer on waking, parting gift paid once (R4) | `not verified` (§6) |
| Grudge refusal by character (who cannot forgive what they saw) | `not verified` (12.7) |
| Field yield ("Sometido"): beaten in the field; respects other cages, lockout and loyalty | `not verified` (12.14) |
| Player defeats saved in the character save, one per knockout (`total_recruit.player_defeats`); the hook's twin call inside 5 s is the same knockout | `not verified` (12.13 K4b) |
| Cheat preset: cage plus a one-minute hold, nothing else (`cheatNeedsCage`) | `verified in game` (§8, R1.3) |
| Cheat: every wait is one game minute (`cheat_waits`: cage wait, hail first and repeat, Golpear/Noquear rests, reject and lost-duel cooldowns; the last two end at the next whole game hour) | `needs in-game QA` (§8 H7-H10, 12.33 W1-W2) |

### 2. Chance, stress and the readout card (7)
| Feature | Status |
|---|---|
| Daily join roll: re-seeded once per game day, no reroll on click (R0.3) | `not verified` (12.18, QA C8) |
| Stress derived from hunger, wounds and tiredness (never stored), plus the saved, fading beating stress (cage-beating-stress, 12.32) | `not verified` (12.8) |
| Readout card over the selected NPC: route, stress bar, chance, defeats, stomach, one blocking line, hours left | `not verified` (12.8, 12.11, 12.16) |
| `EvaluatePeek`: the card asks the same decision with no side effects; a rescue's peek rolls from the existing id, so card and menu agree | `not verified` (12.8 U17, §6 R6) |
| Card pinned beside the offer on Hablar; dismissed by a click elsewhere | `not verified` (12.9, 12.17) |
| Card and head notices hide while a Kenshi dialogue or modal message is open | `not verified` (12.27) |
| Card duel line refreshes after every bout ("Duelos: W-L, si ganas X% se une") | `play-tested informally` (12.31 F1) |

### 3. Prices, haggle and money (5)
| Feature | Status |
|---|---|
| Calibrated price curve by the recruit's level; talent, uniqueness and personality capped at `premiumMaxMultiplier`; `catsHardCap` | `not verified` (12.4 P1-P9) |
| All-or-nothing charge with a refund if the engine refuses the recruit | `not verified` (12.4) |
| Haggle slider: half the ask to the ask, live odds, one try per day, offence markup (+20 %) | `not verified` (12.18 H1-H12) |
| Tribute (pay to be let go) and the rescue gift through one money convention | `not verified` (§6, 12.4) |
| NPC level = TotalSparring's overall skill level (feeds price, tiers, chance, haggle, helper lead, taming) | `play-tested informally` (12.31 F3) |

### 4. Popup, Hablar button and UI (7)
| Feature | Status |
|---|---|
| One styled recruit popup: portrait, route, price, purse, odds; red refusals with the way out | `not verified` (12.15) |
| Every refusal names its reason, keyed in `messages.json` (`reason` block) | `not verified` (12.7 Z1-Z4) |
| Hablar button in the first free slot, 160 x 44, same style as the sibling mods | `not verified` (12.10) |
| Button block stays outside both native columns when a character in a building opens two (prisoner in a cage) | `not verified` (12.10 B8-B9) |
| Hablar fires on right-button release (hold, slide, release) | `not verified` (12.12) |
| Talk cursor over NPCs worth approaching | `not verified` |
| Spawn panel (Shift+K) | `not verified` |

### 5. Cage, hold clock and cage commands (5)
| Feature | Status |
|---|---|
| Hold clock: starts when caged, dropped on leaving, resets if the game clock goes back | `not verified` (12.3) |
| Captives call out when ready (`hail.cpp`), on the game clock | `not verified` (§10 G7, 12.23 N1) |
| Golpear and Noquear: Golpear lands one direct capped blow (prisoner stays caged, never killed), Noquear is the native stealth knockout order; a knockout either causes is recognised even when the engine names no winner, and since cage-beating-stress it is beating stress, not a defeat | `not verified` (12.19) |
| Noquear acts once: the watch stays until the prisoner is down, then the actor is let go (goal removed, stealth off if the order turned it on); stray knockout goals from older saves are healed | `not verified` (12.19 C10-C10e) |
| Noquear's cleanup keeps the player's other orders (the order is dropped only when it is ours and alone in the queue); the sweep removes goals only, only for a prisoner in the player's cage; an abandoned order ends the watch clean | `not verified` (12.19 C10e-C10g) |
| Noquear's watch clock stops while the game is paused (no fallback, no land-window timeout during a pause) | `not verified` (12.19 C10h) |
| Noquear's watch keeps character keys and re-resolves them each tick; a vanished actor drops the watch with no cleanup | `not verified` (12.19 C10i) |
| R2.3 interval between beatings: Golpear rests `submissionBeatingCooldownHours` (1 h) per prisoner after a blow, Noquear `submissionKnockoutCooldownHours` (1 h) after a knockout that landed; both show the wait in the block and the popup; every click re-reads the facts | `not verified` (12.19 C3c-C3g, 12.13 K6) |
| Beating stress (cage-beating-stress): each blow and each Noquear knockout adds `clamp(base x levels / resolve, 0.02, 0.25)`, saved with the character, fading over 24 game hours; the fourth stress term (card "Golpes"); cage-command knockouts are no longer defeats | `not verified` (12.32) |
| Golpear never raises health: a part already at or below the floor gets no blow and is not touched | `not verified` (12.19 C3h) |
| A blow's knockout credit lasts 5 s around the hit (only for a blow that landed); Noquear keeps 2 min | `not verified` (12.19 C5, C5b) |
| Caged popup offers GOLPEAR / NOQUEAR, never IGNORAR / PERDONAR | `not verified` (12.22) |

### 6. Defeats, mercy and the submission event (4)
| Feature | Status |
|---|---|
| Plea for life at low blood; dead and unconscious NPCs never plead | `verified in game` (R3.1, §3.3) |
| Mercy window: ACEPTAR joins free, IGNORAR (tribute for those who can buy their life), RECHAZAR | `not verified` (12.2, 12.1 F7-F8) |
| Submission event: a beaten NPC walks up, kneels and asks (Aceptar / Perdonar / Rechazar, saved reject cooldown) | `not verified` (12.21) |
| Personality decides how much a fighter takes before yielding (R3.5) | `not verified` (§3.4) |

### 7. Animals (3)
| Feature | Status |
|---|---|
| Tamed by the hunger bar, scaled by maturity, level and temperament (R5) | `verified in game` (R5.1, earlier rule) |
| Only animals the player fed at least once can be tamed (meals saved as `total_recruit.fed_by_player`) | `not verified` (12.20) |
| Owned and domestic animals are kept out | `verified in game` (§7) |

### 8. Allies by level, temporary members and helpers (7)
| Feature | Status |
|---|---|
| Ally tiers in Balanced by level and personality group; friendship discount up to 20 % (10 % from 50) (R4b.1-R4b.2) | `not verified` (12.24) |
| Temporary member (80+, close-level leader): 3-10 days, card counts the days, saved | `play-tested informally` (12.25) |
| Mistreatment strikes (hunger, attack, knockout), warning, leave at three | `not verified` (12.25 T3-T5) |
| Daily "Me quedan N dias" info notice (`tempMemberDailyNotice`) | `not verified`, deployed, awaiting play-test (12.25 T12-T14) |
| Leaving member gets the native wander package (5006 squad template -> `2675` "Wandering from town to town"); fallback walks back to the join spot | `play-tested informally` (12.30 N6-N7) |
| Free helper offer: allied, stronger, good or neutral, 15 % per day, walk-up, popup, F8 debug key | `play-tested informally` (12.26 H1-H3) |
| Helper escort: native Bodyguard contract (`5090`) toward the leader; code-driven fallback B | `play-tested informally` (12.30 N1-N5) |

### 9. Duel challenge through TotalSparring (5)
| Feature | Status |
|---|---|
| RETAR A DUELO for 80+ allies (Balanced) through TotalSparring's API; DUELOS head-to-head row | `play-tested informally` (12.28) |
| Loss or draw: 24 h saved cooldown; spars and undecided bouts never count | `play-tested informally` (12.28 T9-T12) |
| Won duel = join chance: 25 % + 20 % per earlier win, seeded roll, 24-72 h / 24-120 h win cooldown | `not verified`, deployed, awaiting play-test (12.29) |
| Cheat: the duel is offered to any 80+ non-animal, and a win always recruits | `not verified`, deployed, awaiting play-test (12.29 J7-J8) |
| Duel head notices are informational (25 s) | `play-tested informally` (12.31 F2) |

### 10. Head notices (3)
| Feature | Status |
|---|---|
| Name and line over the head of whoever wants to talk (captive, submission, helper offer) | `play-tested informally` (12.23) |
| Prompt notices 90 s (`headNoticeSeconds`), info notices 25 s (`headNoticeInfoSeconds`), frozen in pause | `not verified` (12.23 N8, N14-N16) |
| Squad notices for temporary members (not dismissed by selecting them) | `play-tested informally` (12.25 T1) |

### 11. Config, Mod Hub, messages and deploy (6)
| Feature | Status |
|---|---|
| Every setting in Emkej's Mod Hub, two presets (Cheat, Balanced; Vanilla+ retired in v1, a saved `preset` 2 loads as Balanced) | `verified in game` (R6.2); preset range 0..1 `needs in-game QA` (QA P1) |
| Config writer merges into `config.json` (hand edits, comments and unknown keys kept), atomic write | `not verified` (12.5) |
| Every player-facing line in `messages.json`; a missing key announces itself | `not verified` (§1 S4) |
| `scripts/deploy.sh`: never-overwrite key merge, `--set-if` default migration, dry run, refuses while Kenshi runs | `play-tested informally` (used for every deploy; 12.23 N17 not run) |
| `probeOnClick` diagnostics to `RE_Kenshi_log.txt` | `verified in game` |
| Optional FCS dialogue bridge (`totalrecruit accept`) | `not verified`, never run |

### 12. Persistence, identity and build (4)
| Feature | Status |
|---|---|
| Stable character id saved as `total_recruit.character_id`; session state reset on load | `not verified` (12.6 I1-I10, highest risk) |
| Per-character saved ints and strings (defeats, cooldowns, temp expiry, join spot, meals) | `not verified` (12.6, 12.25 T8, 12.28 T10) |
| Builds against vendored KenshiLib 0.5.0 (`deps/kenshilib-0.5.0/`), every import present in the installed DLL | `play-tested informally` (loads and runs) |
| `tests/rules_tests`: engine-free rules harness (44 suites) | runs green on every build |

## Missing / pending

### In-game QA owed
`QA.md` has no pass/fail markers. In priority order:
1. **12.29 and 12.25 T12-T14**: duel join chance and the daily notice (deployed, awaiting play-test).
2. **12.6 I4/I5**: stable character id across a reload and a relaunch. Highest risk: the `Character` hooks are unverified.
3. **12.4 money (P1-P9)** and **12.18 haggle (H1-H12)**; balance re-check after the level change (see risks).
4. **12.23-12.31** formally: head notices (N8, N14-N17), ally tiers, temporary members, helper, card hidden in dialogue, duel bridge, native packages, play-test fixes. Several were play-tested informally.
5. **prisoner-module-fixes** (branch `fix/cage-beat-direct-hit`): 12.19 C10-C10e (Noquear acts once, stuck goals healed), 12.13 K4b (one knockout, one defeat), §6 R6 (card and menu agree on a rescue), 12.10 B8-B9 (block outside both native columns).
6. **The rest:** §1 smoke S1-S4; 12.1-12.3, 12.5, 12.7-12.17; 12.19-12.22; §9 R6.3 (needs the Mod Hub slot dump).

### Risks and known limitations
- **Balance.** The NPC level is now TotalSparring's overall skill level, so prices, tiers, chance, haggle, helper lead and animal taming all moved. Needs a re-check in play.
- **Native helper.** `packageData` (0x68) and `missionEmployer` (0x48) are read at header offsets, because `AI/AIPackage.h` and `AI/Blackboard.h` cannot be included together. A helper taken out of a squad stays in its own platoon. Fallback B leaves the aggressive combat stance on the NPC.
- **Duel.** A winner more than 80 m away is not recruited. The FCS dialogue accept path offers no duel.
- **Temporary members.** The FCS accept path recruits a temporary ally permanently. Warnings, cooldowns and the daily notice can repeat once after a load. The leaver's walk-back fallback is session-only.
- **State not saved.** Talk attempts live in memory. A prisoner re-caged outside the 900-unit sweep keeps its clock. (Resolved: the hold clock start now rides in the character save, `total_recruit.hold_start_hour`, v1-presets-close P3; `needs in-game QA` 12.33 H1-H4.)
- **Cage orders.** Whether `checkPlayerOrderForProblems` reads the right way round is unproven (the 1.5 s order watch decides the fallback).
- **Minor.** The popup's "Sometido" tag is not in `messages.json`. There is an include loop between `src/offer_popup.cpp` and `src/mercy.cpp`. `deploy.sh` prints "would be copied" on a real run. "THE Recruit Anyone" overlaps with this mod if it is installed.

## Ideas (not implemented)
- **Mod Hub (R6.3/R6.4).** Sections, a preset dropdown and text settings. Blocked on the slot layout dump.
- **Level curve (R2.1).** Tune the -6 per level curve in play.
- **More config.** Make the haggle personality shifts and the reputation tiers (75/25/5/-5, `src/relation_rules.cpp`) configurable.
- **Save state.** Persist the hold clock and talk attempts in the save.
- **Unique characters.** Per-name rules (`UNIVERSAL-RECRUIT-BRAINSTORM.md` §4.5).
- **Layered config.** A layered INI-style config (brainstorm §8).
- **Messages.** Wire the `refusal.*` and `currency.*` sections of `messages.json` (partly stale).

## Housekeeping
- Done 2026-10-04: merged branches deleted; `feat/native-ai-packages` fast-forwarded into `main`; TotalSparring `feat/public-api` merged and its worktree removed; REQUISITOS duel requirement renumbered R4b.7 and R4b.3 / R4b.6 updated for the native packages; QA 12.26 rewritten for the native Bodyguard contract; `TODO.md` and `ESTADO.md` point here; feature documents record their Engram mirrors and ship commits.
- Still open: `COMPARATIVA.md` §3.1/§3.2 and `PLAN.md` §8 are stale (marked historical in ESTADO §7).

## Suggested order
1. Play-test the duel join chance and the daily notice (12.29, 12.25 T12-T14).
2. Formal in-game QA in the order above, starting with 12.6.
3. Balance pass after the level change.
4. Ideas, when the user asks.
