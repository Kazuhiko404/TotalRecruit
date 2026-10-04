# Matriz de pruebas

Para validar en partida lo que hoy solo compila. Cada fila dice qué hacer, qué
tiene que pasar, y **qué línea del log lo prueba** — si no aparece esa línea, el
caso falla aunque en pantalla parezca bien.

---

## 0. Preparación

```jsonc
// d:\SteamLibrary\steamapps\common\Kenshi\mods\TotalRecruit\config.json
"probeOnClick": true    // sin esto no hay diagnóstico
```

**Cerrá Kenshi del todo y abrilo de nuevo.** El config se lee al cargar el
plugin; cambiarlo con el juego abierto no hace nada.

Comprobá que arrancó bien: el log debe traer una línea nueva de
`startPlugin called`. Si no está, seguís probando el build anterior.

```
d:\SteamLibrary\steamapps\common\Kenshi\RE_Kenshi_log.txt
```

Filtro útil desde Git Bash:

```bash
tail -f "/d/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi_log.txt" | rg "TotalRecruit"
```

---

## 1. Sanidad — cinco minutos, y si algo de esto falla nada más importa

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| S1 | Abrir el juego | El mod carga y el hub engancha | `startPlugin complete.` y `Mod Hub attached: api v1, size 160 bytes.` |
| S2 | Abrir el panel de Mod Hub | ~106 controles de TotalRecruit (T8 sumo 19), ninguno rechazado | Ningún `Mod Hub rejected` |
| S3 | Click derecho a cualquier NPC | Sale el bloque de diagnóstico completo | `--- probe: <nombre> ---` … `--- end probe ---` |
| S4 | Cerrar el juego | Nombra las claves de texto que falten | `messages.json has no entry for:` (o nada) |

**S4 no es opcional.** Cada regla nueva trae texto; esa línea dice exactamente
cuál falta escribir.

---

## 2. El motor de chance (R2.1, R2.2)

Todo esto se lee de una sola línea:

```
[chance] level=45 relation=0 personality=-20 defeats=0(x0) situation=15 -> 40%
```

### 2.1 Valores esperados, con los defaults de fábrica

`levelBase 45 · penalidad 1.5/nivel · piso 2 · bono 1.5/nivel · techo del bono 40 · clamp 5..95`

**Prisionero en tu jaula** (`situation=15`), sin derrotas, relación neutral:

| Diferencia de nivel | Normal | Cobarde | Honorable |
|---|---|---|---|
| Te saca 40 | 17% | 37% | 5% |
| Te saca 20 | 30% | 50% | 10% |
| **Iguales** | **60%** | **80%** | **40%** |
| Le sacás 20 | 90% | 95% | 70% |
| Le sacás 40 | 95% | 95% | 80% |

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| C1 | Click a un NPC de tu nivel | `level=45` en el log | Da 100 — volvió la saturación |
| C2 | Click a uno que te saque 20 niveles | `level=15`, total ~30% | Sale rechazo seco: quedó una puerta |
| C3 | **Click a uno que te saque 40+ niveles** | Chance baja pero **no cero**, y ofrece algo | *"ni se molesta en escucharte"* — la puerta de +15 sigue viva |
| C4 | **Ser 40+ niveles más fuerte que un honorable** | **No** se une automáticamente | *"se somete sin dudarlo"* — la puerta de −40 sigue viva |
| C5 | Comparar un cobarde y un honorable del mismo nivel | `personality=20` vs `personality=-20` | Ambos dan lo mismo |

**C3 y C4 son la prueba de R2.1.** Si alguno falla, el requisito no está hecho.

### 2.2 El dado no se puede re-rollear (R0.3)

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| C6 | Click al mismo NPC 5 veces seguidas | Idéntico veredicto y `-> N%` las 5 veces | Cambia el número |
| C7 | Guardar, **salir al escritorio**, volver a entrar, click al mismo | **El mismo veredicto** | Cambia — la semilla volvió al puntero |

**C7 es el bug que arreglé esta ronda.** Antes cada recarga tiraba el dado de
nuevo, así que un NPC de 17% era cuestión de insistir con F9.

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| C8 | Un NPC que dijo "todavía no" (chance entre 30% y 70%): esperar a que pase la medianoche del juego (o dormir/acelerar) y volver a hablarle; repetir algunos días | Dentro del mismo día el veredicto no cambia (C6); en algún día siguiente puede decir **que sí**. El log muestra la misma `-> N%` | Dice que no todos los días para siempre, o cambia dentro del mismo día |

---

## 3. Sumisión (R3.1 a R3.5)

### 3.1 Contar derrotas

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| D1 | Dejar inconsciente a un NPC hostil | Se cuenta una derrota | `defeated by the player (knocked down from standing; count 1, id:...)` |
| D2 | Que **otro** NPC noquee a un tercero | **No** cuenta | Ninguna línea `defeated by the player` |
| D3 | Que noqueen a alguien tuyo | **No** cuenta | Ninguna línea |
| D4 | Curarlo, dejarlo inconsciente otra vez | Cuenta 2 (R3.4) | `(knocked down from standing; count 2, ...)` |
| D5 | Golpearlo dos veces en el mismo instante | Cuenta **una** sola | El contador no salta de 1 a 3 |

**D2 y D3 son los filtros.** Si cuentan, el mod está sumando peleas ajenas.

### 3.2 El conteo persiste en el save (R3.3)

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| D6 | Vencerlo 3 veces, guardar, salir, entrar, click | `defeats=3` en la tarjeta / el log: el conteo exacto | Da `0`: el conteo no llegó al save. Da otro número: se leyó de otro lado |
| D7 | Tras D6, cargar un save **anterior** a la tercera derrota | El conteo que tenía ese save (p. ej. 2) | Trae el 3 de la sesión |

Ya no hay piso ni pérdida de precisión: el save guarda el número exacto.

### 3.3 Rogar por vida baja (R3.1)

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| D8 | Bajarle la sangre por debajo del 25% y noquearlo | Se abre la ventana de piedad | `blood=0.18 nearDeath=yes` |
| D9 | Noquearlo con sangre alta | **No** se abre | `nearDeath=no` |

Ojo: la ventana también se abre al perder un miembro, que es la ruta vieja. D9
distingue una de la otra.

### 3.4 Personalidad y aguante (R3.5)

Con `chancePerDefeat=12`, la derrota suma `12 / resolve`:

| Temperamento | Resolve | Por derrota | 3 derrotas |
|---|---|---|---|
| Cobarde / corto de luces | 0.6 | +20 | +45 (techo) |
| Normal | 1.0 | +12 | +36 |
| Valiente | 1.6 | +7 | +22 |
| Honorable | 1.8 | +6 | +20 |

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| D10 | Vencer 3 veces a un cobarde y a un honorable, comparar | `defeats=45(x3)` vs `defeats=20(x3)` | Dan lo mismo |

---

## 4. Golpear al prisionero (R2.3)

> Superado por 12.19 y 12.22: GOLPEARLO (desmayo forzado con enfriamiento) ya no existe; la ventana ofrece GOLPEAR / NOQUEAR, las ordenes nativas.

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| B1 | Click a un prisionero **tuyo** despierto que se niega | Aparece el botón **GOLPEARLO** | No aparece |
| B2 | Apretarlo | Queda inconsciente ~60s y suma derrota | Nada pasa — `knockoutForceTimer` no funciona |
| B3 | Intentar de nuevo enseguida | Rechaza | *"todavía se está recuperando"* |
| B4 | Esperar 6 horas de juego, repetir | Deja | — |
| B5 | Golpear a uno **hambriento** llevando comida | Gana **DARLE COMIDA** | Sale GOLPEARLO: la prioridad está al revés |
| B6 | Golpearlo 10 veces seguidas (con esperas) | **Nunca muere** | Muere — el techo no existe |
| B7 | Click a un NPC en la calle | **No** hay botón | Aparece: falta el chequeo de jaula |

**B2 es el riesgo grande.** `knockoutForceTimer` linkea y es no-virtual, pero
nadie lo llamó nunca desde un plugin. Si no hace nada, R2.3 es una fachada.

**B6 es la garantía de R2.3:** no puede convertirse en matarlo por accidente.

---

## 5. Insistencia (R2.4)

`3 intentos · 4h base · techo 48h · olvida a las 2h`

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| I1 | Hablarle 3 veces seguidas | A la 3ª te ignora | Sigue atendiendo |
| I2 | Insistir hasta 5 veces | El castigo **crece**: 4h, 8h, 12h | Siempre 4h — no acumula |
| I3 | Insistir 20 veces | Corta en 48h | Sube sin freno |
| I4 | Esperar 2h de juego sin hablarle, volver | Contador en cero | Sigue castigado |
| I5 | Guardar y recargar durante el castigo | **Se olvida** — limitación conocida (I5) | — |

I5 **no es un bug**: el estado vive en memoria. Está anotado como pendiente.

---

## 6. Rescate (R4.1 a R4.4)

`base 25 + cura×30`, y `situation` se ve en el log.

| Cuánto curaste | Normal | Honorable |
|---|---|---|
| Solo estabilizado | 70% | 50% |
| Un tercio | 80% | 60% |
| Dos tercios | 90% | 70% |
| Entero | 95% | 80% |

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| R1 | Curar a un moribundo hasta que se levante | Ofrece unirse | `situation=55` (25+30) |
| R2 | Vendarlo a medias y hablarle | Menos chance que R1 | `situation` menor |
| R3 | **Que uno se niegue** | Te **entrega** sus cats | `X paid the player N cats for the rescue` |
| R4 | Volver a hablarle tras R3 | Sigue en deuda, **no** te paga otra vez | No hay segunda línea `paid the player` |
| R5 | Rescatar a uno sin dinero | Se niega y no paga — correcto | Sin línea `paid` |

**R3 es el bug que arreglé:** antes decía *"te deja algo en pago"* y no daba nada.
**R4 es la garantía:** el pago se cobra una vez.

---

## 7. Animales (R5.1 a R5.5)

`hunger 0..3 · una ración = 0.40`

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| A1 | Click a un salvaje hambriento | Pide comida con **porcentaje** | `todavía tiene hambre: le falta N%` |
| A2 | Darle una ración | **La barra sube** | `Fed X. hunger 1.70 -> 2.10 (credited by plugin)` |
| A3 | Seguir dándole hasta el umbral | Se une | `comió de tu mano y te sigue` |
| A4 | Click a un `Goat Pet` / animal de facción real | **Rechaza**, manda a la jaula | `ya tiene dueño` · `notARealFaction=no` |
| A5 | Click a un salvaje (Wild Dogs) | Se puede domesticar | `notARealFaction=yes` |
| A6 | Meter un doméstico en tu jaula y darle comida | **Ahora sí** funciona | La regla no aplica dentro de tu jaula |
| A7 | Comparar dos animales de la misma especie con temperamento distinto | Piden **distinta** cantidad (R5.4) | Piden lo mismo |
| A8 | Click a un cachorro | Pide mucho menos que un adulto | — |

**A2 es la prueba clave.** El log tiene que decir `credited by plugin`: si dice
`engine` o la barra no se mueve, volvió el bug de `eatItem`.

---

## 8. Cheat (R1.1 a R1.4)

Poné `"preset": 0` y **reiniciá el juego**.

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| H1 | Click a un NPC en la calle | **Rechaza** y pide jaula | Se une al instante — R1.1 no está |
| H2 | Meterlo en tu jaula, click inmediato | *"dale un momento"* | Se une ya |
| H3 | Esperar un minuto de juego, click | **Se une**, sin condiciones | Pide cats o comida — Cheat no debe pedir nada |
| H4 | Probar con un único de nivel 90 | Igual: se une | Lo rechaza por nivel |
| H5 | Probar con un animal | Igual: se une | — |
| H6 | Poner `cheatNeedsCage: false` | Vuelve el comportamiento viejo | — |

**H3 y H4 juntos son la definición de Cheat:** cualquiera, incluidos animales y
únicos, sin ninguna condición más que el gesto.

---

## 9. Las dos investigaciones abiertas

### I4 — ¿escribir la marca altera la IA?

**Resuelta (recruit-gating T1):** el mod ya no escribe ni lee
`LT_SQUAD_LOST_TO_ME_ONCE`. El conteo vive en el save del propio personaje
(`total_recruit.player_defeats`). Ver §12.13.

