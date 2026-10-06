# Duelo: Cheat a cualquier nivel y duelos de TotalSparring que cuentan

Localizador: `odd/tasks/duel-any-source.md` - espejo Engram: `odd/duel-any-source/tasks` (proyecto `totalrecruit`).
Rama: `main` (pedido del usuario: "cualquier fix va a main"). Desde `ae94075`.
Ruta: escritor delegado (regla pura + config/Mod Hub + puente de duelos + docs: 2+ archivos no triviales).

## Pedido del usuario
"a los honorables no me deja hablarles o retarlos a un duelo, fijate el log" -> diagnostico (log del 2026-10-05):
- Hablar funciona; Cheat rechaza a una persona libre sin jaula (R1.1, por diseno).
- RETAR A DUELO de TotalRecruit exige nivel >= `eliteFromLevel` (80) a una persona tambien en Cheat (`PersonMayDuel`, `src/duel_challenge_rules.cpp:20-23`); "Recluta Samurai" es nivel ~33.
- El duelo abierto desde el boton propio de TotalSparring se gano pero salio `[duel] bout duel outcome=1 ... -> ignored`: `DuelChallengeBook::Resolve` solo resuelve duelos con desafio pendiente de TotalRecruit (`src/duel_challenge_rules.cpp:128-132`).
El usuario aprobo: (1) Cheat ofrece el duelo a personas desde cualquier nivel; (2) un duelo ganado desde TotalSparring contra un NPC ajeno cuenta como desafio.

## Objetivo
- Cheat: el boton de duelo a personas desde `duelCheatPeopleFromLevel` (nuevo, 1). Balanced sigue en `eliteFromLevel` (80) con aliado por niveles.
- Un duelo de TotalSparring sin desafio pendiente, ganado o perdido por un miembro del grupo contra un NPC que NO es del grupo, se resuelve como si TotalRecruit lo hubiera abierto, siempre que ese NPC pase la misma regla de oferta (`DuelOfferedFor` / `CanOfferDuel`, sin contar el enfriamiento de un desafio que no existio). Interruptor `duelCountExternalBouts` (true). Asi un duelo ganado dispara la oferta proactiva igual que con el boton de TotalRecruit.

## Restricciones
- Toolset v100, sin C++11; lecturas del motor en `TR_TRY/TR_CATCH`.
- Modulos aislados, regla pura con RED -> GREEN en `tests/rules_tests`.
- Plantillas solo con claves NUEVAS. Maqueta para filas Mod Hub nuevas. Sin deploy/push.
- Un duelo entre dos miembros del grupo (sparring propio) nunca cuenta.

