# Golpear: un golpe directo

Rama: `fix/cage-beat-direct-hit` (desde `main` @ `c00c26d`).
Ruta: escritor delegado (un solo escritor acotado; 2+ archivos no triviales).

## Objetivo
Golpear (preso en la jaula propia) quita vida al preso con **un** golpe del personaje seleccionado, en el acto, sin ordenes de combate. El preso se queda en la jaula.

Palabras del usuario: "El comando golpear debe quitarle vida al npc con un golpe del pj, no sacarlo de la jaula y liberarlo para pelear".

## Problema
`cage_commands.cpp` `Issue(Command_Beat, ...)` daba la orden nativa `addOrder(BEAT_CAGE_OCCUPANT)` + `addGoal`. El log prueba que el actor nunca la toma:

```
[cage] Quedo -> Holy Warden beat ordered (task 196, was 0 ...)
... 1,5 s despues ...
[cage] Quedo -> Holy Warden beat via fallback
```

El respaldo (`BeatFallback`, el `fight_starter` de TotalSparring: `clearOrders`/`clearJobs`, `FOCUSED_MELEE_ATTACK`, `attackTarget`, `target->attackingYou`) arma una pelea de verdad que saca al preso de la jaula (la lectura de justo despues muestra `route=free`): los presos se escapan.

## Por que
Golpear es la forma de "ablandar" a un preso (R2.3); una pelea que lo libera rompe la ruta de la jaula entera.

## Alcance
- Regla pura nueva en `cage_command_rules`: cuanto pega el golpe (`BeatBlunt`), el piso de vida de la parte (`BeatHealthFloor`), el tope (`CapBeatBlunt`) y que parte recibe el golpe (`PickBeatPart`).
- `cage_commands.cpp`: Golpear = `LandBeatHit` (lee `CharStats::getTotalAttackDamageFor`, elige el torso, aplica `HealthPartStatus::applyDamage` con solo contundente y tope). Sale el codigo muerto de la paliza: orden nativa, vigilancia y `BeatFallback` / `SafeAttackOrder` / `SafeDropCurrentActivity`. La vigilancia queda solo para Noquear (Noquear sin cambios).
- Firmas `IssueBeat` / `OrderSelectedToBeat` / `CageCommandResult` sin cambios (overlay, GOLPEAR / GOLPEARLO de la ventana).
- Docs: `cage_commands.h`, `config.h`, `defeats.h`, `DEVELOPING.md`, `REQUISITOS.md` R2.3, `QA.md` 12.19 C3, `odd/system-status.md`.

## Restricciones
- Toolset v100: sin C++11.
- Toda lectura del motor detras de `TR_TRY/TR_CATCH` en marcos solo POD (C2712); los `Damages` viven en marcos sin guarda.
- No se toca el `config.json` desplegado; sin deploy, sin push, sin merge.

## Checklist
- [x] T1 regla pura + tests: `BeatBlunt`, `BeatHealthFloor`, `CapBeatBlunt`, `PickBeatPart` (`kBeatDefaultBlunt` = 15, `kBeatHealthFloorFraction` = 0,5, `kBeatTorsoPartType` = 0 = `PART_TORSO`). Ruta: escritor delegado. Evidencia: RED = rules_tests no compila (C3861 `BeatBlunt`, C2065 `kBeatDefaultBlunt`); GREEN = 44/44 suites, 0/64256 checks fallidos.
- [x] T2 motor: `LandBeatHit` en `cage_commands.cpp` (lee `getTotalAttackDamageFor` en un helper sin guarda, elige el torso con `PickBeatPart` sobre `anatomy`, `applyDamage` con `Damages(0, tope, 0, 0, 0)`, y si el motor bajo mas de la cuenta deja `flesh` en el piso). Golpear ya no usa la orden nativa ni la vigilancia; se borraron `BeatFallback` / `SafeAttackOrder` / `SafeDropCurrentActivity`. La vigilancia queda solo para Noquear (sin cambios de comportamiento). Ruta: escritor delegado. Evidencia: TotalRecruit.dll Release|x64 compila sin advertencias nuevas.
- [x] T3 docs: `cage_commands.h`, `cage_command_rules.h`, `config.h`, `defeats.h`, `DEVELOPING.md` (filas `cage_command_rules` y `cage_commands`), `REQUISITOS.md` R2.3, `QA.md` (nota de la seccion 4, 12.19: intro, C3, C3b nuevo, C7, C8, C9), `odd/system-status.md`. Ruta: escritor delegado.
- [x] T4 compilacion, tests y commit de unidad de trabajo (ver Progreso).

Decisiones:
- Parte: la primera `HealthPartStatus` con `whatAmI == PART_TORSO` en `MedicalSystem::anatomy` (en las razas vistas, el pecho antes del estomago); si no hay torso, la primera con `fatal`; si tampoco, falla con log.
- Golpe: todo el daño de ataque del motor (corte + contundente + perforacion) como contundente; un puño no corta, asi que no sangra.
- Piso: -0,5 x `maxHealth()` de la parte. Una parte fatal muere en -max; el torso colapsa bajo cero, asi que el golpe puede noquear pero no matar.

## Criterios de aceptacion
- Golpear baja la vida del torso del preso en un golpe; el preso sigue en la jaula (sin pelea, sin `route=free`).
- El golpe nunca deja la parte por debajo de -0,5 x su vida maxima (nunca mata); solo contundente (no sangra).
- Un noqueo provocado por el golpe se acredita al jugador (`RememberCredit` + `ClaimOrderedKnockoutCredit`).
- Log `[cage] <actor> -> <preso> beat hit (blunt X, part health A -> B)` con `probeOnClick`; fallas siempre.
- Noquear se comporta igual que antes.
- rules_tests pasa entero; TotalRecruit.dll compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`

## Progreso
- Documento creado antes de tocar el codigo.
- T1..T4 hechos en `fix/cage-beat-direct-hit`; commit: `0f042dc` (`fix(cage): golpear lands one direct hit instead of starting a fight`). Sin deploy, sin push.

## Siguiente paso
QA en juego 12.19 C3 / C3b / C5 / C7: que el golpe baje la vida, que el preso no salga de la jaula, que un noqueo por golpe se acredite (`[defeat-hook] knockout fired` + `credited to the player explicitly`). Pendiente fuera de este alcance: `ui-mockup/README.md` y `ui-mockup/readout.html` todavia describen Golpear como `BEAT_CAGE_OCCUPANT`.