### R6.3 — el layout del hub

Buscá al arrancar:

```
Mod Hub slots (19 of 160 bytes): [0]=fn [1]=fn [2]=fn ...
```

Pasame esa línea. Con ella puedo mapear los punteros que faltan y llamar a
`register_setting_section` **sin adivinar** — que es lo que crashea el juego.

---

## 10. Regresión — que lo viejo siga andando

| # | Caso | Esperado |
|---|---|---|
| G1 | Comprar un recluta con cats | Cobra y se une |
| G2 | Sin fondos suficientes | *"pide N cats y tienes M"* |
| G3 | Liberar a un esclavo | Se une |
| G4 | Perder un miembro en combate | Ventana de piedad, 3 botones |
| G5 | **IGNORAR** a uno que puede pagar | Te deja sus cats y se va sin agro |
| G6 | **RECHAZAR** | Mantiene el agro, la pelea sigue |
| G7 | Aviso de cautivo listo | Llama al jugador |
| G8 | Retrato en la ventana | Se dibuja |
| G9 | Burbuja de habla sobre NPCs | Aparece |
| G10 | Click derecho a una jaula | Apunta al ocupante, **no crashea** |

**G10 es la más importante de esta lista.** Es la trampa que se llevó el proceso
una vez.

---

## 11. Qué reportar cuando algo falla

Con esto puedo diagnosticar sin pedirte nada más:

1. **El número de caso** (ej. `B2`).
2. **Las líneas del log** de ese momento — el bloque `--- probe:` entero si lo hay.
3. **Qué esperabas y qué pasó.**

El `[chance]` y el `--- probe:` juntos dicen casi siempre dónde está el problema,
porque muestran cada término por separado en vez de solo el resultado.

---

## 12. Ronda rebalance-and-ui

### Cabecera de la corrida

```
Run header
  Build / commit : ________   DLL md5: ________   (md5sum x64/Release, raiz del repo y carpeta del juego)
  Fecha          : ________   Idioma: es
  Save usado     : ________   Save nuevo o largo: ________
  Resultado      : Humo ___/4   Areas fallidas: ________
```

Antes de empezar: `tasklist | rg kenshi_x64` (si esta abierto, cerralo del todo), desplegar con
`scripts/deploy.sh` y verificar que los tres md5 coinciden (DEVELOPING.md §1b).

### Orden de prioridad

1. **Humo S1-S4** (§1). Si algo falla, nada mas importa.
2. **12.6 I4/I5**: el id estable sobrevive una recarga y un relanzamiento (lo de mayor riesgo: hooks sin verificar).
3. **12.8 U1, U2, U6, U13, U17**: la tarjeta (puntería, numeros iguales a los del clic, sin efectos secundarios).
4. **12.4 P1-P9**: el dinero (precio, cobro, reembolso).
5. El resto: 12.1 (rutas), 12.2 (piedad), 12.3 (modo facil), 12.5 (config), 12.7 (estructura), y lo que quede de 12.6 y 12.8.

Con `"probeOnClick": true`, cada click escribe `[route] <nombre>` en el log: es
la ruta que contestó primero (`field-yield`, `unconscious`, `grudge-refusal`,
`rescue`, `dying`, `easy-healthy`, `surrendered`, `free-world`, `cage`).

### 12.1 Orden de rutas (T2)

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| F1 | Un honorable, valiente o bondadoso que **recuerda que mataste a uno de los suyos**: vencerlo 2 veces **fuera** de la jaula y hablarle | Se niega: *"mataste a alguien suyo, y eso no se compra"*. **No** se rinde | `[route] grudge-refusal` |
| F2 | Mismo caso pero **dentro de tu jaula** | La jaula sí lo toca (el hambre y el tiempo no entienden de honor) | `[route] cage` |
| F3 | Un prisionero **tuyo** con 2 o más derrotas, sano y alimentado, con `easyHoursCaged` > 0 | Entra por el atajo del modo fácil, no por *"todavía no se quiebra"* | `[route] easy-healthy` |
| F4 | Un prisionero tuyo con 2 o más derrotas, hambriento o herido, con `easyHoursCaged` en 0 | Llega a los fallbacks de la jaula: cuidados, precio en cats, o intimidación; **no** el cartel de sumisión del campo | `[route] cage` y una línea `[chance]` con `defeats=` > 0 |
| F5 | Un NPC **libre** vencido 2 veces | Sigue cediendo (es la ruta del campo) | `[route] field-yield` |
| F6 | Vencer a alguien 2 veces, guardar, **salir al escritorio**, volver y hablarle fuera de la jaula | Sigue en la ruta del campo: el progreso sobrevivió | `[route] field-yield`; tras una derrota más, `count 3` |
| F7 | Un NPC pide piedad (perdió un miembro o sangre < 25%). Apretar **ACEPTAR** | **Se une gratis**, también un honorable o un valiente. Nunca un precio en cats ni *"su honor le impide aceptar sobornos"* | `Accepted the plea of <nombre>: joined without a price.` y **ningún** `pagaste` |
| F8 | Mismo caso, apretar **IGNORAR** (solo si puede comprar su vida) | Se va; si llevaba cats, te los deja | `Tribute of N cats from <nombre>` |

### 12.2 Ventana de piedad (T3)

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| M1 | Aceptar una súplica (ACEPTAR) y, **sin recargar**, dejar que otro NPC pierda un miembro | Se abre una **segunda** súplica | `Accepted the plea of <A>` y después el diálogo de piedad de <B> |
| M2 | Cerrar una súplica con la cruz de la ventana, y luego dejar que otro pierda un miembro | También se abre; antes quedaba bloqueado hasta recargar | Ninguna línea `Could not open the mercy window` |
| M3 | Ídem con RECHAZAR | Se abre la siguiente | — |
| M4 | Con una súplica abierta, hacerle click derecho al mismo NPC y abrir su oferta normal (si es un cautivo con precio) | ACEPTAR **cobra** según el veredicto; no lo regala como si fuera piedad | `pagaste N cats` en el mensaje |

### 12.3 Modo fácil y reloj de encierro (T4)

Con `probeOnClick` el reloj deja líneas `[hold]`.

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| E1 | Instalar con la config de fábrica y mirar el hub | `Horas jaula (modo facil)` arranca en **6**, igual para todos | — |
| E2 | Encerrar a alguien en una jaula tuya y quedarse cerca (< ~900 unidades) | El reloj arranca **al encerrarlo**, no al primer click | `<nombre> [hold] clock started: seen in your cage` |
| E3 | Esperar 6 horas de juego y hablarle | Se rinde por tiempo: *"lleva bastante encerrado y se rinde"*, sea honorable, único o cobarde | `[hold] held=6.0h easy-mode needs 6.0h` |
| E4 | Hablarle a las 3 horas | Todavía no (otras rutas pueden decidir) | `[hold] held=3.0h easy-mode needs 6.0h` |
| E5 | Encerrarlo un día, **sacarlo de la jaula** (que te vean pasar con él libre), volver a encerrarlo y hablarle enseguida | **No** se une al instante: el reloj empieza de cero | `[hold] clock dropped: no longer in your cage`, luego `clock started` y `held=0.0h` |
| E6 | Abrir el hub | **Ya no** están `Horas jaula (Orgullosos)` ni `Horas jaula (Únicos)` | S2: sin `Mod Hub rejected` |
| E7 | Cargar una config vieja que todavía trae `hoursCagedForProud` y `uniqueExtraHours` | Carga sin quejas; esas claves se ignoran | Sin errores de config en el log |
| E8 | Con `hailWhenReady` en `false`, repetir E2 y E3 | El reloj anda igual (el barrido sigue vivo) | Las mismas líneas `[hold]` |

### 12.4 Dinero: precio, cobro y reembolso (T5)

Con `probeOnClick` cada cotización deja una línea `[price]` con todos los factores. Anotar el nivel real de varios NPC (level en `[chance]`) y comparar con la curva: ~1.1k a nivel 5, ~3k a 10, ~9.6k a 20, ~14k a 25, ~32k a 40, ~45k a 50, ~57k a 60, ~76k a 75, antes de multiplicadores. Los valores son múltiplos de 10.

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| P1 | Hablarle a un cautivo con precio en cats, con la bolsa llena | La ventana muestra N cats; ACEPTAR cobra **exactamente** N | `Charged N cats for <nombre>.` y el mensaje *pagaste N cats*; la bolsa baja N |
| P2 | Lo mismo con **menos** cats que el precio | *pide N cats y tienes M*; si igual se fuerza ACEPTAR no se cobra nada | Sin línea `Charged`; mensaje *no tienes suficientes fondos* |
| P3 | Comprar a un cobarde de nivel 1 | El precio es el mínimo (200), no menos | `[price] ... -> 200 cats (floor 200, cap 150000)` |
| P4 | Comprar a un único fuerte con personalidad valiente | El precio nunca pasa de 150000 aunque los multiplicadores lo pidan | `[price] ... -> 150000 cats` |
| P5 | Provocar que el reclutamiento falle después de cobrar (si el motor lo rechaza) | La bolsa vuelve al valor de antes | `Refunded N cats: <nombre> could not be recruited.` |
| P6 | Rescatar a alguien que no puede seguirte y lleva cats | Te los deja una sola vez; la bolsa del NPC queda en 0 y la tuya sube exactamente eso | `<nombre> paid the player N cats for the rescue.`; sin segunda línea al reabrir |
| P7 | Soltar con IGNORAR a quien suplicó piedad y llevaba cats | Tu bolsa sube N y la suya queda en 0 | `Tribute of N cats from <nombre>` |
| P8 | Cambiar cualquier ajuste en el hub y reiniciar el juego | Los precios no cambian respecto a antes de tocar el hub (techo, pendiente, punto medio, talento y únicos se conservan) | `[price]` con los mismos factores |
| P9 | Mirar el hub | Aparecen `Talento: tope` y `Cats: tope final`; `Cats por nivel` llega a 300 | S2: sin `Mod Hub rejected` |

### 12.5 Escritor de config (T6)

Cada guardado deja una línea `Saved config to ...config.json (N changed, M added)`. `SaveConfig` corre cada vez que se toca un ajuste del Mod Hub.

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| C1 | Cambiar un ajuste en el hub y abrir `config.json` | Solo cambió ese valor: mismo orden, mismas líneas en blanco y espacios, ninguna clave perdida | `(1 changed, 0 added)` |
| C2 | Antes de tocar el hub, agregar al `config.json` una línea `// mis notas`, una clave propia `"miClave": 5` y un `/* comentario */`. Cambiar un ajuste | Los tres siguen donde estaban | `(1 changed, ...)` |
| C3 | Borrar `catsHardCap` del `config.json` y cambiar cualquier ajuste | La clave vuelve al final del archivo con su valor actual; la coma del último miembro queda bien y el archivo carga | `(1 changed, 1 added)` |
| C4 | Editar a mano `catsCeiling` (p. ej. 30000), cambiar otro ajuste del hub y reiniciar | `catsCeiling` conserva 30000 (antes se perdía y volvía al default) | `[price]` con la curva editada |
| C5 | Un valor escrito como `6.0` o `0.60` que el hub no tocó | Sigue escrito igual (`6.0`, `0.60`), no se reformatea a `6` / `0.6` | — |
| C6 | Borrar `config.json` y tocar un ajuste | Se crea un archivo completo, con el bloque `exclude` anidado | `(N changed, M added)` |
| C7 | Romper el `config.json` a mano (borrar la última llave) y tocar un ajuste | Queda `config.json.bak` con el texto roto y un `config.json` nuevo válido | `config.json is not valid JSON; kept a copy as ...config.json.bak` |
| C8 | Tras varios guardados mirar la carpeta del mod | No hay `config.json.tmp` | — |

### 12.6 Identidad estable y reinicio al cargar (T7)

