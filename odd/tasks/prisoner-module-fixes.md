# Modulo de presos: cuatro arreglos

Rama: `fix/cage-beat-direct-hit` (sobre `0f042dc` + `c66bf10`, Golpear como golpe directo).
Ruta: escritor delegado (un solo escritor acotado; 2+ archivos no triviales por arreglo).

## Objetivo
Que los comandos de la jaula y lo que los rodea hagan lo que dicen, una vez: Noquear noquea una vez y suelta al actor, un noqueo cuenta una derrota, la tarjeta y el menu dicen lo mismo sobre un rescate, y el bloque de botones no tapa la segunda columna del menu nativo.

## Problema (evidencia del log `RE_Kenshi_log.txt`)

**F1 — Noquear deja el sigilo puesto y vuelve a noquear al preso para siempre.** Una sola orden:

```
2576.692 [cage] Quedo -> Grech knockout ordered (task 228 ...)
2576.731 [cage] Quedo -> Grech knockout via task
```

y despues ninguna linea `[cage]`, pero `[defeat-hook] knockout fired for Grech` a 2579.7, 2631.3, 2693.2 y 2753.2, cada una segundos despues de que se levanta. `SafeOrder` hace `addOrder` + `addGoal(STEALTH_KNOCKOUT, preso)`; la meta es persistente y nadie la quita, asi que la IA del actor la sigue persiguiendo (y vuelve a entrar en sigilo); la vigilancia se cierra en "Taken" sin limpiar.

**F2 — Un noqueo cuenta dos derrotas.** Cada noqueo dispara el gancho dos veces, ~1,2 s de diferencia, las dos "(was standing)": 2579.7/2580.9, 2631.3/2632.6, 2693.2/2694.4, 2753.2/2754.8; el conteo sube 2→3, 4→5 ... (Grech muestra derrotas 9/2).

**F3 — La tarjeta contradice el veredicto del rescate.**

```
2786.808 [readout] Grech route=rescue chance=95% ... line="No puede seguirte: te paga con lo suyo"
2786.976 Context menu: Grech -> CAN JOIN (nada) considera que su vida te pertenece por la deuda: te acompaña
```

En modo vistazo `GatherRescueExtra` no arma la tirada y `RollsToJoin` sin clave dice que no.

**F4 — El bloque Golpear/Noquear tapa la segunda columna del menu nativo.** Un personaje dentro de un edificio (la jaula) abre dos columnas lado a lado: el personaje ("Grech": SAQUEAR, ...) y el edificio ("Jaula de Prisionero": ABRIR CON LLAVE, DESMANTELAR, PONER EN LIBERTAD). `AnchorToMenu` solo lee `menuGUI` (0x48) e ignora `menuGUI2` (0x50), y `ResolveSlot` prefiere la izquierda: el bloque cae encima de la otra columna.

## Por que
Noquear tiene que ablandar al preso una vez (R2.3), no convertir al actor en un acosador eterno; el conteo de derrotas alimenta la sumision (R3.3) y un doble conteo la regala; una tarjeta que dice lo contrario que el menu engaña al jugador; un boton encima de PONER EN LIBERTAD es un clic equivocado esperando a pasar.

## Alcance
- F1: modulo nuevo `knockout_order_cleanup` (motor: quitar la meta, la orden, el sigilo si lo puso la orden, repensar; curar partidas ya trabadas) + regla pura nueva `knockout_watch_rules` (ciclo de vida de la vigilancia, decision del sigilo, condicion de curado). `cage_commands.cpp` usa ambos.
- F2: `defeat_episode_rules` (ventana de eco en tiempo real) + `defeats.cpp` le pasa el reloj.
- F3: `character_facts.cpp` arma la tirada del vistazo con el id existente (sin crear uno); regla pura del armado de la tirada en `chance_rules`.
- F4: maqueta primero (`ui-mockup/`), luego modulo nuevo `menu_anchor` (lectura guardada de las dos columnas + union pura); `recruit_overlay.cpp` le pasa la union a `PlaceButtonBlock`.
- Docs: `DEVELOPING.md`, `REQUISITOS.md`, `QA.md`, `odd/system-status.md`, cabeceras de `src/`.

## Restricciones
- Toolset v100: sin C++11.
- Toda lectura del motor detras de `TR_TRY/TR_CATCH` en marcos solo POD (C2712).
- Cada arreglo en su propio modulo o funciones (SOLID), sin parche sobre parche.
- No se toca el `config.json` desplegado; sin deploy, sin push, sin merge.
- CRLF en todo archivo tocado (`unix2dos`).

