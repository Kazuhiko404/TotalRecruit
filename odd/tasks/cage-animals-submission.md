# Feature: cage-animals-submission

Locator: `odd/tasks/cage-animals-submission.md` · Engram mirror: `odd/cage-animals-submission/tasks` (project `totalrecruit`)
Branch: `feat/cage-animals-submission` (from `feat/haggle-and-daily-roll` @ a0a0b53; haggle is not deployed yet because Kenshi was running)

## Objective
Three requests from the user on 2026-10-03:
1. Caged prisoners get **Golpear** and **Noquear** commands next to Hablar. Both use Kenshi's native attack and knockout, so the result depends on the character and the mod computes nothing.
2. Animals must not join unless the player fed them.
3. A submitted NPC walks up to the player and asks to join, with three choices: **Aceptar / Perdonar / Rechazar**. Hostile NPCs are made temporarily neutral.

## Problem / why
**1. No beat command for caged prisoners.**
- The context menu has only Hablar (`src/recruit_overlay.cpp:238`).
- "GOLPEARLO" exists only as the popup's remedy on a refusal (`src/offer_view.cpp:332`). A prisoner willing to join can never be beaten.
- `BeatPrisoner` uses `knockoutForceTimer(60s)`. It does no damage and ignores the attacker, so stress does not change.

**2. Animals join unfed.**
- `DecideAnimal` only compares current fullness with the bar (`src/route_animal.cpp:59`). Wild animals sit at full hunger, so they pass.
- Caged animals hit easy-wait and the intimidation roll before the animal branch (`src/route_cage.cpp:139`, `:152-183`).
- Rescue and the saviour shortcut skip food altogether.
- Nothing records who fed them (`src/feeding.cpp`).
- Log evidence: Cabra and Goat Alpha were never fed and still read "CAN JOIN".

**3. Submitted NPCs.**
- Today the player has to find the NPC and right-click it.
- The user wants an NPC-initiated event. Reference: TotalSparring's event system (user: "ya resolvimos los eventos").

## Design
**Cage commands**
- When the target is in the player's cage and not down, the button block becomes `[Hablar, Golpear, Noquear]`, placed by the existing `PlaceButtonBlock`.
- The actor is the selected player character. If none is selected, show a message.
- **Golpear:** a native `BEAT_CAGE_OCCUPANT` order. Fallback: the TotalSparring `FOCUSED_MELEE_ATTACK` triple (`fight_starter.cpp:179-184`).
- **Noquear:** a native `STEALTH_KNOCKOUT` order, with the label showing the native odds from `getPlayerTaskProbability`. Fallback: `MedicalSystem::knockout(actor assassination skill)`.
- Credit and stress follow from the existing knockout hook, the episode rule and real wounds. A credit bridge is used only if a KO the mod ordered does not name the player.
- `BeatPrisoner`'s forced timer is retired.

**Animals**
- A per-character "fed by the player" amount is persisted in the character save, like defeats.
- `DecideAnimal` requires `fedByPlayer > 0 && fullness >= needed`.
- Caged animals go straight to `DecideAnimal`. Rescue, EasyHealthy and the saviour shortcut skip animals.
- Food can be offered to an animal that has not been fed by the player, even when it is full.
- The text is truthful: "no ha comido de tu mano".

