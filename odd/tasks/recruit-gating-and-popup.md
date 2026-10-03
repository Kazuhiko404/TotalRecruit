# Feature: recruit-gating-and-popup

Locator: `odd/tasks/recruit-gating-and-popup.md` · Engram mirror: `odd/recruit-gating-and-popup/tasks` (pending: engram project is ambiguous in this workspace)
Branch: `fix/recruit-gating-and-popup` (from `feat/price-readout-button-tweaks` @ f21474e)

## Objective
Fix the user's 2026-10-03 in-game feedback:
- NPCs join too easily.
- The cost and refusal outcomes arrive as notifications instead of a styled popup.
- The readout card never appears.

## Problem / why
**A. Easy recruit.**
- Guard Gwynnever joined through field-yield with `defeats=2/2`, yet she was never beaten that session.
- `PersistedDefeatFloor` (`src/defeat_rules.cpp:5-12`) turns the engine memory tag `LT_SQUAD_LOST_TO_ME_ONCE` into `max(1, defeatsToBreak=2)`. The engine writes that tag itself.
- Field-yield runs before the free-world loyalty and cage checks.
- The fixed per-id roll makes her 37% an "always yes".
- `NoteIfDown` on every right-click, with only a 0.5 h debounce, can count one KO twice.

**B. Popup.**
- `PresentOffer` returns false for refusals and unaffordable prices (`src/recruit_core.cpp:295-298`). `AttemptRecruit` then sends a `TellPlayer` notification instead (:577).
- The window itself is stock Kenshi (`src/offer_popup.cpp`), and its text is hardcoded.

**C. Card.**
- The card is never built. `EnsureResources` never logs, so `Present` (`src/readout.cpp:159-186`) exits early. The most likely cause is that the hand-written world→screen projection in `src/readout_target.cpp:61-90` fails silently.
- The card (EvaluatePeek, which never rolls) says "not yet" while the click (Evaluate) says "joins".
- "dejalo sin comer" is shown even for NPCs who are not caged.

## Scope (authorized by the user, 2026-10-03: "revisar y darme un analisis... sigue")
- Persist per-character player defeats the TotalSparring way: a stable id plus a count stored in the character's save state, and one defeat per KO episode. Drop the engine tag as a source.
- Field-yield respects loyalty, cages and the pestering lockout.
- One styled recruit popup for every Hablar outcome:
  - info and amount;
  - insufficient money shown in red, with Accept hidden;
  - Accept / Cancel;
  - mockup style.
- Fix the card so it appears on selection, and make it tell the truth.

## Constraints
- Isolated SOLID modules, no patch-on-patch.
- Reuse TotalSparring's patterns:
  - `phantom_ko_rules` (seen-standing rule);
  - `fighter_down_state` (down predicate);
  - `duel_popup` flat widgets.
- TotalRecruit already ports the identity layer (`character_identity.cpp`, `identity_registry.h`).
- Never overwrite user config edits. Deploy only adds keys; removed keys are ignored by the loader.
- Mockup first (`ui-mockup/`), kept 1:1 with the C++.
- Toolset v100: no C++11.
- Artifacts in English. Player text goes through `messages.json`.

## TDD
- Runner: `tests/rules_tests`.
- Pure rules (the episode rule, the route changes, the readout line choice): RED first, then GREEN.
- Engine-bound code (save hooks, popup, anchor): build, log, and QA.md.

## Delivery
- Commits only, one work-unit commit per task.
- RDD off.

## Tasks
- [x] **T1 Defeats the TotalSparring way.**
  - Per-id player-defeat count in the character save (`idata["total_recruit.player_defeats"]` in `character_identity.cpp` Serialise/Load).
  - New pure `defeat_episode_rules`: one defeat per KO episode, re-armed only once the NPC has been seen standing.
  - Delete the engine-tag floor (`PersistedDefeatFloor`, `EffectiveDefeatCount`, `SafeHasFloor`/`SafeWriteFloor`) and its config keys.
  - Route: delegated.
- [x] **T2 Field-yield gating.**
  - `DecideFieldYield` refuses for someone else's cage and for the ignoring/pestering lockout.
  - Military/law loyalty becomes a chance penalty.
  - Tests first.
  - Route: delegated.
