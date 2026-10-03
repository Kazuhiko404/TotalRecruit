# Feature: haggle-and-daily-roll

Locator: `odd/tasks/haggle-and-daily-roll.md` · Engram mirror: `odd/haggle-and-daily-roll/tasks` (pending: engram project is ambiguous in this workspace)
Branch: `feat/haggle-and-daily-roll` (from `fix/recruit-gating-and-popup` @ 0c9f671)

## Objective
Two additions the user approved on 2026-10-03:
1. **Daily roll seed.** The join roll stays stable (clicking again changes nothing) but re-seeds once per in-game day, so "come back tomorrow" is meaningful and a shown % means what it says over time.
2. **Haggling.** The recruit popup gets a slider for the cats offer, reusing TotalSparring's working slider, so the player can try a lower price.

## Problem / why
- Today `RollsToJoin` hashes only the persisted id (`src/chance_rules.cpp:200-218`). A 37% NPC is therefore "always yes" or "always no" forever, and the player cannot tell which.
- Prices are take-it-or-leave-it. The user wants a negotiation step.

## Design (defaults; all values are named config keys with Mod Hub controls)
**Daily roll**
- The roll key becomes `hash(id, dayIndex)`, where dayIndex = floor(game hours / 24).
- Peek (the card) still never rolls.
- The stable-within-a-day property is kept, as validated by the user in engram #72: no reroll on click.

**Haggle**
- It applies only to quotes in cats: free-world bribes and the captive cats route. Rescue, mercy, field-yield and surrender are free, so they have no slider.
- The slider runs from `haggleMinFraction` (0.5) to 1.0 of the asking price, in steps of 10 cats. It starts at the asking price.
- **Accept** at the full price works exactly as today.
- **Offer** below the asking price makes one haggle attempt. The acceptance chance is:
  `100 - discount% * haggleDiscountWeight + leverage`
  - leverage = the speaker's best of Strength, MeleeAttack and Toughness versus the NPC's level, clamped.
  - Personality shifts it: fearful accept more easily, proud and honourable less.
  - It uses the same daily seed as the join roll.
- **Accepted:** pay the offered price.
- **Refused:** the NPC is offended. For the rest of the day, haggling is locked and the asking price rises by `haggleOffenceMarkup` (+20%).
- The purse checks apply to the offered amount: shown in red, Accept hidden when it is unaffordable.
- The popup shows the live odds next to the slider ("Probabilidad de aceptar: N%").

## Constraints
- Isolated SOLID modules, no patch-on-patch.
- Port TotalSparring's slider instead of inventing one.
- Mockup first (`ui-mockup/`), kept 1:1 with the C++.
- Toolset v100: no C++11.
- `messages.json` and `config.json`: only add keys.
- Artifacts in English; player text in Spanish through `messages.json`.

## TDD
- Runner: `tests/rules_tests`.
- Day-seed and haggle math are pure: RED first, then GREEN.
- The slider and popup wiring are engine-bound: build, log, and QA.md.

## Tasks
- [x] **T1 Daily roll seed.** (8038b11)
  - Pure `RollsToJoin(key, day, pct)`.
  - The day index comes from game hours, using the same hold-clock time source.
  - Tests first.
  - Route: delegated.
- [x] **T2 Haggle rules (pure).** (d3ecd52)
  - A new `haggle_rules` module:
    - odds;
    - slider bounds and step;
    - offence markup;
    - lock state per id per day.
  - Config keys and Mod Hub controls.
  - Tests first.
  - Route: delegated.
- [ ] **T3 Haggle mockup.**
  - Slider row in the popup states:
    - affordable;
    - offer below ask, with live odds;
    - unaffordable offer;
    - offended / locked.
  - Route: delegated.
- [ ] **T4 Slider port + popup wiring.**
  - Port TotalSparring's slider into a TotalRecruit module.
  - Add an OfferView slider section.
  - Offer button flow: haggle → pay / offend → popup refresh.
  - Route: delegated.
- [ ] **T5 Docs + deploy.**

## Progress / evidence
- **T1** (delegated; 8038b11): `RollsToJoin(haveKey, key, dayIndex, pct)` = `DailyRoll(key, day, kJoinRollSalt, pct)`; seed = `Fmix32(HashKey(key) ^ Fmix32(day * 0x9E3779B9 ^ salt))`. Day source: `MintRollKey` reads `SafeGameHours` (same clock as the hold clock) into `RollFact.dayIndex = DayIndexFromHours(h)`; no clock means no key, so the roll refuses. Peek still mints no key. RED: test build failed on the new API (C2660); GREEN: rules_tests 26/26 suites, 0/49217 checks failed; plugin build clean, no warnings in touched files. Docs: REQUISITOS R0.3, DEVELOPING table + test note, QA C8.
- **T2** (delegated; d3ecd52): `haggle_rules` (bounds, clamp, odds, leverage, temperament shift, `ResolveHaggle` under `kHaggleRollSalt`, `OffendedAsk`) and `haggle_ledger` (`SessionHaggleLedger()`, cleared in `load_boundary`). Config keys on `Prices` (`haggleEnabled`, `haggleMinFraction`, `haggleDiscountWeight`, `haggleLeverageWeight`, `haggleOffenceMarkup`) with clamps, Mod Hub controls after the price controls, keys added to the shipped config.json. Markup wired once: `ApplyCatsOffer(..., HaggleFact, prices)` in route_cats.cpp; `HaggleFact` read with `SafeFindCharKey` in `GatherFreeExtra`/`GatherCageExtra`. `SafeSpeakerBestStat` added to character_facts. RED: C1083 (missing header); GREEN: rules_tests 27/27 suites, 0/61180 checks failed; plugin build clean. Size: ~826 changed lines, about half of them tests; above the 400-line heuristic because tests and config plumbing ship with the rules.
- Personality shifts are fixed constants in haggle_rules (Fearful +15; Brave/ColdCruel/Maniacal -10; Honorable -15; no "proud" temperament exists), not config keys: a possible R0.4 follow-up if the user wants them tunable.

## Next step
T3 (haggle mockup in `ui-mockup/`).
