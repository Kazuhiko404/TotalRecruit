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
- [x] U4 (RED: C2065 `kLineDuelRecordNeedWins`, C2039 `duelJoinWinsNeeded`; GREEN: 56/56, 0/73030 tras acortar la linea con empates a "faltan {chance}" por el limite de 40 glifos; DLL compila; maqueta: estado `allyDuelNeedWins`, filas HUB, README 30) Victorias minimas de duelo: `duelMinWinsCheat` / `duelMinWinsBalanced` (1..20, 1). Una victoria por debajo no recluta, no tira y no pone espera (Cheat tambien); desde el minimo Cheat se une y Balanced tira la curva contada desde la primera victoria. La tarjeta dice "Duelos: 1-0, faltan 3 victorias"; la fila SI LE GANAS del popup dice "0% de que se una". Linea `duel.needMoreWins` / `duel.animal.needMoreWins` y cartel `duel.notice.needMoreWins`. Aplica igual a duelos externos (mismo `OnDuelWon`). RED -> GREEN.
- [ ] U5 Texto de jaula en Cheat con `cheatAnyCage`: nueva razon `Reason_CheatNeedsAnyCage` (al final del enum) con `cheat.needsAnyCage`; `cheat.needsCage` intacto con `cheatAnyCage=false`. RED -> GREEN.
- [ ] Extra: `fix(config)`: todas las claves de `src/config.cpp` en la plantilla (`submissionDefeatsToBreak` y las demas que falten).
- [ ] U3 Docs: REQUISITOS, QA (casos con lineas de log), README, DEVELOPING, `odd/system-status.md`.

## Criterios de aceptacion
- Cheat: "Recluta Samurai" (nivel ~33) muestra RETAR A DUELO; Balanced no (sigue 80 + aliado).
- Duelo abierto desde TotalSparring y ganado: `[duel] ... -> won`, sigue la oferta proactiva (o la union inmediata si esta fuera de rango).
- Duelo entre miembros del grupo: ignorado. Con `duelCountExternalBouts=false`: igual que hoy.
- rules_tests pasa entero; la DLL compila.

## Progreso
- Documento creado antes de tocar el codigo.
