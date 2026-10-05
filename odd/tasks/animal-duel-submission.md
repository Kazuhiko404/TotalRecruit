# Duelo, derrotas y curas con animales

Localizador: `odd/tasks/animal-duel-submission.md` - espejo Engram: `odd/animal-duel-submission/tasks` (proyecto `totalrecruit`).
Rama: `feat/animal-duel-submission` (desde `main` `5a6609d`).
Ruta: escritor delegado (un solo escritor acotado; cada tarea toca 2+ archivos no triviales).
Relacion: implementa las partes de duelo y sumision de T7 de `odd/tasks/balanced-tuning.md` (ese documento vive en `feat/balanced-tuning`; aqui no se toca).

## Pedido del usuario (literal)
"Retar a duelo puede dar el chance de que se una; para Cheat que sea 100%, lo mismo con curar y dar de comer; para Balanced un chance y el mismo 2/2 que ya tenemos con NPC, aplicar la misma logica; en Cheat tambien si lo vence mi NPC deberia dejarme hablar para que se una."

## Objetivo
Que un animal libre (sin dueno) se pueda ganar como se gana a una persona: retandolo a duelo (Cheat: siempre se une; Balanced: la misma chance que una persona), venciendolo en el campo (Balanced: el mismo 2/2 de la ruta sometido; Cheat: con una derrota basta) y, en Cheat, curandolo o dandole de comer.

## Problema
- `CanOfferDuel` exige `!isAnimal`; Balanced ademas exige `allyTiered`, que un animal nunca tiene (sale por `DecideAnimal`).
- `ChooseEntryRoute` saca a los animales de `Route_FieldYield` (`!f.animal`): dos derrotas no cambian nada.
- `DecideCheat` solo une a un animal alimentado o enjaulado un minuto: curarlo o vencerlo no cuenta.

## Por que
El usuario quiere la misma logica para animales y personas. Cheat es un gesto, no un costo (R1.2): comer de tu mano, una cura o una derrota ya son el gesto. Balanced cobra con chance, como a una persona.

## Alcance
- Animal = `SafeIsAnimal`, sin dueno (mismo predicado que la doma: `BelongsToSomeone` fuera de la jaula del jugador). `exclude.animals` sigue ganando. Un animal no tiene temperamento en estas reglas: cuenta como `Temperament_Normal`.
- Duelo: `duelAnimalsEnabled` (true) y `duelAnimalsFromLevel` (40, `SafeNpcLevel`); filas en Mod Hub.
- Un duelo perdido o un "no se une" nunca vuelve hostil al animal; mismos enfriamientos que una persona.

## Restricciones
- Toolset v100: sin C++11. Lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos solo POD (C2712).
- No se toca el `config.json` desplegado; plantillas `TotalRecruit/config.json` y `TotalRecruit/messages.json` solo con claves NUEVAS.
- Modulos aislados (SOLID), sin parche sobre parche. Las DLL versionadas y `.engram/` no se commitean.
- TotalSparring es solo lectura. Sin deploy, sin push, sin merge.
- Maqueta antes del codigo para todo cambio visible.

