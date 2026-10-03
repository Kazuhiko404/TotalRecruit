# Feature: price-readout-button-tweaks

Locator: `odd/tasks/price-readout-button-tweaks.md` · Engram mirror: `odd/price-readout-button-tweaks/tasks`
Branch: `feat/price-readout-button-tweaks` (from `feat/rebalance-and-ui` @ 38c7643)

## Objective
Three in-game tweaks requested by the user on 2026-10-03 after the first deploy of rebalance-and-ui.

## Problem / why
1. **Prices too high.** Log evidence (`RE_Kenshi_log.txt`):
   `Somán [price] level=13 base=4526 talent=x1.50 unique=x2.00 personality=x1.00 -> 13580 cats`.
   The user's target for that character (all stats ~12-13) is 2000-2500, 3000 at most.
   Two causes: the base curve is ~2.5x too dear at low/mid levels, and the talent, unique and
   personality multipliers stack without limit (1.5 x 2.0 x 1.6 = 4.8x).
2. **Readout is slow and covers the screen.** `ReadoutTick` (`readout.cpp`) polls the open menu,
   the selection and the hover target, and runs a full `EvaluatePeek` every 250 ms plus a full
   relayout. Separately, `cursor_hint.cpp` `HasDialogueFast_hook` runs a full `Evaluate` per NPC
   behind a one-entry cache that thrashes (most likely cause of the slowdown).
   The user wants the info to appear on an explicit action (clicking Hablar), not on hover.
3. **Hablar button overlaps TotalSparring and TotalLoot buttons.** `recruit_overlay.cpp` always
   anchors right of the menu, top-aligned (same spot as TotalSparring), height 26 / width 120,
   while the other mods use 44 / 160 with the Kenshi_Button1 styled look.

## Scope (authorized by the user, 2026-10-03)
- Re-tune the price formula so a level-13 unique talented NPC costs ~2500 (<= 3000).
- Readout shown only on explicit action (Hablar click); remove the per-frame/per-250ms cost.
- Hablar button: free-slot placement that avoids other mods' buttons; same size/style as them.

## Constraints
- Isolated SOLID changes: each fix in its own module, no patch-on-patch.
- Deployed `config.json` is user-editable: only add missing keys. Price keys whose deployed value
  still equals the old shipped default are updated (the user asked for the price change); any
  key the user changed by hand is left alone.
- UI: update `ui-mockup/` to match the new readout trigger.
- Reuse TotalLoot's tested `menu_slot` / `menu_obstacles` / button style instead of inventing.
- Artifacts in English; player-facing text through `messages.json`.

## TDD
- Runner: `tests/rules_tests` (MSBuild console exe).
- Price and slot geometry are pure: RED first, then GREEN.
- Readout trigger, cursor hint cache wiring and MyGUI placement are engine-bound: build + log + QA.md.

## Delivery
- Commits only on this branch, one work-unit commit per task. RDD: off (global).
- Forecast ~600-900 authored lines (T3 is mostly a port of tested TotalLoot code).

## Tasks
- [x] **T1 Price re-tune.** New curve defaults (catsPerLevel 32, catsCeiling 10000) and a new
  `premiumMaxMultiplier` (default 1.6) that caps talent x unique x personality. Tests first.
  Config key + Mod Hub control; deployed config updated per the constraint above.
  - Route: delegated (price_rules, config, hub, tests, docs).
  - Check: rules_tests green; level 13 unique talented quote in 2000-3000.
- [ ] **T2 Readout on click + cheap cursor hint.** Readout card pinned by the Hablar click
  (computed once, shown alongside the offer, closes with it or on timeout); hover polling only
  when `readoutOnHover` is explicitly enabled (default off); right-click `Evaluate` probe-only;
  cursor hint gets a per-character TTL cache module. Mockup updated.
  - Route: delegated.
  - Check: Release build; rules_tests green; QA.md steps.
- [ ] **T3 Hablar button placement and size.** Port TotalLoot `menu_slot` (+ tests),
  `menu_obstacles` (prefix `TR_`) and button style; button 44 x 160, styled like the others.
  - Route: delegated.
  - Check: rules_tests green (slot tests); Release build; QA.md steps.
- [ ] **T4 Deploy.** `scripts/deploy.sh` (never-overwrite merge).

## Progress / evidence
- T1: rules_tests 19/19 suites, 0/36673 checks failed (RED observed first: 23 failing checks); Release DLL builds; Soman case 2580 cats; commit 21f4b69. Deployed config NOT edited (Kenshi running): T4 must add catsPerLevel 90->32 / catsCeiling 28000->10000 set-if and the premiumMaxMultiplier key.

## Next step
T1.