Para ver la clave de un NPC: `"probeOnClick": true` y mirar la linea `[route] ... [key] id:<uuid>` (o `[key] handle:i-s` si cayo al respaldo). Todo cuelga de `RE_Kenshi_log.txt`. **La persistencia del id dentro del save no esta probada en juego** (ni en TotalSparring ni aca): I4 e I5 son la prueba.

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| I1 | Arrancar el juego | Los dos hooks se instalan | `[identity] Hooked Character::_DESTRUCTOR, _NV_loadFromSerialise, _NV_serialise (persisted character id).` y `[identity] Hooked SaveManager::loadGame / importGame (session reset at load begin).` |
| I2 | Cargar un save y hacer clic en un prisionero | Recibe un id nuevo (solo la primera vez que el mod lo toca) | `[identity] first character id created this session (<uuid>); it reaches the save at the next save.` y `[key] id:<uuid>` |
| I3 | Guardar la partida | El id se escribe en el save | `[identity] first character id written into a save state (<uuid>).` |
| I4 | Sin cerrar el juego, cargar ese mismo save y hacer clic en el mismo prisionero | Antes de cargar se limpia todo; despues el id se lee del save y es **el mismo uuid** de I2 | `[identity] world reset at load begin; the previous world read N id(s) from its save, created M and wrote K.`, `[identity] load begin: session state cleared (...)`, `[identity] first character id read back from the save (<uuid>)` |
| I5 | Guardar, cerrar Kenshi por completo, abrirlo, cargar y clic en el mismo prisionero | El mismo uuid (antes el handle cambiaba) y el **mismo % de unirse** que antes de cerrar (la tirada ya no se vuelve a sortear) | `first character id read back from the save` con el uuid de I2/I3; `[key] id:<uuid>` igual |
| I6 | Probar con TotalSparring (y Kill Counter) cargados a la vez | Los tres mods mantienen su id y funcionan: un duelo de TotalSparring y una oferta de TotalRecruit, sin cuelgues | las tres lineas `Hooked Character::_NV_serialise` (una por mod); tras guardar y cargar, `first fighter id read back` **y** `first character id read back` |
| I7 | Abrir un save de antes de este cambio (sin ids) y hacer clic en un NPC | Recibe un id nuevo; su **tirada cambia una vez** (la clave pasa de handle a id) y desde ahi es estable | `first character id created this session` |
| I8 | Dejarle a un prisionero 1+ derrotas de sesion y un reloj de encierro corriendo, luego cargar otro save (o el mismo) | Nada de lo anterior se arrastra: el reloj de encierro, los avisos (`hail`) y la insistencia empiezan de cero; las derrotas vuelven a lo que guardaba **ese** save | `[identity] load begin: session state cleared` |
| I9 | Tener abierta la ventana de oferta (o la de piedad) y cargar | La ventana se cierra sola, no queda el boton de reclutar apuntando a nadie | `[identity] load begin: session state cleared` |
| I10 | Si el log muestra `hook installation failed` | El mod sigue andando con clave por handle (como antes de T7), una sola vez en el log | `[identity] persisted character id unavailable; per-character state keyed by handle ...` |

### 12.7 Estructura, motivos y numeros sueltos (T8)

T8 es un refactor: **el comportamiento no debe cambiar** salvo A1 (golpiza) y la linea de rencor de Z3. Todo lo de abajo es para confirmar que nada se movio. Los textos al jugador salen ahora de `messages.json` (claves nuevas bajo `reason.*`); con el `messages.json` instalado de antes, el juego sigue andando con los textos internos y al cerrar el log lista las claves que faltan (`messages.json has no entry for: reason....`): es lo esperado hasta T11.

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| A1 | Prisionero en tu jaula, ventana de oferta, **GOLPEARLO** | Queda sin sentido y la derrota **cuenta siempre** (antes se perdia si el motor no nombraba al jugador como ganador); el contador de sesion sube de a 1 | `[defeat] credited to the player explicitly (beating): <nombre> (engine winner is the player / is someone else / unreadable)` y luego `<nombre> defeated by the player (N this session, ...)` |
| A2 | Volver a golpear al mismo prisionero enseguida | "todavia se esta recuperando del anterior": el enfriamiento (6h por defecto) corre desde ESE golpe. Antes, un golpe descartado no dejaba registro y se podia golpear sin espera | (sin linea nueva) |
| A3 | Con `probeOnClick`, golpear a alguien que el motor no registra como tuyo | Mismo `[defeat] credited ...`; **no** aparece `[defeat-skip] ... lastGuyWhoDefeatedMe` para ese golpe | `[defeat-skip]` ausente |
| Z1 | Hacer clic en un prisionero, un NPC libre, un aliado, un animal, un rescatado y un NPC con rencor | El texto del motivo en la ventana y en `Refused ...` es **el mismo de antes**, palabra por palabra | `Refused <nombre> (<moneda>): <motivo>` |
| Z2 | Insistir 3 veces a un NPC libre | Linea "¡Me estás hartando! Déjame en paz." y motivo "está harto de tu insistencia...". Editar `reason.insistence` en `messages.json` (otro texto) **no cambia** la linea que dice el NPC | `[route] free-world` |
| Z3 | Honorable, Valiente o Bondadoso que te guarda rencor (mataste a uno de los suyos), **fuera** de la jaula | Rechazo absoluto (`grudge-refusal`) y **siempre** dice "Sé lo que hiciste. Fuera de mi vista." Antes solo la decia si el texto del motivo contenia "mataste" (2 de las 7 variantes del `messages.json`); las otras 5 caian en "¿Quién eres tú? No tengo nada que hablar contigo." | `[route] grudge-refusal` |
| Z4 | El mismo NPC con rencor, pero **dentro** de tu jaula | Sigue la ruta de jaula (ya no hay rechazo absoluto), igual que antes | `[route] cage` |
| C1 | Hub: buscar los 19 controles nuevos | `Chance: peso de la reputacion`, `Chance: bono de aliado`, 9 `Personalidad: ...`, `Reputacion: honorables se niegan`, `Reputacion: nadie trata`, `Talento: peso de oficio`, `Talento: peso de sombra`, 4 de `Animal: ...` (niveles del descuento y factores del temperamento). Con los valores de fabrica, todo se comporta como antes | S2: sin `Mod Hub rejected` |
| C2 | Poner `"chanceRelationWeight": 0` (o moverlo en el hub) y hacer clic en un aliado | En `[chance]` el termino `relation=` pasa a 0 | `[chance] ... relation=0 ...` |
| C3 | Tocar un control nuevo en el hub y mirar `config.json` | La clave aparece (o cambia) en su lugar, sin perder ninguna otra edicion a mano (T6) | `Saved config to ... (N changed, M added)` |
| C4 | Cargar con un `config.json` viejo (sin las claves nuevas) | Carga normal con los valores de antes; no se escribe nada hasta tocar el hub | `Loaded config from ...` |
| R1 | Prisionero con 6h de encierro (modo fácil), NPC en rescate, cheat, animal con hambre, animal en jaula | Todas las rutas responden como antes (mismos motivos, mismos precios, misma barra de domesticación) | `[route] <ruta>`, `[chance] ...`, `[price] ...`, `[hold] held=...` |
| R2 | Disparar un plea por vida baja (mercy) y por golpiza hasta casi matar | La ventana de piedad sigue abriendose (ahora por el listener de `defeats`, ya no por una llamada directa) | `defeated by the player (...) nearDeath=yes` y la ventana |
| N1 | Insistir 5+ veces a un prisionero **en jaula** | **No** pasa nada: la insistencia no corta la charla de jaula (se registra el intento pero nadie consulta el castigo). R2.4 no dice nada de jaulas; queda como estaba y anotado en el doc de la feature | (sin `harto` en la linea del NPC) |

### 12.8 Lectura de estres y chance (T9)

La tarjeta de `ui-mockup/readout.html` sobre el NPC. Todo se prueba con `"probeOnClick": true` en `config.json`: cada vez que lo dibujado cambia, el log dice **lo mismo que se ve** en una linea `[readout]`, y se compara con `[chance]`/`[price]` de un clic en el mismo NPC (la tarjeta no calcula nada propio: tiene que coincidir). El mod necesita `gui/images/tr_white.png` en su carpeta (sin el, las barras usan el skin `WhiteSkin` y el log lo dice).

Linea: `[readout] <nombre> route=<ruta> chance=<n>% stress=<n>% (hunger H wounds W tired T) defeats=<a/b> food=<n>% line="<texto>" [hablar|hover]`

Desde 12.11 la tarjeta sale al **seleccionar** el NPC (clic izquierdo) y al apretar **Hablar**; el modo hover ya no existe. Las filas de esta tabla que dicen "pasar el mouse" se prueban seleccionando al NPC; las mismas comprobaciones de contenido valen. U4 ahora refresca cada ~2 s, U12 ya no aplica, U13 y U14 los reemplaza 12.11.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| U1 | Pasar el mouse sobre un prisionero en tu jaula (modo facil 6h, precio en cats) | Tarjeta sobre su cabeza: ruta **Cautiverio**, barra de estres con 3 colores y la leyenda `Hambre/Heridas/Cansancio` que suma el total, barra de chance, derrotas `n/2`, linea de precio `Precio N cats (tienes M)` y debajo `Faltan Xh en la jaula` | `[readout] ... route=captivity chance=..% stress=..% ... [hover]` |
| U2 | Hacer clic (menu contextual) en ese mismo prisionero y comparar | `chance=` de la tarjeta == `-> N%` de `[chance] ...` del clic; el precio de la tarjeta == `[price] ... -> N cats`; el estres == `stress=N(...%)` de `[chance]` | `[chance] ... -> N%`, `[price] ... -> N cats` |
| U3 | Esperar en la jaula hasta que pasen las 6h | La segunda linea baja `Faltan 3h` -> `2h` -> `45min` y al llegar a 0 la tarjeta dice `Lleva bastante encerrado: se rinde` en verde. El tiempo coincide con `[hold] held=` | `[hold] held=Xh easy-mode needs 6.0h` |
| U4 | Alimentar al prisionero (o curarlo) con la tarjeta a la vista | El estres baja en la tarjeta en menos de ~0.5 s | `[readout]` con otro `stress=` |
| U5 | NPC que venciste 2 veces en campo abierto (sin jaula) | Ruta **Rendicion**, derrotas `2/2` llenas, chance y estres, linea `Aun no cede: dejalo sin comer` (o la que corresponda) | `route=surrender` |
| U6 | Animal suelto con hambre | Ruta **Animal**, barra **Barriga** con la marca blanca en lo que necesita y `Hambre 56% / necesita 80%` (los numeros del motor, p. ej. un perro hueso a ~56%); sin barra de estres ni de chance | `route=animal ... food=56% line="Hambre 56% / necesita 80%"` |
| U7 | Darle de comer hasta pasar la marca | La barra se pone verde y dice `Comio de tu mano: te sigue` | `line="Comio de tu mano: te sigue"` |
| U8 | NPC honorable/valiente/bondadoso con rencor (mataste a uno de los suyos), fuera de la jaula | Ruta **Libre**, linea roja `Rencor: no perdona fuera de la jaula`, **sin** barras de chance ni estres | `route=free chance=- stress=- ... line="Rencor: ..."` |
| U9 | Aliado (reputacion >= 20) en campo abierto | Ruta **Libre** con barra de chance y `Quiza te siga; si no, hazte mas fuerte`. La tarjeta **no** dice si la tirada va a salir: hacer clic varias veces seguidas en el mismo NPC no cambia ni el % ni la linea | `route=free chance=..%` |
| U10 | Persona en rescate (la salvaste), inconsciente, y moribunda | Ruta **Rescate**; moribundo: `Se muere: curalo primero`; inconsciente: `Inconsciente: espera a que despierte` | `route=rescue` |
| U11 | Pasar el mouse por un NPC **no reclutable** (excluido por config, muerto, o tu propio escuadron) | No aparece tarjeta | (sin linea) |
| U12 | Mouse sobre una ventana del juego (inventario, etc.) con un NPC detras | La tarjeta se oculta (modo hover); vuelve al sacar el mouse | (sin linea) |
| U13 | Seleccionar un NPC que no es tuyo (clic izquierdo), con la configuracion por defecto | **No** aparece tarjeta: la seleccion ya no es una fuente (12.9) | (sin linea) |
| U14 | Abrir el menu contextual sobre un NPC, sin apretar Hablar | **No** aparece tarjeta: el menu abierto ya no es una fuente; el boton `Hablar` sigue a su lado | (sin linea) |
| U15 | Hub: `Lectura: activa` en OFF | Ninguna tarjeta; en ON vuelve en <0.5 s. `Lectura: compacta` ON: tarjeta de 176 px sin leyenda ni derrotas ni segunda linea | S2: sin `Mod Hub rejected` |
| U16 | Mover la camara / el NPC camina con la tarjeta visible | La tarjeta lo sigue cada cuadro, nunca se sale de la pantalla (se pega al borde) | (sin linea) |
| U17 | **No debe tener efectos**: pasar el mouse por un prisionero nuevo (sin id) en tu jaula, y por un NPC libre cualquiera | No aparece `first character id created` por pasar el mouse; el reloj de encierro **no** arranca por mirarlo (la linea `Faltan` aparece recien cuando el barrido de `hail` ya vio al prisionero); sin insistencia registrada | `[identity] first character id created` ausente tras solo pasar el mouse |
| U18 | Cargar un save con la tarjeta a la vista | La tarjeta desaparece sola; no queda apuntando a nadie | `[identity] load begin: session state cleared (offer window, overlay, readout and cursor targets, ...)` |
| U19 | Tarjeta sobre un NPC con **DarkUI o PowerModUI** activo | Fondo oscuro, texto y barras brillantes (no se ponen negros) | `[readout] fills use .../gui/images/tr_white.png` |
| U20 | La tarjeta **no roba clics**: clic izquierdo y derecho a traves de ella sobre el NPC | Funciona como si no estuviera | (sin linea) |
| U21 | Mirar el log al abrir el juego por primera vez con la tarjeta | Una linea `[readout] fills use ...tr_white.png`; si falta la carpeta `gui/images`, `[readout] no gui/images/tr_white.png ...` y las barras igual se ven | |

