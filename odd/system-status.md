# TotalRecruit: system status, 2026-10-06

The single current snapshot of the mod: what it does today, how far each feature got, what is still missing, the risks and the ideas not built yet. Every claim was re-checked against `src/` on 2026-10-06. Built from the feature documents in `odd/tasks/`, `REQUISITOS.md`, `QA.md`, `DEVELOPING.md`, `README.md`, `ESTADO.md` and `TODO.md`. There is no TODO, FIXME or HACK in `src/`.

Engram project: `totalrecruit` (`.engram/config.json`). This document is mirrored under the topic `odd/system-status`; each feature document under `odd/<feature>/tasks`.

## Where things stand
- **`main` is the only branch** (local). Every branch was merged and deleted on 2026-10-06: `fix/beat-feedback`, `feat/priority-one` and `feat/balanced-tuning` (merge `b87d0b2`). The packaged DLL of that build is committed in `3fb81b2` and deployed. The balance sheet was fixed and regenerated afterwards (`b8ca98c`).
- **The user plays the Cheat preset.** Balanced work (balanced-tuning) is lower priority for that reason.
- **Last play-test (2026-10-06, Cheat):** Golpear's walk to the cage, the roller message and the notification work. There was no sound and no animation; both were addressed in priority-one (S1 real Wwise events, C punch clip) and are deployed but **not re-tested**.
- **TotalSparring:** the duel bridge talks to its public API v1 (`TS_GetApi`, on TotalSparring `master`). TotalSparring `master` `8782b2c` hides the wager row against an animal, so the old "keep the bet at 0" limitation is gone.
- **Not pushed.** `origin/main` is an unrelated squash commit (`6257bd6`), so a plain push is rejected. Delivery is the user's call.

## Progress by feature document
Every document in `odd/tasks/` is implemented and on `main`. "QA" is the `QA.md` section to run. No section has a formal pass mark yet; "informal" means the user played it and said it works.

| Feature document | Shipped (code / DLL) | QA | QA status |
|---|---|---|---|
| `rebalance-and-ui` | `38c7643` | 12.1-12.8 | not run |
| `price-readout-button-tweaks` | `f21474e` | 12.9-12.12 | not run |
| `recruit-gating-and-popup` | `0c9f671` | 12.13-12.17 | not run |
| `haggle-and-daily-roll` | `3bf2e03` (DLL `4b9cb9e`) | 12.18 | not run |
| `cage-animals-submission` | `4b9cb9e` | 12.19-12.23 | head notices informal (12.23) |
| `ally-tiers-temp-helpers` | `ac17ed7` (card hidden in dialogue: `c6a3f2a`, 12.27) | 12.24-12.27 | temp member and helper informal |
| `head-notice-lifetime` | `eac73f6` | 12.23 N8, N14-N16 | not run |
| `ts-duel-bridge` | `ac6ed10` / `fe8e490` | 12.28 | informal; T3 is the only unchecked task box in `odd/tasks` (`ts-duel-bridge.md:25`) |
| `native-ai-packages` | `ac6ed10` / `fe8e490` | 12.30, 12.31 | informal (helper escort, leaver wander, play-test fixes) |
| `duel-join-chance` | `3554d19` | 12.29, 12.25 T12-T14 | not run |
| `cage-beat-direct-hit` | `0f042dc` (in `7bf8750`) | 12.19 | superseded in play by beat-feedback / priority-one |
| `prisoner-module-fixes` | up to `ddce78e` (in `7bf8750`) | 12.19 C10-C10i, 12.13 K4b, §6 R6, 12.10 B8-B9 | not run |
| `cage-beating-stress` | up to `dab307c` (in `7bf8750`) | 12.32 | not run |
| `v1-presets-close` | `7bf8750` | 12.33, §8 H7-H10 | not run |
| `animal-insistence-cheat-feed` | `7767860`, `c15675c` (DLL `2e8514d`) | 12.7 N1, §8 H11-H14 | not run |
| `animal-duel-submission` | `fe90e80`..`588491c` (DLL `e6810fb`) | 12.34 | not run |
| `cheat-any-cage` | `a2dacdd` | 12.35 | not run |
| (prisoner hold time, no document) | `a256a76` | 12.36 | not run |
| `proactive-offer` | `5e93243` (DLL `ae94075`) | 12.37 | not run |
| `duel-any-source` | `b6a3133`..`1bb0de4` (DLL `3a42e53`) | 12.38 | not run |
| `beat-feedback` | `320983c` (DLL `6b6ce58`) | 12.39 | informal: walk, message, notification OK; sound and animation missing (moved to priority-one) |
| `priority-one` | `f7b719d`, `fc754c7`, `9b0466a`, `20069d8`, `65c6652`, `fb536ff` (DLL `3fb81b2`) | 12.40 | deployed, not run |
| `balanced-tuning` | tool `af1ff35`, merge `b87d0b2`, sheet `b8ca98c` | (tuning plan) | T5 done by priority-one B; T7 mostly done; T1-T4 and T6 open |
| `next-session-handoff` | (historical handoff, 2026-10-03) | - | - |

