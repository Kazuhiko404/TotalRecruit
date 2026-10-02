# Feature: rebalance-and-ui

Locator: `odd/tasks/rebalance-and-ui.md` · Engram mirror: `odd/rebalance-and-ui/tasks` (project `kenshilib-plugins`)
Branch: `feat/rebalance-and-ui` (from `main` @ 4a23201)

## Objective
Make TotalRecruit sit between "Recruit Prisoners" (cheat) and "Recruit Anyone" (too hard):
fix the known bugs, correct the money formula, make per-character state survive a reload,
improve code structure, and add an in-game readout of each NPC's stress and join chance.

## Problem / why
The 2026-10-01 audit (engram #396) found:
- Shortcuts that bypass the chance engine.
- Several logic bugs in the order the routes are checked.
- A config writer that clobbers hand edits.
- Per-character state keyed by an engine handle that changes on relaunch.
- A 700-line `Evaluate` with no tests.

The player also cannot see stress or join chance.

## Scope (authorized by the user, 2026-10-01)
- Easy-mode cage wait: 12h -> **6h, flat for everyone** (user decision; keep it as an easy fallback).
- Review and correct the money (price / transfer) calculation.
- Fix bugs, improve code, correct formulas, add features at the agent's discretion.
- New UI: stress and success-% readout per character.

Out of scope:
- Push and PRs. `origin/main` is an unrelated squash commit (engram #395), so delivery is the user's call.

## Constraints
- Isolated SOLID changes: each fix lives in its own module, with no patch-on-patch.
- Never overwrite user config edits. Deployed `config.json` and `messages.json` only get missing keys added. The 6h default is the one explicit user-requested value change.
- Do not drop user-validated behaviour. These design decisions from engram #72 stay:
  - No meter.
  - A stable seeded roll, not a reroll on click.
  - Rescue offers and never charges.
  - Field submission is a route.
  - The grudge rule.
  - Animals are tamed with food.
  - The button reads IGNORAR.
  - Mod Hub over `config.json`.
- UI: make an HTML mockup in `ui-mockup/` before implementing it in game.
- Reuse what already works in sibling mods:
  - TotalSparring: `fighter_identity`, `session_hooks`, `plugin_config` merge, `duel_tests` harness, `spar_overlay`, `npc_portrait`.
  - Shutori and the other mods.
- Kenshi SDK traps: read `../KENSHI-MODDING-NOTES.md` §11. Never declare an engine virtual. `_NV_` thunks run base code.
- Artifacts (code, comments, docs) in English. Player-facing text goes through `messages.json`.

## TDD
- Mode: **strict, enabled**. Source: the Gentle AI session config (Strict TDD Mode: enabled).
- Runner: none existed. T1 creates `tests/rules_tests`, a standalone console exe built with MSBuild, with no engine. It is modelled on `TotalSparring/tests/duel_tests`.
- Pure-rule changes need a failing test first (RED), then GREEN, then refactor.
- Engine-bound code (hooks, UI) is verified with build plus log lines, and listed in QA.md.

## Delivery
- Strategy: commits only, on a single feature branch. One work-unit commit per task, using Conventional Commits.
- RDD: off (global). There is no native review.

## Tasks
- [x] **T1 Test harness.** Add `tests/rules_tests` (vcxproj, stubs, runner) able to compile pure rule code from `src/`. Move or extract the pure pieces it needs.
  - Route: delegated (needs 4+ files).
  - Check: tests build and run green.
- [x] **T2 Route-order bugs** (rules), with tests:
  - The field-yield branch runs before the grudge check (rules.cpp 1797 vs 1896).
  - A caged NPC with 2 or more defeats loses the cage fallbacks (easy wait, healthy, Care, cats).
  - The reload floor is too low: `persistedFloorCount` 1 is less than `defeatsToBreak` 2, which breaks R3.3.
  - A mercy plea accepted is answered with a price or an honour refusal (R3.6/R3.7).
  - Route: delegated.
- [x] **T3 Mercy and popup bugs:**
  - `OnAccept` never clears `g_pleading` (offer_popup.cpp:623).
  - `g_pleading` is a raw pointer.
  - Route: delegated.
- [x] **T4 Easy mode 12h -> 6h, plus dead controls:**
  - Default `easyHoursCaged` 6.
  - Remove or wire the dead hub controls `hoursCagedForProud` and `uniqueExtraHours`.
  - Fix the hold clock so it starts at cage time, not first evaluate.
  - Prune stale hold entries when a prisoner leaves the cage.
- [ ] **T5 Money audit and fix.** Re-derive the price curve against the reference mods (Recruit Prisoners for Cats, and others in this workspace or installed). Fix:
  - The formula.
  - Transfer correctness: `takeMoney` sign, the purse gift, bribes.
  - The faction and personality multipliers.

  Tests for the curve at fixed levels.
- [ ] **T6 Config writer.** `SaveConfig` must merge (update known keys, keep unknown and hand-edited ones) instead of truncating. Model it on `TotalSparring/core/plugin_config` add-missing-never-overwrite.
- [ ] **T7 Stable identity.** Port TotalSparring `fighter_identity` (a UUID in the character save, key `total_recruit.character_id`) plus a load-begin reset hook. Re-key `CharKey` maps and the `StableChance` seed onto it, with the handle as fallback.
- [ ] **T8 Code structure:**
  - Split `Evaluate` into per-route rule functions.
  - Dedupe the `Safe*` helpers into one module.
  - `SpeakOpeningLine` keys off a reason enum, not Spanish substrings.
  - Move hardcoded magic numbers into config where R6.1 expects them.
- [ ] **T9 Stress and chance readout UI:**
  - Make the mockup in `ui-mockup/` first.
  - Then implement an in-game bar or label: NPC stress, join %, active route and blocking reason. Hover or selection based, reusing the recruit_overlay and cursor_hint pipeline and the TotalSparring overlay patterns.
  - Make it configurable in the Mod Hub.
- [ ] **T10 Docs.** Refresh README Status, TODO.md, SPEC-ESTADO (mark superseded), ESTADO.md, QA.md (new cases for T2-T9), and REQUISITOS R6.4.
- [ ] **T11 Build and deploy.**
  - Release|x64 build.
  - Deploy the DLL with an md5 check.
  - Add only the missing keys to the deployed `config.json` and `messages.json`, plus `easyHoursCaged` 6.

## Acceptance criteria
- `tests/rules_tests` is green and covers the T2/T4/T5 rules.
- The Release build succeeds, and the deployed md5 matches the build.
- No user-edited config value is lost.
- QA.md lists the in-game cases that prove each engine-bound change.

## Progress / evidence
(updated per task; commit hashes are filled in by the next task's commit, since a commit cannot name itself)

### T1 Test harness - done
- Commit: `c0b00fa` (`test: add rules_tests harness and extract pure rule modules`).
- Route: delegated (one writer for T1-T4). Trigger: 4+ files plus 2+ non-trivial files.
- Pure modules extracted from `rules.cpp`, compiled by both the DLL and the tests (no duplicated logic):
  `chance_rules` (ChanceFrom*, JoinChance, StressFromState, StableRoll), `price_rules` (BasePriceInCats,
  PersonalityPriceMultiplier), `route_rules` (`ChooseEntryRoute`, the route precedence), plus `char_key.h`
  and `temperament.h`. `temperament_map.cpp` is the one engine-side PT_* mapping.
- `Evaluate` now builds `RouteFacts`, asks `ChooseEntryRoute`, and each block runs `if (route == Route_X)`.
  Order is no longer implied by block order. Behaviour is unchanged (characterization only).
- Tests: `tests/rules_tests` (own vcxproj, v100). Characterization tests of current behaviour: 3 suites, 15654 checks, all green
  on first run (nothing was changed, so there is no RED by design; RED/GREEN starts in T2).
- Builds: `MSBuild TotalRecruit.vcxproj Release|x64` OK (only the known C4482/C4091 SDK warnings);
  `MSBuild tests/rules_tests/rules_tests.vcxproj Release|x64` OK; `rules_tests.exe` exit 0.
- Docs: DEVELOPING.md module map and a new "Tests" section.
- QA: nothing engine-visible changed. Smoke only: with `probeOnClick` the log now has a `[route] <name>` line per Evaluate.

### T2 Route-order bugs - done
- Commit: `381bdde` (`fix(rules): correct route order, defeat floor and plea accept`). Route: delegated (same writer).
- RED (before any production change): `rules_tests.exe` printed `2/5 suites passed, 22/15705 checks failed`:
  route_rules 9 failures (grudge vs field-yield x2, caged-with-defeats x6, reload x1), defeat_rules 3 (floor 1 < 2),
  mercy_rules 10 (accept not a join).
- GREEN: `5/5 suites passed, 0/15705 checks failed`.
- a) `ChooseEntryRoute` now tries the unforgivable grudge before field-yield.
- b) Field-yield applies outside the player's cage only; in the cage defeats still feed the chance and the cage routes
  (easy wait, easy-healthy, care, cats, intimidation) stay reachable. The reach of "one more way in" was not needed.
- c) New `defeat_rules` (`PersistedDefeatFloor`): the floor never drops below `defeatsToBreak`. `persistedFloorCount`
  stays configurable (it only matters when set above `defeatsToBreak`); Mod Hub description updated. `DefeatCount` uses it.
