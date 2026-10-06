# Golpear: acercarse a la jaula y avisar que el golpe entro

Localizador: `odd/tasks/beat-feedback.md` - espejo Engram: `odd/beat-feedback/tasks` (proyecto `totalrecruit`).
Rama: `fix/beat-feedback` (desde `main` @ `3a42e53`).
Ruta: escritor delegado (caminata + golpe diferido + voz + mensaje + config/messages + maqueta + docs: 2+ archivos no triviales).

## Pedido del usuario (literal)
"El boton golpear funciona pero como el pj no hace animacion pareciera que no hace nada, capaz mostrar algun mensaje o hacer algo porque no se entiende pareciera que no funciona"
Decision del usuario:
1. primero que todo el pj debe acercarse a la jaula, ya tenemos esto resuelto como acercarse a algo
2. que el preso se queje en voz alta ("¡Ugh!"), aunque si esta inconsciente no dice nada
3. un mensaje en pantalla con el sonido de notificacion
4. animacion: revisar TotalTraining primero (notas y logs) - investigacion aparte, sin implementar todavia

## Problema
- `OrderSelectedToBeat` -> `LandBeatHit` (`src/cage_commands.cpp:378-441, 612-625`) escribe el dano directo al torso: sin orden, sin animacion, sin chequeo de distancia (se puede golpear desde la otra punta del mapa).
- Solo los fallos avisan: `ReportCageCommand` (`src/recruit_overlay.cpp:141-147`) y `OrderCageCommand` (`src/offer_popup.cpp:965-973`) llaman `TellPlayer` solo si el resultado no es `CageCommand_Issued`.
- Historia: `BEAT_CAGE_OCCUPANT` nunca lo toma un plugin y el ataque nativo saca al preso de la jaula (`odd/tasks/cage-beat-direct-hit.md`). No se reintenta.

## Alcance
- T1 El personaje camina hasta la jaula y recien al llegar se aplica el golpe.
- T2 El preso se queja en voz alta al recibir el golpe (solo si puede hablar).
- T3 Mensaje en pantalla con el sonido de notificacion cuando el golpe entra.
- T5 (agregado por el usuario) Sonido de golpe sobre el preso al pegar: `Character::audioEvent(nombre, SOUNDRANGE_ALWAYS)` como Loot-Scoot-Execute (`Loot-Scoot-Execute.cpp:73-85`, `:1396-1463`).
- Fuera de alcance: animacion (T4, investigacion), Noquear (ya camina con la orden nativa), cambios de balance.

## Restricciones
- Modulos aislados (SOLID), sin parche sobre parche; reglas puras test-first en `tests/rules_tests`.
- Toolset v100, sin C++11. Lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos POD.
- Texto al jugador en `messages.json` (claves nuevas); `scripts/deploy.sh` solo agrega claves.
- Maqueta `ui-mockup/` al dia. Finales de linea CRLF.
- Reusar la caminata existente (`npc_motion.h`, `approach_driver.h`); no copiarla.

