# Quitar el Mod Hub: la config vive solo en config.json

Rama: `main` (pedido del usuario, 2026-10-07). Engram: proyecto `totalrecruit`, espejo `odd/remove-mod-hub/tasks`.

## Objetivo
El mod deja de integrarse con el Mod Hub de Emkej. Toda la configuracion se lee solo de `config.json` (editado a mano por el jugador), y el mod nunca escribe ese archivo.

## Problema
`src/mod_hub_integration.cpp` registra unos 183 controles. Cada cambio en el hub llama a `SaveConfig()`, que reescribe `config.json` (`config.cpp:658`). El usuario ya no va a usar el Mod Hub. Mantenerlo obliga a sumar una fila de hub por cada clave nueva (T2-T4 de balanced-tuning) y deja un camino que reescribe el config del jugador.

## Por que
- Una sola fuente de config: `config.json`.
- Menos codigo que mantener por cada numero nuevo.
- El mod nunca pisa el `config.json` del jugador (regla del usuario).

## Evidencia (explorador, 2026-10-07)
- Codigo real: `TotalRecruit.cpp:27` (include), `:190` `InitModHub()`; `frame_tick.cpp:6` (include), `:31` `ModHubTick()`. El resto de menciones en `src/` son comentarios.
- `src/emc/mod_hub_client.h` no lo incluye nadie; `src/emc/mod_hub_api.h` solo lo incluyen el hub y ese cliente.
- El hub no guarda estado propio ni pisa valores al cargar: `LoadConfig` lee solo `config.json` y acota (clamps). Quitarlo no cambia los valores efectivos en juego.
- `SaveConfig` solo lo llama el hub. `json_merge.*` y `atomic_file.*` solo los usa `SaveConfig`.
- Las constantes de rango con nombre (`kAnimalBeating*`, `kBeat*`, `kHail*`) las comparten los clamps de `LoadConfig` y las reglas: se quedan.
- `TotalRecruit/RE_Kenshi.json` no depende del DLL del hub; `scripts/deploy.sh` no tiene logica de hub.

## Alcance
- Quitar la integracion, la carpeta `src/emc/`, `SaveConfig` y sus ayudantes sin uso (`json_merge`, `atomic_file`) con sus pruebas.
- Mantener intactos `LoadConfig`, sus clamps y las constantes de rango compartidas.
- Actualizar maquetas y docs que describen el comportamiento actual. Los `odd/tasks/*.md` cerrados son historia: no se tocan.

## Restricciones
- Toolset v100 sin C++11; CRLF; codigo y comentarios en ingles, docs en espanol.
- No tocar el `config.json` ni el `messages.json` desplegados ni los valores de las plantillas.
- No tocar otros repos. Sin push.
- Rama de Ctrl en `TotalRecruit.cpp:105-108`: se queda (evita que Shift dispare); solo cambia el comentario.

## Checklist
- [x] M1 Maqueta: quitar la seccion "4. Mod Hub controls" de `ui-mockup/readout.html` y las menciones en `ui-mockup/js/data.js` y `ui-mockup/README.md`. Ruta: escritor delegado (junto con M2-M3). Commit `28bf061`; la seccion 5 (popup) pasa a ser la 4.
- [x] M2 Codigo: borrar `src/mod_hub_integration.*`, `src/emc/`, llamadas, entradas del `.vcxproj`; borrar `SaveConfig`, `json_merge.*`, `atomic_file.*` y sus pruebas y entradas en `tests/rules_tests` y `tests/balance_sheet`; reescribir los comentarios que citan el hub. Ruta: escritor delegado. Commit `82da5a7`.
- [x] M3 Docs: README, REQUISITOS (R6.2-R6.4, I6 retirados), QA (S1/S2, §9, 12.5, filas 'Mod Hub "X"' pasan a la clave de `config.json`), DEVELOPING, TODO, `odd/system-status.md`, ESTADO, COMPARATIVA, `odd/balance/START-PROMPT.md`, criterios de `odd/tasks/balanced-tuning.md` ("fila en Mod Hub" -> "clave en config.json"). Ruta: escritor delegado. Commit `1fb8bda`.

## Criterios de aceptacion
- `rg -i "mod.?hub|emkej|InitModHub|ModHubTick|SaveConfig" src tests TotalRecruit.vcxproj` sin resultados.
- `rules_tests` verde; DLL y `balance_sheet` compilan.
- Docs vigentes sin instrucciones de usar el hub.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- `"$MSBUILD" tests/balance_sheet/balance_sheet.vcxproj -p:Configuration=Release -p:Platform=x64`

## Progreso
- M1-M3 hechos (2026-10-07) por un escritor delegado.
- Verificado tras M2 y al final: `rules_tests` 61/61 suites, 0/73291 checks fallidos; `TotalRecruit.dll` y `balance_sheet` compilan.
- `rg -i "mod.?hub|emkej|InitModHub|ModHubTick|SaveConfig" src tests TotalRecruit.vcxproj`: queda solo `src/spawn_core.cpp:14` ("Emkej's Test Kit", credito de otra herramienta, no del hub; fuera de las superficies permitidas).
- Pendiente fuera de alcance: `tests/rules_tests/test_support.h:26-27` aun declara `RunJsonMergeTests` / `RunAtomicFileTests` (declaraciones sin uso, compila igual); `odd/tasks/balanced-tuning.md` "Proximo paso" todavia dice "Quitar el Mod Hub".

## Proximo paso
Limpiar las dos declaraciones de `test_support.h` y el credito de `spawn_core.cpp` si se quiere el `rg` en cero; despues seguir con balanced-tuning T2 (sin filas de hub: clave en `config.json` + clamp en `LoadConfig`).
