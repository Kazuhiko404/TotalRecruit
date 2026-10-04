# KenshiLib 0.5.0 (vendored)

TotalRecruit builds against this copy instead of the shared `$(KENSHILIB_DIR)` (0.4.0), because it needs 0.5.0's `AI/Blackboard.h`. The other mods keep using `KENSHILIB_DIR`.

- `Include/`, `LICENSE`, `UPSTREAM-README.md`: source tag `v0.5.0`, commit `11cd65d557dea76c6540239acf467e0e06e09cf7` of https://github.com/BFrizzleFoShizzle/KenshiLib (`UPSTREAM-README.md` is the upstream `README.md`, renamed).
- `Libraries/KenshiLib.lib`: from the release asset `KenshiLib_v0.5.0.zip` (https://github.com/BFrizzleFoShizzle/KenshiLib/releases/tag/v0.5.0). md5 `6ec1f2b4b6a269d494317a6fef8f8809`, 4191636 bytes.
- The release `KenshiLib.dll` (md5 `210344fbf1bacfbd94a2644182a679ed`) is byte-identical to the one installed with the game.
- Ogre and MyGUI import libraries still come from `$(KENSHILIB_DIR)\Libraries`.

Files here are byte-identical to upstream (`.gitattributes`: `deps/** -text`). Do not edit them; replace the whole directory to upgrade.