**Lo que hay que confirmar jugando** (no se puede saber sin correr el juego): que el mouse sobre un NPC se detecta (radio de 48 px sobre el centro del cuerpo, proyectando con la camara de Kenshi); que `getPositionBip01()` queda a la altura del torso y el ancla de 70 px queda sobre la cabeza; que la leyenda cabe en 264 px con la fuente `Kenshi_StandardFont_MediumSmall` (una linea larga se corta, no se ajusta).

### 12.9 Tarjeta al apretar Hablar y globo barato (price-readout-button-tweaks T2)

Por defecto la tarjeta sale **solo** al apretar `Hablar`, al lado de la ventana de oferta; el modo hover es opcional. El globo de chat (`showTalkCursor`) ahora pregunta como espectador y recuerda la respuesta por NPC 2 s. Con `"probeOnClick": true`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| H1 | Config por defecto. Pasar el mouse por NPCs reclutables **sin** seleccionarlos | **Ninguna** tarjeta (seleccionar uno: ver 12.11) | (sin `[readout]`) |
| H2 | Clic derecho en un prisionero, apretar `Hablar` | Se abre la oferta y la tarjeta aparece sobre su cabeza; si taparia la ventana, se corre **de costado** al lado mas cercano de la ventana, a la altura de su cabeza (nunca arriba ni abajo; ver 12.16) | `[readout] ... [hablar]` una sola vez |
| H3 | Con la oferta abierta, mover la camara | La tarjeta sigue al NPC cada cuadro y sigue sin tapar la oferta; su contenido no cambia (se calculo al clic) | (sin linea nueva) |
| H4 | Cerrar la oferta (ACEPTAR, RECHAZAR, la X) | La tarjeta se va con ella | |
| H5 | `Hablar` en alguien que se une sin ventana, o donde no se abre oferta | Sin tarjeta, o la tarjeta se va sola a los 8 s | |
| H6 | `Hablar` en un NPC y que muera / salga de pantalla con la tarjeta a la vista | La tarjeta desaparece | |
| H7 | Rescate automatico (un NPC que curaste se ofrece solo) | La oferta aparece **sin** tarjeta (el rescate no la pide) | |
| H8 | (Reemplazado por 12.11: el modo hover se quito) | | |
| H9 | Hub: `Lectura: activa` OFF y apretar `Hablar` | Oferta sin tarjeta | |
| H10 | Zona con muchos NPCs en pantalla (una ciudad, 20+ personajes) con la config por defecto | FPS normales (comparar con el mod apagado en el hub); antes caian por el globo y la tarjeta | |
| H11 | Globo de chat (`showTalkCursor` ON): mirar prisioneros reclutables y NPCs comunes | El globo sigue apareciendo sobre quien vale la pena hablar (incluye rescates con chance > 0); alimentar o golpear a uno cambia su globo en ~2 s | |
| H12 | **Sin efectos** del globo: mirar un prisionero nuevo en tu jaula sin hacer clic | No aparece `first character id created` ni `[route]`/`[hold]` por solo mirarlo | |
| H13 | Clic derecho en un NPC con `probeOnClick` OFF y luego ON | Con OFF no hay evaluacion extra al abrir el menu; con ON sigue la linea `Context menu: <nombre> -> ...` | `Context menu: ...` solo con probe |

### 12.10 Boton Hablar en un hueco libre (price-readout-button-tweaks T3)

`Hablar` ya no se pone siempre a la derecha del menu: busca el primer hueco libre (izquierda arriba, derecha arriba, izquierda debajo de lo que haya, derecha debajo) y tiene el tamaño y el look de los botones de TotalSparring y TotalLoot. Con **TotalSparring** y **TotalLoot** instalados.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| B1 | Clic derecho en un NPC reclutable que tambien muestre los botones de TotalSparring (`Sparring`, perfil) y de TotalLoot (`Lootear`, etc.) | `Hablar` no tapa ningun boton, ni siquiera en parte; ninguno de los otros tapa a `Hablar` | |
| B2 | Comparar `Hablar` con el boton de TotalSparring | Mismo alto (44) y ancho (160, o el del menu si es mas angosto), misma skin, fuente, texto ambar con sombra; al pasar el mouse se rellena ambar con texto oscuro, igual que los otros | |
| B3 | Con el menu abierto, quedarse quieto unos segundos y mover un poco la camara | `Hablar` no salta de lado ni parpadea entre cuadros | |
| B4 | Abrir el menu sobre un NPC pegado al borde **izquierdo** de la pantalla | `Hablar` va a la derecha del menu (o debajo de lo que ya este ahi), entero en pantalla | |
| B5 | Lo mismo pegado al borde **derecho** y al borde **inferior** | Queda entero en pantalla; si no hay hueco libre, se ajusta a la izquierda del menu dentro de la pantalla | |
| B6 | Solo TotalRecruit (sin los otros mods) | `Hablar` a la izquierda del menu, alineado arriba, 160 x 44 | |
| B7 | Clic en `Hablar` | Se abre la oferta como antes (con la tarjeta de 12.9) y el boton se va | |

### 12.11 Tarjeta al seleccionar el NPC (price-readout-button-tweaks T5)

La tarjeta sigue a la **seleccion**: clic izquierdo en un NPC que no es tuyo y aparece sobre su cabeza; se calcula al cambiar la seleccion y se refresca cada 2 s, no cada cuadro. `Hablar` la sigue poniendo al lado de la oferta. Hub: `Lectura: al seleccionar` (ON por defecto). Con `"probeOnClick": true`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| S1 | Clic izquierdo en un prisionero reclutable | Aparece la tarjeta sobre su cabeza al instante | `[readout] ... [select]` una vez |
| S2 | Deseleccionar (clic en el suelo, Esc, o seleccionar a uno de los tuyos) | La tarjeta se va | |
| S3 | Con uno seleccionado, seleccionar a otro NPC reclutable | La tarjeta cambia al nuevo, sin quedar la del anterior | `[readout] <nuevo> ... [select]` |
| S4 | Seleccionar un NPC **no** reclutable (excluido, muerto, tuyo) | Sin tarjeta | (sin linea) |
| S5 | Con la tarjeta a la vista, alimentar o golpear al prisionero | El estres o el precio cambian en ~2 s, sin volver a seleccionarlo | `[readout]` con otro `stress=` / precio |
| S6 | Zona con muchos NPCs (ciudad, 20+) con uno seleccionado; mover el mouse por encima de todos | FPS normales (comparar con `Lectura: activa` OFF); pasar el mouse no muestra nada | sin `[readout]` nuevos al mover el mouse |
| S7 | Con la tarjeta a la vista, clic derecho sobre ese NPC | El menu contextual y su boton `Hablar` (y los de TotalSparring / TotalLoot) quedan sin tapar: la tarjeta se oculta mientras el menu esta abierto y vuelve al cerrarlo | |
| S8 | Clic izquierdo y derecho a traves de la tarjeta | Funcionan como si no estuviera (no roba clics ni el foco del mouse) | |
| S9 | Seleccionado, apretar `Hablar` | La tarjeta se pone al lado de la oferta (no encima); al cerrar la oferta sigue sobre el NPC porque sigue seleccionado (recalculada) | `[readout] ... [hablar]` y luego `[select]` |
| S10 | `Hablar` sobre un NPC **sin** seleccionarlo, y cerrar la oferta | La tarjeta se va con la oferta | |
| S11 | Seleccionado y que el NPC salga de pantalla (mover la camara) y vuelva | Se oculta fuera de pantalla y reaparece sola al volver, sin linea nueva de log | |
| S12 | Hub: `Lectura: al seleccionar` OFF | Seleccionar ya no muestra nada; `Hablar` sigue mostrandola junto a la oferta | |
| S13 | `config.json` viejo con `"readoutOnHover": true` | Se ignora sin error; la tarjeta funciona por seleccion | sin `[config]` de error |
| S14 | Cargar un save con la tarjeta a la vista | Desaparece; no queda apuntando a nadie | `[identity] load begin ...` |

### 12.12 Hablar al soltar el clic derecho (price-readout-button-tweaks T6)

Como en TotalSparring y TotalLoot: mantener clic derecho sobre el NPC, deslizar el mouse hasta `Hablar` y soltar ejecuta la accion, igual que las opciones nativas del menu. El clic izquierdo sigue andando. Con `"probeOnClick": true` para ver los eventos ignorados.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| G1 | Mantener clic derecho sobre un NPC reclutable, deslizar hasta `Hablar` sin soltar, soltar encima | Se abre la oferta **una** vez | un solo `[readout] ... [hablar]` |
| G2 | Abrir el menu y hacer clic izquierdo en `Hablar` | Se abre la oferta **una** vez (el soltar del mismo clic no la abre de nuevo) | un solo `[readout] ... [hablar]` |
| G3 | Mantener clic derecho sobre el NPC y soltar en otro lado (el suelo, una opcion nativa) | `Hablar` no se ejecuta; la opcion nativa, si se solto sobre ella, funciona como siempre | sin `[readout] ... [hablar]` |
| G4 | Clic derecho rapido (apretar y soltar en el NPC) | El menu se abre normal y `Hablar` **no** se ejecuta solo | sin `[readout] ... [hablar]` |
| G5 | Mismo gesto (G1) sobre los botones de TotalSparring y TotalLoot en el mismo menu | Cada uno ejecuta solo su accion; `Hablar` no se dispara al soltar sobre otro boton | |
| G6 | Clic con la rueda o el boton del medio sobre `Hablar` | No hace nada | |
| G7 | Abrir el menu, cerrarlo sin elegir, abrirlo de nuevo y hacer G1 | Funciona igual (cada menu arma un gesto nuevo) | |

### 12.13 Derrotas en el save, una por noqueo (recruit-gating T1)

Con `"probeOnClick": true`. El conteo vive en el save del personaje y un noqueo cuenta una sola vez.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| K1 | Vencer dos veces al mismo NPC (fuera de una jaula), dejar que se levante entre medio | Dos derrotas; al hablarle entra por `field-yield` | `count 1` y luego `count 2` |
| K2 | Tras K1, guardar, salir del juego, volver a entrar y cargar | La tarjeta y el clic siguen diciendo 2 derrotas | `first character id read back from the save` |
| K3 | Cargar un save **anterior** a la segunda derrota | El conteo de ese save (1), no el de la sesion | |
| K4 | Noquearlo una vez y, mientras sigue inconsciente un rato largo, hacer clic derecho varias veces | Cuenta **una** sola | un `defeated by the player`; los demas `[defeat-skip] ...: already counted this knockout` |
| K5 | Un guardia que nunca venciste (ni en esta partida ni antes) | `defeats=0`; nunca `field-yield` | sin `defeated by the player` para el |
| K6 | Golpearlo (`GOLPEARLO`) en tu jaula | Cuenta una; el enfriamiento corre | `credited to the player explicitly (beating)` o un solo `defeated by the player` |
| K7 | Que otro NPC noquee a alguien y hacerle clic derecho mientras esta en el suelo | No cuenta | `[defeat-skip] ...: lastGuyWhoDefeatedMe is not yours` |