## Checklist
- [x] F1 Noquear actua una vez: vigilancia viva hasta que el preso cae (o muere) o vence la ventana; limpieza del actor (meta, orden, sigilo si no estaba antes, repensar) tras la orden, tras el respaldo, al reemplazar la vigilancia y en la carga; curado periodico de metas `STEALTH_KNOCKOUT` trabadas. Ruta: escritor delegado. Evidencia: RED = rules_tests no compila (C1083 `knockout_watch_rules.h`); GREEN = 45/45 suites, 0/64291 checks fallidos; la DLL compila sin advertencias nuevas.
  - Regla pura nueva `knockout_watch_rules` (`NextKnockoutWatchStep`, `ShouldEndStealthAfterKnockout`, `ShouldClearStrayKnockoutGoal`, `StrayGoalSweepDue`); motor nuevo `knockout_order_cleanup` (`ReadStealthMode`, `CleanUpKnockoutOrder`, `SweepStrayKnockoutGoals`); `cage_commands.cpp` solo los conecta (`EndWatch`, `ReadWatchFacts`).
  - Commit: `993c0bd` (`fix(cage): noquear acts once and lets the actor go`).
  - Decisiones: ventana de caida = `kOrderedKnockoutWindowMs` (2 min, la misma del credito); `clearOrders` solo si la orden de noqueo sigue en cola (`hasPlayerOrder`), para no borrar una orden mas nueva; curado: solo si ninguna vigilancia tiene al actor, la meta es la actual y nombra al objetivo, y ese objetivo ya esta caido o es un preso de tu jaula; en el curado el sigilo se deja (no se sabe quien lo encendio).
- [x] F2 Un noqueo, una derrota: una segunda llamada del gancho para la misma victima dentro de la ventana de eco es el mismo episodio, se lea de pie o no. Ruta: escritor delegado. Evidencia: RED = rules_tests no compila (C2660 `Judge` / `AfterCount`, C2065 `kKnockoutEchoWindowMs`); RED de comportamiento con la ventana anulada a mano = 6 checks fallidos (doble llamada a 1,2 s, rearme por verlo de pie, vuelta de `GetTickCount`); GREEN = 45/45 suites, 0/64302 checks fallidos; la DLL compila.
  - Commit: `c17e7fc` (`fix(defeats): one knockout counts one defeat inside a 5 s echo window`).
  - Decisiones: la regla documentada ("verlo de pie rearma", sin rebote en horas de juego) se mantiene; se agrega una ventana de eco en tiempo real (`kKnockoutEchoWindowMs` = 5 s) desde la ultima derrota contada que vale para todo avistamiento y sobrevive a `NoteStanding`. Tiempo real y no de juego: el doble llamado es del motor (~1,2 s reales) y a velocidad x3 una ventana en horas de juego se estiraria o encogeria. `defeats.cpp` solo pasa `GetTickCount()`.
- [x] F3 El vistazo del rescate tira con el id existente: tarjeta y menu dan el mismo veredicto para un NPC con id. Ruta: escritor delegado. Evidencia: RED = rules_tests no compila (C3861 `RollFactFrom`); GREEN = 45/45 suites, 0/64312 checks fallidos; la DLL compila.
  - Regla pura `RollFactFrom` en `chance_rules` (la usan `MintRollKey` y el nuevo `PeekRollKey`); `GatherRescueExtra` en vistazo usa `PeekRollKey` (`SafeFindCharKey` + `SafeGameHours`, como `ReadHaggle`). Tests en `route_decisions_tests.cpp`: el vistazo y el clic de un NPC con id dan el mismo veredicto; sin clave o sin reloj no hay clave.
  - Decision: solo el rescate (el alcance pedido). Las otras rutas siguen sin tirada en vistazo: la tarjeta muestra sus probabilidades (`IsLostRoll`), no un veredicto contrario. El comentario de `readout.cpp` ("the peek mints no id, so it has no roll key") sigue siendo cierto para `rollKeyed` y no se toco (fuera de las superficies).
- [ ] F4 Bloque fuera de las dos columnas: maqueta de dos columnas, `menu_anchor` con la union, una sola columna igual que hoy. Ruta: escritor delegado.

## Criterios de aceptacion
- F1: tras un Noquear, el preso cae una vez; despues de caer, el actor no vuelve a ir por el (ninguna `[defeat-hook] knockout fired` nueva sin otra orden), sale del sigilo si no estaba en sigilo antes, y el log dice `[cage] <actor> knockout cleanup (goal removed, stealth off|kept)`. Una partida ya trabada se cura sola en segundos.
- F2: un noqueo sube el conteo en uno aunque el gancho llegue dos veces "(was standing)" en ~1,2 s.
- F3: para un NPC con id, `[readout]` y `Context menu:` dan el mismo veredicto de rescate.
- F4: con dos columnas el bloque queda fuera de ambas; con una sola columna la posicion es exactamente la de antes. Log `[menu-anchor] gui1=(...) gui2=(...|none) union=(...)`.
- rules_tests pasa entero; TotalRecruit.dll compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`

## Progreso
- Documento creado antes de tocar el codigo. Linea base: 44/44 suites, 0/64256 checks fallidos; la DLL compila.

## Siguiente paso
F1.