**Submission event** (port of TotalSparring's NPC challenge, `src/duel/challenge/`)
- States `Idle -> Approaching -> Holding (popup) -> Forgiven (fleeing, calm) -> Done`, one event at a time, on a paused-aware real-seconds clock.
- A sweep every 2 s finds a nearby NPC that newly yields (field-yield or surrender) with a peek, and draws an open field-yield roll once with the key.
- Calm is per character (end combat, clear temp-enemy marks toward the squad, re-asserted every second), never a faction change.
- The NPC walks to the squad leader (repath 1 s, arrive at 3 m, timeout = `submissionApproachSeconds`) and opens the popup: ACEPTAR / PERDONAR / RECHAZAR.
- Accept recruits. Forgive runs 80 m away, calm for `submissionForgiveSeconds`, then restores. Reject wipes defeats, starts a saved reject cooldown (`submissionRejectCooldownHours`) that blocks every route, and marks the leader as an enemy again. The X is no answer: calm in place for the forgive window.

## Constraints
- Isolated SOLID modules, no patch-on-patch.
- Native engine behaviour over own arithmetic.
- Reuse proven sibling code.
- Toolset v100.
- Only add keys to `config.json` / `messages.json`.
- Mockup first for new UI.
- Artifacts in English.

## Tasks
- [x] **T1 Cage commands Golpear / Noquear (native).**
  - Route: delegated (writer trigger: overlay, new engine module, defeats, recruit_core, docs, mockup).
  - Evidence: `dea7589` -- rules_tests 30/30 suites, 0/61383 failed (SkillFraction RED observed first); TotalRecruit.dll builds with no new warnings in touched files. In-game QA pending (`QA.md` 12.19).
- [x] **T2 Animals need the player's food.**
  - Route: delegated (writer trigger: routes, ledger, identity bridge, facts, feeding, docs).
  - Evidence: `a13f51e` -- RED observed (18 checks in route_rules / route_decisions), then rules_tests 31/31 suites, 0/61418 failed; TotalRecruit.dll builds with no new warnings. In-game QA pending (`QA.md` 12.20).
- [x] **T3 Submitted-NPC join event.** Port TotalSparring's event system.
  - Route: delegated (writer trigger: new pure rules + tracker, engine director, motion, pacify, popup dispatch, config, docs, mockup).
  - Evidence: `43f16f0` -- RED observed (28/33 suites, 106 failing checks in submission_rules / submission_tracker / offer_view / reason and readout catalogs), then rules_tests 33/33 suites, 0/61580 failed; TotalRecruit.dll builds with no new warnings. In-game QA pending (`QA.md` 12.21).
- [x] **T4 Docs + deploy (with haggle).** Docs shipped with each feature commit (README, DEVELOPING, QA 12.19-12.22: `dea7589`, `a13f51e`, `43f16f0`, `c5ba3f8`); DLL shipped in `4b9cb9e build: ship the cage, animals, submission and head notice DLL` (it also carries the haggle commits). In-game QA still owed (QA.md 12.19-12.22).

## Progress / evidence
- **T1** (`dea7589`). Kept from the stopped writer: `cage_command_rules.{h,cpp}` and its tests (sound; all passed), the `cage_commands.h` shape (reworked: returns `CageCommandResult` so it never calls `TellPlayer`, which would ring with `recruit_core`). Added `SkillFraction` (RED first), `cage_commands.cpp`, the 3-button overlay with one gesture per button, the credit bridge in the knockout hook, `TickCageCommands` in `frame_tick`, `ForgetCageCommands` at load. Retired `BeatPrisoner`, `CreditDefeatToPlayer`, `knockoutForceTimer` and the `submissionBeatingKnockoutSeconds` knob (config + Mod Hub; the user's config.json is untouched). GOLPEARLO issues the native beat.
  - `checkPlayerOrderForProblems` is logged, not trusted: its polarity is unproven (the RVA in the headers did not disassemble cleanly against the installed exe), and reading it backwards would send every order to the fallback. The 1.5 s order watch decides the fallback.
  - Beat fallback drops TotalSparring's `initCombatMode`: `combat/CombatClass.h` includes `"Enums.h"` relative to the kenshi folder (not on this include path) and the method is virtual.
  - `Tasker.h` and `AI/AITaskSystem.h` both define `taskPriority`: the current-task read moved to `engine_reads.cpp` (`SafeCurrentTask`).
- **T2** (`a13f51e`). `saved_ints.{h,cpp}`: one table of per-character idata ints (key + `saved(id)` + `restore(id, value)`); `character_identity` loops over it instead of the defeat-only bridge. `feeding_ledger.{h,cpp}`: meals per CharKey (unit: one per successful `FeedCaptive` on an animal), saved as `total_recruit.fed_by_player`. `DecideAnimal` joins only with `fedByPlayer > 0` and fullness at the bar; new `Reason_TameNotFed`. Caged animals go to `DecideAnimal` first in `DecideCage`; `CareVerdict` lost its animal branches (`Reason_AnimalWary`, `Reason_CaredForAnimal` removed); `ChooseEntryRoute` never gives an animal rescue or easy-healthy.

- **T3** (`43f16f0`). Pure: `approach_rules` (TS `ApproachStep`; a busy window waits instead of aborting), `submission_rules` (clock, candidate screen, peek, ask-once-per-beating, calm strikes, hold, forgiven, restore plans, flee point, reject cooldown and its one route predicate), `submission_tracker` (asked / failed roll today / cooldown, saved as `total_recruit.reject_cooldown_until`). Engine: `submission_director` (sweep, walk-up, hold, answers, load reset), `npc_motion` (TS `IssueMoveGuarded`, `StopWalkingGuarded`, stance, `ChallengeRun`), `pacify` calm / hostility, `defeats::ClearDefeats`, `offer_popup` single `Answer` dispatch (X = no decision for a submission). `Reason_RejectCooldown` is checked once at the top of `rules.cpp` Run (cheat included) and in `OfferMercy`.
  - Surface note: the new reason key needed one row in `src/readout_catalog.cpp` (outside the listed surfaces; its test requires one row per ReasonKey).
  - Choices: approach walks at the NPC's own pace (only the forgive flight runs); while the forgive window runs no other NPC starts an event.
- **Fix: caged popup** (user report, caged defiant Holy Warden got only CERRAR). One rule in `offer_view` `ApplyCageCommands`: refusal + no slider + `CageCommandsAllowed` (the context menu's predicate) -> GOLPEAR / NOQUEAR NN%; beating off -> defiant reason without "hierelo". Remedy is food only (cooldown-gated GOLPEARLO retired; `CanBeatPrisoner` now has no caller). `inCage` (SafeInPrison) hides IGNORAR / PERDONAR. Popup wraps CERRAR to a second row past 3 buttons. Evidence: RED 10/61619 failing in offer_view, then rules_tests 33/33 suites, 0/61619 failed; TotalRecruit.dll builds with no warnings in touched files. In-game QA pending (`QA.md` 12.22).

## Next step
Shipped (`4b9cb9e`). In-game QA of `QA.md` 12.19, 12.20, 12.21 and 12.22 is still owed.
