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
- [x] **T5 Money audit and fix.** Re-derive the price curve against the reference mods (Recruit Prisoners for Cats, and others in this workspace or installed). Fix:
  - The formula.
  - Transfer correctness: `takeMoney` sign, the purse gift, bribes.
  - The faction and personality multipliers.

  Tests for the curve at fixed levels.
- [x] **T6 Config writer.** `SaveConfig` must merge (update known keys, keep unknown and hand-edited ones) instead of truncating. Model it on `TotalSparring/core/plugin_config` add-missing-never-overwrite.
- [x] **T7 Stable identity.** Port TotalSparring `fighter_identity` (a UUID in the character save, key `total_recruit.character_id`) plus a load-begin reset hook. Re-key `CharKey` maps and the `StableChance` seed onto it, with the handle as fallback.
- [x] **T8 Code structure:**
  - Split `Evaluate` into per-route rule functions.
  - Dedupe the `Safe*` helpers into one module.
  - `SpeakOpeningLine` keys off a reason enum, not Spanish substrings.
  - Move hardcoded magic numbers into config where R6.1 expects them.
  - Also: break the defeats/mercy/recruit_core ring, credit the GOLPEARLO beating to the player, insistence-in-cage decision.
- [x] **T9 Stress and chance readout UI:**
  - Make the mockup in `ui-mockup/` first.
  - Then implement an in-game bar or label: NPC stress, join %, active route and blocking reason. Hover or selection based, reusing the recruit_overlay and cursor_hint pipeline and the TotalSparring overlay patterns.
  - Make it configurable in the Mod Hub.
- [x] **T10 Docs.** Refresh README Status, TODO.md, SPEC-ESTADO (mark superseded), ESTADO.md, QA.md (new cases for T2-T9), and REQUISITOS R6.4.
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
- Commit: `842f157` (`fix(easy-mode): 6h default, drop dead controls, start the hold clock at caging`). Route: delegated (same writer).
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

### T5 Money audit and fix - done
- Commit: `cad2cf8` (`fix(money): recalibrate the price curve, cap stacked multipliers and move cats safely`). Route: delegated (one writer for T5-T6).
  Trigger: 2+ non-trivial files, and reading that prepares the write.
- RED (stubs for `TalentMultiplier`, `FactionPriceMultiplier`, `FinalPriceInCats`, `CanAfford`, `ChargePrice`,
  `RefundCharge`, `TransferCats`, `TransferAllCats`, plus the new tests): `6/8 suites passed, 64/15842 checks failed`
  (price_rules 40 failures, money_rules 24; the old curve also fails the reference tiers at 5, 50, 60 and 75).
- GREEN: `8/8 suites passed, 0/15842 checks failed`. DLL Release|x64 builds (only the known C4482/C4091 warnings).
- New engine-free modules: `money_rules` (purse interface, `ChargePrice`, `TransferCats`, refund) and extended `price_rules`
  (`FinalPriceInCats`, `TalentMultiplier`, `FactionPriceMultiplier`). Engine glue: `cats_purse` (`CharacterPurse`).

#### Audit: every money path, with findings
Trace: price = `PriceInCats` (rules.cpp, was 726-774) -> shown in Evaluate (1947, 2136: `purse < cats` check) -> charged by
`CompleteOffer` / `RescueTick` via `SettlePrice` -> `ChargePlayer` -> `leader->takeMoney`. Other directions: rescue gift
(`PayPartingGift`), mercy tribute (`PayTribute`).