## Features
Status tags:
- `verified in game`: seen working in a real game with the log line that proves it (ESTADO "Jugado"). It predates the later rounds and is not a formal QA pass.
- `play-tested informally`: the user played it and reported it working. No QA row was marked.
- `not verified`: built and unit-tested (`tests/rules_tests`, 63 suites), not yet seen in game. The QA.md section to run is given.

### 1. Recruitment routes and gating
| Feature | Status |
|---|---|
| Works on any `Character`, including NPCs from other mods (no per-NPC dialogue package) | `verified in game` |
| One chance engine: relation, level gap, personality, defeats, stress and situation as separate terms, logged as `[chance]` (R0.1, R2.1) | `verified in game` (§2) |
| Route order: unbeatable grudge before field yield; cage defeats feed the chance (`[route] <name>`) | `not verified` (12.1) |
| Captivity: needs by who they are (food and healing, cats, freedom, beating then time) | `not verified` (§2, 12.4) |
| Balanced cage without shortcuts: the easy wait (`easyHoursCaged`) and "healthy thanks to you" were removed (v1-presets-close P4); a cared-for prisoner gets a stable daily bonus on the roll (5..20, `submissionCaredForBonusMin/Max`) | `not verified` (12.33 C1-C6, 12.3 E3, 12.8 U3) |
| Insistence cooldown, cumulative and configurable (R2.4); only a real refusal counts, never a state, an instruction or a yes; animals have no insistence (`insistence_rules`) | `not verified` (§5 I1, I6-I9, 12.7 N1) |
| Rescue: `LT_MY_LIFESAVER` debt, offer on waking, parting gift paid once (R4) | `not verified` (§6) |
| Grudge refusal by character (who cannot forgive what they saw) | `not verified` (12.7) |
| Field yield ("Sometido"): beaten in the field; respects other cages, lockout and loyalty | `not verified` (12.14) |
| Player defeats saved in the character save, one per knockout (`total_recruit.player_defeats`) | `not verified` (12.13 K4b) |
| Cheat preset: a cage plus a one-minute hold, nothing else (`cheatNeedsCage`) | `verified in game` (§8, R1.3) |
| Cheat: any cage counts (`cheatAnyCage`, default on): another faction's cage, shackles or a kidnap; the hold clock runs for them; cage commands stay on the player's cages. Outside a cage the answer says "any cage" (`Reason_CheatNeedsAnyCage`) | `not verified` (12.35, 12.38 D14-D15) |
| Cheat: a free animal joins once it has eaten from the player's hand, caged or not (`CheatFacts::fedByPlayer`) | `not verified` (§8 H11-H14) |
| Cheat: a free animal also joins when the player's squad healed it (`healing_ledger`, `total_recruit.healed_by_player`) or knocked it out once | `not verified` (12.34 A15-A20) |
| Cheat: every wait is at most one game minute (`cheat_waits`, `kCheatWaitHours` = 1/60 h). The reject and duel cooldowns are now stored to the game minute (`cooldown_minutes`, saved as `..._until_min` next to the old hour rows), and waits under an hour read in minutes ("Golpear (1 min)", `wait_format`) (priority-one B) | `not verified` (12.40 B1-B7, §8 H7-H10, 12.33 W1-W2) |

