# Prioridad 1: sonido y animacion de Golpear, duelo lejano, esperas de Cheat, animales golpeados, frases editables

Localizador: `odd/tasks/priority-one.md` - espejo Engram: `odd/priority-one/tasks` (proyecto `totalrecruit`).
Rama: `feat/priority-one` (desde `fix/beat-feedback` @ `6b6ce58`, que sigue sin mergear en `main`).
Ruta: escritor delegado por tarea (cada tarea toca 2+ archivos no triviales).

## Pedido del usuario (literal)
"yas probe el golpeo, sale el mensaje y la notificacion pero no suena nada, tampoco hace animacion. revisa eso y empieza con Prioridad 1 luego actualizame todo este reporter"
El usuario juega con preset Cheat (`"preset": 0` en el config desplegado).

## Evidencia de la prueba en juego (RE_Kenshi_log.txt, 2026-10-06)
- La caminata funciona: `Mini -> Holy Warden beat walk arrived (2.8 m)`, llega entre 2.8 y 4.0 m.
- Mensaje y notificacion: funcionan (usuario).
- Sonido: el log dice `beat hit sound Heavy_Hit on prisoner` (audioEvent devolvio true) pero no se oye nada.
- Causa: `Heavy_Hit` no es un evento de Wwise. Hash FNV-1 de los nombres contra los objetos Event (HIRC tipo 4) de `data/audio/*.bnk`: `Heavy_Hit`, `Light_Hit`, `HtH_Punch`, `HtH_Impacts`, `Play_*` no existen como evento; si existen `VO_Get_Hit` y `VO_Creature_Take_Damage` (VO_Bank.bnk), `Impact` y `Attack` (SFX_Bank.bnk, SFX_General_Bank.bnk). `Heavy_Hit`/`HtH_*` aparecen como cadenas en el exe: probablemente valores de switch. `audioEvent` devuelve true aunque el evento no exista, asi que no sirve para elegir candidato.
- Animacion: no existe (T4 de beat-feedback fue solo investigacion).

## Checklist
- [x] S1 Sonido de Golpear que se oiga: eventos reales (`VO_Get_Hit` sobre el preso humano, `VO_Creature_Take_Damage` sobre un animal, `Impact` como golpe), con switch si hace falta; deteccion real de fallo. Ruta: escritor delegado. Investigacion (Python de solo lectura sobre el HIRC): `Impact` -> accion Play -> switch container del grupo `Impacts` (default `Light_Hit`; valores `Heavy_Hit`, `Light_Hit`, `Parry`, ..., `HtH_Impacts`, `HtH_Impacts_Robot`): `Heavy_Hit` era un VALOR de switch. `VO_Get_Hit` -> switch `VO_Races`, `VO_Creature_Take_Damage` -> switch `Creature_VO` (los pone el juego por personaje). `kenshi_x64.exe` exporta `AK::SoundEngine::PostEvent(const char*,...)` y `SetSwitch(const char*,const char*,AkGameObjectID)`. Modulos nuevos `beat_hit_sound_rules` (puro: `BeatHitSoundCues`) y `wwise_bridge` (GetProcAddress, marcos POD); `beat_walk_sound` postea sobre el preso `Impact` con `Impacts`=`submissionBeatHitSound` (default "HtH_Impacts") y la voz, loguea el playing id o el fallo, y cae a `audioValue`+`audioEvent`. Se quito `BeatHitSoundCandidates` (la lista de LSE). Evidencia: RED = C1083 `beat_hit_sound_rules.h`; GREEN = 58/58 suites, 0/73164 checks fallidos; DLL compila; imports nuevos (`audioValue`, `getAudioObject`) presentes en el KenshiLib.dll del juego. Commit `f7b719d`. **Bloqueado parcial**: `--set-if submissionBeatHitSound Heavy_Hit HtH_Impacts` no se agrego a `deploy.sh`, porque `scripts/add_missing_keys.py` (fuera de las superficies permitidas) solo compara valores numericos en `--set-if`; con un string imprimiria "kept" y no cambiaria nada. No rompe: "Heavy_Hit" es un valor real del switch `Impacts` (suena el golpe pesado, QA 12.40 S4). Para migrar: hacer `--set-if` sensible a strings en `add_missing_keys.py` y agregar la linea. QA en juego: 12.40 S1-S5, 12.39 G3/G17.
- [x] A Duelo ganado con el rival a mas de 80 m: unificar el radio de busqueda (`kDuelSearchMeters` 80, `recruit_core.cpp:890`) con el alcance de la oferta (120 m). Ruta: escritor delegado. `kOfferReachMeters` (= `kSubmissionAbortMeters`) en `offer_request_rules` como unica fuente; `InOfferReach` (lo usa `WithinOfferReach`) y `DuelRivalSearchMeters` (lo usa `FindNearSquad`). Evidencia: RED = C2065 `kOfferReachMeters` / C3861 `InOfferReach`; GREEN = 58/58 suites, 0/73173 checks fallidos. Commit `fc754c7`. QA en juego: 12.40 A1-A2.
- [ ] B Esperas de Cheat en minutos: enfriamientos de rechazo y duelo en sub-hora; etiquetas "(1 min)" en botones y mensajes.
- [ ] E Frases de rechazo y de moneda editables: cablear `refusal.*` y `currency.*` de `messages.json` en lugar del texto fijo de `npc_speech.cpp`.
- [ ] D Golpear a un animal en la jaula ayuda a domarlo: el estres de golpes entra en `DecideAnimal`, con resolucion por nivel/especie.
- [ ] C Animacion de Golpear: clip `badpunch` (tecnica vanilla `ma badpunch`, rebirth.mod) sobre el personaje al llegar; el golpe cae despues del amago.

## Restricciones
- Modulos aislados (SOLID), sin parche sobre parche; reglas puras test-first en `tests/rules_tests`.
- Toolset v100 sin C++11; lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos POD.
- Texto al jugador en `messages.json`; config y messages solo agregan claves; `--set-if` para cambiar un default desplegado.
- Maqueta `ui-mockup/` al dia; CRLF; vcxproj UTF-8 con BOM.
- No tocar TotalSparring ni otros repos.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- `bash scripts/deploy.sh --dry-run`: solo agrega claves.

## Progreso
- Documento creado. Siguiente: S1, A, B, E, D, C en ese orden, un commit por tarea.