### 12.14 Rendirse en el campo: jaula ajena, insistencia y lealtad (recruit-gating T2)

`field-yield` ya no se salta las preguntas del mundo libre. Con `"probeOnClick": true`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| Y1 | Vencer 2 veces a un prisionero en la jaula de **otra** faccion y hablarle | "esta en la jaula de otro"; no se une | `[route] field-yield` y luego `Refused ...: está en la jaula de otro` |
| Y2 | Vencer 2 veces a un NPC libre, insistirle hasta que te ignore, hablarle de nuevo | "esta harto de tu insistencia"; no tira el dado | `[route] field-yield` y luego `Refused ...: está harto de tu insistencia` |
| Y3 | Vencer 2 veces a un guardia o soldado (no a un bandido) | Tira el dado con 25 menos que un bandido igual; puede unirse | `chance=` 25 por debajo del bandido |
| Y4 | Hub: `Sumision: lealtad al rendirse` en 0 y repetir Y3 | La misma chance que el bandido | |
| Y5 | `config.json` con `"submissionFieldYieldLoyaltyPenalty": 500` | Se lee como 100 (tope) | |

### 12.15 Ventana de reclutamiento unica (recruit-gating T4)

Toda salida de Hablar abre la misma ventana; ninguna termina en una notificacion. Comparar con
`ui-mockup/readout.html`, seccion 5 (los siete estados). Con `"probeOnClick": true`.

| # | Que hacer | Esperado |
|---|---|---|
| P1 | Hablar a un prisionero que se une por cats, con plata de sobra | Marco de Kenshi con titulo "Reclutamiento" en bronce; adentro placa oscura, retrato enmarcado, nombre en bronce y etiqueta "Cautiverio" ambar; "se unira a tu grupo por N cats"; caja PRECIO (ambar) / TIENES / PROBABILIDAD si tiro el dado; botones ACEPTAR (ambar) y CANCELAR. ACEPTAR cobra una vez ("pagaste N cats") y lo suma |
| P2 | El mismo con menos plata que el precio | Misma ventana; TIENES en rojo; linea roja "No te alcanzan los cats (te faltan N)"; **sin** ACEPTAR, solo CANCELAR a la derecha. Ninguna notificacion |
| P3 | Hablar a un soldado o paladin libre que se niega por lealtad / honor | Etiqueta "Libre"; "no se une todavia."; el motivo en rojo con barra roja; solo CERRAR |
| P4 | Hablar a un prisionero hambriento llevando comida | Motivo en rojo; DARLE COMIDA + CERRAR. DARLE COMIDA alimenta y reabre la ventana con el nuevo veredicto (Cancelar dice AHORA NO; si sigue con hambre, DARLE MAS COMIDA). En tu jaula, despierto y con `allowBeating`: ademas GOLPEAR y NOQUEAR (12.22) |
| P5 | Dejar a un enemigo lisiado para que pida piedad | Etiqueta "Piedad"; la suplica como cuerpo; ACEPTAR / IGNORAR / RECHAZAR (IGNORAR solo si su temperamento lo permite y no esta enjaulado, encadenado ni cargado) |
| P6 | Rescatar a alguien (curarlo cuando esta tirado) y hablarle | Etiqueta "Rescate"; PRECIO "Gratis" en verde; ACEPTAR / CANCELAR |
| P7 | Vencer 2 veces a un bandido hasta que se rinda y hablarle | Etiqueta "Sometido" (desde 12.16; "Rendicion" queda para el que se rinde antes que morir); PRECIO "Gratis"; PROBABILIDAD con el % tirado; ACEPTAR / CANCELAR |
| P8 | En P1, la X del titulo | Igual que CANCELAR: "lo dejaste donde estaba", no cobra |
| P9 | En P1, gastar plata con la ventana abierta hasta quedar corto y apretar ACEPTAR | No lo suma; aviso `money.short` (el unico aviso de dinero que queda: el chequeo de ultimo momento) |
| P10 | Un nombre largo o un motivo largo | El texto se parte en renglones y la ventana crece; nada queda cortado ni fuera del marco |
| P11 | Editar `TotalRecruit/messages.json` bloque `popup` (por ejemplo `popup.button.cancel`) y reiniciar | La ventana usa el texto nuevo; borrar la clave vuelve al texto interno |
| P12 | Con DarkUI o PowerModUI | Los colores se ven igual (rellenos con `tr_white.png`) |

### 12.16 La tarjeta sobre la cabeza y lineas que no mienten (recruit-gating T5)

Antes la tarjeta salia "volando arriba de la pantalla": el punto del NPC (espacio del juego) pasaba por la
camara (espacio de render, corrido por el origen relativo de la escena) sin restar ese origen, y caia en el
horizonte. Ahora se resta como lo hace el propio motor. Con `"probeOnClick": true`: cada resultado se loguea
**una vez** por NPC:

`[readout-anchor] <nombre> [select|hablar] <resultado> head=(x,y) view=WxH world=(x,y,z) origin=(x,y,z) [card=(l,t wxh)] [offer=(l,t wxh)]`

`head` es el punto proyectado sobre la cabeza (el borde de abajo de la tarjeta queda 8 px encima),
`card` donde quedo la tarjeta, `offer` la ventana. Resultados: `shown over the head`, `shown beside the offer`,
`hidden: no side of the offer has room`, `hidden: anchor failed (behind the camera|off the screen|no camera|no position)`,
`hidden: right-click menu open`, `over: dead`, `over: the dead-read failed`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| A1 | Seleccionar (clic izquierdo) un NPC ajeno en el centro de la pantalla | La tarjeta aparece **justo sobre su cabeza**, centrada en el | `[readout-anchor] ... [select] shown over the head head=(x,y) ...` con `x,y` cerca de la cabeza en la captura |
| A2 | Con A1 a la vista, mover la camara, hacer zoom y dejar que el NPC camine | La tarjeta lo sigue cada cuadro sobre la cabeza; con zoom lejano baja junto con la cabeza; nunca se va al borde de arriba | (sin linea nueva: ya se logueo) |
| A3 | Llevar al NPC al borde de arriba de la pantalla | La tarjeta cuelga **debajo** de la cabeza, no pegada arriba | |
| A4 | Girar la camara hasta que el NPC quede detras o fuera de pantalla | La tarjeta se oculta; vuelve al verlo | `hidden: anchor failed (behind the camera)` o `(off the screen)` |
| A5 | Clic derecho en un prisionero, `Hablar` | Se abre la ventana; la tarjeta queda sobre la cabeza si no la tapa; si la tapa, se corre al costado mas cercano de la ventana **a la altura de la cabeza**. **Nunca** arriba de la pantalla | `[hablar] shown beside the offer ... card=(l,t ...) offer=(...)`: `t` igual a la altura sobre la cabeza, `l` al lado de `offer` |
| A6 | A5 con resolucion chica (o NPC justo en el centro con la ventana ancha) hasta que no haya lugar a ningun lado | La tarjeta se oculta mientras la ventana esta abierta; vuelve al cerrarla si sigue seleccionado | `hidden: no side of the offer has room` |
| A7 | Vencer 2 veces a un NPC en campo abierto y seleccionarlo | Etiqueta **Sometido**; la linea dice `NN%: puede unirse si le hablas` con el mismo % que la barra, **nunca** "Aun no cede" (la tarjeta no puede tirar el dado) | `[readout] ... route=field-yield chance=NN% ... line="NN%: puede unirse si le hablas"` |
| A8 | A7 y apretar `Hablar` | La ventana dice lo que sale del dado; si se une, la tarjeta no lo habia negado | `[chance] ... -> NN%` con el mismo `NN` |
| A9 | Aliado (reputacion >= 20) en campo abierto, seleccionarlo | `NN%: puede unirse si le hablas` en vez de "Quiza te siga" | `route=free ... line="NN%: ..."` |
| A10 | Cualquier NPC **libre** (fuera de tu jaula) en cualquier ruta | Ninguna linea dice "dejalo sin comer" | (buscar `sin comer` en el log: solo para presos en tu jaula) |

### 12.17 La tarjeta se va con un clic en otro lado (fix recruit-gating-and-popup)

Kenshi no limpia la seleccion al caminar con clic derecho, asi que la tarjeta quedaba para siempre. Ahora un clic izquierdo o derecho que no cae en un widget MyGUI ni sobre el NPC la guarda. Con `probeOnClick` el log dice `[readout] <nombre> dismissed: left|right` y `[readout] <nombre> shown again: ...`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| C1 | Clic izquierdo en un NPC reclutable | Aparece la tarjeta sobre su cabeza | `[readout] ... [select]` |
| C2 | Con la tarjeta a la vista, clic derecho en el suelo para caminar | La tarjeta se va al instante; el personaje camina igual | `[readout] <nombre> dismissed: right` |
| C3 | Volver a C1 y hacer clic izquierdo en el suelo | La tarjeta se va | `dismissed: left` |
| C4 | Con la tarjeta guardada, clic izquierdo sobre el mismo NPC (cuerpo, no solo la cabeza) | La tarjeta vuelve, con numeros al dia | `shown again: left` |
| C5 | Con la tarjeta a la vista, clic en la ventana de oferta, en el Mod Hub o en el boton `Hablar` | La tarjeta **se queda** | (sin `dismissed`) |
| C6 | Con la tarjeta guardada, seleccionar a otro NPC reclutable | La tarjeta aparece para el nuevo | `shown again: ... (selection)` o un `[readout] ... [select]` nuevo |
| C7 | Con la tarjeta a la vista, clic derecho sobre el NPC | Se abre el menu (la tarjeta se oculta mientras esta abierto, como antes); al cerrarlo con Esc vuelve | (sin `dismissed`) |
| C8 | Con la tarjeta guardada, clic derecho en el NPC y `Hablar` | La tarjeta vuelve al lado de la oferta; clics en el mundo con la oferta abierta no la guardan; se va con la oferta | `shown again: ... (hablar)` |
| C9 | Guardar la tarjeta y cargar un save | Nada queda guardado: el primer NPC que se seleccione muestra su tarjeta | `[identity] load begin ...` |

### 12.18 Regateo en la ventana de reclutamiento (haggle-and-daily-roll T4)