### 2. Chance, stress and the readout card
| Feature | Status |
|---|---|
| Daily join roll: re-seeded once per game day, no reroll on click (R0.3) | `not verified` (12.18, QA C8) |
| Stress from hunger, wounds and tiredness (never stored), plus the saved, fading beating stress (`total_recruit.beating_stress`) | `not verified` (12.8, 12.32) |
| Readout card over the selected NPC: route, stress bar, chance, defeats, stomach, one blocking line, hours left, hours held (12.36) | `not verified` (12.8, 12.11, 12.16, 12.36) |
| `EvaluatePeek`: the card asks the same decision with no side effects | `not verified` (12.8 U17, §6 R6) |
| Card pinned beside the offer on Hablar; dismissed by a click elsewhere | `not verified` (12.9, 12.17) |
| Card and head notices hide while a Kenshi dialogue or modal message is open | `not verified` (12.27) |
| Card duel line refreshes after every bout ("Duelos: W-L, si ganas X% se une") | `play-tested informally` (12.31 F1) |

### 3. Prices, haggle and money
| Feature | Status |
|---|---|
| Price curve by the recruit's level; talent, uniqueness and personality capped (`premiumMaxMultiplier`); `catsHardCap` | `not verified` (12.4 P1-P9) |
| All-or-nothing charge with a refund if the engine refuses the recruit | `not verified` (12.4) |
| Haggle slider: half the ask to the ask, live odds, one try per day, offence markup | `not verified` (12.18 H1-H12) |
| Tribute and the rescue gift through one money convention | `not verified` (§6, 12.4) |
| NPC level = TotalSparring's overall skill level (feeds price, tiers, chance, haggle, helper lead, taming) | `play-tested informally` (12.31 F3) |

### 4. Popup, Hablar button, speech and UI
| Feature | Status |
|---|---|
| One styled recruit popup: portrait, route, price, purse, odds; red refusals with the way out | `not verified` (12.15) |
| Every refusal names its reason, keyed in `messages.json` (`reason` block) | `not verified` (12.7 Z1-Z4) |
| The NPC's opening line is read from `messages.json` `refusal.*` / `currency.*` (`opening_line_rules`; built-in text as fallback, byte for byte the old lines) (priority-one E) | `not verified` (12.40 E1-E5) |
| Hablar button in the first free slot, same style as the sibling mods; block outside both native columns for a caged prisoner | `not verified` (12.10, B8-B9) |
| Hablar fires on right-button release | `not verified` (12.12) |
| Talk cursor over NPCs worth approaching; spawn panel (Shift+K) | `not verified` |

### 5. Cage, hold clock and cage commands
| Feature | Status |
|---|---|
| Hold clock: starts when caged, dropped on leaving, saved in the character save (`total_recruit.hold_start_hour`) | `not verified` (12.3, 12.33 H1-H4) |
| Captives call out when ready (`hail.cpp`), on the game clock | `not verified` (§10 G7, 12.23 N1) |
| **Golpear**: the selected character walks to the cage (`beat_walk`, reach `submissionBeatReachMeters` 4 m, gives up after `submissionBeatWalkSeconds` 30 s or 4 s without progress), stops and faces the prisoner, plays the punch clip `badpunch` (`beat_strike`, 0.9 s, the blow lands after a 0.35 s wind-up; a clip the engine does not report lands at once), then one direct capped blow (prisoner stays caged, never killed, never healed). The prisoner complains aloud (not an animal, not when unconscious), the roller says "{actor} golpea a {name}" with the notification sound, and the hit sound is posted through the game's Wwise (`wwise_bridge`: `Impact` with the `Impacts` switch, plus `VO_Get_Hit` or `VO_Creature_Take_Damage`) | walk, message and notification `play-tested informally`; sound and punch `not verified` (12.39, 12.40 S1-S5, C1-C7) |
| **Noquear**: the native stealth knockout order; acts once, then the actor is let go (goal removed, stealth off if the order turned it on); keeps the player's other orders; watch clock frozen in pause | `not verified` (12.19 C10-C10i) |
| Rests: Golpear `submissionBeatingCooldownHours` (1 h) after a blow, Noquear `submissionKnockoutCooldownHours` (1 h) after a knockout that landed; the wait shows on the buttons and in the popup (minutes under an hour) | `not verified` (12.19 C3c-C3g, 12.40 B) |
| Beating stress: each blow and each Noquear knockout adds `clamp(base x levels / resolve, 0.02, 0.25)`, saved, fading over 24 game hours; cage-command knockouts are not defeats | `not verified` (12.32) |
| Caged popup offers GOLPEAR / NOQUEAR, never IGNORAR | `not verified` (12.22) |

