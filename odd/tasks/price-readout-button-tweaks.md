# Feature: price-readout-button-tweaks

Locator: `odd/tasks/price-readout-button-tweaks.md` · Engram mirror: `odd/price-readout-button-tweaks/tasks` (project `totalrecruit`)
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
- [x] **T2 Readout on click + cheap cursor hint.** Readout card pinned by the Hablar click
  (computed once, shown alongside the offer, closes with it or on timeout); hover polling only
  when `readoutOnHover` is explicitly enabled (default off); right-click `Evaluate` probe-only;
  cursor hint gets a per-character TTL cache module. Mockup updated.
  - Route: delegated.
  - Check: Release build; rules_tests green; QA.md steps.
- [x] **T3 Hablar button placement and size.** Port TotalLoot `menu_slot` (+ tests),
  `menu_obstacles` (prefix `TR_`) and button style; button 44 x 160, styled like the others.
  - Route: delegated.
  - Check: rules_tests green (slot tests); Release build; QA.md steps.
- [x] **T4 Deploy.** `scripts/deploy.sh` (never-overwrite merge). deploy.sh now migrates catsPerLevel 90->32 and catsCeiling 28000->10000 only when untouched (3deddb5); dry run OK. Real deploy done: `f21474e build: ship the price, readout and button tweaks DLL`. Docs shipped with the feature commits (`21f4b69`, `1401c1f`, `d0a4a7f`, `9e28fa4`, `8fb1afa`).
- [x] **T5 Readout card on selection** (user request 2026-10-03: "que se enable y disable al dar
  click en el NPC, no hover; mejor cuando lo seleccionas"). The card follows the selected non-player
  NPC: per-frame pointer compare for the change, one `EvaluatePeek` when it changes, slow named
  refresh (2 s) for live values. Hablar pin kept beside the offer; after it closes the card stays
  only if the NPC is still selected. Hover path and `readoutOnHover` removed; `readoutOnSelect`
  (default true) + Mod Hub control. Card never takes input and is hidden while a context menu is open.
  Mockup + QA.md updated.
  - Route: delegated.
  - Check: Release build; rules_tests green; QA.md steps.
- [x] **T6 Hablar activates on right-button release** (user request): hold right click on the NPC,
  slide onto Hablar, release -> acts once, like TotalSparring / TotalLoot. Port TotalLoot's tested
  `click_gesture` rule (RED first) and wire pressed + released.
  - Route: delegated.
  - Check: rules_tests green (gesture suite); Release build; QA.md steps.

## Progress / evidence
- T1: rules_tests 19/19 suites, 0/36673 checks failed (RED observed first: 23 failing checks); Release DLL builds; Soman case 2580 cats; commit 21f4b69. Deployed config NOT edited (Kenshi running): T4 must add catsPerLevel 90->32 / catsCeiling 28000->10000 set-if and the premiumMaxMultiplier key.
- T2: rules_tests 20/20 suites, 0/36721 checks failed (RED first: 114 failing talk_hint_cache checks); Release DLL rebuilds with no warnings outside SDK headers; mockup scene measured in Chrome (card 8 px right of the offer); commit 1401c1f. In-game QA pending: QA.md 12.9 H1-H13. readoutOnHover not added to deployed config (shipped config has no readout keys).
- T3: rules_tests 21/21 suites, 0/36796 checks failed (menu_slot suite: verbatim port of TotalLoot's tested resolver, no RED possible; added Hablar-vs-TS/TL characterization case, green on first run); Release DLL builds, warnings only in SDK headers; OverlayMenuTarget removed (no callers); commit d0a4a7f. In-game QA pending: QA.md 12.10 B1-B7.
- T5: rules_tests 21/21 suites, 0/36796 checks failed (engine-bound: no pure part, no RED); Release DLL builds with no warnings outside SDK headers; mockup scenes measured in Chrome (selected card at 54,118; menu scene hides it; Hablar card 959,118 beside the offer); commit 9e28fa4. In-game QA pending: QA.md 12.11 S1-S14.
- T6: RED first (stubbed rule: 8 failing menu_button_release checks), then rules_tests 22/22 suites, 0/36808 checks failed; Release DLL builds clean; commit 8fb1afa. Mechanism ported from TotalLoot click_gesture/context_button_input (same as TotalSparring spar_overlay.cpp:307). In-game QA pending: QA.md 12.12 G1-G7.

## Next step
Shipped (`f21474e`). In-game QA: QA.md 12.9 H1-H13, 12.10 B1-B7, 12.11 S1-S14, 12.12 G1-G7.
