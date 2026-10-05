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

## Restricciones
- Toolset v100: sin C++11. Lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos POD.
- Modulos aislados (SOLID), sin parche sobre parche; reglas puras con RED -> GREEN en `tests/rules_tests`.
- Plantillas `config.json` / `messages.json` solo con claves NUEVAS; el `config.json` desplegado no se toca.
- Maqueta antes del codigo. Sin deploy, sin push, sin merge.

## Checklist
- [ ] P0 Maqueta: popup del evento con ACEPTAR / IGNORAR / RECHAZAR (persona y animal), lineas de narrador del animal, aviso de calma tras tirada mala; filas Mod Hub nuevas. Estados al final de las listas.
- [ ] P1 Reglas puras: cola de ofertas pedidas (pedir, consumir, una por gesto), cribado (animal admitido; una oferta pedida salta `NotBeaten`), planes de salida (IGNORAR = perdonar con huida; RECHAZAR nuevo; tirada mala = calma sin huida). RED -> GREEN.
- [ ] P2 Disparadores en el motor: `OnDuelWon` pide la oferta en vez de unir (Balanced con tirada mala: calma); ganchos de cura piden la oferta; Cheat animal con 1 derrota entra por el cribado. Animales en la caminata con narrador.
- [ ] P3 Config y textos: `offer.failCalmSeconds`, interruptores del evento por disparador, Mod Hub, plantillas (solo claves nuevas), etiqueta IGNORAR.
- [ ] P4 Docs: REQUISITOS, QA (casos con lineas de log), README, DEVELOPING, `odd/system-status.md`.

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
