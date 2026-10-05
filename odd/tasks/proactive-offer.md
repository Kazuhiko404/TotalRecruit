# Oferta proactiva: el NPC o animal viene a ofrecerse

Localizador: `odd/tasks/proactive-offer.md` - espejo Engram: `odd/proactive-offer/tasks` (proyecto `totalrecruit`).
Rama: `feat/proactive-offer` (desde `main` despues de la linea "Encerrado"; politica: features grandes en `feat/`).
Ruta: escritor delegado (cada tarea toca 2+ archivos no triviales).
Entrega: `exception-ok` (este repo no usa PR: el usuario hace fast-forward de `main` a mano). Pronostico: ~500-700 lineas con tests, maqueta y docs.

## Pedido del usuario (literal, resumido)
"agregar que los animales se unan despues de un duel, para cheat solamente requiere ganarle 1 vez y se ofrece unir, para balanced la misma logica que ya tenemos para los npc [...] agregar un evento que el npc o animal abra el popup de que quiere unirse, no tener yo que hablar de nuevo, porque despues de ganarle en duelo varias veces o curar o darle de comer a un animal sigue igual con agro y me ataca, al menos agregar unos segundos de neutralidad si no se quiere unir, si la tirada es buena que me hable y ofrezca, con las opciones de siempre Aceptar / Ignorar / Rechazar: aceptar se une, Ignorar le da neutralidad por unos 30 s y se va (ver que paquete de IA hace a los npc arrancar lejos), rechazar no hace nada: que el npc o animal haga lo que estaba haciendo, limpiar todos los flags y dismiss el popup."

## Estado de partida (ya en `main`)
- Duelo a animales libres desde nivel 40 en ambos presets; duelo ganado: Cheat se une siempre, Balanced la misma curva (`OnDuelWon`, `src/recruit_core.cpp:1028`). Hoy se une AL INSTANTE sin popup; si la tirada de Balanced falla solo pone enfriamiento (`OnDuelWonStays`): el rival sigue con agro.
- Cheat: animal libre se une con 1 derrota, una cura o una comida (`DecideCheat`); Balanced: 2/2 de sometido para animales.
- Curar (`NoteAnimalTreatment`, `src/animal_healing.cpp`) solo marca el ledger: hay que volver a Hablar.
- Comer: el boton de comida ya esta en el popup y lo reabre (`FeedCaptive` -> `PresentOffer`): no necesita evento.
- Evento de sometido (`src/submission_director.cpp`, `submission_rules`): Idle -> Approaching -> Holding (popup) -> Forgiven -> Done. Solo dispara por derrotas/rendicion, excluye animales (`CandidateSkip_Animal`), no lee el preset. Botones: ACEPTAR (Recruit), PERDONAR (calma `submission.forgiveSeconds` 30 + huida 80 m con `NpcMotion`), RECHAZAR (borra derrotas, enfriamiento de rechazo, `SafeMarkHostile`), X (calma en el sitio).
- No hay paquete de IA nativo de huida conocido (`odd/tasks/native-ai-packages.md`); la huida que ya funciona es `NpcMotion::BeginRun/MoveToPoint` hacia un punto a 80 m (`FleePoint`). Se reusa.

## Objetivo
Despues de un gesto del jugador (duelo ganado, cura, derrota), si la tirada es buena el NPC o animal camina hasta el jugador y abre el popup con ACEPTAR / IGNORAR / RECHAZAR, sin que el jugador tenga que hablarle. Si la tirada es mala, queda neutral unos segundos en vez de seguir atacando.

## Decisiones
- Se extiende el evento de sometido (no un evento nuevo paralelo): una cola de "ofertas pedidas" (modulo puro nuevo) que el barrido atiende antes que los candidatos por derrotas.
- Disparadores: duelo ganado (ambos presets, en vez de unir al instante), cura del jugador (`ApplyFirstAid`/`ApplyDoctoring`), y en Cheat un animal libre con 1 derrota (ya es `willJoin` en `DecideCheat`). La comida no dispara (ya reabre el popup).
- Tirada: la misma decision de siempre (`Evaluate`); nada se recalcula aparte. `willJoin` -> camina y ofrece. No -> calma de `offer.failCalmSeconds` (nuevo, 20 s) sin huida.
- Animales entran al evento: lineas de narrador (como `duel.animal.*`), nunca hablan ni levantan cartel de cabeza.
- IGNORAR = el PERDONAR de hoy (calma 30 s + huida 80 m); solo cambia la etiqueta.
- RECHAZAR = no hace nada: cierra el popup, termina el evento y limpia sus marcas; sin borrar derrotas, sin enfriamiento de rechazo y sin volverlo hostil. Si era hostil antes del evento, vuelve a serlo (lo que estaba haciendo). La oferta pedida se consume: no vuelve a ofrecerse hasta un gesto nuevo.
- Cheat y Balanced comparten el evento; Cheat no tiene esperas extra.