## Checklist
- [x] U0 Maqueta: filas Mod Hub `duelCheatPeopleFromLevel` y `duelCountExternalBouts` (al final de `HUB`). Tambien: descripcion de "Duelo: retar NPC 80+", estado `cheatDuel` (nivel 33), README item 29. Evidencia: `node --check` de data.js y popup_data.js OK. Ruta: escritor delegado.
- [x] U1 Cheat a cualquier nivel: `PersonMayDuel` usa el umbral Cheat en Cheat; config, Mod Hub, plantilla. RED -> GREEN. Commit `b6a3133`. RED: C2039 `cheatPeopleFromLevel` no es miembro de `DuelOfferFacts`. GREEN: 56/56 suites, 0/72934. DLL compila. Ruta: escritor delegado.
- [x] U2 Duelos externos: `JudgeExternalBout` (regla pura nueva) + `Resolve(..., externalEligible)`; `DuelBoutRecord::bIsSquadMember` (B ilegible = del grupo); B se evalua con `DuelOfferedWith` y el nivel de aliado "en reposo" (`IsTieredAllyAtRest`, porque tras el KO la ruta es inconsciente); log `[duel] external bout: <name> counts (won|lost)` / `... ignored (<reason>)`. RED -> GREEN. Commit `56f8ff8`. RED: C2146/C4430 (`ExternalBoutFacts` no existe). GREEN: 56/56, 0/72966. DLL compila. Ruta: escritor delegado.
- [x] U4 (commit `15c10e9`; RED: C2065 `kLineDuelRecordNeedWins`, C2039 `duelJoinWinsNeeded`; GREEN: 56/56, 0/73030 tras acortar la linea con empates a "faltan {chance}" por el limite de 40 glifos; DLL compila; maqueta: estado `allyDuelNeedWins`, filas HUB, README 30) Victorias minimas de duelo: `duelMinWinsCheat` / `duelMinWinsBalanced` (1..20, 1). Una victoria por debajo no recluta, no tira y no pone espera (Cheat tambien); desde el minimo Cheat se une y Balanced tira la curva contada desde la primera victoria. La tarjeta dice "Duelos: 1-0, faltan 3 victorias"; la fila SI LE GANAS del popup dice "0% de que se una". Linea `duel.needMoreWins` / `duel.animal.needMoreWins` y cartel `duel.notice.needMoreWins`. Aplica igual a duelos externos (mismo `OnDuelWon`). RED -> GREEN.
- [x] U5 Texto de jaula en Cheat con `cheatAnyCage`: nueva razon `Reason_CheatNeedsAnyCage` (al final del enum) con `cheat.needsAnyCage` (5 lineas neutras) y fila de tarjeta `readout.line.cheat.needsAnyCage` ("Metelo en una jaula"); `DecideCheat` la elige con `cheatAnyCage`; `cheat.needsCage` intacto con `cheatAnyCage=false`; insistencia: no cuenta (default). 7 checks de `route_decisions_tests` pasan a la nueva razon (usan el Config por defecto, `cheatAnyCage` true). RED: C2065 `Reason_CheatNeedsAnyCage`. GREEN: 56/56, 0/73052. DLL compila. Maqueta: estado `cheatDuel` y README 28.
- [x] Extra: `fix(config)` `7797965`: 70 claves de `src/config.cpp` que faltaban en la plantilla, con el default de `config.h` (incluye `submissionDefeatsToBreak: 2`).
- [x] U6 (commit `5eff64d`) Fila SI LE GANAS con victorias faltantes: `OfferDuelInput::joinWinsNeeded` (= `DuelNextWinOdds` `winsNeeded`, puesto por `recruit_core` `DuelInputFor`) y `offer_view` `DuelJoinText`: "Ganale {wins} veces mas" (`popup.info.duelJoinNeedWins`) y el singular "Ganale una vez mas" (`popup.info.duelJoinNeedWin`, no lo alcanza la regla actual: con una victoria faltante la proxima decide y se muestra la chance); un faltante gana sobre "se une". Claves nuevas en `npc_speech.cpp` y `messages.json`. RED: C2039 `joinWinsNeeded` / `duelJoinNeedWin`. GREEN: 56/56, 0/73059. DLL compila. Maqueta: estado de ventana `allyDuelNeedWins` (al final de `POPUP_STATES`), nota del estado de tarjeta, README 31. Ruta: escritor delegado.
- [x] U7 (commit `1bb0de4`) Duelos externos en Balanced con cualquier NPC reclutable: `DuelOfferFacts::externalBout` / `recruitable` / `externalBalancedFromLevel`; `PersonMayDuel` en Balanced acepta el tramo 80+ de antes o, solo para un duelo externo, una persona reclutable desde `duelExternalBalancedFromLevel` (nuevo, 1, 1..500). `DuelOfferedWith(..., externalBout)`: el boton pasa false (sin cambio); `ExternalRivalOfferedUnguarded` pasa true y lee `recruitable` con `RecruitableUnder(npc, NULL, false)` (la lista de exclusion de `IsRecruitable` sin `exclude.unconscious`, el KO es el resultado del duelo). Cheat y animales sin cambio. Config, Mod Hub "Duelo externo Balanced: nivel minimo", plantilla. RED: C2039 `externalBout` / `recruitable` / `externalBalancedFromLevel`. GREEN: 56/56, 0/73073. DLL compila. Maqueta: fila `HUB` al final, README 32. Ruta: escritor delegado.
- [x] U3 Docs: REQUISITOS, QA 12.38 D1-D16 (con lineas de log; QA 12.35 X5 corregido), README, DEVELOPING (6 filas), `odd/system-status.md`. Cubre U1, U2, U4, U5. Evidencia: `deploy.sh --dry-run` = config 74 claves nuevas (4 de duelo + 70 de la plantilla), 0 cambiadas; messages 7 nuevas, 0 cambiadas.

## Criterios de aceptacion
- Cheat: "Recluta Samurai" (nivel ~33) muestra RETAR A DUELO; Balanced no (sigue 80 + aliado).
- Duelo abierto desde TotalSparring y ganado: `[duel] ... -> won`, sigue la oferta proactiva (o la union inmediata si esta fuera de rango).
- Duelo entre miembros del grupo: ignorado. Con `duelCountExternalBouts=false`: igual que hoy.
- rules_tests pasa entero; la DLL compila.

## Progreso
- Documento creado antes de tocar el codigo.
- Commits en `main`: U0 `c2d5ff8`, U1 `b6a3133`, U2 `56f8ff8`, U4 `15c10e9`, config `7797965`, U5 `aac3413`, U3 `ac6c79a`, DLL `959833d`, U6 `5eff64d`, U7 `1bb0de4`, docs U6/U7 (este commit). Sin deploy ni push.
- U6/U7 docs: QA 12.38 D7 y D12 actualizadas, D17-D20 nuevas; REQUISITOS (duelos externos U7, victorias minimas U6), README, DEVELOPING (`duel_challenge_rules`, `offer_view`), `odd/system-status.md`. `deploy.sh --dry-run`: config 1 clave nueva (`duelExternalBalancedFromLevel`), messages 2 nuevas (`popup.info.duelJoinNeedWins`, `popup.info.duelJoinNeedWin`), 0 cambiadas.
- Decisiones: un duelo externo lee el tramo de aliado "en reposo" (`IsTieredAllyAtRest`) porque tras el KO la ruta es inconsciente; la comprobacion de acceso del mundo libre (jaula ajena, insistencia) no se aplica a ese duelo. Una victoria bajo el minimo no pone espera en ningun preset. La fila SI LE GANAS del popup decia "0% de que se una" mientras faltaban victorias; U6 la cambia a "Ganale N veces mas". U7: un aliado 80+ con tramo sigue contando en un duelo externo aunque `duelExternalBalancedFromLevel` sea mayor (no se pierde comportamiento validado).
- Siguiente: deploy (DLL nueva, sin empaquetar en este commit) y QA 12.38 en juego, incluidas D17-D20 (decision del usuario).