## Checklist
- [x] D0 Maqueta: tarjeta y ventana de un animal con RETAR A DUELO, fila DERROTAS de un animal, lineas Cheat "Lo venciste / Lo curaste: te sigue", filas Mod Hub del duelo de animales. Estados nuevos (al final de las listas, sin mover indices): tarjeta `animalDuel`, `animalBeaten`, `animalYield`, `cheatAnimalDefeated`; ventana `animalDuel`, `animalYield`; HUB `duelAnimalsEnabled`, `duelAnimalsFromLevel`; README decision 27. Ruta: escritor delegado. Evidencia: `node --check` de data.js y popup_data.js. Commit `5c24107`.
- [x] D1 Duelo a animales (ambos presets): rama animal en la regla pura `CanOfferDuel` (`animalAllowed`, `animalOwned`, `animalFromLevel`; la rama de personas no cambia); `DuelOfferedFor` lee `SafeIsAnimal` en ambos presets, `IsOwnedAnimal` (nuevo en character_facts: fuera de la jaula del jugador + `BelongsToSomeone`), `exclude.animals`, `animalsEnabled` (solo Balanced) y `SafeNpcLevel`; config `duelAnimalsEnabled` / `duelAnimalsFromLevel` (clamp 1..500), Mod Hub, plantilla. Tarjeta, ventana y menu usan el mismo `DuelOfferedFor` (readout.cpp `AddDuelRecord`, offer_view `ApplyDuelChallenge`). TotalSparring `OpenDuelPopupImpl` -> `DuelPopup::Show` solo pide `IsForeign`: acepta animales (leido, no tocado). Ruta: escritor delegado. Evidencia: RED = C2039 `animalAllowed`, luego 3 checks; GREEN = 51/51, 0/72580; DLL compila. Commit `fe90e80`.
- [x] D2 Duelo ganado -> se une: Cheat siempre, Balanced la misma curva (`DecideDuelWin` sin cambios: no lee temperamento; banda de enfriamiento por nivel, un animal de 40-99 usa la de 80-99); `OnDuelWon` llega a `Recruit()` igual que con una persona. Regla pura nueva `duel_narration_rules` (`DuelNarrationFor(id, animal)`, `DuelRaisesHeadNotice(animal)`): un animal lee `duel.animal.challenged/joined/stays/lost` (lineas de narrador) y nunca levanta cartel sobre la cabeza; una persona sigue igual. No hay estado de doma de sesion que limpiar: la unica marca es el ledger de comidas (persistido), que no estorba a un miembro del grupo. Un duelo perdido no toca relaciones (TotalRecruit nunca las cambia). Ruta: escritor delegado. Evidencia: RED = C1083 `duel_narration_rules.h`, luego stub con 3+ checks fallando; GREEN = 52/52, 0/72619; DLL compila. Commit `0717f2e`.
- [x] D3 Balanced: el 2/2 de sometido para animales libres. `CoreFacts::animalOwned` (solo animales, fuera de la jaula del jugador, `BelongsToSomeone`) -> `RouteFacts::animalOwned`; `ChooseEntryRoute` deja entrar a `Route_FieldYield` a un animal sin dueno (mismos interruptores: `surrenderEnabled`, `countDefeats`, `defeatsToBreak`). Regla pura nueva `DecideAnimalFieldYield` (route_animal_yield.cpp): `JoinChance` con temperamento Normal, `fieldYieldBonus` sin penalizacion de lealtad, misma semilla diaria; si la tirada falla responde la doma (`DecideAnimal`): un animal alimentado pasado la barra se une igual, uno sin alimentar conserva el boton de comida y la barra de barriga bajo `Reason_AnimalYieldNotYet`. Razones nuevas `Reason_AnimalYieldJoins/NotYet` (messages `animal.yield.*`, tarjeta `readout.line.animal.yield.*`); la tarjeta muestra DERROTAS n/2 de un animal libre fuera de la jaula (con la ruta sometido o >= 1 derrota) y las probabilidades al espiar sin llave (`IsLostRoll`). Evento de sometido: no aplica a animales (sigue excluido en `submission_rules` `CandidateSkip_Animal`: el NPC camina y pide unirse hablando); un animal se ofrece por Hablar -> ACEPTAR. El ledger de derrotas ya cuenta animales: el gancho `MedicalSystem::knockout` no filtra animales y la llave es `SafeCharKey` (igual que las comidas). Ruta: escritor delegado. Evidencia: RED = C2065 `Reason_AnimalYieldNotYet` / C2039 `animalOwned` / C3861 `DecideAnimalFieldYield`; luego 6 checks (un test tiraba a 79 con mas estres: corregido para tirar a su propia chance); GREEN = 52/52, 0/72681; DLL compila. Commit `6d956a1`.
- [ ] D4 Cheat: animal libre se une al 100% si comio de tu mano (ya existia), si lo curaste o si lo venciste (ledger de derrotas >= 1).
- [ ] D5 Docs: REQUISITOS, QA, DEVELOPING, README, `odd/system-status.md`, limitacion conocida de TotalSparring (apuesta con animales).

## Criterios de aceptacion
- Un animal libre de nivel >= 40 ve RETAR A DUELO (tarjeta, ventana, menu) en Cheat y en Balanced con TotalSparring; uno con dueno, de nivel < 40, excluido o con el interruptor apagado no.
- Duelo ganado: Cheat se une siempre; Balanced tira la misma chance que una persona (25 + 20 por victoria, semilla estable). Las lineas son de narrador, nunca habla el animal.
- Balanced: un animal libre con 2 derrotas en el campo tira la chance de sometido (temperamento Normal, sin lealtad); con 1 no; con dueno nunca; Cheat no cambia por esto.
- Cheat: comida, cura o derrota unen al animal libre; uno con dueno sigue solo por jaula; personas sin cambios.
- rules_tests pasa entero; TotalRecruit.dll compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- `scripts/deploy.sh --dry-run`: config.json / messages.json solo agregan claves.

## Progreso
- Documento creado antes de tocar el codigo. Linea base: 51/51 suites, 0/72566 checks.

## Siguiente paso
D0 maqueta.