## Checklist
- [x] T1 Caminar hasta la jaula antes de golpear. Ruta: escritor delegado. Modulos nuevos `beat_walk_rules` (puro: `NextBeatWalkStep`, `AdvanceBeatProgress`, `BeatWalkLimitsFrom`, mensajes de aborto) y `beat_walk` (motor: un golpe pendiente por actor, hasta 8, por `CharKey`; `NpcMotion::MoveTo` una vez, `EventClock` de `approach_driver`; al llegar `StopWalking(true)` + `NpcMotion::Face` nuevo). `cage_commands` solo delega (`OrderSelectedToBeat(target, report)` arma `BeatWalkHandlers` con `LandBeatHit` y el veredicto del clic; `TickCageCommands` -> `TickBeatWalks`; `ForgetCageCommands` -> `ForgetBeatWalks`). Config `submissionBeatReachMeters` (4, 1..15) y `submissionBeatWalkSeconds` (30, 5..300) con entradas de Mod Hub. Evidencia: RED = rules_tests no compila (C1083 `beat_walk_rules.h`); GREEN = 57/57 suites, 0/73143 checks fallidos. Commit `320983c`.
- [x] T2 Queja en voz alta del preso: `beat_walk_feedback` (`TellBeatOutcome`) dice `cage.beatOuch` (lista de 4, estable por preso) con `SpeakAloud`; un animal no habla (`BeatComplaintSpoken`, como `DuelRaisesHeadNotice`), un inconsciente calla solo (`canSpeakNormally`). Ruta: escritor delegado. Commit `320983c`.
- [x] T3 Mensaje en pantalla + sonido de notificacion: `TellPlayer(cage.beatLanded "{actor} golpea a {name}")`; los abortos avisan como problema (`cage.beatAbort.timeout/stalled/actor`, y `cage.notAllowed` / `cage.beatResting` / `cage.failed` reusados); reemplazo y carga callan. Sin linea al empezar a caminar. Menu y GOLPEAR de la ventana pasan el mismo `TellBeatOutcome`. Ruta: escritor delegado. Commit `320983c`.
- [x] T5 Sonido de golpe: `beat_walk_sound` (`PlayBeatHitSound`): `audioEvent` en marco POD con `TR_TRY`, devuelve bool, asi que prueba `BeatHitSoundCandidates` (el configurado primero, despues la lista de LSE, sin repetir) sobre el preso y despues sobre el actor, y loguea cual acepto. Config `submissionBeatHitSoundEnabled` (true, Mod Hub "Golpear: sonido del golpe") y `submissionBeatHitSound` ("Heavy_Hit"). Evidencia: RED = C3861 `BeatHitSoundCandidates`; GREEN = 57/57 suites, 0/73152 checks fallidos. Sin verificar en juego. Ruta: escritor delegado. Commit `320983c`.
- [x] T4 Animacion: notas y logs de TotalTraining (explorador, solo lectura). Sin logs de TotalTraining (no instalado). Vanilla no tiene accion de golpe; los golpes sin arma son COMBAT_TECHNIQUE de rebirth.mod (`ma badpunch` clip `badpunch`). Vias: tecnica de combate renovada por frame, registro ANIMATION propio con `is action` (un `playAction`), o `runSlaveAnim("badpunch")` por frame + `endSlaveAnim`. Ninguna probada con un golpe. Engram `odd/beat-feedback/t4-animation`. Ruta: explorador.

## Criterios de aceptacion
- Click en Golpear lejos de la jaula: el personaje camina, y el golpe, la queja y el mensaje llegan al llegar.
- Click cerca: golpe inmediato con queja y mensaje.
- Preso inconsciente: golpe y mensaje, sin queja.
- Si la caminata no llega (bloqueado, otra orden, preso sale de la jaula), no hay golpe ni descanso consumido, y el jugador recibe un aviso.
- `rules_tests` verde; DLL compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`

## Progreso
- Documento creado. Siguiente: T1-T3 con un escritor; T4 en paralelo con un explorador.
- T1, T2, T3 y T5 hechos en `fix/beat-feedback`: codigo + tests en `320983c` (`feat(cage): Golpear walks to the cage before the blow lands`); docs (QA 12.39 G1-G17, REQUISITOS R2.3 estado beat-feedback, README, `ui-mockup/README.md` item 17, `readout.html` callout de la caminata) en `fd23727`. TotalRecruit.dll Release|x64 compila sin advertencias nuevas. Sin deploy, sin push.
- Decisiones: "otra orden" se detecta por falta de avance (4 s sin acercarse 0,5 m), no por la tarea del actor (no esta probado que `MOVE_CUS_ORDERED` se lea como tarea actual en un personaje del jugador). En un aborto no se toca ninguna orden (no pisar lo que el jugador mando). Una carga no avisa (el mundo de la caminata ya no existe). `DEVELOPING.md` no estaba en las superficies permitidas: falta su fila para `beat_walk*`.

## Siguiente paso
QA en juego 12.39: que el personaje del jugador tome la orden de caminar del plugin hacia un preso dentro de una jaula y se detenga a menos de 4 m (si se queda mas lejos, subir `submissionBeatReachMeters`); que el sonido se oiga y que evento acepta el motor (G3, G17); que "otra orden" aborte en ~4 s (G6).
