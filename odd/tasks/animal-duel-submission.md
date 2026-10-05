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
- [ ] D0 Maqueta: tarjeta y ventana de un animal con RETAR A DUELO, fila DERROTAS de un animal, lineas Cheat "Lo venciste / Lo curaste: te sigue", filas Mod Hub del duelo de animales.
- [ ] D1 Duelo a animales (ambos presets): rama animal en la regla pura `CanOfferDuel` (`animalAllowed`, `animalOwned`, `animalFromLevel`); hechos reunidos en `DuelOfferedFor`; config + Mod Hub.
- [ ] D2 Duelo ganado -> se une: Cheat siempre, Balanced la misma curva (`DecideDuelWin`); narracion de animal sin dialogo (`duel.animal.*`), sin carteles hablados sobre la cabeza.
- [ ] D3 Balanced: el 2/2 de sometido para animales libres (`Route_FieldYield` + `DecideAnimalFieldYield`), sin penalizacion de lealtad, temperamento Normal; si la tirada falla, la comida sigue siendo otra entrada; fila DERROTAS en la tarjeta.
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