### 6. Defeats, mercy and the submission event
| Feature | Status |
|---|---|
| Plea for life at low blood; dead and unconscious NPCs never plead | `verified in game` (R3.1) |
| Mercy window: ACEPTAR joins free, IGNORAR (tribute for those who can buy their life), RECHAZAR | `not verified` (12.2, 12.1 F7-F8) |
| Submission event: a beaten NPC walks up, kneels and asks; Rechazar does nothing (no defeats wiped, no reject cooldown) | `not verified` (12.21, 12.37 O7-O9) |
| Proactive offer: a won duel or a heal by the squad asks for an offer; the NPC or animal walks up and opens the window; a bad roll calms a hostile one for `offerFailCalmSeconds`; `offerOnDuelWin` / `offerOnHeal` | `not verified` (12.37) |
| Animals in the submission event (Cheat: beaten once; Balanced: twice); narrator lines, no head notice | `not verified` (12.37 O6, O16) |

### 7. Animals
| Feature | Status |
|---|---|
| Tamed by the hunger bar, scaled by maturity, level and temperament (R5) | `verified in game` (R5.1, earlier rule) |
| Only animals the player fed at least once can be tamed (`total_recruit.fed_by_player`) | `not verified` (12.20) |
| Owned and domestic animals are kept out | `verified in game` (§7) |
| Balanced: a free animal beaten twice in the open takes the field-yield roll; card shows DERROTAS n/2 | `not verified` (12.34 A9-A14) |
| Balanced: beating stress lowers a **caged** animal's stomach bar: `bar x (1 - min(0.9, weight x stress / resolve))`, resolve = `animalCageBeatingResolveBase` (1) + level x `animalCageBeatingResolvePerLevel` (0.02); the first meal is still required (`animal_beating_rules`, `DecideCagedAnimal`, priority-one D; numbers in `odd/balance/balance-sheet.md` §6) | `not verified` (12.40 D1-D5) |

### 8. Allies by level, temporary members and helpers
| Feature | Status |
|---|---|
| Ally tiers in Balanced by level and personality group; friendship discount | `not verified` (12.24) |
| Temporary member (80+): 3-10 days, card counts the days, saved | `play-tested informally` (12.25) |
| Mistreatment strikes, warning, leave at three | `not verified` (12.25 T3-T5) |
| Daily "Me quedan N dias" info notice (`tempMemberDailyNotice`) | `not verified` (12.25 T12-T14) |
| Leaving member gets the native wander package; fallback walks back to the join spot | `play-tested informally` (12.30 N6-N7) |
| Free helper offer (allied, stronger, good or neutral) with the native Bodyguard contract (`5090`) | `play-tested informally` (12.26, 12.30 N1-N5) |

### 9. Duel challenge through TotalSparring
| Feature | Status |
|---|---|
| RETAR A DUELO for 80+ allies (Balanced) through TotalSparring's API; DUELOS head-to-head row | `play-tested informally` (12.28) |
| Loss or draw: saved cooldown; spars and undecided bouts never count | `play-tested informally` (12.28 T9-T12) |
| Won duel = join chance (25 % + 20 % per earlier win, seeded roll, win cooldown) | `not verified` (12.29) |
| Cheat: the duel is offered to any non-animal from `duelCheatPeopleFromLevel` (1), and a win always recruits | `not verified` (12.29 J7-J8, 12.38 D1-D3) |
| A duel opened from TotalSparring's own window counts when the rival is an offerable outsider (`duelCountExternalBouts`); Balanced counts it against any recruitable person from `duelExternalBalancedFromLevel` (1) | `not verified` (12.38 D4-D10, D17-D20) |
| Minimum duel wins before a win can recruit (`duelMinWinsCheat` / `duelMinWinsBalanced`); the popup says how many are missing | `not verified` (12.38 D11-D13, D16, D19) |
| Free animals of level 40+ (`duelAnimalsFromLevel`) can be challenged in both presets; narrator lines; TotalSparring offers no wager against them | `not verified` (12.34 A1-A8) |
| A won duel's rival is looked for across the whole offer reach, 120 m (`DuelRivalSearchMeters` = `kOfferReachMeters` = `kSubmissionAbortMeters`; was 80 m) (priority-one A) | `not verified` (12.40 A1-A2) |
| Duel head notices are informational (25 s) | `play-tested informally` (12.31 F2) |