- [x] **T3 Mockup of the unified recruit popup.** States to cover:
  - joins;
  - can't afford (red, no Accept);
  - loyalty/honour refusal;
  - remedy;
  - mercy;
  - rescue.
  - Route: delegated.
- [x] **T4 Shared ui_style + recruit popup.**
  - Extract the palette and fills from `readout_ui.cpp` into `ui_style`.
  - Port the TotalSparring flat button and red reason widgets.
  - Rebuild `offer_popup` from an `OfferView`.
  - Always open the popup; remove the `TellPlayer` fallbacks.
  - Move the popup text into `messages.json`.
  - Route: delegated.
- [ ] **T5 Card fixes.**
  - Anchor via `UtilityT::nPX`, with a log line once per exit reason.
  - Remove the Hablar pin, since the popup now carries the info.
  - Neutral line when a rolled route has no peek key.
  - "dejalo sin comer" only when the NPC is in the player's cage.
  - Field-yield gets its own label.
  - Route: delegated.
- [ ] **T6 Docs + deploy.** README, DEVELOPING, QA, then `bash scripts/deploy.sh`.

## Progress / evidence
- **T1** (delegated; writer trigger: 2+ non-trivial files) -- `78d11f4`. RED: 34 failing checks in `defeat_episode_rules`, `defeat_ledger` and `route_rules` (stubs). GREEN: 24/24 suites, 0/36857 failed. `TotalRecruit.vcxproj` Release x64 rebuild: no warnings in `src/`.
  - New pure `defeat_ledger` (CharKey -> count) and `defeat_episode_rules` (seen-standing rule, `CountsForPlayer`).
  - `character_identity` writes `idata["total_recruit.player_defeats"]` next to the id (always, 0 included) and restores it on load.
  - Removed the engine-tag floor (`PersistedDefeatFloor`, `EffectiveDefeatCount`, `SafeHasFloor`/`SafeWriteFloor`, the I4 intent logging), the 0.5 h debounce, and the config keys `submissionDefeatDebounceHours` / `submissionPersistedFloorCount`. The loader reads only known keys, so old configs stay harmless.
- **T2** (delegated) -- `a4595d8`. RED: 8 failing checks in `route_decisions`. GREEN: 24/24 suites, 0/36877 failed. DLL build is clean.
  - Shared `DecideAccessRefusal` / `ServesStandingOrganisation` in `route_free`, plus an `AccessFact` used by both extras.
  - New `submissionFieldYieldLoyaltyPenalty` (default 25, clamped 0..100) with its Mod Hub control.
- **T3** (delegated) -- `126db79`. `ui-mockup/readout.html` section 5 (`css/popup.css`, `js/popup_data.js`, `js/popup.js`): seven states (cats, can't afford, loyalty refusal, remedy, mercy, rescue, free join). Checked in Chrome: 540 px client, no child outside the client, no clipped label, no page h-scroll; the Hablar scene centres the popup and the card steps beside it. `node --check` OK on every JS file.
- **T4** (delegated; writer trigger: 2+ non-trivial files) -- `324a27b`. RED: 53 failing checks in `offer_view` (stub). GREEN: 25/25 suites, 0/36973 failed. `TotalRecruit.vcxproj` Release x64: builds, no warnings in `src/`.
  - New `ui_style` (palette, `EnsureResources`, fills, labels, wrap text, TotalSparring `FlatButton`); `readout_ui` uses it (no visual change).
  - New pure `offer_view` (`BuildOfferView`, `BuildMercyOfferView`); `offer_popup` rebuilt from it (Kenshi_WindowCX frame, mockup content, measured text). Stock `Kenshi_EditBox` / `Kenshi_Button1` chains and the 48-char height guess deleted.
  - `PresentOffer` always opens the popup; the no-window branch and the `TellPlayer` fallbacks in `AttemptRecruit` are gone. `money.short` stays only for the re-check in `SettlePrice`. `DescribeOffer` replaced by `OfferWording` + `messages.json` `popup` block (added only).
- In-game QA still pending: QA.md 3.2, 12.13 (K1-K7), 12.14 (Y1-Y5) and 12.15 (P1-P12).

## Next step
T5.
