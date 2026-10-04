# Feature: ts-duel-bridge

Locator: `odd/tasks/ts-duel-bridge.md` · Engram mirror: `odd/ts-duel-bridge/tasks` (pending, kept by the orchestrator)
Branch: `feat/ts-duel-bridge` (from `main` @ c6a3f2a)

## Objective
User request (2026-10-03): "The duel for the 80+ tier: challenge him to a duel so he joins you. TotalSparring already has the duel logic (it decides friendly vs to-the-death); we only need to keep the counter after the duel and communicate between plugins."

## Scope
- Talk to TotalSparring through its public API v1 (`src/third_party/ts_api.h`, a verbatim copy of `TotalSparring-worktrees/public-api/src/api/ts_api.h`): lazy attach, IsReady, GetDuelStats, a bout listener, OpenDuelPopup.
- Balanced allies at `allyTierEliteFromLevel` (80) and above get RETAR A DUELO next to their tier's answer, plus the head-to-head record (popup row DUELOS, card line "Duelos: 2-1").
- A won duel recruits permanently; a lost or drawn one starts a saved cooldown; spars, undecided bouts and unrelated pairs never count.
- Without TotalSparring nothing changes.

## Constraints
- Toolset v100 (no C++11), POD-only across the DLL boundary, SEH guard on every call into TotalSparring, RAII locks, no foreign call under a lock, nothing done inside TotalSparring's callback beyond copying.
- Isolated modules: pure `duel_challenge_rules`, plumbing `ts_bridge`, engine glue in `recruit_core` (the offer's answers live there).
- `config.json` / `messages.json`: only add keys. Mockup before UI. Artifacts in English, player text in Spanish.

## Tasks
- [x] **T1 Duel challenge through TotalSparring.**
  - Route: delegated (writer trigger: 2+ non-trivial files).
  - Commit: `0cd51c5 feat(duel): challenge 80+ allies through TotalSparring and recruit on a win`.
- [x] **T2 Docs** (README, DEVELOPING, REQUISITOS R4b.6, QA 12.28, this document). Commit: `docs(duel): ...` (the commit after 0cd51c5).
- [ ] **T3 In-game QA** (QA.md 12.28, T1-T16) and deploy (not done: deploy is the user's call).

## Design
- `duel_challenge_rules.h` (pure, with a locked process-wide instance like `submission_tracker`): `CanOfferDuel` is the one predicate (enabled, TotalSparring available, `Verdict::allyTiered`, not an animal, level >= elite cut, no cooldown). `DuelChallengeBook::Resolve`: duel + A won + B is the pending NPC + A is a squad member -> Joined; B won or draw -> Lost and `ceil(now + duelChallengeCooldownHours)`; anything else Ignored (undecided keeps the challenge). Expiry after `duelChallengeExpiryHours`. Any squad member's win counts (TotalSparring's window lets the player switch fighters).
- `ts_bridge`: attach from the frame tick every 2 s (GetModuleHandleA, then a Toolhelp32 scan for `TS_GetApi`), absent after 15 tries; table size and entry points checked; one listener. The callback turns `a`/`b` into CharKeys (`SafeFindCharKey`) and reads "is A mine", then pushes into a 32-entry locked queue; `DuelChallengeTick` drains it the next frame. `ShutdownTsBridge` runs first in `stopPlugin` and unregisters only while TotalSparring's module is still mapped (`GetModuleHandleExA` from the table's address).
- Joined: the NPC is found again by key within 80 m of any squad member (the event's pointers are dead by then), dead -> "murio en el duelo", else `Recruit` (permanent) + squad head notice "Me ganaste. Me uno a ti" + roller line. Lost: head notice "Vuelve cuando seas mas fuerte" + roller line with the hours.
- The record is read on popup open (`PresentOffer`) and, for the card, only when the (NPC, selected squad member) pair changes (`readout.cpp` cache), never on the 2 s refresh.
- Popup: RETAR A DUELO after the extra slot (sumi plate, amber border); info box grows to 4 rows for DUELOS. Mockup section 5 states "Ally 80+ can be challenged ..." and "Cruel ally 80+ ...", section 1 card "Ally 80+ you can challenge"; README decision 25.
- Config: `duelChallengeEnabled` (true), `duelChallengeCooldownHours` (24, 0..720), `duelChallengeExpiryHours` (6, 0.5..168); Mod Hub `duel_enabled`, `duel_cooldown_hours`, `duel_expiry_hours`. Saved int `total_recruit.duel_cooldown_until`.

## Progress / evidence
- RED: rules_tests with a stub `duel_challenge_rules.cpp`, the new view/model/catalog fields declared but not filled -> `36/40 suites passed, 59/63402 checks failed` (readout_catalog, readout_model, offer_view, duel_challenge_rules).
- GREEN: `rules_tests.exe` -> `40/40 suites passed, 0/63412 checks failed`.
- Plugin: `MSBuild TotalRecruit.vcxproj Release x64` -> exit 0, 0 errors; only the known KenshiLib header warnings (C4482, C4091), none from `src/`.
- Mockup rendered in Chrome: RETAR A DUELO row and DUELOS 2-1 / 0-3-1 rows lay out in one button row.
- Size: the feature commit is ~2.3k changed lines (the copied API header, a new pure module with its tests, the bridge and docs-in-code); one coherent behaviour, not split.
- In-game: pending (QA.md 12.28).

## Known limits
- If the NPC who lost the duel is more than 80 m from every squad member when the result arrives, they are not recruited ("Ganaste el duelo, pero tu rival ya no esta cerca para unirse.").
- The card's record is cached per pair: it refreshes on the next selection change after a duel, not instantly.
- The FCS-dialogue accept path does not offer the duel (it never shows the popup).

## Next step
In-game QA 12.28 with TotalSparring's public-api build loaded, then deploy (`scripts/deploy.sh`) when the user decides.