| # | Finding (HEAD) | Where | Fix |
|---|---|---|---|
| 1 | `catsMinimum` ("minimum for any recruit") was applied to the base curve BEFORE the multipliers, so a coward (x0.6) or a friendly faction (down to x0.25) quoted below it: lv1 coward 120, not 200. | price_rules.cpp:24-27, rules.cpp:770-773 | Base curve is raw; `FinalPriceInCats` floors last. |
| 2 | Multipliers (talent x unique x personality x faction) were unbounded: talent had no cap (a stat 20x the level multiplied the price ~30x), so a brave unique craftsman at lv60 could be quoted 700k+; the `(int)` cast of an out-of-range float is undefined. | rules.cpp:757, 767-773 | `talentMaxMultiplier` (2.0), `catsHardCap` (150000, 0 = lifted but still kept under 1e9); clamped in float before the int cast. |
| 3 | Truncation, not rounding: `(int)cats` quoted 5624 where 5625.9 was computed; odd numbers everywhere. | rules.cpp:773 | Round to the nearest 10 (halves up), then clamp. |
| 4 | Curve miscalibrated against its own source. The comment claimed ~1k at 5, ~7k at 12, ~20k at 25, ~35k at 40, ~55k at 50, ~80k at 75, but the shipped params gave 467 / ~2.4k / 9.2k / 32k / 55k / 91k. Cheap end too cheap (a lv5 below the 1k "weak NPC" floor), top end too dear (above the 50-60k band at 55-65 stats and the ~80k at 70-80+). | price_rules.cpp:7-29, config.h defaults | Refit (below). |
| 5 | Every Mod Hub edit wiped five price keys from `config.json` (`catsCeiling`, `priceCurveSteepness`, `priceCurveMidpoint`, `talentWeight`, `uniqueMultiplier`): `SaveConfig` never wrote them, so after any hub tweak and a restart the curve silently reset to defaults. The most likely cause of "the money is wrong" on a tuned install. | config.cpp:343-398 | `SaveConfig` writes them (and the two new keys) now; T6 makes the writer merge instead of truncate. |
| 6 | Paid but not delivered: `CompleteOffer` / `RescueTick` charged, then `Recruit()` could fail ("engine refused the recruit") and the cats were gone. | recruit_core.cpp:714-728, rescue.cpp:223-232 | `RefundPrice` puts the exact charge back; `Refunded N cats` log line. |
| 7 | Rescue gift debited with `addMoney(-amount)` on the NPC (no funds check, relies on a negative add) and credited with `addMoney(amount)`; a failed second step left the NPC debited. | rules.cpp:1342-1343 | `TransferAllCats`: checked `takeMoney` debit, positive `addMoney` credit, rollback on failure. |
| 8 | Mercy tribute used the opposite convention (`takeMoney(-purse)` to credit) and ignored its result; if the credit was refused the NPC's cats were destroyed. | mercy.cpp:179-183 | Same `TransferAllCats`: one convention, one module (`cats_purse`). |
| 9 | `CompleteOffer` charged `verdict.cats` whatever the currency, while the animal route reuses `Verdict::cats` for "% of stomach missing". Harmless today only because that value is set only when `willJoin` is false. | rules.cpp:878, recruit_core.cpp:714 | `CatsDue()` is non-zero only for `Currency_Cats`. |
| 10 | Funds: no pre-check before the debit, so a `takeMoney` that partially charges would have been trusted. | recruit_core.cpp:224-236 | `ChargePrice` is all-or-nothing and checks the balance first (Short vs Failed). |

Checked and fine: the sign of `takeMoney` for the player's purchase (positive removes, false on short funds); price shown vs charged (the
offer re-evaluates at ACEPTAR, so the number charged is the number of that moment and is reported by `pagaste N`); faction multiplier
direction (hated seller dearer, clamped 0.25..3); rescue never charges; the gift is paid once (`LT_I_SCREWED_THIS_GUY`).

Not verifiable here: whether `Character::getMoney` and `Ownerships::getMoney` are the same pool. The header RVAs do not match the installed
`kenshi_x64.exe` (disassembly at those addresses is padding or unrelated code), so the adapter keeps the calls confirmed in play
(`getMoney` / `takeMoney` for the player, as the offer window and QA G1 do) and uses `Ownerships::addMoney(+n)` only to credit, the one
confirmed way to add (TotalSparring bets, KEP "Give money"; the Crooked Dice mod also confirms `takeMoney(-n)` adds).

#### Decision: the price does NOT take the player's level
Engram #72 left this pending. Decided: no.
1. The price is what the person is worth; the player's level is already the dominant term of `JoinChance` (levelBase 45, -1.5 per level
   the NPC out-levels you), so pricing it too would charge twice for one fact.
2. The reference mod prices by the recruit's own stats only.
3. The mod's economy is cats, and the player's real power curve is their purse, not their level: a high-level player with an empty
   purse should still find a lv40 fighter at 32k, and a level-5 player should not get a discount for being weak.
Documented in `PriceInCats`.

#### Calibration (target curve, anchors and sources)
Anchors (cats):
- Recruit Prisoners for Cats, published ranges by base stats, via `Kenshi_Guia_Completa_Mods_Reclutamiento.md` s5.4 (the mod itself is
  NOT installed here, so this guide is the only source): weak NPCs 1k-6k; modest bandits/fighters 6k-25k; 40-50 stats 30k-50k; 55-65
  stats 50k-60k; 70-80+ stats ~80k; strongest uniques 100k-150k, some leaders with their own price.
- Adventurers Guild (workshop 1401033215, strings read from its `.mod`): headhunting fee 5,000 cats per delivery, 10,000 per head for
  skeleton recruits, 20,000 for hivers. A "standard" delivery costs about a lv15-20 quote here.
- Better Mercenary Contracts (workshop 1387683396): merc hire 2,000 cats/day, 60-day contract 92,000; its author notes the FCS cannot
  charge more than 100,000 in one dialogue line, which is why reference uniques stop around 100-150k.
- Recruit Anyone (workshop 1672832964): free (the cheat end). Its numeric hire costs are binary FCS records, not strings, so nothing
  was invented from it.
- `RE_Kenshi_log.txt` holds no TotalRecruit lines (no earlier quotes to compare); the new `[price]` log lines (probeOnClick) will.

