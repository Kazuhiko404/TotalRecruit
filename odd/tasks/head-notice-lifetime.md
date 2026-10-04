# Head notice lifetime

Branch: `fix/head-notice-lifetime` (from `main` @ `c6a3f2a`). Ships without the duel-bridge work.
Route: delegated writer (one bounded writer; 2+ non-trivial files).

## Objective
Head notices go away in a sensible time: 90 s by default for notices that ask the player to act, about 25 s for one-line informational notices.

## Problem
Play test (handoff issue 1): "Te acompanare 5 dias" over a temporary member "never goes away". The notice does expire, but after `headNoticeSeconds = 300` real seconds (frozen on pause), and squad notices are never dismissed by selecting the character. Five minutes reads as "forever".

## Why
A one-line informational notice (join, strike, farewell) only needs to be read once. A notice that asks the player to act (hail "wants to talk", helper offer, submission "approaching") needs longer, but not five minutes.

## Scope
- `headNoticeSeconds` default 300 -> 90 (config ctor, rules constant, comments, shipped `TotalRecruit/config.json`).
- `scripts/deploy.sh` config merge: `--set-if headNoticeSeconds 300 90` (only while the deployed value still equals the old default).
- New key `headNoticeInfoSeconds` (default 25, clamp 5..3600): config read + clamp, shipped config.json (the deploy merge adds missing keys from the shipped file), Mod Hub.
- `HeadNoticeKind { Prompt, Info }` resolved to seconds by a pure rule; `RaiseHeadNotice` / `RaiseSquadHeadNotice` take a kind (default Prompt).
- Info: temp_member join / strike / farewell / mistreated / warning / expiring; helper start / betrayed / farewell / rearmed / expiring.
- Prompt: hail `wantsToTalk`, helper `helperOffer`, submission `approaching`.
- Mockup + QA 12.23.

## Constraints
- Toolset v100: no C++11.
- Pure rules in `head_notice_rules.{h,cpp}`, tested RED then GREEN; engine glue stays thin.
- CRLF working tree where the file is CRLF; deployed config is hand-edited, deploy only adds keys / `--set-if`.
- No deploy, no merge, no push.

## Checklist
- [x] T1 default + migration: default 90 (config.h, head_notice_rules.h, comments, shipped config.json, Mod Hub text), `--set-if headNoticeSeconds 300 90.0` in deploy.sh. Route: delegated writer. Evidence: merge dry-run on a scratch config with 300.0 -> `~ headNoticeSeconds: 300.0 -> 90.0 (still the old default)`; with 120.0 -> `= headNoticeSeconds: 120.0 kept`; `bash -n scripts/deploy.sh` OK.
- [x] T2 info lifetime: `headNoticeInfoSeconds` key (config read + clamp, Mod Hub row, shipped config.json so the merge adds it: dry-run shows `+ headNoticeInfoSeconds`), `HeadNoticeKind` + `HeadNoticeSecondsFor`, `RaiseHeadNotice` / `RaiseSquadHeadNotice` take a kind (default Prompt), info call sites marked. Route: delegated writer. Evidence: TotalRecruit.dll Release|x64 builds.
- [x] T3 tests: `TheConfigDefaultIsNinetySeconds`, `InfoSecondsAreClamped`, `EachKindResolvesToItsOwnSeconds`; `SecondsAreClamped` expects default 90. RED observed (C2065 / C3861 on the missing symbols), then GREEN: 39/39 suites, 0/63304 checks failed.
- [x] T4 mockup + QA: `ui-mockup/js/data.js` (`HEAD_NOTICE.seconds` 90, `infoSeconds` 25, both Mod Hub rows word for word), `ui-mockup/readout.html` callouts (notice, temporary member, helper), `ui-mockup/README.md` items; QA.md 12.23 intro, N1, N8, new N14..N17.

Info (headNoticeInfoSeconds): temp_member join, strike / warning / expiring (StepNotice), farewell, farewell mistreated; helper start, betrayed, farewell, rearmed, expiring.
Prompt (headNoticeSeconds): hail `headNotice.wantsToTalk`, helper `headNotice.helperOffer`, submission `headNotice.approaching` (default kind, call sites unchanged).

## Acceptance criteria
- A fresh config gives 90 s for prompt notices, 25 s for info notices.
- A deployed config with `headNoticeSeconds` 300 becomes 90; any other hand-edited value stays.
- `headNoticeInfoSeconds` is added to the deployed config by the merge.
- rules_tests all pass; TotalRecruit.dll builds; `bash -n scripts/deploy.sh` passes.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- `bash -n scripts/deploy.sh`

## Progress
- Doc created before any source write.
- T1..T4 done on `fix/head-notice-lifetime`; commit: HASH_PENDING (`fix(notice): shorten head notice lifetime and add an info lifetime`).
- Not deployed, not merged, not pushed.
- `DEVELOPING.md` still says `headNoticeSeconds` (300) in the head_notice row: it was outside this task's edit surface.

## Next step
Parent: review, deploy TotalRecruit (`bash scripts/deploy.sh`), commit the DLL copies, fast-forward `main`, then in-game QA 12.23 N8 and N14..N17.