### 10. Head notices
| Feature | Status |
|---|---|
| Name and line over the head of whoever wants to talk (captive, submission, helper offer) | `play-tested informally` (12.23) |
| Prompt notices 90 s, info notices 25 s, frozen in pause | `not verified` (12.23 N8, N14-N16) |
| Squad notices for temporary members | `play-tested informally` (12.25 T1) |

### 11. Config, Mod Hub, messages and deploy
| Feature | Status |
|---|---|
| Every setting in Emkej's Mod Hub: 183 controls registered in `src/mod_hub_integration.cpp` (41 bool, 60 int, 82 float); two presets (Cheat, Balanced; a saved `preset` 2 loads as Balanced) | `verified in game` (R6.2); preset range `not verified` (12.33 P1) |
| Config writer merges into `config.json` (hand edits and unknown keys kept), atomic write | `not verified` (12.5) |
| Every player-facing line in `messages.json`; a missing key announces itself | `not verified` (§1 S4) |
| `scripts/deploy.sh`: never-overwrite key merge, `--set-if` default migration (numbers only), dry run, refuses while Kenshi runs | `play-tested informally` (used for every deploy) |
| `probeOnClick` diagnostics to `RE_Kenshi_log.txt` | `verified in game` |
| FCS dialogue bridge | deliberately disabled: `OpenConversation` returns false (`src/dialogue_bridge.cpp:235`); every caller uses the plugin's own window |

### 12. Persistence, identity, build and tools
| Feature | Status |
|---|---|
| Stable character id saved as `total_recruit.character_id`; session state reset on load | `not verified` (12.6 I1-I10, highest risk) |
| Per-character saved ints and strings: defeats, beating stress, reject / duel cooldowns (hour rows plus `_min` rows), hold start, temp expiry / home / join spot / strikes, helper expiry / reject, meals, heals | `not verified` (12.6, 12.25 T8, 12.28 T10, 12.40 B) |
| Builds against vendored KenshiLib 0.5.0 (`deps/kenshilib-0.5.0/`) | `play-tested informally` (loads and runs) |
| `tests/rules_tests`: engine-free rules harness, 63 suites | runs green on every build |
| `tests/balance_sheet`: prints the Balanced numbers from the real pure rules into `odd/balance/balance-sheet.md` (regenerated at `3fb81b2`; §5 Cheat waits in minutes, §6 caged-animal beating bar) | builds and runs |

## Missing / pending

### In-game QA owed
`QA.md` has no pass/fail markers. In priority order:
1. **12.39 + 12.40** (the Cheat build the user is playing): Golpear sound (S1-S5, look for the posted event / playing id) and punch (C1-C7, `beat anim: clip 'badpunch' found playing`), minute waits (B1-B7), caged animal beating bar (D1-D5, Balanced), editable opening lines (E1-E5), duel rival at up to 120 m (A1-A2).
2. **12.6 I4/I5**: stable character id across a reload and a relaunch. Highest risk: the `Character` hooks are unverified.
3. **12.28**: the duel bridge (the only unchecked task box, `ts-duel-bridge.md:25`).
4. **The rest**, newest first: 12.38, 12.37, 12.36, 12.35, 12.34, 12.33, 12.32, 12.29 and 12.25 T12-T14, 12.19 C10-C10i, 12.13 K4b, §6 R6, 12.10 B8-B9, 12.4 money and 12.18 haggle, 12.23-12.31 formally, then §1 smoke S1-S4 and 12.1-12.22.

