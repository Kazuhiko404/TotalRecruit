# TotalRecruit: system status, 2026-10-04

A snapshot of everything that is in progress, deployed, pending, risky or still an idea across the whole mod. It was built from a read-only audit of every feature document in `odd/tasks/`, `QA.md`, `REQUISITOS.md`, `TODO.md`, `ESTADO.md`, `DEVELOPING.md`, the session handoff and the workspace brainstorm notes. There is no TODO, FIXME or HACK in `src/`.

Engram project: `totalrecruit` (`.engram/config.json`). Each feature document is mirrored under the topic `odd/<feature>/tasks`.

## 1. Done, awaiting play-test
Branch `feat/native-ai-packages`, feature document `odd/tasks/duel-join-chance.md`. Commits:
- `d64713f` D1 mockup;
- `0b52abb` D2 rules;
- `73f1922` D3 engine;
- `31423d1` D5 daily notice;
- `29e5a14` D4 docs.

QA: 12.29, 12.25 T12-T14, 12.30, 12.31. The tasks:
- **D1 Mockup.** The join chance shown after a duel win.
- **D2 Rules and tests.**
  - Join chance: 25 % base + 20 % per win against that NPC, capped at 100.
  - A deterministic roll, seeded by NPC + win count.
  - Win cooldown: 24-72 h for levels 80-99, 24-120 h for 100+, all configurable in game hours.
- **D3 Engine.**
  - Cheat: the duel is also offered to 80+ NPCs, and a win means an immediate join.
  - Balanced: a win rolls the chance. A miss gives an informational notice and the win cooldown.
- **D5 Daily notice.** A temporary member says "Me quedan N dias" once per game day as a 25 s informational notice. Config `tempMemberDailyNotice`.
- **D4 Docs.** Also closes the deferred T4 of `native-ai-packages`:
  - QA, DEVELOPING and mockup for the native helper and the leaving member;
  - the TotalSparring level, the duel record refresh and the 25 s duel notices;
  - the `helperNativeBodyguard` Mod Hub row.

## 2. Done and deployed, not merged into `main`
TotalRecruit `feat/native-ai-packages`, play-tested by the user:
- `dd159ba` builds against KenshiLib 0.5.0, vendored in `deps/kenshilib-0.5.0/`.
- `e5e5f4b` gives the free helper the native Bodyguard contract (`src/ai_packages.*`), with an improved code fallback.
- `031eb37` makes a leaving temporary member get the native wander package, with a walk back to the join spot as the fallback (`src/leave_homing.*`).
- `511586e` merges `feat/ts-duel-bridge`: the 80+ duel through TotalSparring.
- Play-test fixes:
  - `7cb5c1f`: the card's duel record refreshes after a bout.
  - `1782ffa`: the duel notices are informational (25 s).
  - `7727771`: the NPC level is TotalSparring's overall skill level.
- Deployed DLLs: `ac6ed10`, `fe8e490`.

TotalSparring `feat/public-api` (worktree `TotalSparring-worktrees/public-api`):
- The DLL is deployed in the game, built from `master` plus the API.
- It is not merged, on the user's instruction. The previous game DLL is kept as `TotalSparring.dll.bak-master`.

`main` @ `eac73f6`: deployed head notice lifetime (90 s prompt, 25 s info).

## 3. In-game QA still owed
`QA.md` has no pass/fail markers, so nothing is formally verified. In priority order:
1. **12.6, stable character id across a reload and a relaunch (I4/I5).** This is the highest risk, because the `Character` hooks are unverified.
2. **12.4, money (P1-P9),** and **12.18, haggle (H1-H12).**
3. **12.23-12.28:**
   - 12.23 head notices, including N14-N17;
   - 12.24 ally tiers, 12.25 temporary members, 12.26 helper;
   - 12.27 card hidden during a dialogue;
   - 12.28 duel bridge.
   Several of these were play-tested informally.
4. **The rest:**
   - §1 smoke test S1-S4;
   - 12.1 route order, 12.2 mercy, 12.3 easy mode and the cage clock;
   - 12.5 config writer, 12.7 reasons, 12.8 card;
   - 12.9-12.17 Hablar and card behaviour;
   - 12.19 cage commands, 12.20 animals, 12.21 submission, 12.22 caged popup;
   - §9 R6.3, which needs the Mod Hub slot dump.

No QA rows exist yet for the native bodyguard, the leave homing, B1-B3, the duel join chance or the daily notice. D4 adds them.

## 4. Risks and known limitations
- **Balance.** The new level source changes prices, tiers, chance, haggle, helper lead and animal taming, so balance needs a re-check in play.
- **Native helper.**
  - `packageData` (0x68) and `missionEmployer` (0x48) are read at header offsets, because `AI/AIPackage.h` and `AI/Blackboard.h` cannot be included together.
  - A helper taken out of a squad stays in its own platoon.
  - Fallback B leaves the aggressive combat stance on the NPC.
- **Duel.**
  - A winner more than 80 m away is not recruited.
  - The FCS dialogue accept path offers no duel.
- **Temporary members.**
  - The FCS accept path recruits a temporary ally permanently.
  - Warnings and cooldowns can repeat once after a load.
  - The leaver's walk home is session-only.
- **State not saved.** The hold clock and talk attempts live in memory. A prisoner re-caged outside the 900-unit sweep keeps its clock.
- **Cage orders.** Whether `checkPlayerOrderForProblems` reads the right way round is unproven.
- **Minor.**
  - The popup's "Sometido" tag is not in `messages.json`.
  - The FCS dialogue bridge has never run.
  - There is an include loop between `src/offer_popup.cpp` and `src/mercy.cpp`.
  - `deploy.sh` prints "would be copied" on a real run.
  - "THE Recruit Anyone" overlaps with this mod if it is installed.

## 5. Ideas not implemented
- **Mod Hub (REQUISITOS R6.3/R6.4).** Sections, a preset dropdown and text settings. Blocked on the slot layout dump.
- **Level curve (REQUISITOS R2.1).** Tune the -6 per level curve in play.
- **More config.** Make the haggle personality shifts and the reputation tiers (75/25/5/-5, `src/relation_rules.cpp`) configurable.
- **Save state.** Persist the hold clock and talk attempts in the save.
- **Unique characters.** Per-name rules (`UNIVERSAL-RECRUIT-BRAINSTORM.md` §4.5).
- **Layered config.** A layered INI-style config (brainstorm §8).
- **Messages.** Wire the `refusal.*` and `currency.*` sections of `messages.json` (partly stale).

## 6. Housekeeping
- **Merged branches that can be deleted:**
  - feat/ally-tiers-temp-helpers, feat/cage-animals-submission, feat/haggle-and-daily-roll;
  - feat/price-readout-button-tweaks, feat/rebalance-and-ui;
  - fix/card-hide-on-dialogue, fix/head-notice-lifetime, fix/recruit-gating-and-popup;
  - `list`, which looks accidental.
  `feat/ts-duel-bridge` is contained in `feat/native-ai-packages`.
- **Push is blocked.** `origin/main` is an unrelated squash commit (`6257bd6`). Delivery is the user's call.
- **Stale docs.**
  - `TODO.md` and `ESTADO.md` date from 2026-10-01.
  - REQUISITOS uses R4b.6 twice, and R4b.3 is superseded by the native leave.
  - QA 12.26 still describes the old escort ladder.

## Suggested order
1. Finish D1-D5, deploy, and play-test the duel join chance and the daily notice.
2. Merge `feat/native-ai-packages` into `main` once the user approves.
3. In-game QA in the order of section 3.
4. Branch and doc cleanup.
5. Ideas from section 5, when the user asks.