Solo para precios en cats (soborno en el mundo libre y la ruta de cautiverio con precio). Mockup: `ui-mockup/readout.html` seccion 5, estados 2 a 4 (el slider se arrastra). Con `probeOnClick` cada OFRECER deja `[haggle] <nombre> ask=.. offer=.. odds=..% -> accepted|offended`. Config por defecto: `haggleEnabled` true, minimo 0.5 del pedido, peso del descuento 2.0, ofensa +20%.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| H1 | `Hablar` con un prisionero o mercenario que pide cats y te alcanza | Debajo de la caja de datos: OFERTA, el slider con el pulgar a la derecha, el pedido en ambar, el minimo y el maximo debajo, "Probabilidad de aceptar: 100%" en verde; botones ACEPTAR + CANCELAR | — |
| H2 | Arrastrar el pulgar (y hacer clic en otro punto de la fila) | El valor baja de a 10 cats, la probabilidad baja en vivo y cambia de color (verde >= 70, ambar 30-69, rojo < 30); bajo el pedido el boton dice OFRECER; la ventana no cambia de tamano ni parpadea | — |
| H3 | Volver el pulgar al extremo derecho | Vuelve a decir ACEPTAR y 100% | — |
| H4 | ACEPTAR en el pedido | Igual que antes: cobra el precio pedido y se une | `Charged N cats for ...` |
| H5 | OFRECER con probabilidad alta (dejarlo cerca del pedido) hasta que uno acepte | Cobra **la oferta**, no el pedido: `pagaste <oferta> cats` y "<nombre>: acepta <oferta> cats y se une a tu grupo" | `[haggle] ... -> accepted`, `Charged <oferta> cats` |
| H6 | OFRECER con probabilidad baja | "<nombre>: se ofende con tu oferta"; la ventana se reabre sin slider, con "Se ofendio: hoy no regatea" en rojo y el PRECIO subido un 20% | `[haggle] ... -> offended` |
| H7 | Mismo dia: cerrar y volver a `Hablar` | Sigue ofendido: sin slider, precio subido; la tarjeta tambien muestra el precio subido | — |
| H8 | Esperar a la medianoche del juego (o dormir) y volver a `Hablar` | Sin ofensa: slider de vuelta y el precio normal | — |
| H9 | Mismo dia, misma oferta, otro intento tras cargar el save | La ofensa se olvida al cargar (solo memoria); la misma oferta el mismo dia da el mismo resultado (semilla diaria) | `[haggle] ...` igual que antes de cargar |
| H10 | Un mercenario que pide mas de lo que tenes | Al abrir: TIENES en rojo, "No te alcanzan para esa oferta (te faltan N)" en la linea del slider y solo CANCELAR; bajando la oferta por debajo de tu bolsa aparece OFRECER | — |
| H11 | Rescate, piedad, rendicion / sometido y un rechazo sin precio | Ninguno tiene slider ni linea de regateo | — |
| H12 | `haggleEnabled` en false (Mod Hub "Regateo: activo") y `Hablar` a alguien con precio | La ventana de siempre, sin slider | — |

### 12.19 Golpear y Noquear al preso de tu jaula (cage-animals-submission T1)

Golpear y Noquear son ordenes del propio juego (`BEAT_CAGE_OCCUPANT`, `STEALTH_KNOCKOUT`) que da el personaje **seleccionado**: el mod no calcula daño ni probabilidad. Si en 1,5 s el personaje no cambio de tarea, corre el respaldo (Golpear: el ataque de TotalSparring `FOCUSED_MELEE_ATTACK`; Noquear: `MedicalSystem::knockout` con su habilidad de asesinato). Mockup: `ui-mockup/readout.html` seccion 2, "Menu on your prisoner". Reemplaza A1-A3 (12.7) y K6 (12.13): el viejo GOLPEARLO (desmayo forzado de 60 s) ya no existe y la clave `submissionBeatingKnockoutSeconds` no se lee. Con `probeOnClick`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| C1 | Seleccionar a uno de los tuyos, clic derecho sobre un preso **despierto** en tu jaula | Al lado del menu, un bloque de tres: Hablar, Golpear, "Noquear NN%" (o "Noquear" si el juego no da probabilidad); el bloque no tapa los botones de TotalSparring / TotalLoot | `Context menu: <nombre> -> ... [cage commands]` |
| C2 | Clic derecho sobre un NPC libre, un preso inconsciente, o con `allowBeating` apagado | Solo Hablar | sin `[cage commands]` |
| C3 | Golpear (clic izquierdo, y otra vez con el gesto: mantener clic derecho, deslizar, soltar) | El seleccionado va a la jaula y lo golpea con sus propios golpes; una sola orden por gesto | `[cage] <tuyo> -> <preso> beat ordered (task 475, was N, checkPlayerOrderForProblems=...)` y luego `[cage] ... beat via task` (o `via fallback`) |
| C4 | Noquear | El seleccionado intenta el noqueo sigiloso; sale o no segun su habilidad (el % del boton) | `[cage] ... knockout ordered (task 507, ...)` y `... knockout via task` o `via fallback` |
| C5 | Cuando el preso cae por C3 o C4 | La derrota cuenta para el jugador aunque el juego no nombre a nadie como ganador | `[defeat-hook] knockout fired for <preso>`, `[defeat] credited to the player explicitly (cage command): <preso>` (o un `defeated by the player` normal) |
| C6 | Golpear o Noquear sin nadie de tu grupo seleccionado | "Selecciona a alguien de tu grupo para hacerlo"; no pasa nada mas | — |
| C7 | Rechazo en la ventana de un preso de tu jaula: **GOLPEAR** o **NOQUEAR** (ver 12.22) | La ventana se cierra y el seleccionado va a golpearlo / noquearlo (la misma orden que C3 / C4); sin seleccionado, el mensaje de C6 | igual que C3 / C4 |
| C8 | Anotar si alguna vez aparece `via fallback` y para que orden | Evidencia: si el juego no toma `BEAT_CAGE_OCCUPANT` / `STEALTH_KNOCKOUT` desde un plugin, el respaldo es lo que se ve; reportar tambien el valor de `checkPlayerOrderForProblems` | `via fallback` |
| C9 | Dar la orden y cargar un save antes de que se resuelva | Nada queda apuntando a nadie | `load begin: session state cleared (... cage command watch and credits ...)` |

### 12.20 Los animales solo se doman con tu comida (cage-animals-submission T2)

Un animal se une solo si el jugador le dio de comer al menos una vez **y** su barriga llego a la barra. Las comidas se guardan en el save del animal (`total_recruit.fed_by_player`). Con `probeOnClick`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| F1 | Clic derecho y Hablar con un animal salvaje lleno que nunca alimentaste (Cabra, Goat Alpha) | **No** se une: "todavia no ha comido de tu mano"; la ventana ofrece DARLE COMIDA si llevas comida; la tarjeta dice "No comio de tu mano: dale comida" con la barriga llena | `Context menu: <animal> -> refuses (care) todavia no ha comido de tu mano` |
| F2 | DARLE COMIDA a ese animal | Come; la ventana se reabre y ahora se une ("comio de tu mano y te sigue") si la barriga sigue sobre la barra | `Fed <animal> with <item>. hunger a -> b (...); meals from the player 1` |
| F3 | Un animal con hambre (barriga bajo la barra) que nunca alimentaste | "todavia tiene hambre: le falta N% de barriga" y DARLE COMIDA; tras comer lo suficiente se une | `meals from the player N` |
| F4 | Animal en tu jaula, sin alimentar, mas de 6 h (espera facil) | No se une por tiempo: el mismo motivo que F1 | sin `easyWait` para el |
| F5 | Animal en tu jaula sano y lleno, sin alimentar | No hay "intimidado por tu poder" ni "Desconfia": el motivo es F1 | — |
| F6 | Animal que rescataste (te recuerda como salvador) y sin alimentar | No entra por rescate ni por "sano gracias a ti": pide comida (F1) | `[route] free-world` o `cage`, nunca `rescue` / `easy-healthy` |
| F7 | Alimentar un animal, guardar, salir del juego, volver y cargar | Sigue contando como alimentado: si esta lleno se une | `first character id read back from the save` |
| F8 | Cargar un save **anterior** a la comida | Vuelve a "todavia no ha comido de tu mano" | — |
| F9 | Dar de comer a una **persona** presa | Igual que antes (el motor la alimenta); no se registra comida del jugador | `Fed <nombre> ... (engine)` sin `meals from the player` |

### 12.21 El sometido viene a pedirte unirse (cage-animals-submission T3)

Un NPC vencido que se rinde (derrotas >= `submissionDefeatsToBreak` y la tirada del dia pasa, o huye / se desangra tras perder contigo) camina solo hasta el lider del grupo, calmado, y abre la ventana con ACEPTAR / PERDONAR / RECHAZAR. Mockup: `ui-mockup/readout.html` seccion 5, "Submitted NPC asks to join". Port del evento de desafio de TotalSparring. Con `probeOnClick` para ver el resto del log; las lineas `[submission]` salen siempre.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| S1 | Vencer a un NPC **hostil** (bandido) hasta que ceda: noquearlo `submissionDefeatsToBreak` veces y esperar a que se levante, cerca (<40 m) | A los pocos segundos de levantarse dice "Espera... no quiero pelear mas", el registro dice "<nombre> se acerca para hablar contigo" y camina hasta tu lider | `[submission] <nombre> submitted (submission.broken|reason.surrendered), walks up from N m, defeats N; hostile before yes [key id:...]` |
| S2 | Mirar al NPC mientras camina (hasta 60 s) | **No ataca** a nadie de tu grupo; si alguien de tu grupo lo ataca a mano, a los ~3 s el evento se cancela y vuelve a ser hostil | `approach aborted: combat: ...` solo en el segundo caso |
| S3 | Al llegar (~3 m) | Se queda quieto mirandote y se abre la ventana: etiqueta Sometido, "<nombre> se arrodilla y te pide unirse a tu grupo.", PRECIO Gratis, botones ACEPTAR / PERDONAR / RECHAZAR | `[submission] <nombre>: arrived: ...` |
| S4 | ACEPTAR | Se une al grupo; "<nombre>: se une a ti." | `[submission] <nombre>: accepted ...`, `joined the squad` |
| S5 | (otro NPC) PERDONAR | "Gracias... no lo olvidare"; sale corriendo ~80 m en direccion contraria a ti y se queda tranquilo; durante 5 minutos no ataca y **Hablar** sigue funcionando con el | `forgiven ...`, `calm for 300 s, running off`, `ran off; stays calm` |
| S6 | Esperar los 5 minutos de S5 | Vuelve a su rutina y a su velocidad normal; si era hostil, vuelve a ser hostil | `forgive window over after 300 s (time up)`, `forgive window over (from forgiven, hostile before yes)` |
| S7 | (otro NPC) RECHAZAR | Dice "Entonces nos volveremos a ver" y vuelve a ser hostil al instante; "<nombre>: lo rechazaste. No querra saber nada de ti en 72 h." | `rejected ...`, `[defeat] <nombre>: defeats cleared`, `reject cooldown until game hour N (72 h)` |
| S8 | Tras S7, Hablar con el (o meterlo en tu jaula y Hablar) | Ventana de rechazo: "No quiere saber nada de ti"; la tarjeta dice lo mismo; tampoco ruega piedad si pierde un miembro | `Context menu: <nombre> -> refuses ... no quiere saber nada de ti` |
| S9 | Tras S7, guardar, salir, volver a cargar y Hablar con el | Sigue en enfriamiento hasta que pasen las 72 h de juego; despues se puede volver a vencer y a reclutar | `first character id read back from the save` |
| S10 | Cerrar la ventana de S3 con la X | Ninguna decision: se queda calmado donde esta durante 5 minutos y luego vuelve a lo suyo (como S6, sin correr) | `dismissed ...`, `calm for 300 s, in place` |
| S11 | Alejarte mas de 120 m mientras camina, o dejar pasar 60 s sin que llegue | Se cancela: vuelve a su rutina y, si era hostil, ataca de nuevo | `approach aborted: too far: ...` / `timeout: ...` |
| S12 | Pausar el juego mientras camina | El reloj de 60 s se congela; al despausar sigue | — |
| S13 | Tener la ventana de otro NPC abierta cuando llega | Se queda parado mirandote hasta que cierres la otra ventana; entonces abre la suya (si no pasaron los 60 s) | `arrived, waits for the window to be free` |
| S14 | Cargar un save mientras camina o con la ventana abierta | Nada queda colgado; el NPC del mundo cargado no recuerda el evento | `load begin: session state cleared (... submission event ... reject cooldowns ...)` |
| S15 | Vencer al mismo NPC otra vez despues de PERDONAR | Vuelve a acercarse (una vez por paliza, no una vez por sesion) | `submitted (...)` de nuevo |
| S16 | `submissionEventEnabled` apagado en Mod Hub | Ningun NPC viene; Hablar sigue igual | sin `[submission]` |

### 12.22 La ventana de un preso: GOLPEAR / NOQUEAR, nunca IGNORAR / PERDONAR (fix tras T3)