### Decisiones tomadas al implementar (escritor, 2026-10-05)
- D1 Etiqueta IGNORAR: la ventana del sometido usa `popup.button.ignore` (la clave que ya usa la piedad). No se cambia ningun valor de plantilla; `popup.button.forgive` queda sin uso y el campo `OfferTexts::forgive` se borro.
- D2 RECHAZAR cambia para TODO el evento (tambien el de derrotas): se restaura como un abort (`PlanFor`: sin borrar derrotas, sin enfriamiento, hostil solo si lo era). El enfriamiento de rechazo guardado sigue leyendose (rutas), pero el evento ya no lo pone; la fila Mod Hub "Sometido: horas sin reclutar al rechazar" ahora lo dice.
- D3 La calma por tirada mala solo se aplica si el NPC es hostil al escuadron (`PlanRequestedOffer(..., hostile)` -> `RequestedPlan_Nothing`): curar a un vecino amistoso que no se une no imprime nada. Pedido literal: "unos segundos de neutralidad" en vez de que "me ataca".
- D4 Una tirada de rendicion fallida en el barrido de derrotas tambien calma a un hostil (objetivo: "derrota" es un gesto).
- D5 Las ofertas pedidas saltan el enfriamiento de rechazo, ya preguntado, tirada del dia y no vencido; `Evaluate` decide. Solo un si gratis y duradero camina (`JoinsForFree`: sin cats, no temporal), porque la ventana es gratis y ACEPTAR es `Recruit`; un si con precio se calma/ignora y se registra "talk to it".
- D6 Cura de un NPC por la ruta de rescate (`viaRescue`): la peticion se consume, porque la ruta de rescate ya ofrece al despertar (evita dos ventanas). Hablar a mano (`AttemptRecruit`) tambien consume la peticion.
- D7 Un gesto = una peticion: cada duelo ganado es un gesto nuevo; una cura que sigue (el gancho llama cada cuadro) es el mismo gesto mientras no pasen 30 s sin curar. Una peticion no servida expira a los 300 s reales (congelados en pausa). Duelo ganado fuera de 120 m del lider: comportamiento viejo.
- D8 Interruptores apagados (`offerOnDuelWin` / `offerOnHeal`): el duelo vuelve al comportamiento viejo; la cura solo marca el ledger. Las ofertas pedidas no dependen de `submissionEventEnabled` (ese interruptor sigue gobernando el barrido de derrotas).
- D9 `submissionForgiveSeconds` pasa de 300 a 30 ("unos 30 s"): default en `config.h` y plantilla, y `deploy.sh --set-if submissionForgiveSeconds 300 30.0` (solo si el config del usuario sigue en el viejo default).

## Restricciones
- Toolset v100: sin C++11. Lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos POD.
- Modulos aislados (SOLID), sin parche sobre parche; reglas puras con RED -> GREEN en `tests/rules_tests`.
- Plantillas `config.json` / `messages.json` solo con claves NUEVAS; el `config.json` desplegado no se toca.
- Maqueta antes del codigo. Sin deploy, sin push, sin merge.

## Checklist
- [x] P0 Maqueta: popup del evento con ACEPTAR / IGNORAR / RECHAZAR (persona y animal), lineas de narrador del animal, aviso de calma tras tirada mala; filas Mod Hub nuevas. Estados al final de las listas.
- [x] P1 Reglas puras: cola de ofertas pedidas (pedir, consumir, una por gesto), cribado (animal admitido; una oferta pedida salta `NotBeaten`), planes de salida (IGNORAR = perdonar con huida; RECHAZAR nuevo; tirada mala = calma sin huida). RED -> GREEN.
- [x] P2 Disparadores en el motor: `OnDuelWon` pide la oferta en vez de unir (Balanced con tirada mala: calma); ganchos de cura piden la oferta; Cheat animal con 1 derrota entra por el cribado. Animales en la caminata con narrador.
- [x] P3 Config y textos: `offer.failCalmSeconds`, interruptores del evento por disparador, Mod Hub, plantillas (solo claves nuevas), etiqueta IGNORAR.
- [x] P4 Docs: REQUISITOS, QA (casos con lineas de log), README, DEVELOPING, `odd/system-status.md`.

