# Cierre v1: presets Cheat y Balanced, esperas de Cheat de 1 minuto, horas de jaula guardadas, jaula sin atajos

Rama: `fix/cage-beat-direct-hit` (sobre `dab307c`).
Ruta: escritor delegado (un solo escritor acotado; 2+ archivos no triviales por tarea).

## Objetivo
Cerrar la v1 con dos presets (Cheat y Balanced), que en Cheat toda espera dure 1 minuto de juego sin borrar logica, y que las horas que un preso lleva en la jaula sobrevivan a guardar y cargar.

Palabras del usuario (P2): "las esperas ponelas todas 1m para no tener que borrar ni tocar logica asi es mas seguro. La espera que sean 1m y te llama."

## Problema
- Vanilla+ (preset 2) solo agregaba un peso de faccion al precio; nadie lo usa y duplica la matriz de QA.
- En Cheat el jugador todavia se topa con esperas de horas (el primer llamado del preso, descansos de Golpear/Noquear, enfriamientos de rechazo y de duelo).
- El reloj de la jaula vive solo en memoria: recargar reinicia la espera de Cheat y las horas de Balanced.

## Por que
Cheat es un gesto, no un costo (R1.2). La jaula de Balanced cobra tiempo real de juego (R2.x): una recarga no deberia borrarlo.

## Alcance
- P1 fuera Vanilla+: `Preset_VanillaPlus`, `FactionMultiplier`, `FactionPriceMultiplier`, `factionWeight` (config, Mod Hub). Fila de preset 0..1. Un `preset` fuera de rango (un 2 guardado) pasa a Balanced con un log. Una clave `factionWeight` vieja se ignora.
- P2 esperas de Cheat: modulo puro `cheat_waits` (`kCheatWaitHours` = 1/60 h); las lecturas de horas que un jugador de Cheat puede encontrar pasan por el.
- P3 horas de jaula guardadas: el inicio del reloj de la jaula viaja en `saved_ints` (`total_recruit.hold_start_hour`, decimas de hora).
- P4 (agregada por el usuario) jaula de Balanced sin atajos: fuera la espera facil (`easyHoursCaged`) y la ruta "sano gracias a ti" (`Route_EasyHealthy`); cuidarlo pasa a ser un bonus de la tirada (`caredFor`), sorteado por NPC y por dia entre `submissionCaredForBonusMin` (5) y `submissionCaredForBonusMax` (20) con `DailySeed` + `kCaredForBonusSalt`. Maqueta primero.

## Restricciones
- Toolset v100: sin C++11.
- Lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos solo POD (C2712).
- No se toca el `config.json` desplegado; las plantillas solo reciben claves NUEVAS (P1 quita `factionWeight` de la plantilla). Sin deploy, sin push.
- Modulos aislados (SOLID), sin parche sobre parche. No se borra logica en P2.

## Checklist
- [x] P1 fuera Vanilla+: `preset_rules` (`IsKnownPreset`, `ClampPreset`), `config.cpp` lo aplica al cargar y loguea una vez; fuera `Preset_VanillaPlus`, `FactionMultiplier`, `FactionPriceMultiplier`, `factionWeight` (config, plantilla, Mod Hub); fila de preset 0..1; tests de Vanilla+ reescritos a Balanced con tiers apagados. Ruta: escritor delegado. Evidencia: RED = C1083 `preset_rules.h`; GREEN = 48/48 suites, 0/66778; DLL compila. Commit `036c02f`.
- [x] P2 esperas de Cheat de 1 minuto: `cheat_waits` (`kCheatWaitHours` = 1/60, `EffectiveWaitHours` = en Cheat `min(config, 1 min)`), leido por `route_cheat` (espera en jaula), `hail` (primer llamado y repeticiones), `submission_director` (rechazo), `recruit_core` (duelo perdido), `cage_commands` (Golpear / Noquear). Ruta: escritor delegado. Evidencia: RED = C1083 `cheat_waits.h`; GREEN = 49/49, 0/66807; DLL compila. Commit `9ad8a61`.
- [x] P3 horas de jaula guardadas: `HoldClock::SavedStartTenths` / `RestoreStartTenths`, `HoldStartToSavedTenths` (>= 1, <= INT_MAX), fila `total_recruit.hold_start_hour`, lock en `hold_tracking`. Ruta: escritor delegado. Evidencia: RED = C2039 `SavedStartTenths`; GREEN = 49/49, 0/66827; DLL compila. Commit `98777fd`.
- [x] P4 jaula de Balanced sin atajos + bonus por cuidarlo: maqueta (`ui-mockup`: fila "Lo cuidaste +12%" en tarjeta y ventana, filas de Mod Hub) commit `908b2a6`; codigo: `cared_for_rules` (`CaredForBonusFromSeed`, `CaredForBonus`), `IsFullyCaredFor` (reemplaza `IsEasyHealthy`), `ChanceInputs::caredFor` en `JoinChance` y en `[chance]`, `CageExtra::care` (clave del clic o `PeekRollKey`), `ReadoutView::caredForPct`, `OfferView::showCaredFor`; fuera `easyHoursCaged`, `Route_EasyHealthy`, `Reason_EasyHealthy`, `Reason_EasyWait`, `kLineWaitCage`, `LogHold`, `DecideEasyHealthy`, el reloj de encierro en la jaula de Balanced; `deploy.sh` sin `--set-if easyHoursCaged`. Ruta: escritor delegado. Evidencia: RED = C1083 `cared_for_rules.h`, C3861 `IsFullyCaredFor`; GREEN = 50/50, 0/72419; DLL compila. Commit `cb1c74d`.