Reporte del usuario (2026-10-03): un "Holy Warden" en tu jaula, Cautiverio 87%, "Sigue desafiante: dejalo con hambre o hierelo", y solo CERRAR. Regla: todo rechazo de un preso para el que el menu contextual ofreceria Golpear / Noquear (en tu jaula, despierto, `allowBeating`; `CageCommandsAllowed`) lleva GOLPEAR y NOQUEAR en la ventana. Quien no puede irse (jaula, cadenas, cargado) nunca ve IGNORAR ni PERDONAR. Mockup: `ui-mockup/readout.html` seccion 5, "Caged and defiant", "Caged and hungry", "Mercy plea from someone who cannot leave".

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| W1 | Hablar con un preso **despierto** y desafiante en tu jaula, con uno de los tuyos seleccionado | Motivo en rojo "...dejalo con hambre o hierelo"; botones GOLPEAR, "NOQUEAR NN%" (el mismo % que el menu contextual) y CERRAR, en una fila | — |
| W2 | GOLPEAR en W1 | La ventana se cierra; el seleccionado va a golpearlo (como C3) | `[cage] ... beat ordered ...` |
| W3 | NOQUEAR en W1 | La ventana se cierra; el seleccionado intenta el noqueo (como C4) | `[cage] ... knockout ordered ...` |
| W4 | W2 / W3 sin nadie seleccionado | "Selecciona a alguien de tu grupo para hacerlo" | — |
| W5 | El mismo preso con hambre, llevando comida | DARLE COMIDA, GOLPEAR, NOQUEAR en la primera fila y CERRAR solo a la derecha en una segunda | — |
| W6 | Un preso de tu jaula que **si** se une (intimidado, alimentado y curado...) | Solo ACEPTAR / CANCELAR | — |
| W7 | `allowBeating` apagado (Mod Hub "Sumision: permitir golpear") y W1 | Sin GOLPEAR ni NOQUEAR; el motivo dice "Sigue desafiante: dejalo con hambre" (no pide herirlo) | — |
| W8 | Preso inconsciente en tu jaula, o un preso en la jaula de otro | Sin GOLPEAR ni NOQUEAR (como C2) | — |
| W9 | Un preso de tu jaula lisiado que pide piedad (si llega a pasar) | ACEPTAR / RECHAZAR, **nunca** IGNORAR | — |
| W10 | Durante el evento de 12.21, levantar (cargar) al NPC antes de que llegue | Si la ventana se abre, ACEPTAR / RECHAZAR, **nunca** PERDONAR | — |

### 12.23 Quien quiere hablar, sobre su cabeza (head notice)

Pedido del usuario (2026-10-03): "el juego te avisa, pero si no lees el nombre no sabes quien quiere hablar". Quien te llama lleva un cartel sobre la cabeza: su nombre y "Quiere hablar contigo" (cautivo listo, `hail.cpp`) o "Se acerca para hablar contigo" (el sometido, `submission_director.cpp`). Dura `headNoticeSeconds` (90 por defecto, segundos reales, congelado en pausa) cuando pide algo al jugador ("Quiere hablar contigo", "Se acerca para hablar contigo", "Quiere ofrecerte ayuda"); un cartel de una linea que solo informa (miembro temporal: union, faltas, aviso, despedida; ayudante: inicio, aviso, despedida, traicion, "Sigo contigo") dura `headNoticeInfoSeconds` (25 por defecto). Arreglo del play test (fix/head-notice-lifetime): "Te acompanare 5 dias" "nunca se iba" con 300 s. Mockup: `ui-mockup/readout.html` seccion 2, "Someone wants to talk" y "Select the one who called". Con `probeOnClick` el log dice `[notice]`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| N1 | Esperar a que un preso de tu jaula llame (`hailFirstHours`; o Cheat con `cheatHoursCaged` bajo) | Ademas del aviso en el registro, un cartel compacto sobre su cabeza: nombre en bronce, "Quiere hablar contigo" en ambar, borde ambar a la izquierda | `[notice] <nombre>: "Quiere hablar contigo" over the head [key ...] (prompt)` |
| N2 | Mover la camara | El cartel sigue la cabeza cada cuadro; fuera de pantalla se oculta y vuelve al volver | — |
| N3 | Abrir un menu contextual con el cartel visible | El cartel se oculta mientras el menu esta abierto | — |
| N4 | Seleccionar (clic izquierdo) a **ese** NPC | El cartel se va para siempre y aparece la tarjeta normal; nunca los dos a la vez | `[notice] dismissed by selection [key ...]` (con `probeOnClick`) |
| N5 | Con el cartel sobre A, seleccionar a otro NPC B | B tiene la tarjeta normal; el cartel de A sigue | — |
| N6 | Clic izquierdo o derecho en el suelo con el cartel visible | El cartel sigue (solo seleccionarlo o el tiempo lo quitan) | — |
| N7 | B seleccionado justo al lado de A, de modo que la tarjeta de B tape el cartel de A | El cartel de A se oculta mientras se tocarian; la tarjeta gana | — |
| N8 | No hacer nada `headNoticeSeconds` (90 s por defecto; bajar a 10 s en Mod Hub "Aviso: segundos sobre la cabeza" para probar) | El cartel se va solo, a los ~90 s con el valor por defecto | — |
| N9 | Pausar el juego con el cartel visible | El tiempo del cartel se congela; al despausar sigue contando | — |
| N10 | Evento del sometido (12.21) | Al empezar a caminar: cartel "Se acerca para hablar contigo"; se va cuando abre la ventana o si se cancela el evento | `[notice] <nombre>: "Se acerca para hablar contigo" ...` |
| N11 | Que el NPC del cartel muera o se una al grupo | El cartel se va (en menos de 1 s) | — |
| N12 | Cargar un save con carteles visibles | No queda ningun cartel | — |
| N13 | Ventana de oferta abierta y el cartel caeria encima | Se corre de costado a la altura de la cabeza (como la tarjeta), u oculto sin lugar | — |
| N14 | Tomar un miembro temporal (12.25) y no hacer nada | "Te acompanare N dias" se va solo a los ~25 s (`headNoticeInfoSeconds`), no a los 90 | `[notice] <nombre>: "Te acompanare N dias" over the head [key ...] (squad) (info)` |
| N15 | Una falta o la despedida de un temporal; el inicio, aviso o despedida de un ayudante (12.26) | Cada linea dura ~25 s; la oferta del ayudante ("Quiere ofrecerte ayuda") dura ~90 s | `... (info)` para las lineas, `... (prompt)` para la oferta |
| N16 | Mod Hub "Aviso: segundos de un cartel informativo" a 5 s | Los carteles informativos se van a los ~5 s; "Quiere hablar contigo" sigue usando "Aviso: segundos sobre la cabeza" | — |
| N17 | Deploy (`bash scripts/deploy.sh`) sobre un `config.json` desplegado con `"headNoticeSeconds": 300` sin tocar | La salida dice `~ headNoticeSeconds: 300.0 -> 90.0 (still the old default)` y `+ headNoticeInfoSeconds`; con otro valor puesto a mano (p. ej. 120) dice `= headNoticeSeconds: 120.0 kept` y no lo cambia | — |

### 12.24 Aliados por nivel y descuento por amistad (ally-tiers-temp-helpers T1)

Pedido del usuario (2026-10-03): en Balanced, el nivel y la personalidad de un aliado deciden el trato (REQUISITOS §4b). Preset 1 (Balanced), `allyTierEnabled` encendido, un aliado = relacion >= `friendlyRelation` (cursor amarillo). El nivel del jugador es el del lider del escuadron activo. Mockup: `ui-mockup/readout.html` seccion 5, los estados "Ally ...". Con `probeOnClick` el log del precio muestra `faction=x0.86` para un descuento de 14%.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| A1 | Hablar con un aliado Honorable o Calido de nivel < 20 | PRECIO Gratis, PROBABILIDAD (la tirada de hoy), cuerpo "<nombre> es tu aliado (nivel N) y se une gratis por amistad."; si la tirada no da, "te aprecia, pero te mide" | `[chance] ...` |
| A2 | Aliado Normal (u otro no Bueno) de nivel 20-49, relacion 40 | Cuerpo "Aliado (nivel N): <nombre> te cobra X cats, con 20% de descuento por amistad."; PRECIO "X cats  -20%"; sin PROBABILIDAD; regateo con slider sobre X | `[price] ... faction=x0.80 -> X cats` |
| A3 | El mismo aliado sin cats suficientes | TIENES en rojo, "No te alcanzan..." (o la linea del slider), sin ACEPTAR; la tarjeta dice "Aliado: X cats (tienes Y)" | — |
| A4 | Aliado de nivel 50-79 con relacion alta | Descuento tope 10% ("-10%") | `faction=x0.90` |
| A5 | Aliado Traicionero o Astuto de nivel 80-99 | Paga con hasta 10% de descuento | — |
| A6 | Aliado Bueno o Neutral de nivel 80+ y tu lider a 15 niveles o menos | PRECIO Gratis, "Aliado (nivel N): <nombre> te acompanara D dias como miembro temporal." (D entre 3 y 10, el mismo todo el dia); ACEPTAR lo recluta | `<nombre> [temp] joined for D days (until hour H), home <sid>` |
| A7 | El mismo con el lider a mas de 15 niveles | Rechazo en rojo "Eres demasiado debil para que te acompane", CERRAR | — |
| A8 | Aliado Frio y cruel o Maniaco de nivel 80+ | Rechazo "no camina al lado de nadie: su crueldad no se pone a tus ordenes" | — |
| A9 | Aliado Traicionero o Astuto de nivel 100+ | Rechazo "vale mas de lo que podrias pagarle nunca" | — |
| A10 | Seleccionar al aliado de A2 (tarjeta) | Linea "Aliado: X cats con descuento" en verde, sin barra de chance | — |
| A11 | Preset 2 (Vanilla+) con el aliado de A2 | Como antes: una tirada diaria, PROBABILIDAD, sin "-N%" | — |
| A12 | Preset 0 (Cheat) | Sin cambios: jaula y un momento | — |
| A13 | Mod Hub "Aliados por nivel: activo" apagado, preset 1 | Todos los aliados vuelven a la tirada diaria | — |
| A14 | Mod Hub: bajar "Aliados: peso de la amistad" a 0.5 con relacion 28 | El descuento pasa a 14% | — |

### 12.25 El miembro temporal: avisos, maltrato y partida (ally-tiers-temp-helpers T2)

Un aliado Bueno o Neutral de nivel 80+ reclutado como en 12.24 A6. Preset 1 (Balanced). Defaults: 3 faltas, hambre 12 h seguidas bajo 25%, 12 h entre faltas del mismo tipo, aviso 12 h antes, 7 dias sin volver. Para no esperar dias de juego: Mod Hub "Temporal: horas de hambre" = 1, "Temporal: horas entre faltas" = 1, `allyTierTempMinDays` = `allyTierTempMaxDays` = 1, y la velocidad del juego al maximo. Mockup: `ui-mockup/readout.html` seccion 2, "Temporary member (squad notices)". Las faltas se loguean siempre (`[temp] strikes N (...)`).

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| T1 | Reclutar el aliado 80+ (popup "te acompanara D dias", ACEPTAR) | Se une; cartel sobre su cabeza "Te acompanare D dias" que se queda aunque ya es de tu grupo y aunque lo selecciones | `[temp] joined for D days (until hour H), home <sid>` y `[notice] ... (squad)` |
| T2 | Seleccionarlo | Su tarjeta: ruta Libre y una sola linea verde "Temporal: le quedan D dias", nada mas | — |
| T3 | Dejarlo sin comer hasta que el estomago baje del 25% y pasen las horas de hambre | Cartel "Tengo hambre... no me trates asi" | `[temp] strikes 1 (hunger 1, ...)` |
| T4 | Ordenarle a otro de tu grupo que lo ataque | Cartel "Si sigues asi me voy" (es la falta 2 de 3); un segundo golpe enseguida no suma otra | `[temp] strikes 2 (... attacked 1 ...) warned` |
| T5 | Seguir hasta que alguien de tu grupo lo noquee (pasada la espera entre faltas si fue el mismo tipo) | Falta 3: se va. Cartel "Me largo", roller "<nombre> se harto del maltrato y se fue de tu grupo."; desaparece de los retratos, vuelve a su faccion y se aleja caminando | `[temp] <nombre> left the squad for faction <sid>.` y `left: mistreated.` |
| T6 | Otro miembro temporal, bien tratado, hasta 12 h antes de su fin | Cartel "Me ire pronto" | `[temp] strikes 0 (...) expiry soon` |
| T7 | Dejar pasar su hora de vencimiento | Cartel "Me voy. Fue un honor", roller "<nombre> cumplio su tiempo y se fue de tu grupo."; vuelve a su faccion (la de antes de reclutarlo) en un pelotón propio y camina lejos; ya no es controlable | `left the squad for faction <sid>`, `left: time up.` |
| T8 | Guardar con un miembro temporal a mitad de su estadia, salir del juego, volver a cargar | Su tarjeta dice los mismos dias que antes (menos el tiempo pasado); las faltas que tenia siguen contando (con 2, una falta mas lo hace irse) | — |
| T9 | Despues de T5 o T7, Hablar con el mismo NPC | "no quiere saber nada de ti" durante 7 dias de juego; no se lo puede volver a tomar como temporal | — |
| T10 | Un miembro temporal golpeado por enemigos (no tu grupo) o noqueado por hambre | No suma falta | — |
| T11 | Faccion de origen que no existe o es del jugador (raro) | Se va igual a "1233-gamedata.base" o a la faccion vacia | `left the squad for faction 1233-gamedata.base` |