## Criterios de aceptacion
- Duelo ganado con tirada buena (Cheat siempre): el rival camina hasta el jugador y abre el popup; ACEPTAR lo une.
- Duelo ganado con tirada mala (Balanced): deja de atacar `failCalmSeconds` segundos.
- Curar a un animal libre en Cheat: viene y se ofrece sin Hablar.
- IGNORAR: 30 s neutral y se aleja 80 m. RECHAZAR: popup cerrado, nada mas cambia (sin enfriamiento, sin borrar derrotas), vuelve a lo que hacia.
- Hablar a mano sigue funcionando igual. rules_tests pasa entero; la DLL compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- `bash scripts/deploy.sh --dry-run`: solo claves nuevas.

## Riesgos
- Un animal con IA propia puede no sostener la caminata o la quietud en el popup: se confirma solo jugando (QA).
- El rival del duelo tiene que seguir a 80 m (`kDuelSearchMeters`) y el evento aborta a 120 m.

## Progreso
- Documento creado antes de tocar el codigo.
- P0 `61acb97` docs(mockup): estado `submission` con IGNORAR y RECHAZAR nuevo; ventanas nuevas al final ("Duel rival walks up...", "Animal walks up..."); `OFFER_EVENT` (lineas de narrador, calma); escena "Proactive offer" al final de la barra; 3 filas HUB al final. `node --check` data.js / popup_data.js: OK.
- P1 `a3dba00` feat(offer): `offer_request_rules` (cola, gesto, plan, `JoinsForFree`, `RequestSkipConsumes`), `offer_narration_rules`, `submission_rules` (`requested`, animales candidatos, `SubmissionBeaten`, `JudgePeek` de animales, `Outcome_RollFailed`, RECHAZAR nuevo, `ForgivenWindowSec` de 3 args), `BuildAnimalSubmissionOfferView` e IGNORAR. RED: el build de rules_tests fallo (C1083 headers nuevos, C2039 `requested` / `bodyAnimalSubmitted`, C2065 `Outcome_RollFailed`, C2660 `ForgivenWindowSec`, C3861 `SubmissionBeaten` / `BuildAnimalSubmissionOfferView`). GREEN: 56/56 suites, 0/72922. (La DLL de este commit no compila sola: el director se adapta en P2.)
- P3 `4c48457` feat(config): `ProactiveOffer` (`offerOnDuelWin`, `offerOnHeal`, `offerFailCalmSeconds` 20, clamp 0..600), 3 filas Mod Hub, textos de las filas del sometido (Ignorar; enfriamiento viejo), plantillas solo con claves nuevas (`popup.body.animalSubmitted`, `submissionEvent.calmNotice` / `ignoredNotice` / `animal.*`).
- P2 `5e93243` feat(offer): `offer_requests` (motor de la cola), director (barrido de pedidas, `ServeRequest`, `StartCalm`, animales con narrador, log `[offer]`), `approach_driver` `BeginWalkerInPlace`, `OnDuelWon` -> `RequestDuelOffer`, ganchos de cura -> `NoteHealForOffer`, `AttemptRecruit` consume. RED: `PlanRequestedOffer` de 3 args y `RequestedPlan_Nothing` (C2660 x8, C2065 x3). GREEN: 56/56 suites, 0/72927; DLL Release x64 compila (solo los C4482 de KenshiLib de siempre).
- P4 docs: QA 12.37 (O1-O23 con lineas de log), REQUISITOS R3.7b, README (Surrender), DEVELOPING (3 modulos nuevos, filas del sometido / ventana / rescate), `odd/system-status.md`.
- Checks finales: rules_tests 56/56, 0/72927; DLL compila; `deploy.sh --dry-run`: config +offerOnDuelWin +offerOnHeal +offerFailCalmSeconds (y +cheatAnyCage de main), messages +submissionEvent.calmNotice +ignoredNotice +animal +popup.body.animalSubmitted (y +readout.line.hold de main), 0 valores cambiados.
- Siguiente: QA en juego (12.37); despues deploy y fast-forward de `main` por el usuario.