Fit (grid search of the existing two-term form, least squares on log error against targets 5:1.1k, 10:2.9k, 15:5.6k, 25:15k, 40:32k,
50:45k, 60:56k, 75:78k): `catsPerLevel 90, catsCeiling 28000, steepness 0.09, midpoint 31`. The mod is not currently installed under
`D:\SteamLibrary\...\mods`, so no deployed user value is affected; T11 deploys the repo `config.json`.

| Level | Before: base | Before: unique (x2) | After: quote | After: unique (x2) |
|---|---|---|---|---|
| 5 | 467 | 933 | 1,060 | 2,130 |
| 10 | 1,394 | 2,788 | 2,950 | 5,910 |
| 20 | 5,420 | 10,840 | 9,570 | 19,130 |
| 40 | 32,236 | 64,472 | 32,160 | 64,320 |
| 60 | 74,294 | 148,588 | 56,860 | 113,720 |
| 75 | 91,433 | 182,865 | 76,480 | 150,000 (capped; 152,960 raw) |

"Quote" is an ordinary-personality character with no talent or faction effect; brave/cruel x1.6, coward x0.6, hard cap 150,000.
Judgement: the middle (lv25-40) stays where it was because that is where it already matched the reference. What changed is the cheap end
(a lv5 NPC is no longer the price of a ration pack) and the top end (a lv60 is a serious but reachable purchase instead of a 74k wall,
and uniques land in the reference 100-150k band instead of overshooting it).
- New config keys (old files load unchanged): `catsHardCap` (150000), `talentMaxMultiplier` (2.0); both in the Mod Hub.
- New messages.json keys (only added): `money.paid`, `money.short`, `money.refunded`.
- In-game QA: QA.md 12.4 (P1-P9).
- Open question for the user: is a talent bonus of up to x2 on top of the guide tiers acceptable? The tiers are by base stats and Kenshi
  characters rarely have flat stats, so the median quote may sit 20-50% above the curve; the `[price]` lines will show it.

### T6 Config writer - done
- Commit: `ec07ff1` (`fix(config): merge into config.json instead of truncating it`). Route: delegated (same writer).
- RED (stubs for `MergeJsonKeys`, the `Json*` encoders and `WriteFileAtomically`, plus the new suites): `8/10 suites passed, 72/15933 checks
  failed` (json_merge 65, atomic_file 7). GREEN: `10/10 suites passed, 0/15933 checks failed`. DLL Release|x64 builds.
- New pure module `json_merge` (the text merge): matches keys at any depth by whole name, first occurrence wins (the same rule as the reader);
  replaces a value in place only when it differs (`6.0` vs 6, `0.60` vs 0.6 and `[ "a" ]` vs `["a"]` are left exactly as written); appends missing
  keys to the top-level object keeping the file's indent and line endings; understands strings with escapes, nested objects and arrays,
  `//` and `/* */` comments, a BOM and a trailing comma; a file that does not parse returns `ok=false` instead of being guessed at.
- New `atomic_file` (`WriteFileAtomically`): temp file in the same directory, flushed, then `MoveFileExA(REPLACE_EXISTING | WRITE_THROUGH)`; the temp is
  deleted on failure and the target is untouched (tested against the real file system, including a locked target).
- `config.cpp`: one table (`ConfigFields`) now drives both `LoadConfig` and `SaveConfig`, so a key cannot be read and not saved. Parity with the
  previous reader checked mechanically: 95 keys on both sides, identical. `SaveConfig` merges into the existing file, skips the write when nothing differs,
  and on an unparsable file copies it to `config.json.bak` and writes a fresh one (a missing file starts from a seed that keeps the nested `exclude` block).
  Keys that `SaveConfig` used to drop on every hub edit and that are now preserved: the price curve (`catsCeiling`, `priceCurveSteepness`,
  `priceCurveMidpoint`), `talentWeight`, `uniqueMultiplier`, `chanceIntimidationBonus`, `chanceGrudgePenalty`, `chanceUniquePenalty`,
  `chanceGrudgeIsAbsolute`, `animalOwnedNeedCage`, `animalNutritionPerMeal`, `animalPersonalityMatters`; string values are now escaped (a quote in
  `recruitLabel` used to corrupt the file).
- One-off check on the shipped `TotalRecruit/config.json`: merging no updates is byte-identical; a three-key update changed only those values and
  appended one key after the nested `exclude` block with the comma in place.
- In-game QA: QA.md 12.5 (C1-C8).
- Model: `TotalSparring/src/core/plugin_config.cpp` (add-missing, never-overwrite) in intent only; that file is a fixed-schema INI-style reader, so the
  merge logic here is new rather than ported.

