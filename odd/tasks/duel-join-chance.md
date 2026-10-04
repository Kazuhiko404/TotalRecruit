# Feature: duel-join-chance

Locator: `odd/tasks/duel-join-chance.md` · Engram mirror: `odd/duel-join-chance/tasks` (project `totalrecruit`)
Branch: `feat/native-ai-packages` (continues the duel bridge, `odd/tasks/ts-duel-bridge.md`); fast-forwarded into `main` @ `3554d19` on 2026-10-04 and deleted.

## Objective
User-approved design (2026-10-04): beating an 80+ NPC in a TotalSparring duel no longer recruits for sure in Balanced; it gives a join chance that grows with every head-to-head win. Cheat also offers the duel to 80+ NPCs, and there a win always recruits. Extra task (same day): a temporary member says once per game day how many days they have left.

## Scope
- Cheat: RETAR A DUELO for any person (not an animal) at `allyTiers.eliteFromLevel` (80) or above; a win joins.
- Balanced: `joinChance = min(100, base + perWin * (wins - 1))`, wins = TotalSparring's head-to-head wins of the winning squad member against this NPC (the just-won bout included). Defaults 25 / 20 (25, 45, 65, 85, 100).
- Deterministic roll seeded by the NPC key and the win count (a reload does not reroll).
- Win without a join: info head notice + roller line with the chance and the retry time, then a win cooldown drawn (seeded by the NPC key and the win count) in [min, maxUnder100] game hours for levels 80-99 and [min, max] for 100+. Stored in the existing duel cooldown book (saved with the NPC).
- Loss / draw: unchanged.
- Popup info row "SI LE GANAS" and a card record line carrying the chance.
- Config `duelJoinBaseChance`, `duelJoinChancePerWin`, `duelWinCooldownMinHours`, `duelWinCooldownMaxHoursUnder100`, `duelWinCooldownMaxHours`; Mod Hub rows; mockup rows.
- T-DAYS: daily "Me quedan N dias" info head notice over a temporary member (not on the join day, not on the day the expiry warning fires), `tempMemberDailyNotice`.

## Constraints
- Toolset v100 (no C++11). Isolated modules, pure rules tested RED then GREEN. Mockup before UI. `config.json` / `messages.json`: only add keys. Artifacts in English, player text in Spanish.

## Tasks
- [x] **D1 Mockup**: popup "SI LE GANAS" row (Balanced chance, Cheat "se une"), card record line with the chance, new state "Cheat: an 80+ NPC ...", HUB rows (duel keys, `helperNativeBodyguard`, `tempMemberDailyNotice`), README decision 26, temp member daily notice in section 2. Route: inline (one writer session). Commit `d64713f docs(mockup): duel join chance, Cheat duel and daily days-left notice`.
- [x] **D2 Rules + tests**: pure `duel_join_rules` (chance, seeded roll, win cooldown band and pick, `DecideDuelWin`, `DuelNextWinOdds`); `CanOfferDuel` gains `cheatPreset`; `DuelResolution_Joined` renamed `DuelResolution_Won` with the `winner` key; `SetDuelCooldownUntil`; popup (`OfferDuelInput::join*`, `OfferView::showDuelJoin`), card (`ReadoutInput::duelJoin*`, four catalog lines); config fields. Commit `0b52abb feat(duel): join chance rules for a won duel and the Cheat duel gate`.
- [x] **D3 Engine**: `DuelOfferedFor` Cheat gate (+ `SafeIsAnimal` for Cheat) and `SafeNpcLevel`; `OnDuelWon` reads the winner's wins (`TsDuelStats`), rolls `DecideDuelWin`, joins or sets the win cooldown with the info notice and roller line; `DuelNextWinOddsNow` feeds the popup row and the card; fifth popup info row; config.cpp / Mod Hub / shipped config.json / messages.json. Commit `73f1922 feat(duel): roll the join chance on a won duel, offer the duel in Cheat`.
- [x] **D5 T-DAYS daily days-left notice**: `ShouldAnnounceDaysLeft` / `TempExpiryWarningFallsOnDay` and the step flag in `temp_member_rules` (session-only join day and last day: a reload repeats the day's count once), `tempMember.notice.daily` / `dailyOne`, `tempMemberDailyNotice` (config, Mod Hub, shipped config.json). Commit `31423d1 feat(temp): daily days-left head notice`.
- [x] **D4 Docs**: QA.md 12.25 T12-T14, 12.28 T3/T4/T8/T15 updated, new 12.29 (join chance, Cheat), 12.30 (native helper contract, fallback B, temp leaver wander / walk-back), 12.31 (play-test fixes); DEVELOPING.md rows (`duel_join_rules`, `duel_record_cache_rules`, `level_rules`, `npc_level`, `ai_packages`, `leave_homing(_rules)`, updated `duel_challenge_rules`, `ts_bridge` TsBoutGeneration, `helper`, `squad_leave`, `temp_member(_rules)`, `recruit_core`, `saved_strings`); README and REQUISITOS. Commit: the `docs: ...` commit after `31423d1`.

## Design
- Cheat gate: the Cheat verdict has no tiers and no level (`DecideCheat` never reads who they are), so `DuelOfferFacts::cheatPreset` replaces `allyTiered` for Cheat; `DuelOfferedFor` reads the level with `SafeNpcLevel` (falls back to `verdict.allyLevel`) and, in Cheat only, `SafeIsAnimal` (the Cheat verdict's `isAnimal` is always false). It returns early, without engine reads, when neither Balanced tiers nor Cheat apply.
- Wins: the winner's (side A) head-to-head record against the NPC, the challenger as fallback; unreadable -> 1 (never more chance than earned).
- Seeds: `DailySeed(npc, wins, kDuelJoinRollSalt)` for the roll, `DailySeed(npc, wins, kDuelWinCooldownSalt)` for the cooldown (1/1000 of the band).
- Persistence: the win cooldown goes into the existing `DuelChallengeBook` cooldown map, saved as `total_recruit.duel_cooldown_until` (no new row).
- Card: the chance rides on the record line (two-line card): `readout.line.duel.recordChance` / `recordDrawsChance` / `recordSure` / `recordDrawsSure`; widest fills checked by the catalog test (<= 40 chars).

## Progress / evidence
- D2 RED: `duel_join_rules` stub -> 234 failing checks; then view/model/catalog/duel fields declared but not filled -> `40/44 suites passed, 23/64185 checks failed`. GREEN: `44/44 suites passed, 0/64205 checks failed`.
- D5 RED: stubbed `ShouldAnnounceDaysLeft` / `TempExpiryWarningFallsOnDay` -> 11 failing checks; GREEN `44/44 suites passed, 0/64230 checks failed` (one test hour fixed: hour 1010 is day 42, not 41).
- Plugin: `MSBuild TotalRecruit.vcxproj Release x64` exit 0; only the known KenshiLib header warning C4482.
- `dumpbin`: 149 KenshiLib.dll imports, 0 missing from the installed KenshiLib.dll's 10131 exports.
- `json.load` of shipped config.json and messages.json: OK. `scripts/deploy.sh` adds missing keys only, so the new keys reach a deployed install without changes to the script.
- In-game: pending (QA.md 12.29, 12.25 T12-T14).

## Next step
Deployed in `3554d19 build: ship the duel join chance and daily notice DLL`; `feat/native-ai-packages` was fast-forwarded into `main` (`3554d19`) and deleted. Awaiting the user's play-test of QA.md 12.29 and 12.25 T12-T14.
