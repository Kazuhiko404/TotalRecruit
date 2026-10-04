# Handoff: TotalRecruit, 2026-10-03

Written for the next Claude session. Read this whole document first, then the feature documents it names.

## Hard rules
- Only touch the repo the user is talking about.
- Never branch, switch, merge or commit in `F:\Proyectos\Kenshi-mods\TotalSparring` (the main checkout). Another session works there.
- TotalSparring work happens only in the worktree `F:\Proyectos\Kenshi-mods\TotalSparring-worktrees\public-api` (branch `feat/public-api`), and only when the user says so.
- Never use CodeGraph in this workspace. Use plain reads and engram.
- Deployed `config.json` and `messages.json` are hand-edited by the user. `scripts/deploy.sh` only adds keys. Change a default's deployed value only through `--set-if KEY OLD NEW` while it still equals the old shipped default.
- Isolated SOLID modules, no patch-on-patch. Pure rules are tested RED then GREEN in `tests/rules_tests`.
- Toolset v100: no C++11.
- Keep each file's line endings: CRLF working tree. Files created with the Write tool come out LF and must be converted to CRLF. `TotalRecruit.vcxproj` is UTF-8 with BOM.
- Never use bash heredocs or `sed -i` for edits. Use the Edit tool or Python scripts written with the Write tool.
- UI changes: update `ui-mockup/` 1:1. Player text goes in `messages.json`. Artifacts are in English.
- Check sibling mods for proven engine code before inventing: TotalSparring, TotalLoot, TotalPredator, KenshiExtensionPlugin (KEP).
- Build and test:
  `MSBUILD="/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"`
  `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
  `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- Deploy with `bash scripts/deploy.sh` (it refuses while `kenshi_x64.exe` runs). Commit the DLL copies after a deploy, then fast-forward `main`.

## Repo state
**TotalRecruit**
- `main` @ `c6a3f2a` is deployed. It contains:
  - prices and haggle;
  - the readout card and head notices;
  - the cage commands;
  - animals that need food;
  - the submission event;
  - ally tiers, temporary members and helpers;
  - the card hidden while a dialogue is open.
- `feat/ts-duel-bridge` (commits `0cd51c5`, `0cd71ff`, plus this handoff) is built and tested (40/40 suites) but **not deployed and not merged**. It adds:
  - the 80+ ally "RETAR A DUELO" button through TotalSparring's API;
  - recruiting the NPC on a duel win;
  - the duel record on the popup and the card.
  - Its doc is `odd/tasks/ts-duel-bridge.md`, and its QA is QA.md 12.28.

**TotalSparring**
- Worktree `TotalSparring-worktrees/public-api`, branch `feat/public-api` (commits `12c91ad`, `f6ec581`, based on `feat/duel-chance-card` @ `0affade`).
- It adds `src/api/` with an exported `TS_GetApi` (v1: `GetDuelStats`, bout listener, `OpenDuelPopup`, `IsReady`). Duel logic is untouched. Tests pass: 155/155.
- **Not deployed and not merged.** Ask the user before deploying TotalSparring or merging it, and coordinate with the other session's `feat/duel-chance-card`.
- `docs/PUBLIC-API.md` explains the API.

**Feature docs:** `odd/tasks/*.md` (rebalance-and-ui, price-readout-button-tweaks, recruit-gating-and-popup, haggle-and-daily-roll, cage-animals-submission, ally-tiers-temp-helpers, ts-duel-bridge). The engram mirror is pending for all of them, because the engram project name is ambiguous in this workspace. Ask the user which project to use.

## Open issues from the user's play test (fix these first)

### 1. Head notices stay too long
- **Evidence:** the screenshot shows "Te acompanare 5 dias" over a temporary member and it "never goes away".
- **How it works:** `src/head_notice.cpp` `HeadNoticeTick` does expire notices (`g_board.Expire(g_clock)`).
  - The default is `headNoticeSeconds = 300` (`src/config.h:691`, clamped by `ClampHeadNoticeSeconds` 10..3600).
  - The clock counts real seconds and freezes while the game is paused.
  - Squad notices (`RaiseSquadHeadNotice`) are never dismissed by selecting the character.