### Tuning after T6 - talent cap 2.0 -> 1.5
- Commit: `84e4d77` (`tune(price): lower default talentMaxMultiplier from 2.0 to 1.5`). Answers the T5 open question: talent already raises the join chance, and quotes were
  landing 20-50% above the calibrated tiers, so `talentMaxMultiplier` default is now **1.5** (`config.h`, repo `TotalRecruit/config.json`, value change only).
- RED: `price_rules` default-cap and cap-at-1.5 checks failed (`9/10 suites passed, 2/15935 checks failed`). GREEN: `10/10 suites passed, 0/15935`.
- A user who already tuned the key keeps their value (the loader only falls back to the default for a missing key).

### T7 Stable identity and load-begin reset - done
- Commit: `d8b6381` (`feat(identity): persist a stable character id in the save and reset session state on load`). Route: inline by the single writer the parent delegated the whole unit to
  (ODD trigger: 2+ non-trivial files). TDD: strict, runner `tests/rules_tests` (MSBuild Release|x64).
- Evidence for the fix (TotalSparring CLAUDE.md:342-346, "Fighter identity"): the engine handle changes on a game relaunch (Kazuhiko
  `1-50170900-...` became `14-3181518592-...`), so every `CharKey{index,serial}` map AND the `StableChance` seed were reset on a relaunch. The comment in
  `rules.cpp` ("the handle's index and serial travel with the save") was wrong and is rewritten; the same claim in `chance_rules.h` is fixed too.
- RED (stubs only: `CharKey::FromId` ignoring the id, a `Registry` that tracks nothing, `StableRoll(CharKey)` forwarding to the handle form):
  `9/12 suites passed, 35/46208 checks failed` (char_key 9, identity_registry 23, stable roll 3).
- GREEN: `12/12 suites passed, 0/31499 checks failed`. DLL Release|x64 builds (only the known C4482/C4091 warnings).
- Pure, tested: `char_key.h` (`CharKey` is now an id OR a handle: `FromId`, `HasId`, `ToString`; id keys zero index/serial so two reads of one character
  always compare equal; usable as `std::map`/`std::set` key), `identity_registry.h` (port of TotalSparring `FighterIdentityPolicy::Registry`:
  Begin/Current/Load/Bind/Find/SavedId/End/Clear with lifetimes and sticky duplicate-id conflicts; `ChooseKey` is the id-or-handle policy),
  `StableRoll(const CharKey&, int)` (FNV-1a plus a Murmur finaliser of the id; a handle-only key rolls exactly as before).
- Engine-bound (build + log lines + QA 12.6): `character_identity.{h,cpp}` (the three `Character` hooks, save key **`total_recruit.character_id`**),
  `char_key_reader.{h,cpp}` (`SafeCharKey` mints, `SafeFindCharKey` only reads, so the hold-clock sweep over everyone in range does not mint ids for
  bystanders; the handle is the fallback, logged once), `load_boundary.{h,cpp}` (hooks `SaveManager::loadGame` / `importGame`).
- Hooks and addresses (header RVAs; KenshiLib maps them to the running exe): `Character::_DESTRUCTOR` 0x622300, `Character::_NV_loadFromSerialise` 0x6267D0,
  `Character::_NV_serialise` 0x627920, `SaveManager::loadGame` 0x3739E0, `SaveManager::importGame` 0x378510. `CharacterHuman` overrides serialise /
  loadFromSerialise, so the `_NV_` trap (base code only) could apply to humans; not verified by disassembly (the installed exe does not match the header RVAs).
  The evidence is Kill Counter, a published mod that relies on the same two base hooks for per-character data in the save, and TotalSparring. QA I4/I5 prove it.