### 12.26 El ayudante gratis (ally-tiers-temp-helpers T3)

Pedido del usuario (2026-10-03): aliados mas fuertes y de buena personalidad se ofrecen a acompanarte unos dias, gratis, sin que los puedas controlar (REQUISITOS R4b.6). Preset 1 (Balanced). **Mecanismo B (escolta por codigo)**: lo primero a probar es que la escolta y la pelea funcionan en juego; si un peldaño no sigue al lider, el log dice cual (`engine task now N on rung ...`, `left behind ... ->`). Para probar sin esperar la tirada: Mod Hub "Ayudante: tecla de prueba" = 119 (F8) o `"helperDebugKey": 119` en `config.json`; seleccionar un NPC ajeno y apretar F8. Para no esperar dias: `helperDaysMin` = `helperDaysMax` = 1 y la velocidad al maximo. Mockup: `ui-mockup/readout.html` seccion 5 "Free helper offers to escort you" y seccion 2 "Free helper (offer and escort notices)". Todo se loguea como `[helper]`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| H1 | Tecla de prueba: seleccionar un aliado ajeno y apretar F8 | Camina hasta tu lider con el cartel "Quiere ofrecerte ayuda" y dice "Eh, espera. Puedo ayudarte."; al llegar, el popup con la etiqueta Ayuda: "<nombre> te ofrece acompanarte D dias, gratis. No podras darle ordenes.", PRECIO Gratis, ACEPTAR / RECHAZAR | `[helper] debug key on <nombre>: screen says <motivo> (...)` y `[helper] <nombre> offers D days (debug key), walks up from X m` |
| H2 | F8 sin nada seleccionado o con uno tuyo | Roller "[TotalRecruit] Selecciona un NPC que no sea tuyo." | `[helper] debug key: select an NPC that is not yours first` |
| H3 | Natural: quedarse cerca (<40 m) de un aliado Bueno o Neutral con nivel > el de tu lider + 10 | Con 15% por dia y por NPC se ofrece como en H1; si no toca, no vuelve a tirar ese dia | `[helper] <nombre>: no offer today (level L vs leader P, ...)` o `offers D days (roll: ...)` |
| H4 | ACEPTAR | Cartel "Te cubro las espaldas", roller "<nombre> te acompanara D dias. No acepta ordenes."; no aparece en tus retratos; te sigue cuando caminas | `escorts the player for D days (until hour H)`, `escort order BODYGUARD given`, `engine task now N on rung BODYGUARD` |
| H5 | Seleccionarlo | Tarjeta con una sola linea verde "Ayudante: le quedan D dias" | — |
| H6 | Intentar darle ordenes (clic derecho en el suelo con el seleccionado) | No obedece: sigue escoltando a tu lider | — |
| H7 | Alejarse corriendo varios cientos de metros | Si BODYGUARD no lo trae, sube de peldaño y te alcanza | `left behind (X m), escort BODYGUARD -> FOLLOW_PLAYER_ORDER` (y `-> walk` si hace falta) |
| H8 | Que tu lider pelee con bandidos | Ataca al mismo blanco que tu lider | `fights for the leader against <enemigo>` |
| H9 | Pelea entre dos de tu grupo (sparring) o tu lider ataca a alguien de la faccion del ayudante | El ayudante no se mete | — |
| H10 | 12 h antes del final | Cartel "Me ire pronto" | `expiry warning (N h left)` |
| H11 | Dejar pasar el final | Cartel "Hasta aqui llego. Suerte", roller "<nombre> cumplio su palabra y sigue su camino."; vuelve a su rutina | `<nombre> stops helping: time up` |
| H12 | RECHAZAR en el popup | "Como quieras. Suerte en el camino.", roller "<nombre>: rechazaste su ayuda."; F8 o la tirada no lo hacen ofrecerse de nuevo por 3 dias (la tirada natural; F8 lo salta) | `offer rejected; no offer for 3 days` |
| H13 | Cerrar el popup con la X | Se va sin enfriamiento; no vuelve a preguntar ese dia | `offer dismissed (closed with no answer)` |
| H14 | Guardar a mitad del contrato, salir, cargar, acercarse | Cartel "Sigo contigo", vuelve a escoltar; la tarjeta dice los dias que quedaban | `re-armed after a load` |
| H15 | Ordenar a uno de tu grupo que lo ataque | Cartel "No vuelvo a ayudarte", roller "<nombre> dejo de ayudarte: tu grupo lo ataco."; deja de seguirte; 3 dias sin ofrecerse | `stops helping: attacked by the squad` |
| H16 | Noquearlo con tu grupo | Igual que H15 | `stops helping: knocked out by the squad` |
| H17 | Mientras un sometido camina hacia vos (12.x) | Ningun ayudante se ofrece hasta que termina, y al reves | — |
| H18 | Mod Hub "Ayudante: activo" apagado, o preset 0/2 | Nadie se ofrece (los contratos en curso siguen hasta su fin) | — |

### 12.27 La tarjeta y los carteles se ocultan con el dialogo de Kenshi (fix card-hide-on-dialogue)

Reporte del usuario (2026-10-03): al hablar con el NPC y abrirse el dialogo, la tarjeta no se ocultaba y tapaba la conversacion. Ahora la tarjeta (tambien la fijada por Hablar) y los carteles sobre la cabeza se ocultan mientras hay una conversacion o un mensaje modal de Kenshi, y vuelven al cerrarse. Con `probeOnClick` el log dice `[native-window] open: dialogue=<0|1> window=<0|1> modal=<0|1>` al abrirse, `[native-window] closed` al cerrarse, y `[readout-anchor] <nombre> [select] hidden: dialogue or message window open`. Si D2 falla, el `open:` dice cual de las dos senales (`dialogue` = `inDialogue`, `window` = la `DialogueWindow` visible) no se encendio.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| D1 | Clic izquierdo en un NPC reclutable | Aparece la tarjeta sobre su cabeza | `[readout] ... [select]` |
| D2 | Con la tarjeta a la vista, hablarle (clic derecho > Hablar de Kenshi, o que te hable el) | Se abre el dialogo de Kenshi; la tarjeta **no** se ve | `[native-window] open: dialogue=1 window=1 modal=0` y `hidden: dialogue or message window open` |
| D3 | Cerrar el dialogo con el NPC todavia seleccionado | La tarjeta vuelve sobre su cabeza en el mismo cuadro | `[native-window] closed` |
| D4 | Con un cartel "Quiere hablar contigo" sobre alguien (12.23), hablar con otro NPC | El cartel se oculta mientras dura el dialogo y vuelve al cerrarlo (no se descarta: sigue su tiempo) | `[native-window] open: ...` / `closed` |
| D5 | Hablar con el NPC que tiene el cartel, sin seleccionarlo antes | El cartel se oculta durante el dialogo; al cerrarlo vuelve (solo seleccionarlo lo descarta) | — |
| D6 | Abrir el inventario propio o comerciar con la tarjeta a la vista | La tarjeta sigue como antes (comercio/inventario no estan cubiertos a proposito) | (sin `open:`) |
| D7 | Regresion: clic derecho sobre el NPC (menu) y Hablar de TotalRecruit | El menu la oculta como antes; con la oferta abierta la tarjeta queda al costado (12.9, 12.17 C8) | (sin `open:`) |

### 12.28 Retar a duelo a un aliado 80+ (ts-duel-bridge)

Pedido del usuario (2026-10-03): retar a un aliado 80+ a un duelo de TotalSparring para que se una. Preparacion: preset Balanced, un aliado (reputacion >= `friendlyRelation`) de nivel 80 o mas, uno de tu grupo seleccionado. TotalSparring con la API publica v1 (`TS_GetApi`) para D2 en adelante.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| T1 | Sin TotalSparring instalado: Hablar con el aliado 80+ | La ventana de siempre (temporal / precio / negativa), **sin** RETAR A DUELO ni fila DUELOS; la tarjeta sin "Duelos" | `[ts-bridge] TotalSparring duels unavailable: ...` (una vez, ~30 s despues de cargar) |
| T2 | Con TotalSparring, partida cargada: Hablar con el aliado 80+ | La respuesta del tramo sigue, mas RETAR A DUELO y la fila DUELOS con el historial del seleccionado ("0-0" si nunca pelearon) | `[ts-bridge] attached to TotalSparring API v1 and listening for bouts.` |
| T3 | Seleccionar el aliado 80+ (clic izquierdo) con uno de tu grupo seleccionado antes | La tarjeta muestra "Duelos: W-L" como segunda linea; cambiar de miembro seleccionado cambia el numero | — |
| T4 | Aliado de nivel 79, o un no aliado, o preset Vanilla+/Cheat | Nunca RETAR A DUELO | — |
| T5 | RETAR A DUELO sin nadie de tu grupo seleccionado (o el seleccionado inconsciente) | Roller "Selecciona a alguien de tu grupo para hacerlo"; nada se abre | — |
| T6 | RETAR A DUELO con tu luchador seleccionado | Se cierra la ventana de TotalRecruit y se abre la ventana **Dueling** de TotalSparring con ese luchador preseleccionado; roller "<nombre> acepta el reto: ..." | `[duel] <tuyo> challenges <nombre> [key ...]` |
| T7 | Con la ventana Dueling ya abierta, volver a Hablar y RETAR A DUELO | Roller "Ya hay un duelo abierto en TotalSparring."; el reto nuevo se borra | `TotalSparring did not open the duel ... challenge withdrawn` |
| T8 | Ganar el duelo (amistoso) | El NPC se une **para siempre** (no temporal: su tarjeta no dice "Temporal"); cartel "Me ganaste. Me uno a ti" sobre el; roller "<nombre> perdio el duelo y se une a tu grupo para siempre." | `[duel] bout duel outcome=1 ... -> joined` y `lost the duel and joins for good` |
| T9 | Perder (o empatar) el duelo | Cartel "Vuelve cuando seas mas fuerte"; roller con las horas; RETAR A DUELO desaparece por 24 h de juego | `-> lost` y `won the duel: no challenge until hour N` |
| T10 | Despues de T9: guardar, salir, cargar, Hablar con el mismo NPC antes de 24 h | Sigue sin RETAR A DUELO (el enfriamiento viajo en el save); pasadas las 24 h vuelve | — |
| T11 | Sparring (no duelo) entre tu luchador y ese NPC despues de retarlo | No cuenta: ni se une ni enfriamiento | `[duel] bout spar ...` solo con `probeOnClick` |
| T12 | Retar y cancelar la ventana Dueling, o interrumpir el duelo | Nada pasa; el reto queda pendiente; un duelo terminado dentro de 6 h de juego todavia cuenta; despues de 6 h no | `-> ignored`; `challenge(s) expired unanswered` |
| T13 | Duelo a muerte ganado | Si murio: roller "<nombre> murio en el duelo." y nadie se une | `died in the duel: nobody to recruit` |
| T14 | La fila DUELOS contra el Fight Log de TotalSparring para ese par | Mismos ganados-perdidos (empates como tercer numero) | — |
| T15 | Mod Hub "Duelo: retar aliados 80+" apagado | Nunca RETAR A DUELO | — |
| T16 | Salir de la partida / cerrar el juego con TotalSparring cargado | Sin crash; TotalRecruit se descarga limpio (el listener se da de baja) | `stopPlugin called.` sin errores de `[ts-bridge]` |