### Balanced tuning (only Balanced; the user plays Cheat)
From `odd/tasks/balanced-tuning.md`, measured against `odd/balance/balance-sheet.md`:
- **T1** target table of days to join by temperament: needs the user's sign-off.
- **T2** beating dominates: diminishing returns per blow and a daily cap.
- **T3** breaking is worth far more than caring: a cared-for streak bonus.
- **T4** time in the cage counts for nothing: a resignation term.
- **T6** review with the user: the care / freedom currencies always join, the level term saturates fast, strong uniques are unreachable in the cage, field-yield odds are near-certain.
- **T5 done** (priority-one B). **T7 mostly done** (animal duel, animal field yield, Cheat heal/defeat, caged beating bar); remaining: per-species resolve (today by level).

### Later (player value, not started)
- Per-unique-character rules and prices (brainstorm §4.5).
- An animal food bag like TameBeasties.
- Mod Hub sections (R6.3): blocked on the slot layout dump (`Mod Hub slots (...)` log line).

### Dropped / cleanup (no player value)
- FCS dialogue bridge: deliberately disabled (`dialogue_bridge.cpp:235`); not worth re-enabling.
- Layered INI config; configurable reputation tiers (75/25/5/-5, `relation_rules.cpp`) and haggle personality shifts; saving talk attempts (they still live in memory: no saved row for them).
- Popup scroll; the mutual include between `offer_popup.cpp` and `mercy.cpp` (each includes the other's header).
- `scripts/deploy.sh` prints "would be copied" in its summary before a real run too.
- `scripts/add_missing_keys.py --set-if` only migrates numbers, so `submissionBeatHitSound` "Heavy_Hit" -> "HtH_Impacts" was not migrated ("Heavy_Hit" is still a real value of the `Impacts` switch).
- Dead deployed keys `easyHoursCaged` / `factionWeight`: no longer read by `src/`; the user can delete them by hand.
- Native helper reads `packageData` (0x68) and `missionEmployer` (0x48) at header offsets (`ai_packages.cpp`), because `AI/AIPackage.h` and `AI/Blackboard.h` cannot be included together.

### Risks and known limitations
- **Sound and animation unproven in game.** `PostEvent` reports a playing id, but nobody heard it yet; the punch relies on `runSlaveAnim("badpunch")`.
- **Punch fallback.** If `getAnimationPlaying_animName` does not report the clip on the first frame, the blow lands at once (the old behaviour) and the log says so once per session.
- **Temporary member via FCS** would recruit permanently; moot while the bridge is disabled.
- **"THE Recruit Anyone"** overlaps with this mod if both are installed.
- **Balance.** The NPC level is TotalSparring's overall skill level, so prices, tiers, chance, haggle, helper lead and taming moved with it; Balanced needs a pass in play (balanced-tuning).
- **Cage orders.** Whether `checkPlayerOrderForProblems` reads the right way round is unproven (`cage_commands.cpp`; the order watch decides the fallback).
- **Temporary members.** Warnings, cooldowns and the daily notice can repeat once after a load; the leaver's walk-back fallback is session-only.

## Ideas (not implemented)
- Mod Hub preset dropdown and text settings (with R6.3).
- Per-species animal resolve (balanced-tuning T7).
- Unique characters with their own rules (`UNIVERSAL-RECRUIT-BRAINSTORM.md` §4.5).
- Animal food bag.

## Housekeeping
- Done 2026-10-06: every branch merged into `main` and deleted; packaged DLL committed (`3fb81b2`); balance sheet builds against the minute cooldowns and was regenerated (`b8ca98c`); `ESTADO.md` marked historical with its stale rows fixed; `TODO.md` rewritten; `next-session-handoff.md` marked historical; QA 12.34, REQUISITOS and README no longer ask to keep the bet at 0 against an animal; REQUISITOS R2.1 states the real level curve (1.5 per level).
- Still open: `COMPARATIVA.md` §3.1/§3.2 and `PLAN.md` §8 are stale (marked historical in ESTADO §7).

## Suggested order
1. Play-test 12.39 + 12.40 in Cheat (sound, punch, minute waits, animal bar, phrases, 120 m duel).
2. 12.6 I4/I5, then 12.28, then the rest of the QA list.
3. Balanced tuning T1 (needs the user's target table), then T2-T4, T6, per-species resolve.
4. Later items when the user asks.