- d) New `mercy_rules` (`CanBuyTheirLife`, `ResolvePlea`) holds R3.6/R3.7. ACEPTAR on a plea now calls `AcceptMercy`
  (mercy.cpp): `Recruit` directly, no `Evaluate`, no price, no honour refusal. New message key `mercy.joinedNotice`.
  `AcceptMercy` also clears the pending plea (see T3).
- Builds: `MSBuild TotalRecruit.vcxproj Release|x64` OK; `MSBuild tests/rules_tests` OK; tests green.
- Tracked: T1 hash filled in above. `D6` in QA.md changed from `floor+session=1` to `2`.
- In-game QA: QA.md section 12.1 (F1-F8).

### T3 Mercy and popup bugs - done
- Commit: `a7a274c` (`fix(mercy): clear the plea on every close path and key it by handle`). Route: delegated (same writer).
- RED: new `plea_tracker` suite with a stub that never cleared and matched on index only:
  `5/6 suites passed, 7/15728 checks failed` (clear/next-plea x4, Clear() x2, reused slot serial x1).
- GREEN: `6/6 suites passed, 0/15728 checks failed`.
- New engine-free `PleaTracker` (src/plea_tracker.{h,cpp}) holds the pending plea as a `CharKey`; `CharKey` gained `operator==`.
  `mercy.cpp` replaced `Character* g_pleading` with it. A reused slot has a new serial so a stranger cannot match.