- Chaining: `KenshiLib::AddHook` chains (MinHook); each mod's detour receives the previous detour as its "original". Ours call the original exactly once with
  every argument untouched (serialise forwards `arg1`/`arg2` as received and recovers `this` as the one that is not the returned slot), take the registry lock
  only AFTER the original returned (native callbacks may re-enter load/destroy), and use a distinct save key from TotalSparring (`total_sparring.fighter_id`)
  and Proving Grounds (`proving_grounds.fighter_id`). Load order only decides call order; behaviour does not depend on it. The id is written to the
  `GAMESTATE_CHARACTER` instance state and to `STATS` (Kill Counter's proven location); a load reads the former first. NOT fail-closed: hooks missing or a
  duplicate id means the handle key is used, and a failed save write is logged once and never flips the key mid-session.
- Why the load reset is at `loadGame`/`importGame` BEGIN and not `GameWorld::loadAllPlatoons` (the hook TotalSparring's `session_hooks` uses): `loadAllPlatoons`
  runs inside the load, after characters bound the ids they read back from the save, so a reset there would discard them. TotalSparring resets identity at
  load begin for the same reason; this mod resets everything there, from one place (`LoadBoundary::ResetSessionState`).
- Re-keyed onto `CharKey` (id, handle fallback): `g_defeats`, `g_hailed`, `g_begged`, `PleaTracker`, `HoldClock`, `g_talkStates`, and the `StableChance` seed
  (verdict roll). They already used `CharKey`, so the type change carried them; reads that must not mint (`DefeatCount`, `CanBeatPrisoner`, `IsMercyPending`,
  `ForgetMercy`, `IsIgnoringPlayer`, the non-caged branch of `NoteCageState`) use `SafeFindCharKey`.
- Cleared at load begin (every file-scope map/set and every raw `Character*` / `RootObject*` global found by grepping `src/`):
  1. `g_defeats` (defeats.cpp, `ForgetAllDefeats`)
  2. `g_hailed` + `g_nextSweep` (hail.cpp, `ForgetAllHails`)
  3. `g_begged` + `g_pleading` (mercy.cpp, `ForgetAllMercy`)
  4. `g_holdClock` (rules.cpp, `ResetHoldClock`)
  5. `g_talkStates` (rules.cpp, `ResetTalkStates`)
  6. dialogue bridge: `g_conversationNpc`, `g_conversationActive`, `g_pendingRecruit`, `g_pendingCats`, `g_quotedFor`, `g_quotedCats` (`ForgetConversation`;
     this is the pending-recruit hand-off that `RescueTick` consumes)
  7. `g_lastTarget`, `g_lastWasRecruitable` (cursor_hint.cpp, `ForgetCursorTarget`)
  8. `g_targetObject`, `g_targetCharacter` and the button (recruit_overlay.cpp, `ForgetOverlayTarget`)
  9. the open offer / mercy window and its `g_subject` (offer_popup.cpp, `HideOffer`)
  10. the identity registry (`CharacterIdentity::AbandonWorldState`, last)
  Deliberately kept: `g_spawnedThisSession` (spawn_core.cpp: a per-process cap on panel spawns, a reload must not reset it), the FCS data caches in
  game_data.cpp (static game data, not per-save), config and messages.
- Behaviour change to expect: saves from before this have no ids, so a character gets a new UUID on first sight (written at the next save). That
  character's stable join roll is **recast once** (the seed moves from the handle to the id); from then on it survives a relaunch.
- Persisting per-character state through the save: NOT done. Defeats already have an engine-backed floor (layer 1 of `defeats.h`); the hold clock
  restarting after a reload is the deliberate safe direction (the player waits again, nobody is freed for time they did not serve). Writing more fields means
  the serialise hook calling back into rules/defeats code from inside engine save callbacks, which cannot be verified without in-game saves, and the
  id write itself is still unproven in game. Left in memory; revisit after QA I4/I5 pass.
- Not verified here: the game was not run. The id surviving a save and a relaunch is only proven by QA 12.6 I4/I5 (TotalSparring's own relaunch test, H.33,
  is also still pending; its log has only the `Hooked` lines).
- Docs: DEVELOPING.md module map (identity_registry, character_identity, char_key_reader, load_boundary). New `probeOnClick` log: `[route] <name> [key] id:<uuid>`.
- In-game QA: QA.md 12.6 (I1-I10).

### T8 Code structure - done
- Commits (5 work units, each built with `MSBuild TotalRecruit.vcxproj` Release|x64 and `MSBuild tests/rules_tests` + `rules_tests.exe`):
  1. `b1db262` `refactor(reads): share the Safe* engine reads in one engine_reads module`
  2. `ba0a941` `refactor(defeats): break the defeats-mercy-recruit_core include ring`
  3. `01c5414` `fix(defeats): credit the GOLPEARLO beating to the player explicitly`
  4. `724dbfe` `refactor(config): make the audit's magic numbers tunable, with hub controls`
  5. `cf6ab8d` `refactor(rules): split Evaluate into per-route rule modules over one facts struct`
- Route: single writer, inline (the parent delegated the whole unit). Triggers: 4+ files mapped, 2+ non-trivial files written.
  TDD: strict, runner `tests/rules_tests`. RDD off (global), so no native review.
- Size: `rules.cpp` 2221 -> 189 lines (`Evaluate` itself is ~70 lines of dispatch); `rules.h` 144 -> 21. New modules: `engine_reads` (272), `character_facts`
  (~590, the only engine reads on the decision path), `entry_facts.h`, `verdict.h`, `route_decisions.h` + `route_gate/cheat/rescue/yield/animal/free/cage/cats.cpp`
  (28 to 258 each), `route_rules.cpp` (+`MakeRouteFacts`), `body_rules`, `relation_rules`, `animal_rules`, `reason_catalog`, `reason_text`, `npc_speech`,
  `hold_tracking`, `insistence`, `rescue_memory`, `pacify`, `temperament_map.h`, `archetype.h`.

#### 1. `Evaluate` as a dispatcher (goals 1, 2)
- Engine reads are gathered once: `GatherCoreFacts` (everything every route may look at) plus one small extra per route (`RescueExtra`, `FreeExtra`, `CageExtra`...,
  read only when that route answers). `ChooseEntryRoute` (over `MakeRouteFacts`) picks the route, one pure function per route decides to a `Verdict`
  (reason = `ReasonKey`), `RenderReason` turns the key into the player's line. Per-route gathering stops where the decision stops (another cage, a pestered
  NPC, an animal), so a click on a bystander still does not run the talent survey.
- The cats pay-or-join block existed twice (free world and cage), word for word: it is `ApplyCatsOffer` (route_cats). The level was read three times
  (rescue, field yield, the rest): `CoreFacts` reads it once.
- Roll keys are only minted for a route that can roll (`FreeWorldMayRoll`), as before, so asking about a bystander does not create an id.
- Dead code found and dropped (provably unreachable): the `Preset_Cheat` branches inside the free-world ally block and inside the price, because Cheat returns at the
  top of `Evaluate` for everyone. Dead code found and KEPT as shipped (not a refactor's call): the `BrokenNoPride` branch of the cage submission case (no input reaches
  it, `ChooseCurrency` sends the starving and the wounded to care first); marked in `route_cage.cpp`.
- Tests (`tests/rules_tests`, new suites `route_decisions`, `body_rules`, `reason_catalog`; 16 suites, 35765 checks). Evaluate is engine-bound, so these cannot run
  against the old function: every expectation was read off the old code branch by branch (order, strict vs non-strict comparisons, which input a branch ignores)
  and written BEFORE the modules. First build RED (modules and `MakeRouteFacts` missing, compile errors), then `16/16 suites passed, 0/35765 checks failed` on
  the first complete build. Rolls are tested without depending on the hash (the tests search for a key that rolls the way the case needs at the percent the case produces).
- Pure pieces moved into the characterized layer: `body_rules` (wounded/starving/easy-healthy/restored), `relation_rules` (bands), `animal_rules` (taming bar),
  `RollsToJoin` (the verdict roll), `ChooseCurrency`, `UniqueTakesMoney`, and the whole decision of every route.

#### 2. Safe* helpers (goal 3)
- `engine_reads.{h,cpp}`: `SafeGameHours`, `SafePlayerLeader`, `SafePlayerFaction`, `SafeIsPlayerCharacter`, `SafeIsAnimal`, `SafeIsDead`, `SafeIsDown`, `SafeMemoryTag`,
  `SafeRememberTag`, `SafeInPrison`, `SafeIsPretending`, `IsGenuinelyDying`, `IsInPlayerCage`, `CharacterName`. Copies removed from rules, defeats, surrender, mercy,
  hail, rescue, recruit_core, probe and dialogue_bridge. C2712: each guard is a POD-only frame; `CharacterName` builds its `std::string` outside the guard.
- The one `SafePlayerLeader` requires an active platoon (the `rules.cpp` copy did; defeats, surrender, mercy, probe and dialogue_bridge did not). The stricter rule
  is a deliberate behaviour change at one edge: while the squad is still being built the five other callers now get "no leader" instead of asking the engine to read
  through a null active platoon.

#### 3. Include ring (goal 4)
- `defeats` called `OfferMercy` (mercy), mercy needs the recruit path (`recruit_core`), the recruit path needs `defeats` (`CanBeatPrisoner`). `defeats` now notifies a
  `NearDeathListener` that `startPlugin` wires to `OfferMercy` (`SetNearDeathListener`): the chain is mercy -> recruit_core -> defeats and nothing points back.
  `CharacterName` and `IsInPlayerCage` moved to `engine_reads`, which also removed `defeats -> rules` and `rules -> recruit_core`.
- Also broken: `rules <-> surrender` (`IsGenuinelyDying` moved to `engine_reads`).
- Still a ring, not asked for and left: `mercy <-> offer_popup` (mercy opens the window, the window answers by calling `AcceptMercy`/`GrantMercy`).

#### 4. Reason keys and messages (goal 5)
- `Verdict` carries `reasonKey` (a `ReasonKey`) next to the text. `reason_catalog.cpp` holds each key's messages.json path, built-in text and refusal tone.
  `SpeakOpeningLine` picks a refusing NPC's line from `RefusalToneFor(reasonKey)`; the substring search ("honor", "lealtad", "mataste"...) is gone.
- Test (characterization): the old matcher is copied into the test as an oracle and fed every built-in reason text; the key-based tone equals it. T8 block: rewording a
  message cannot change the tone.
- 29 reasons that bypassed `Say()` are now messages.json keys under a new top-level `reason` block (same Spanish text as values). Keys that already existed
  (`cheat.*`, `grudge.never`, `submission.*`, `rescue.*`, `captive.intimidated`, `unique.zealot`, `animal.*`, `faction.<band>.*`) keep their paths. `Fill` gained `{purse}`.
  The deployed messages.json needs the new block added in T11 (the mod works without it: built-in text, and the log names the missing keys).
- Visible consequence of a key instead of a text search: a loyal NPC (honourable, brave, kind) refusing over a grudge outside your cage always says "Se lo que hiciste"
  (before: only when the message text happened to contain "mataste", 2 of the 7 shipped variants).
- Fixed on the way: two fallbacks carried a raw Latin-1 byte (`animal.refused`, `unique.zealot`) that is only visible if the key is missing.
- NOT done, flagged: `messages.json` already ships `refusal.*` and `currency.*` sections that nothing reads (the opening lines are literals in `npc_speech.cpp`). Wiring them
  would change what NPCs say (different wording and variants), so it is a content decision, not a refactor. The other hardcoded Spanish (opening lines, `DescribeOffer`,
  `HailLine`, the mercy and beating messages) stays in code for the same reason.

#### 5. Magic numbers to config (goal 6)
- Defaults equal the old literals (tests pin them); every key goes through the T6 table (`ConfigFields`), so load/save/merge stay one list. Old config files load unchanged;
  the repo `config.json` is not touched (it never held the chance keys; T11 adds only missing keys to the deployed file).
- 19 new keys, each with a Mod Hub control (hub registrations 87 -> 106):
  `chanceRelationWeight` (0.25), `chanceAlliedBonus` (10), `chancePersonalityFearful` (20) / `Traitorous` (15) / `Dumb` (10) / `Smart` (-5) / `Brave` (-15) /
  `Honorable` (-20) / `Maniacal` (-25) / `ColdCruel` (0) / `WarmKind` (0), `relationRefuseGoodNatured` (-15), `relationRefuseAll` (-30),
  `talentCraftWeight` (1.6), `talentShadowWeight` (1.3), `animalEdgeLevel` (10), `animalBigEdgeLevel` (20), `animalTemperamentBase` (0.85), `animalTemperamentWeight` (0.15).
  Values are clamped on load to ranges the rules were written for.
- RED (stubs that ignored the config): `10/13 suites passed, 23/31576 checks failed` (chance 14, price 3, animal 6); GREEN `13/13 suites passed, 0/31576 checks failed`.
  The shipped values were pinned first (existing chance tests, plus a new `animal_rules` suite for the taming bar, all green on first run).
- Not moved, still literals: the relation band edges 75/25/5/-5 (naming only), the 3.0 full-stomach reading (engine fact), the `health >= 1.0` of easy-healthy.

#### 6. Insistence in the cage (goal 7): decided, left as it is
- The audit's wording ("RecordInteraction only in the non-cage branch") is slightly off: `AttemptRecruit` records every attempt, caged or not; what only the free
  route does is CHECK `IsIgnoringPlayer`. REQUISITOS R2.4 ("cumulative, configurable cooldown") says nothing about cages, so it is not explicit; applying it would also let the
  player lock themselves out of their own prisoner for 4-48 h, against the cage routes (easy wait, beating, care) that need repeated visits. Left unchanged, covered by QA 12.7 N1.

#### 7. GOLPEARLO credit (goal 8)
- `BeatPrisoner` forced a knockout and then asked `NoteDefeat`, which asks the engine who put them down. When `lastGuyWhoDefeatedMe` did not name the player the beating was
  dropped, the window still said "lo dejaste sin sentido", and no record existed for the cooldown to read. `CreditDefeatToPlayer` counts it explicitly; the own-side filter and the
  debounce still apply. Pure, tested: `DecideDefeatCredit`, `IsEchoOfLastDefeat` (defeat_rules).
- RED (stub that ignored the explicit credit): `11/12 suites passed, 2/31513 checks failed`; GREEN `12/12 suites passed, 0/31513 checks failed`.
- Log line for QA: `[defeat] credited to the player explicitly (beating): <name> (engine winner is the player / is someone else / unreadable)`.
- Builds: DLL Release|x64 OK after each commit (only the known C4482/C4091 SDK warnings); tests green after each.
- In-game QA: QA.md 12.7 (A1-A3, Z1-Z4, C1-C4, R1-R2, N1).

### T9 Stress and chance readout UI - done
- Docs commit: `98d5d0f` (`docs(readout): module map, QA 12.8 and the T9 evidence`).
- Commits (4 work units; DLL Release|x64 and `tests/rules_tests` built after each code one):
  1. `83cb624` `docs(ui): HTML mockup of the stress and chance readout` (`ui-mockup/readout.html`, `css/`, `js/`, `README.md` with the design decisions)
  2. `4be4298` `feat(readout): pure view-model for the stress and chance readout`
  3. `5796127` `feat(rules): EvaluatePeek, the decision asked as a bystander without side effects`
  4. `2028525` `feat(readout): in-game stress and chance readout over the NPC, with Mod Hub controls`
- Route: single writer, inline (the parent delegated the whole unit). Triggers: 4+ files mapped, 2+ non-trivial files. TDD strict, runner `tests/rules_tests`.
- Mockup first (own commit, screenshot-checked in Chrome): six states (caged bandit, field yield, animal, refused/grudge, ally, saved life), expanded and compact, the
  on-screen placement (above the NPC head, hover / right-click menu) and the Mod Hub controls. Decisions are in `ui-mockup/README.md`.
- RED: stubs for `StressTermsFromState`, `FormatCats/FormatHoursLeft/FillLine`, the catalog and `BuildReadout` plus the new tests: `15/19 suites passed, 107/36434 checks failed`.
  GREEN: `19/19 suites passed, 0/36664 checks failed` (the one failure on the way was a test whose rolled stress contradicted its own facts; the test was corrected, not the code).
- Pure and tested: `readout_model` (what the card shows), `readout_catalog` (one short line per `ReasonKey` + the cage wait; the test enforces <= 40 ASCII chars at the widest
  fill, unique keys, one row per reason), `readout_format` (`9.570`, `3h`/`45min` rounded up), `StressTermsFromState` (the three terms; `StressFromState` is now built from it, so the
  bar and its parts cannot drift), and `Verdict::tameFullness/tameNeeded` set by `DecideAnimal` (the stomach bar behind an animal verdict, -1 otherwise).
- Honesty: every number is one the decision computed. Chance = `Verdict::chancePercent`; price = `Verdict::cats`; stress = the stress the roll used (`chanceInputs.stress`) when it rolled,
  else `StressFromState`; animal bar = the two numbers `DecideAnimal` compared.
- `EvaluatePeek` (rules.cpp, one `Run(character, mode, detail)` shared with `Evaluate`; `GatherMode` in character_facts): no hold clock started (`PeekHoursHeld` reads a clock that already
  runs, else the wait line is simply absent), no id minted, **no roll key supplied** (so the roll can never pass), no probe logging. Consequence, deliberate: the card shows the odds and what is
  needed if luck does not carry them, never whether the roll will pass (a peek that read the existing id would reveal the stable outcome and make the percent meaningless, and
  would behave differently for NPCs with and without an id). `SafeFindCharKey` is therefore not even needed on this path.
- Side finding, not changed: the existing cursor-hint/`hasDialogue_Fast` path calls `Evaluate` (decide mode) for every candidate NPC, so a caged bystander already gets an id and a hold clock from the
  talk marker. That is T7/T4 behaviour; the readout does not add to it.
- Engine bits: `readout_target` (menu target from `recruit_overlay`, `PlayerInterface::selectedObject`, hover = characters near the camera centre projected through `Ogre::Camera`
  view*projection, nearest to the mouse within 48 px, never the player's own; hidden when the mouse is over a window), `readout_ui` (MyGUI on the `ToolTip` layer, `Pick=false`,
  no mouse focus anywhere, fills = shipped `gui/images/tr_white.png` registered like TotalSparring `ui_resources`, fallback `WhiteSkin`, stock Kenshi fonts), `readout` (250 ms update, per-frame follow,
  hides while the offer window is open, `ForgetReadout` in the load-boundary reset (item 11 of the list), `ShutdownReadout` at stop). `engine_reads` gained `CharactersNear` / `SafeAsCharacter`
  (hail uses them: its private copies are gone).
- Config (T6 table, hub controls): `readoutEnabled` (true), `readoutCompact` (false), `readoutOnHover` (true). The repo `config.json` is untouched (defaults apply); `messages.json` gained a `readout` block
  (labels, six route names, 49 short lines, ASCII Spanish); `TotalRecruit/gui/images/tr_white.png` is new and must be deployed with the mod (T11).
- Log line (probeOnClick, only when what is drawn changes): `[readout] <name> route=<r> chance=<n>% stress=<n>% (hunger H wounds W tired T) defeats=a/b food=<n>% line="..." [hover|selection|menu]`.
- Not available / not verified (no running game): that the hover pick lands on the NPC (projection is plain view*projection of `getPositionBip01()`), that `selectedObject` holds a selected NPC, and
  that the longest lines fit 264 px in `Kenshi_StandardFont_MediumSmall`. All listed in QA 12.8. The offer for Piedad (mercy) route name exists but a plea target is rarely under the cursor.
- In-game QA: QA.md 12.8 (U1-U21).

### T10 Docs - done
- Commit: filled in by the next task's commit. Docs only.
- README (Status, Cheat/easy mode/price, readout card, identity coexistence, files, no Ctrl+K), ESTADO.md (section 2.1 with every T1-T9 change at
  "Compila + tests", next steps, git remote note), TODO.md rewritten, SPEC-ESTADO.md and COMPARATIVA.md bannered as historical, REQUISITOS R6.4 fixed
  and R3.3/R3.6/R3.7 marked implemented, QA.md section 12 run header and priority order.

## Next step
T11: Release|x64 build, then `scripts/deploy.sh` (dry-run first) once Kenshi is closed; in-game QA per QA.md 12 afterwards.
