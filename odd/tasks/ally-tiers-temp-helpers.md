# Feature: ally-tiers-temp-helpers

Locator: `odd/tasks/ally-tiers-temp-helpers.md` · Engram mirror: `odd/ally-tiers-temp-helpers/tasks` (pending: engram project is ambiguous in this workspace)
Branch: `feat/ally-tiers-temp-helpers` (from `main` @ 4b9cb9e)

## Objective
Three features the user requested on 2026-10-03, all in the Balanced preset (Cheat is unchanged):
1. **Allied NPCs by level**, with friendship discounts.
2. **Temporary squad member** for allies of level 80 and above.
3. **Free mercenary-like helper** that the player cannot control.

Every one of them must tell the player what is going on, through the recruit popup or the head notice (`src/head_notice.h`), so nothing is left hanging.

## Personality groups (from `src/temperament.h`)
| Group | Temperaments |
|---|---|
| Good | Honorable, WarmKind |
| Neutral | Normal, Dumb, Fearful, Brave |
| Greedy-like | Traitorous, Smart |
| Bad (cruel / psychopath) | ColdCruel, Maniacal |

The group mapping lives in one pure function and is documented in REQUISITOS.

## Rules (Balanced, `relation >= friendlyRelation`)
**By NPC level** (`getRoughLevel`); the player level is the active squad leader's:

| NPC level | Who | What happens |
|---|---|---|
| 1-20 | Good | Joins free, using today's ally roll. |
| 1-20 | Others | Pays cats (personality price as today), discount up to 20%. |
| 20-50 | Everyone | Pays cats, friendship discount up to 20%. |
| 50-80 | Everyone | Pays cats, friendship discount up to 10%. |
| 80-100 | Greedy-like | Pays cats, discount up to 10%. |
| 80+ | Good / Neutral | Temporary member (feature 2). |
| 80+ | Bad | Refuses. The duel tier is a later feature. |
| 100+ | Greedy-like | Refuses. |

**Friendship discount**
- `discount = min(tierMax, relation/100 * allyDiscountScale)`.
- Applied through `PriceInputs.factionMultiplier` in `character_facts.cpp`. In Balanced that multiplier is 1.0 today, so nothing is counted twice.

**Temporary member**
- Requirements: an ally of level 80 or more, Good or Neutral, and close in level (`npc - leaderLevel <= closeLevelGap`, default 15).
- They join for a random number of days, `tempMemberMinDays..tempMemberMaxDays` (3..10).
- The expiry day and the home faction are persisted in the character save, through `saved_ints` plus a string field.
- Mistreatment strikes come from hunger, being attacked by the squad, or being knocked out by the squad.
- After 3 strikes, or when the time is up, they leave. The leave uses KEP's dismiss sequence (`KenshiExtensionPlugin/.../CharacterExtension.cpp:72-108`).
- Head notices cover the warnings and the farewell. The popup states "por N días".

**Helper**
- A low-chance event: an allied NPC of higher level than the player, Good or Neutral, walks up to the player through the submission director's approach pattern and offers to help for N days, for free.
- Mechanism: the native mercenary "AI contract", with no cost. Free precedent: "Escaping slave- work together civilian" (`14191-Dialogue (4).mod`). Bodyguard is `5090-gamedata.base`, with val0 = hours.
- The NPC stays in its own faction and cannot be controlled.
- This needs an in-game proof first: a debug trigger plus logs. If the native contract cannot be driven, report it and stop rather than invent.

## Constraints
- Isolated SOLID modules, no patch-on-patch.
- Pure rules are tested RED first.
- Native engine behaviour wherever it exists.
- Check sibling repos first: TotalSparring, KEP, TotalPredator.
- Toolset v100.
- `config.json` / `messages.json`: only add keys.
- Mockup before new UI.
- Artifacts in English.

## Tasks
- [x] **T1 Ally tiers + friendship discount.**
  - Pure `ally_tier_rules`, `personality_groups`, route integration, popup texts, config and Mod Hub.
  - Route: delegated (writer trigger: 2+ non-trivial files).
  - Commit: `8177ecd feat(allies): level tiers with friendship discounts in Balanced`.
- [ ] **T2 Temporary member (80+).**
  - Persisted expiry and home faction, mistreatment monitor, native leave, head notices, and the popup "por N días".
  - Route: delegated.
- [ ] **T3 Free helper (native contract).**
  - Proof of concept first, with a debug trigger.
  - Then the offer event, the popup, and head notices.
  - Route: delegated.
- [ ] **T4 Docs + deploy.**

## Progress / evidence
**T1 (done, 8177ecd).**
- Tier boundaries: every cut level belongs to the tier it starts (`<20` low, `20-49`, `50-79`, `80-99`, `100+`), config `allyTier*From*Level`.
- Paid allies do not roll: paying is the gate. The discount reaches the price only through `PriceInputs.factionMultiplier` (`character_facts.cpp` `AllyFriendshipMultiplier`, free world only); route and price share `DecideBalancedAllyTier`.
- Temporary allies join with `Verdict::temporary` / `temporaryDays` (seeded, `kTempDaysSalt`), and `CompleteOffer` calls `OnRecruitedTemporarily(Character*, int days)` (`src/temp_member.h`, log only) for T2.
- FCS-dialogue accept path (`ConsumePendingRecruit`) does not call the hook; T2 should if it matters (dialogue ids are empty by default).
- RED: the rules tests built with stubs showed 5 failing suites (route_decisions, readout_model, offer_view, personality_groups, ally_tier_rules), 1173 failed checks.
- GREEN: `rules_tests.exe` -> 36/36 suites passed, 0/63005 checks failed.
- Plugin: `MSBuild TotalRecruit.vcxproj Release x64` -> exit 0, no warnings from `src/` (only the existing KenshiLib header warnings).
- In-game: pending (QA.md 12.24).

## Next step
T2 (temporary member: fill `OnRecruitedTemporarily`).