- Clear-on-every-path: `AcceptMercy`/`GrantMercy`/decline call `ForgetMercy`; `offer_popup.cpp` `Close()` now calls
  `ForgetMercy(NULL)` (any end of the window: cross, HideOffer, shutdown) and the handlers read `IsMercyPending` BEFORE
  `Close()`. `ShowOffer` clears any pending plea on entry so a later ordinary offer cannot inherit it (it would have turned
  its ACEPTAR into a free join); `OfferMercy` therefore marks the plea after `ShowOffer` succeeds.
- Builds: DLL Release|x64 OK; tests green.
- In-game QA: QA.md 12.2 (M1-M4).

### T4 Easy mode and dead controls - done
- Commit: `{T4}` (`fix(easy-mode): 6h default, drop dead controls, start the hold clock at caging`). Route: delegated (same writer).
- RED: new `hold_clock` suite with a stub that never dropped a clock and never handled a backwards game clock:
  `6/7 suites passed, 7/15751 checks failed` (release drops the clock x2, re-cage starts over x1, per-prisoner x2,
  clock goes backwards x1, `Prices().easyHoursCaged == 6` x1).
- GREEN: `7/7 suites passed, 0/15751 checks failed`.
- `easyHoursCaged` default 12 -> 6 in `config.h` and the repo copy `TotalRecruit/config.json` (the one user-requested value
  change). Flat for everyone: nothing reads personality or uniqueness for the wait.
- Removed `hoursCagedForProud` and `uniqueExtraHours` from the `Prices` struct, `LoadConfig`/`SaveConfig`, the Mod Hub
  registration and the repo `config.json`. `LoadConfig` looks keys up by name, so an old file that still has them loads fine.
  The deployed `config.json` is NOT touched (T11 adds `easyHoursCaged` 6 there).
- Hold clock: new engine-free `HoldClock` (src/hold_clock.{h,cpp}). `NoteCageState` (rules.cpp) is called by the periodic sweep in
  `hail.cpp` for every character in range: first sighting in the player's cage starts the clock, a sighting outside drops it.
  `HoursHeld` (used by Evaluate) only observes "caged" as a fallback and never restarts a running clock. A game clock that went
  backwards (older save loaded) restarts the hold. The sweep now also runs with `hailWhenReady` off (`SweepForHails(hailing)`).
  Known limit: a prisoner released and re-caged entirely out of the 900-unit sweep radius is not seen being free, so keeps its clock.
- Probe log lines: `[hold] clock started/dropped` and `[hold] held=Xh easy-mode needs Yh` (probeOnClick only).
- Builds: DLL Release|x64 OK; tests green.
- In-game QA: QA.md 12.3 (E1-E8). `S2` control count note updated (~76).

## Next step
T5 (money audit). Fill in the `{T4}` commit hash above on the next commit.