- **Fix:**
  - Default `headNoticeSeconds` to 90.
  - Add `--set-if headNoticeSeconds 300 90` to `scripts/deploy.sh`'s config merge.
  - Consider shorter lifetimes for one-line informational notices: join, strike, farewell around 20-30 s. "Wants to talk" keeps the configurable value. Do this with a per-notice duration argument (`RaiseHeadNotice(npc, text, seconds)`), defaulting to the config value.
  - Update tests (`head_notice_rules`), mockup and QA 12.23.

### 2. Free helper does not escort, gets lost, does not fight
- **Evidence:** `RE_Kenshi_log.txt`. Helper "Bnui":
  - It is stuck on the last escort rung: `engine task now N on rung walk (MOVE_CUS_ORDERED)`.
  - BODYGUARD and FOLLOW_PLAYER_ORDER did not hold for an NPC without a contract.
  - `fights for the leader against ...` is logged, but the user sees no real help in fights. It stays far away, doesn't run after the squad, and gets lost.
- **Code:**
  - `src/helper.{h,cpp}`: a sweep every 5 s and an escort every 3 s.
  - `src/helper_rules.{h,cpp}`: the escort ladder (`NextEscortRung`), `ShouldAssist`.
  - It is mechanism B, code-driven. Mechanism A, the native contract, was rejected because the local KenshiLib has no `AI/Blackboard.h`.
- **The user wants the native guard/bodyguard AI package**, like hired mercenaries.
- **Research already done:**
  - Native hire is a dialogue line with an "AI contract" reference to an AI package. Bodyguard is `5090-gamedata.base`; its val0 is hours (48 = "Two days").
  - A free precedent is "Escaping slave- work together civilian" (`14191-Dialogue (4).mod`).
  - Mercenaries stay in their own faction. `Platoon::blackboard->contractJob->missionEmployer` is the player (KEP `KenshiExtensionPlugin/src/bugfix/UniqueCharacterFix.cpp:406-414`).
  - KEP replaces packages with `blackboard->replaceAIPackage(GameData*)`, then resets with `clearAllPackages / addFallbackPackages / addPackages` (`kep-mod-tool/PlatoonTool.cpp:156-180`). It reads `contractJob` and `currentPackage` in `kep-mod-tool/SquadInfo.cpp:33-60`.
  - KEP compiles against a NEWER KenshiLib that has `AI/Blackboard.h`.
- **Plan:**
  1. Find KEP's KenshiLib: check its vcxproj include paths, `KENSHILIB_DIR`, or a nuget/vendored copy. Get `Blackboard.h` (+ its RVAs) and confirm the RVAs match the installed `kenshi_x64.exe` version.
  2. If it is available, assign the Bodyguard package to the helper's platoon:
     - `replaceAIPackage(bodyguardPkg)`;
     - set `contractJob` / `missionEmployer` to the player squad leader;
     - restore the original packages on expiry.
     - Alternatively, fire the native contract dialogue line through `src/dialogue_authoring.cpp` / `dialogue_bridge.cpp` if that path can be proven.
  3. If it is not available, improve mechanism B:
     - use RUN speed when far (`npc_motion` `BeginRun`);
     - repath every 1 s instead of 3 s;
     - teleport-free catch-up: issue a move to the leader's position plus an offset;
     - make assistance real with the TotalSparring `fight_starter` sequence (`addOrder FOCUSED_MELEE_ATTACK` + `attackTarget` + `attackingYou`) against the leader's current attacker, every check while the leader is in combat.
     - Log each step.
  - Use the debug trigger to test: `"helperDebugKey": 119` (F8) with a selected NPC.

## In-game QA still owed (QA.md)
- 12.19 cage commands.
- 12.20 animals.
- 12.21 submission event.
- 12.22 caged popup.
- 12.23 head notices.
- 12.24 ally tiers.
- 12.25 temporary members.
- 12.26 helpers.
- 12.27 card hidden in dialogue.
- 12.28 duel bridge (needs both the TotalSparring and TotalRecruit branch builds).

## Next steps, in order
1. Fix issue 1 (head notice lifetime), then deploy TotalRecruit.
2. Fix issue 2 (helper escort): research step first, then implement. Deploy.
3. Ask the user whether to deploy and merge the TotalSparring `feat/public-api` worktree branch and TotalRecruit `feat/ts-duel-bridge` to test the duel tier.
