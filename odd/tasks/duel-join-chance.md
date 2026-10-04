# Feature: duel-join-chance

Locator: `odd/tasks/duel-join-chance.md` · Engram mirror: `odd/duel-join-chance/tasks` (pending, kept by the orchestrator)
Branch: `feat/native-ai-packages` (continues the duel bridge, `odd/tasks/ts-duel-bridge.md`)

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
- [ ] **D1 Mockup**: popup "SI LE GANAS" row (Balanced chance, Cheat "se une"), card record line with the chance, HUB rows (duel keys, `helperNativeBodyguard`), README decision 26.
- [ ] **D2 Rules + tests**: pure `duel_join_rules` (chance, seeded roll, win cooldown band and pick, Cheat/Balanced decision); `CanOfferDuel` gate for Cheat; card / popup view fields. RED then GREEN.
- [ ] **D3 Engine**: `DuelOfferedFor` Cheat gate + `SafeNpcLevel`; the win path rolls, joins or sets the win cooldown; popup and card show the chance; config + Mod Hub + shipped config.json / messages.json.
- [ ] **D5 T-DAYS daily days-left notice**: mockup first, pure rule in `temp_member_rules`, engine, config + Mod Hub.
- [ ] **D4 Docs**: QA.md and DEVELOPING.md rows for this feature and the deferred T4 / play-test docs of `odd/tasks/native-ai-packages.md`.

## Progress / evidence
- (filled per task)

## Next step
D1 mockup.
