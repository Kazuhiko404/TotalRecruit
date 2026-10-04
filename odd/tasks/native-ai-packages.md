# Native AI packages

Branch: `feat/native-ai-packages` (from `main` @ `eac73f6`).
Route: T1 delegated writer (one bounded writer; vendoring + project + 2+ non-trivial source files).

## Objective
Fix issues 2 and 3 of the 2026-10-03 handoff (`git show feat/ts-duel-bridge:odd/tasks/next-session-handoff.md`, sections 2-3) with the engine's own AI packages:
- a free helper escorts the player and fights for them through the native Bodyguard package, like a hired mercenary;
- a temporary member who leaves at expiry goes back home (or to where they joined) through a native package instead of standing still.

## Problem
- Issue 2 (free helper): `RE_Kenshi_log.txt` shows helper "Bnui" stuck on the last escort rung (`engine task now N on rung walk (MOVE_CUS_ORDERED)`). BODYGUARD and FOLLOW_PLAYER_ORDER tasks did not hold for an NPC without a contract. `fights for the leader against ...` is logged, but in play the helper stays far away, does not run after the squad, gets lost and gives no real help in fights. The current helper is mechanism B (code-driven escort ladder in `src/helper.{h,cpp}` / `src/helper_rules.{h,cpp}`); mechanism A (native contract) was rejected because KenshiLib 0.4.0 has no `AI/Blackboard.h`.
- Issue 3 (leaving temp member): `src/squad_leave.{h,cpp}` ports KEP's dismiss sequence (`setFaction`, `createNewEmptyActivePlatoon("5006-squad.mod")`, clear tasks, `reThinkCurrentAIAction`) but skips KEP's blackboard reset + `addPackages(squadTemplate)`, so the new platoon has no AI package and the NPC idles forever.

## Research verdict
- The installed game (`D:\SteamLibrary\steamapps\common\Kenshi`) runs KenshiLib 0.5.0; its `KenshiLib.dll` is byte-identical to the GitHub v0.5.0 release. It exports every `Blackboard` method we need: `replaceAIPackage`, `_setContractJob`, `addPackages`, `clearAllPackages`, `addFallbackPackages`, `endContractJob`.
- `Platoon::blackboard` is at offset `0xF8` in both 0.4.0 and 0.5.0 headers.
- 0.5.0 `AI/Blackboard.h` cannot be cherry-picked onto 0.4.0: it needs 0.5.0's `AI/AITaskSystem.h` (`TaskRequest`, `TaskEndEvent`). So TotalRecruit moves to a vendored KenshiLib 0.5.0 (`deps/kenshilib-0.5.0/`), leaving the shared `KENSHILIB_DIR` (0.4.0) untouched for the other mods.
- Bodyguard package: `5090-gamedata.base` (`SIG_BODYGUARD`); val0 is hours (48 = "Two days").
- Wandering Squad `5006-squad.mod` references only `2675-gamedata.base` "Wandering from town to town".
- Candidate return packages for a leaving member: `2668` "Go home everybody", `14545` / `14546` travel to target town.

## Scope
- T1: build against vendored KenshiLib 0.5.0 (no behaviour change).
- T2: helper gets the native Bodyguard package with the player as employer; restore the original packages on expiry. Improved code fallback (run when far, faster repath, real assistance) when the native path is unavailable.
- T3: leaving temp member gets a native package reset (squad template / go-home package) and, as fallback, walks back to the saved join position.
- T4: docs, QA.md, deploy.

## Constraints
- Toolset v100: no C++11 in our code.
- Isolated SOLID modules, no patch-on-patch. Pure rules tested RED then GREEN in `tests/rules_tests`.
- Keep each file's line endings; vendored third-party files stay byte-identical to upstream (`.gitattributes` `deps/** -text`).
- Never modify `KENSHILIB_DIR` or the shared KenshiLib copy; never touch sibling repos.
- No deploy, merge or push without the user.

## Checklist
- [x] T1 KenshiLib 0.5.0 build: vendor `Include/` (tag v0.5.0, commit `11cd65d`) and `Libraries/KenshiLib.lib` (release `KenshiLib_v0.5.0.zip`) under `deps/kenshilib-0.5.0/`; point `TotalRecruit.vcxproj` include/lib/forced-include paths there; fix the 0.4.0 -> 0.5.0 compile churn in `src/` without behaviour change; confirm `AI/Blackboard.h` compiles; imports of the DLL all resolve against the installed `KenshiLib.dll`. Route: delegated writer. Commit `dd159ba`. Evidence: 861 vendored files (11.6 MB, lib md5 `6ec1f2b4b6a269d494317a6fef8f8809`); release `KenshiLib.dll` md5 = installed md5 `210344fbf1bacfbd94a2644182a679ed`; Release|x64 Rebuild OK with no `src/` change (only the known `C4482` header warnings); `tests/rules_tests` stays pure (no KenshiLib), 39/39 suites, 0/63304 checks failed; `dumpbin -imports`: 138 KenshiLib.dll imports, 0 missing from the installed DLL's 10131 exports. Throwaway probe (reverted) calling `Platoon::blackboard->replaceAIPackage / addFallbackPackages / addPackages / endContractJob` compiled, linked and resolved against the installed DLL (142 imports, 0 missing). Ogre/MyGUI libs still from `KENSHILIB_DIR`.
- [ ] T2 helper native bodyguard + improved code fallback. Route: TBD.
- [ ] T3 leaving temp member native reset + walk-back fallback. Route: TBD.
- [ ] T4 docs / QA / deploy. Route: TBD.

## Acceptance criteria
- T1: `TotalRecruit.dll` Release|x64 rebuilds against the vendored 0.5.0; rules tests 39/39; every import the DLL takes from `KenshiLib.dll` exists in the installed `KenshiLib.dll`.
- T2: a free helper follows the player at mercenary distance, runs to catch up, and fights the player's attackers; on expiry it returns to its own AI.
- T3: a leaving temp member walks or travels away (home, nearest town or join spot) instead of standing; log `[temp] <name> heading back ...`.
- T4: DEVELOPING.md, QA.md and mockups match; deployed after user approval.

## Checks
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64 -t:Rebuild`
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `dumpbin /imports x64/Release/TotalRecruit.dll` compared with the installed `KenshiLib.dll` exports.

## Progress
- T1 done (`dd159ba`). Engram mirror `odd/native-ai-packages/tasks`: pending (engram project name is ambiguous in this workspace).

## Next step
T2: helper native Bodyguard package.
