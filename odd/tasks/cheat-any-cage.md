# Cheat: reclutar presos de cualquier jaula

Localizador: `odd/tasks/cheat-any-cage.md` - espejo Engram: `odd/cheat-any-cage/tasks` (proyecto `totalrecruit`).
Rama: `main` (pedido del usuario: "hace la 1 primero aca en main nomas"). Desde `e6810fb`.
Ruta: escritor delegado (regla pura + hechos del motor + reloj de jaula + config/Mod Hub + docs: 2+ archivos no triviales).

## Pedido del usuario (literal)
"para cheat mode sacar la limitacion de solo reclutar npc o animales de tu celda, tambien permitir reclutar en otras celdas (es cheat mode)"

## Problema
- `DecideCheat` (`src/route_cheat.cpp`) exige `CheatFacts::inMyCage`, que sale de `IsInPlayerCage` (`src/engine_reads.cpp`): jaula de la faccion del jugador o esclavo del jugador. Un preso en la jaula de otra faccion (esclavistas, prisiones de ciudades) responde `Reason_CheatNeedsCage`.
- El reloj de espera (`NoteCageState`, `src/hold_tracking.cpp`) solo corre en jaulas del jugador, asi que el minuto de Cheat nunca empezaria en una jaula ajena.

## Objetivo
En Cheat, cualquier jaula, grillete o secuestro (`SafeInPrison`) cuenta como jaula: el mismo minuto de espera y se une. Balanced no cambia.

## Alcance
- Config nueva `cheatAnyCage` (true por defecto), fila Mod Hub, plantilla `TotalRecruit/config.json` (solo clave nueva).
- `CheatFacts::inAnyCage` (`SafeInPrison`); regla pura: la jaula de Cheat la cumple `inMyCage || (cheatAnyCage && inAnyCage)`.
- Reloj de jaula: en Cheat con `cheatAnyCage`, `NoteCageState` observa cualquier prision; su valor de retorno (lo que usa `hail.cpp`) sigue siendo "jaula del jugador".
- Sin cambios: Balanced, comandos de jaula (Golpear/Noquear solo en jaulas del jugador), rutas de animales libres.

## Riesgo conocido
El preso se une pero sigue encerrado en la jaula ajena: hay que abrir el candado, y los guardias de esa faccion pueden reaccionar como a una fuga.

## Checklist
- [x] C0 Maqueta: fila Mod Hub `cheatAnyCage` en `ui-mockup/` (al final de la lista, sin mover indices). Evidencia: ultima fila de `HUB` en `ui-mockup/js/data.js`, decision 28 en `ui-mockup/README.md`; `node --check ui-mockup/js/data.js` OK. Commit `docs(mockup): add the Cheat any-cage Mod Hub row` (hash en Progreso).
- [ ] C1 Regla + hechos + reloj + config: RED en `tests/rules_tests` (DecideCheat con preso en jaula ajena), GREEN, DLL compila.
- [ ] C2 Docs: REQUISITOS (R1 Cheat), QA (caso nuevo con lineas de log), README, DEVELOPING, `odd/system-status.md`.

## Criterios de aceptacion
- Cheat + `cheatAnyCage`: un preso en jaula ajena se une tras el minuto de espera; uno libre fuera de jaula sigue con `Reason_CheatNeedsCage`.
- Cheat + `cheatAnyCage=false`: igual que hoy (solo jaulas del jugador).
- Balanced: sin cambios. rules_tests pasa entero; TotalRecruit.dll compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`

## Progreso
- Documento creado antes de tocar el codigo.