## Criterios de aceptacion
- Mod Hub ofrece 0: Cheat, 1: Balanced; un config con `preset` 2 carga Balanced y lo dice una vez en el log.
- En Cheat cada espera del mapa de P2 dura 1 minuto de juego (o lo configurado si es menor); Balanced no cambia.
- En Balanced nadie se une por tiempo ni por estar sano; un preso cuidado tira con `caredFor` (5..20, estable por dia) y se ve en tarjeta, ventana y `[chance]`.
- Un preso en la jaula conserva sus horas tras recargar; uno que ya no esta en la jaula las pierde; un save mas viejo reinicia el reloj.
- rules_tests pasa entero; TotalRecruit.dll compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- `scripts/deploy.sh --dry-run`: config.json / messages.json solo agregan claves.

## Progreso
- Documento creado antes de tocar el codigo.
- P1..P4 hechos en `fix/cage-beat-direct-hit`: `036c02f`, `9ad8a61`, `98777fd`, `908b2a6`, `cb1c74d`. Sin deploy, sin push.
- `scripts/deploy.sh --dry-run`: config.json agrega 11 claves (9 de cage-beating-stress pendientes de deploy + `submissionCaredForBonusMin/Max`), messages.json 4 (`cage.knockoutResting`, `readout.beaten`, `readout.caredFor`, `popup.info.caredFor`), 0 valores cambiados. Las claves `factionWeight` y `easyHoursCaged` del desplegado quedan y se ignoran.

Decisiones:
- P1: `PriceInputs::factionMultiplier` se conserva: lleva el descuento de amistad de un aliado de Balanced.
- P2: Cheat usa `min(config, 1 min)`, nunca mas lento que lo configurado. Rechazo y duelo perdido se guardan en horas enteras: en Cheat terminan en la proxima hora entera (no se toco esa logica). Quedan fuera: el enfriamiento tras ganar un duelo (en Cheat ganar siempre une), la insistencia, los temporales y los ayudantes (solo Balanced), el plazo de un reto (`expiryHours`, es una ventana, no una espera) y el desvanecimiento del estres. La etiqueta "Golpear (1 h)" redondea hacia arriba un descanso de 1 minuto.
- P3: 0 en el save = sin reloj; el inicio se guarda en decimas, al menos 1.
- P4: el bonus necesita una clave para sembrarse (sin clave, 0, como la tirada); en el peek se usa la clave existente (`PeekRollKey`) para que la tarjeta muestre el mismo valor que el clic. La plantilla `messages.json` pierde `reason.easyHealthy`, `reason.easyWait`, `readout.line.easyHealthy`, `readout.line.easyWait`, `readout.line.waitCage` y `config.json` pierde `easyHoursCaged` y `factionWeight` (los desplegados las conservan, ignoradas). Desde P4 el reloj de encierro solo decide la espera de Cheat.

## Siguiente paso
Deploy (lo hace el padre) y QA en juego: 12.33 P1-P3, W1-W2, H1-H4, C1-C8; 8 H7-H10; 12.3 E3/E8; 12.8 U3; 12.1 F3.
