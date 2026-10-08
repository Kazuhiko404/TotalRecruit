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

No hay panel de ajustes: toda la configuración vive en `config.json` y el mod
nunca lo escribe. Cuando una fila nombra una clave (por ejemplo `readoutEnabled`
en OFF), se edita esa clave en `config.json` y se reinicia el juego.

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
| S1 | Abrir el juego | El mod carga | `startPlugin complete.` |
| S2 | Mirar el log tras cargar | El config se leyó del archivo | `Loaded config from ...config.json` |
| S3 | Click derecho a cualquier NPC | Sale el bloque de diagnóstico completo | `--- probe: <nombre> ---` … `--- end probe ---` |
| S4 | Cerrar el juego | Nombra las claves de texto que falten | `messages.json has no entry for:` (o nada) |

**S4 no es opcional.** Cada regla nueva trae texto; esa línea dice exactamente
cuál falta escribir.

---

## 2. El motor de chance (R2.1, R2.2)

Todo esto se lee de una sola línea:

```
[chance] level=35 relation=0 personality=-20 defeats=0(x0) situation=15 -> 30%
```

### 2.1 Valores esperados, con los defaults de fábrica

`levelBase 35 · penalidad 1.5/nivel · piso 2 · bono 1.5/nivel · techo del bono 40 · clamp 5..95`

**Prisionero en tu jaula** (`situation=15`), sin derrotas, relación neutral:

| Diferencia de nivel | Normal | Cobarde | Honorable |
|---|---|---|---|
| Te saca 40 | 17% | 37% | 5% |
| Te saca 20 | 20% | 40% | 5% |
| **Iguales** | **50%** | **70%** | **30%** |
| Le sacás 20 | 80% | 95% | 60% |
| Le sacás 40 | 90% | 95% | 70% |

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| C1 | Click a un NPC de tu nivel | `level=35` en el log | Da 100 — volvió la saturación |
| C2 | Click a uno que te saque 20 niveles | `level=5`, total ~20% | Sale rechazo seco: quedó una puerta |
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

> Superado por 12.19 y 12.22: GOLPEARLO (desmayo forzado con enfriamiento) ya no existe; la ventana ofrece GOLPEAR (un golpe directo con tope) y NOQUEAR (la orden nativa).

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
| I1 | Hablarle 3 veces seguidas a alguien que **se niega de verdad** (honor, lealtad, tirada fallida) | Los 3 dicen su motivo; a la 4ª te ignora (animal-insistence-cheat-feed A1: el intento se cuenta despues de contestar) | Sigue atendiendo |
| I2 | Insistir hasta 5 veces | El castigo **crece**: 4h, 8h, 12h | Siempre 4h — no acumula |
| I3 | Insistir 20 veces | Corta en 48h | Sube sin freno |
| I4 | Esperar 2h de juego sin hablarle, volver | Contador en cero | Sigue castigado |
| I5 | Guardar y recargar durante el castigo | **Se olvida** — limitación conocida (I5) | — |

I5 **no es un bug**: el estado vive en memoria. Está anotado como pendiente.

Solo cuenta un no real (animal-insistence-cheat-feed A1, tabla en `src/insistence_rules.h`):

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| I6 | Balanced: un animal herido libre; Hablar 5+ veces seguidas | Siempre su motivo de cuidados, **nunca** "harto" | `refuses (cuidados) ...` cada vez, sin `está harto de tu insistencia` |
| I7 | Un NPC inconsciente, o herido en tu jaula ("herido: cúralo"); Hablar 5+ veces | Siempre su estado; al despertar o curarlo atiende normal | `Refused ... inconsciente: espera a que despierte` / `herido: cúralo`, sin `harto` |
| I8 | Un animal libre sin comer de tu mano, Hablar 5+ veces, darle comida y Hablar | Nunca "harto"; tras la comida sigue la barra de domesticacion (12.20) | sin `harto`; la tarjeta nunca muestra "Harto de ti" |
| I9 | Un honorable que no acepta sobornos: Hablar 3 veces, luego una 4ª | 3 veces "su honor le impide aceptar sobornos..."; la 4ª "está harto de tu insistencia" | `Refused ... (rechazo): está harto de tu insistencia` |

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
| R6 | Con la tarjeta de lectura visible sobre alguien que rescataste (con id: ya le hablaste o lo derrotaste antes), mirar la tarjeta y luego clic derecho (prisoner-module-fixes F3) | La tarjeta y el menú dicen lo mismo: si la tarjeta dice que se une, el menú dice CAN JOIN; si dice "No puede seguirte: te paga con lo suyo", el menú dice que no | `[readout] <nombre> route=rescue chance=N% ... line="..."` y `Context menu: <nombre> -> CAN JOIN ...` (o el rechazo) coinciden |

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
| H7 | Meterlo en tu jaula y esperar un minuto de juego sin hacer click (cerca, dentro de los 900 de la barrida) | Llama: "quiere hablar contigo" y el cartel sobre su cabeza (12.23 N1); lo repite cada minuto hasta `hailRepeats` | Tarda 24 h (`hailFirstHours`): P2 no esta |
| H8 | Golpear (12.19 C3) o Noquear (12.19 C4) a un preso de tu jaula y volver a abrir el menu | Al minuto de juego Golpear/Noquear vuelven a estar listos (el boton dice "(1 h)" mientras tanto: la etiqueta redondea hacia arriba) | Descansa 1 h |
| H9 | Con `cheatNeedsCage: false`, rechazar al sometido que se acerca (12.21) y volver a hablarle | Vuelve a aceptar en la proxima hora entera de juego (como mucho ~1 h), no en 72 h; el aviso dice "en 1 h" | Dice "en 72 h" |
| H10 | Retar a duelo a un 80+ (12.29 J7) y perder | Se lo puede retar otra vez en la proxima hora entera de juego (como mucho ~1 h), no en 24 h | `no challenge until hour` 24 h despues |

| H11 | Con un animal libre sin dueño: darle de comer desde la ventana (Dar comida) y Hablar | **Se une** al momento, sin jaula ni espera: "comió de tu mano y te sigue"; la tarjeta dice "Comio de tu mano: te sigue" (animal-insistence-cheat-feed A2) | Pide jaula — el camino de la comida no está |
| H12 | Un animal libre que nunca comió de tu mano: Hablar | Pide la jaula ("metelo en una de tus jaulas...") y la ventana ofrece Dar comida si llevas | Se une sin comer, o no hay boton de comida |
| H13 | Un animal herido y con hambre que ya comió de tu mano | Se une igual: en Cheat ni heridas ni barriga cuentan | "cúralo" o "le falta N% de barriga" |
| H14 | Un animal con dueño (de otra facción), alimentado | Solo por la jaula: pide jaula, sin boton de comida; en la jaula + 1 minuto se une (H2-H3) | Se une fuera de la jaula |

**H3 y H4 juntos son la definición de Cheat:** cualquiera, incluidos animales y
únicos, sin ninguna condición más que el gesto.

---

## 9. Las dos investigaciones abiertas

### I4 — ¿escribir la marca altera la IA?

**Resuelta (recruit-gating T1):** el mod ya no escribe ni lee
`LT_SQUAD_LOST_TO_ME_ONCE`. El conteo vive en el save del propio personaje
(`total_recruit.player_defeats`). Ver §12.13.

### R6.3 — el layout del hub

**Retirada (2026-10-07):** el Mod Hub ya no se usa; la config vive solo en
`config.json`.

---

## 10. Regresión — que lo viejo siga andando

| # | Caso | Esperado |
|---|---|---|
| G1 | Comprar un recluta con cats | Cobra y se une |
| G2 | Sin fondos suficientes | *"pide N cats y tienes M"* |
| G3 | Liberar a un esclavo | Se une (aunque tenga hambre o este herido: la libertad va antes que el cuidado) |
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
5. El resto: 12.1 (rutas), 12.2 (piedad), 12.3 (reloj de encierro), 12.5 (config), 12.7 (estructura), y lo que quede de 12.6 y 12.8.

Con `"probeOnClick": true`, cada click escribe `[route] <nombre>` en el log: es
la ruta que contestó primero (`field-yield`, `unconscious`, `grudge-refusal`,
`rescue`, `dying`, `surrendered`, `free-world`, `cage`; `easy-healthy` se retiro en v1-presets-close P4).

### 12.1 Orden de rutas (T2)

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| F1 | Un honorable, valiente o bondadoso que **recuerda que mataste a uno de los suyos**: vencerlo 2 veces **fuera** de la jaula y hablarle | Se niega: *"mataste a alguien suyo, y eso no se compra"*. **No** se rinde | `[route] grudge-refusal` |
| F2 | Mismo caso pero **dentro de tu jaula** | La jaula sí lo toca (el hambre y el tiempo no entienden de honor) | `[route] cage` |
| F3 | Un prisionero **tuyo** con 2 o más derrotas, sano y alimentado | Entra por la jaula y tira con el bonus por cuidarlo (12.33 C1); ya no hay atajo | `[route] cage` y `[chance] ... caredFor=+N` |
| F4 | Un prisionero tuyo con 2 o más derrotas, hambriento o herido | Llega a los fallbacks de la jaula: cuidados, precio en cats, o intimidación; **no** el cartel de sumisión del campo | `[route] cage` y una línea `[chance]` con `defeats=` > 0 |
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

### 12.3 Reloj de encierro (T4; la espera facil se retiro en v1-presets-close P4)

Con `probeOnClick` el reloj deja líneas `[hold]`.

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| E1 | Mirar la plantilla `config.json` | **No** hay clave de horas de jaula del modo facil (retirada en P4) | — |
| E2 | Encerrar a alguien en una jaula tuya y quedarse cerca (< ~900 unidades) | El reloj arranca **al encerrarlo**, no al primer click | `<nombre> [hold] clock started: seen in your cage` |
| E3 | Balanced: esperar 6 horas de juego con el preso en la jaula y hablarle | **No** se rinde por tiempo: tira la chance normal (estres, personalidad, nivel, bonus por cuidarlo si corresponde) | `[chance] ...`; ninguna linea `easy-mode needs` |
| E4 | Cheat: hablarle antes y despues del minuto | Antes: "dale un momento"; despues se une (el reloj de encierro solo decide la espera de Cheat) | `[hold] clock started` |
| E5 | Encerrarlo un día, **sacarlo de la jaula** (que te vean pasar con él libre), volver a encerrarlo y hablarle enseguida | **No** se une al instante: el reloj empieza de cero | `[hold] clock dropped: no longer in your cage`, luego `clock started` y `held=0.0h` |
| E6 | Mirar la plantilla `config.json` | **Ya no** están `hoursCagedForProud` ni `uniqueExtraHours` (ver E7) | — |
| E8 | Con un preso encerrado, guardar, salir y cargar ese save | El reloj sigue desde el encierro original, no desde la carga (v1-presets-close P3, 12.33 H1-H4) | sin `[hold] clock started` nuevo tras la carga |
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
| P8 | Reiniciar el juego sin tocar `config.json` | Los precios no cambian (techo, pendiente, punto medio, talento y únicos se conservan) | `[price]` con los mismos factores |
| P9 | Poner `catsPerLevel` en 300 en `config.json` y reiniciar | El clamp acepta 300; `talentMaxMultiplier` y `catsHardCap` se leen del archivo | `[price]` con la curva editada |

### 12.5 Escritor de config (T6)

**Retirado (2026-10-07):** el mod ya no escribe `config.json` (`SaveConfig` se quitó junto con el Mod Hub). Solo queda una comprobación:

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| C1 | Editar a mano `config.json` (agregar `// mis notas`, una clave propia, cambiar `catsCeiling`), jugar y cerrar el juego | El archivo queda exactamente como se dejó: el mod nunca lo reescribe | Ninguna línea `Saved config to` |

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
| A1 | Prisionero en tu jaula, ventana de oferta, **GOLPEAR** hasta que quede sin sentido (o **NOQUEAR**) | La derrota **cuenta siempre** (antes se perdia si el motor no nombraba al jugador como ganador); el conteo sube de a 1 | `[defeat] credited to the player explicitly (cage command): <nombre> (engine winner is the player / is someone else / unreadable)` y luego `<nombre> defeated by the player (...; count N, ...)` |
| A2 | Volver a golpear al mismo prisionero enseguida | "todavia se esta recuperando del anterior": el enfriamiento (6h por defecto) corre desde ESE golpe. Antes, un golpe descartado no dejaba registro y se podia golpear sin espera | (sin linea nueva) |
| A3 | Con `probeOnClick`, golpear a alguien que el motor no registra como tuyo | Mismo `[defeat] credited ...`; **no** aparece `[defeat-skip] ... lastGuyWhoDefeatedMe` para ese golpe | `[defeat-skip]` ausente |
| Z1 | Hacer clic en un prisionero, un NPC libre, un aliado, un animal, un rescatado y un NPC con rencor | El texto del motivo en la ventana y en `Refused ...` es **el mismo de antes**, palabra por palabra | `Refused <nombre> (<moneda>): <motivo>` |
| Z2 | Insistir 3 veces a un NPC libre | Linea "¡Me estás hartando! Déjame en paz." y motivo "está harto de tu insistencia...". Editar `reason.insistence` en `messages.json` (otro texto) **no cambia** la linea que dice el NPC | `[route] free-world` |
| Z3 | Honorable, Valiente o Bondadoso que te guarda rencor (mataste a uno de los suyos), **fuera** de la jaula | Rechazo absoluto (`grudge-refusal`) y **siempre** dice "Sé lo que hiciste. Fuera de mi vista." Antes solo la decia si el texto del motivo contenia "mataste" (2 de las 7 variantes del `messages.json`); las otras 5 caian en "¿Quién eres tú? No tengo nada que hablar contigo." | `[route] grudge-refusal` |
| Z4 | El mismo NPC con rencor, pero **dentro** de tu jaula | Sigue la ruta de jaula (ya no hay rechazo absoluto), igual que antes | `[route] cage` |
| C1 | Buscar en `config.json` las 19 claves nuevas | `chanceRelationWeight`, `chanceAlliedBonus`, 9 `chancePersonality*`, `relationRefuseGoodNatured`, `relationRefuseAll`, `talentCraftWeight`, `talentShadowWeight`, `animalEdgeLevel`, `animalBigEdgeLevel`, `animalTemperamentBase`, `animalTemperamentWeight`. Con los valores de fabrica, todo se comporta como antes | `Loaded config from ...` |
| C2 | Poner `"chanceRelationWeight": 0` y hacer clic en un aliado | En `[chance]` el termino `relation=` pasa a 0 | `[chance] ... relation=0 ...` |
| C4 | Cargar con un `config.json` viejo (sin las claves nuevas) | Carga normal con los valores de antes; las claves que faltan usan su default y el archivo no se toca | `Loaded config from ...` |
| R1 | Prisionero con 6h de encierro (modo fácil), NPC en rescate, cheat, animal con hambre, animal en jaula | Todas las rutas responden como antes (mismos motivos, mismos precios, misma barra de domesticación) | `[route] <ruta>`, `[chance] ...`, `[price] ...`, `[hold] held=...` |
| R2 | Disparar un plea por vida baja (mercy) y por golpiza hasta casi matar | La ventana de piedad sigue abriendose (ahora por el listener de `defeats`, ya no por una llamada directa) | `defeated by the player (...) nearDeath=yes` y la ventana |
| N1 | Insistir 5+ veces a un prisionero **en jaula** | **No** pasa nada: la insistencia no corta la charla de jaula (solo un desafiante o un zelote registran el intento, y nadie consulta el castigo en la jaula). R2.4 no dice nada de jaulas; queda como estaba y anotado en el doc de la feature | (sin `harto` en la linea del NPC) |

### 12.8 Lectura de estres y chance (T9)

La tarjeta de `ui-mockup/readout.html` sobre el NPC. Todo se prueba con `"probeOnClick": true` en `config.json`: cada vez que lo dibujado cambia, el log dice **lo mismo que se ve** en una linea `[readout]`, y se compara con `[chance]`/`[price]` de un clic en el mismo NPC (la tarjeta no calcula nada propio: tiene que coincidir). El mod necesita `gui/images/tr_white.png` en su carpeta (sin el, las barras usan el skin `WhiteSkin` y el log lo dice).

Linea: `[readout] <nombre> route=<ruta> chance=<n>% stress=<n>% (hunger H wounds W tired T) defeats=<a/b> food=<n>% line="<texto>" [hablar|hover]`

Desde 12.11 la tarjeta sale al **seleccionar** el NPC (clic izquierdo) y al apretar **Hablar**; el modo hover ya no existe. Las filas de esta tabla que dicen "pasar el mouse" se prueban seleccionando al NPC; las mismas comprobaciones de contenido valen. U4 ahora refresca cada ~2 s, U12 ya no aplica, U13 y U14 los reemplaza 12.11.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| U1 | Pasar el mouse sobre un prisionero en tu jaula (precio en cats) | Tarjeta sobre su cabeza: ruta **Cautiverio**, barra de estres con 3 colores y la leyenda `Hambre/Heridas/Cansancio` que suma el total, barra de chance, derrotas `n/2`, linea de precio `Precio N cats (tienes M)` | `[readout] ... route=captivity chance=..% stress=..% ... [hover]` |
| U2 | Hacer clic (menu contextual) en ese mismo prisionero y comparar | `chance=` de la tarjeta == `-> N%` de `[chance] ...` del clic; el precio de la tarjeta == `[price] ... -> N cats`; el estres == `stress=N(...%)` de `[chance]` | `[chance] ... -> N%`, `[price] ... -> N cats` |
| U3 | Prisionero sano, comido y sin heridas en tu jaula (Balanced) | Debajo de la chance, la fila `LO CUIDASTE +N%` (N entre 5 y 20, el mismo todo el dia de juego); ya no hay linea `Faltan Xh en la jaula` | `[chance] ... caredFor=+N` |
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
| U15 | `readoutEnabled` en OFF | Ninguna tarjeta; en ON vuelve. `readoutCompact` ON: tarjeta de 176 px sin leyenda ni derrotas ni segunda linea | — |
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
| H9 | `readoutEnabled` OFF y apretar `Hablar` | Oferta sin tarjeta | |
| H10 | Zona con muchos NPCs en pantalla (una ciudad, 20+ personajes) con la config por defecto | FPS normales (comparar con el mod apagado, `"enabled": false`); antes caian por el globo y la tarjeta | |
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
| B8 | Clic derecho sobre un preso **dentro** de tu jaula (el menu abre dos columnas: el personaje y "Jaula de Prisionero"; prisoner-module-fixes F4). Mockup: `ui-mockup/readout.html`, "Menu on your prisoner, two columns" | El bloque (Hablar, Golpear, Noquear) queda **fuera de las dos columnas**: nunca tapa ABRIR CON LLAVE, DESMANTELAR ni PONER EN LIBERTAD; Hablar se ve | `[menu-anchor] gui1=(...) gui2=(l,t,w,h) union=(...)` con `union` que contiene a las dos |
| B9 | Clic derecho sobre un NPC en campo abierto (una sola columna) | El bloque queda exactamente donde quedaba antes (B1-B6) | `[menu-anchor] gui1=(l,t,w,h) gui2=none union=(l,t,w,h)` con `union` = `gui1` |

### 12.11 Tarjeta al seleccionar el NPC (price-readout-button-tweaks T5)

La tarjeta sigue a la **seleccion**: clic izquierdo en un NPC que no es tuyo y aparece sobre su cabeza; se calcula al cambiar la seleccion y se refresca cada 2 s, no cada cuadro. `Hablar` la sigue poniendo al lado de la oferta. `readoutOnSelect` (ON por defecto). Con `"probeOnClick": true`.

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
| S12 | `readoutOnSelect` OFF | Seleccionar ya no muestra nada; `Hablar` sigue mostrandola junto a la oferta | |
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
| K4b | Noquearlo una vez en el campo (desde cage-beating-stress, Noquear en la jaula no es derrota: 12.32) y mirar el log (prisoner-module-fixes F2) | El conteo sube **uno**, aunque el gancho llegue dos veces | dos `[defeat-hook] knockout fired for <nombre> (was standing)` ~1,2 s aparte; el primero da `defeated by the player (...; count N ...)`, el segundo `[defeat-skip] <nombre>: already counted this knockout` (o un motivo de credito si el motor no nombra al jugador); nunca dos `defeated by the player` |
| K5 | Un guardia que nunca venciste (ni en esta partida ni antes) | `defeats=0`; nunca `field-yield` | sin `defeated by the player` para el |
| K6 | Golpear (12.19 C3) hasta que el preso de tu jaula quede inconsciente | **No** cuenta (cage-beating-stress, 12.32); Golpear queda en descanso las horas de `submissionBeatingCooldownHours` (12.19 C3c) | `[defeat] <preso> cage knockout: not a defeat (stress +0%, the blow added its own)`; ningun `defeated by the player` |
| K7 | Que otro NPC noquee a alguien y hacerle clic derecho mientras esta en el suelo | No cuenta | `[defeat-skip] ...: lastGuyWhoDefeatedMe is not yours` |

### 12.14 Rendirse en el campo: jaula ajena, insistencia y lealtad (recruit-gating T2)

`field-yield` ya no se salta las preguntas del mundo libre. Con `"probeOnClick": true`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| Y1 | Vencer 2 veces a un prisionero en la jaula de **otra** faccion y hablarle | "esta en la jaula de otro"; no se une | `[route] field-yield` y luego `Refused ...: está en la jaula de otro` |
| Y2 | Vencer 2 veces a un NPC libre, insistirle hasta que te ignore, hablarle de nuevo | "esta harto de tu insistencia"; no tira el dado | `[route] field-yield` y luego `Refused ...: está harto de tu insistencia` |
| Y3 | Vencer 2 veces a un guardia o soldado (no a un bandido) | Tira el dado con 25 menos que un bandido igual; puede unirse | `chance=` 25 por debajo del bandido |
| Y4 | `submissionFieldYieldLoyaltyPenalty` en 0 y repetir Y3 | La misma chance que el bandido | |
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
| P8 | En P1, la X del titulo | Igual que CANCELAR: sin mensaje de sistema, no cobra |
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
| C5 | Con la tarjeta a la vista, clic en la ventana de oferta, en otra ventana del juego o en el boton `Hablar` | La tarjeta **se queda** | (sin `dismissed`) |
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
| H12 | `haggleEnabled` en false (`haggleEnabled`) y `Hablar` a alguien con precio | La ventana de siempre, sin slider | — |

### 12.19 Golpear y Noquear al preso de tu jaula (cage-animals-submission T1)

Golpear y Noquear los da el personaje **seleccionado**. Golpear (desde `fix/cage-beat-direct-hit`) es **un** golpe directo, en el acto y sin ordenes: el daño de ataque del motor para ese personaje, solo contundente, al torso del preso, con tope (la parte nunca baja de -0,5 x su vida maxima: puede noquear, nunca matar); el preso no sale de la jaula. Noquear es la orden del propio juego (`STEALTH_KNOCKOUT`): si en 1,5 s el personaje no cambio de tarea, corre el respaldo `MedicalSystem::knockout` con su habilidad de asesinato. Mockup: `ui-mockup/readout.html` seccion 2, "Menu on your prisoner". Reemplaza A1-A3 (12.7) y K6 (12.13): el viejo GOLPEARLO (desmayo forzado de 60 s) ya no existe y la clave `submissionBeatingKnockoutSeconds` no se lee. Con `probeOnClick`. Desde `fix/beat-feedback` Golpear camina primero hasta la jaula y avisa: en C3 / C7 el personaje tiene que estar a `submissionBeatReachMeters` (4 m) del preso o caminar hasta ahi; ver 12.39.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| C1 | Seleccionar a uno de los tuyos, clic derecho sobre un preso **despierto** en tu jaula | Al lado del menu, un bloque de tres: Hablar, Golpear, "Noquear NN%" (o "Noquear" si el juego no da probabilidad); el bloque no tapa los botones de TotalSparring / TotalLoot | `Context menu: <nombre> -> ... [cage commands]` |
| C2 | Clic derecho sobre un NPC libre, un preso inconsciente, o con `allowBeating` apagado | Solo Hablar | sin `[cage commands]` |
| C3 | Golpear (clic izquierdo, y otra vez con el gesto: mantener clic derecho, deslizar, soltar) | Un solo golpe en el acto por gesto: la vida del torso del preso baja; el preso **sigue en la jaula** (nadie se pone a pelear, la lectura no pasa a `route=free`) | `[cage] <tuyo> -> <preso> beat hit (blunt X, part health A -> B)` con B < A |
| C3b | Con `submissionBeatingCooldownHours` en 0,5, golpear al mismo preso muchas veces (esperando el descanso entre golpes, a velocidad x3) | Puede quedar inconsciente; **nunca muere** ni sangra hasta morir; los golpes cuando la parte ya esta en el piso no bajan nada | `beat hit (blunt 0.0, part health -N -> -N)` en el piso (N = la mitad de la vida maxima de la parte) |
| C3h | Golpear a un preso despierto cuyo torso ya esta por debajo del piso (-0,5 x su vida maxima), por ejemplo muy herido de una pelea anterior; si no se consigue, anotarlo | El golpe no pega y la vida del torso **no sube** | `beat hit (blunt 0.0, part health A -> A)` con A igual en los dos lados |
| C3c | Justo despues de C3, clic derecho otra vez sobre el mismo preso (horas entre golpizas en 1, el valor por defecto desde cage-beating-stress) | El bloque sigue con Golpear, que dice "Golpear (1 h)" (las horas que faltan, redondeadas hacia arriba); otro preso sin golpes dice "Golpear" | — |
| C3d | Golpear en C3c | No hay golpe; mensaje "Podras volver a golpearlo en N h" | sin `beat hit` |
| C3e | Igual en la ventana: rechazo del mismo preso tras C3 | GOLPEAR dice "GOLPEAR (N h)"; al pulsarlo la ventana se cierra con el mensaje de C3d; NOQUEAR funciona normal (su descanso corre solo desde un noqueo de Noquear: 12.32) | sin `beat hit` |
| C3f | Pasadas las horas (o tras cargar un save: el descanso es de sesion) | "Golpear" sin horas y el golpe pega | `beat hit` |
| C3g | Abrir la ventana de un preso de tu jaula, dejarla abierta y, con otro personaje, noquearlo o liberarlo; despues pulsar GOLPEAR o NOQUEAR | "Ya no puedes hacerle eso"; no pasa nada mas | sin `beat hit` ni `knockout ordered` |
| C4 | Noquear | El seleccionado intenta el noqueo sigiloso; sale o no segun su habilidad (el % del boton) | `[cage] ... knockout ordered (task 507, ...)` y `... knockout via task` o `via fallback` |
| C5 | Cuando el preso cae por C3 o C4 | Se reconoce como noqueo de la jaula aunque el juego no nombre a nadie como ganador; desde cage-beating-stress **no** es derrota: suma estres por golpes (12.32) | `[defeat-hook] knockout fired for <preso>`, `[defeat] <preso> cage knockout: not a defeat (stress +X%)` |
| C5b | Golpear a un preso sin dejarlo inconsciente y, mas de 5 s despues (dentro de los 2 min), que caiga por otra causa (hambre, otro personaje sin orden) | Esa caida **no** es del golpe (el credito de Golpear dura 5 s; el de Noquear 2 min): la deciden las reglas normales de derrota | sin `cage knockout` para esa caida |
| C6 | Golpear o Noquear sin nadie de tu grupo seleccionado | "Selecciona a alguien de tu grupo para hacerlo"; no pasa nada mas | — |
| C7 | Rechazo en la ventana de un preso de tu jaula: **GOLPEAR** o **NOQUEAR** (ver 12.22) | La ventana se cierra; GOLPEAR da el golpe de C3, NOQUEAR la orden de C4; sin seleccionado, el mensaje de C6 | igual que C3 / C4 |
| C8 | Anotar si alguna vez aparece `knockout via fallback` | Evidencia: si el juego no toma `STEALTH_KNOCKOUT` desde un plugin, el respaldo es lo que se ve; reportar tambien el valor de `checkPlayerOrderForProblems` | `knockout via fallback` |
| C9 | Dar la orden de Noquear y cargar un save antes de que se resuelva | Nada queda apuntando a nadie; el actor queda suelto (sin meta de noqueo) | `[cage] <tuyo> knockout cleanup (...) after at load` y `load begin: session state cleared (... cage command watch and credits ...)` |
| C10 | Noquear (prisoner-module-fixes F1) y esperar a que el preso caiga; despues esperar varios minutos a que se levante | El preso cae **una** vez; tu personaje sale del sigilo (si no estaba en sigilo antes de la orden) y no vuelve a ir por el cuando se levanta | `knockout via task` (o `via fallback`), luego `[cage] <tuyo> knockout cleanup (goal removed, stealth off) after target down`; ningun `[defeat-hook] knockout fired for <preso>` nuevo sin otra orden |
| C10b | Igual que C10 con tu personaje **ya en sigilo** antes de Noquear | Al caer el preso sigue en sigilo | `knockout cleanup (goal removed, stealth kept) after target down` |
| C10c | Dar Noquear y, antes de que caiga, dar Noquear de nuevo (mismo u otro personaje) | El primer actor queda suelto antes de la orden nueva | `knockout cleanup (...) after replaced by a new order` y otro `knockout ordered` |
| C10d | Cargar un save hecho con la version anterior en el que un personaje tuyo seguia noqueando al preso | En segundos deja de perseguirlo (el sigilo puede quedar puesto: apagarlo a mano una vez) | `[cage] <tuyo> knockout cleanup (goal removed, stealth kept) stray goal for <preso>` |
| C10e | Con el menu **del juego** ordenar noquear a un NPC libre y de pie | El noqueo nativo sigue igual; el curado no lo toca (ni despues de que caiga: solo toca metas sobre un preso de tu jaula, y nunca ordenes) | sin `stray goal` |
| C10f | Noquear y, mientras tu personaje camina hacia la jaula, darle con Shift otra orden en cola (o tener ya una en cola) | Al caer el preso (o vencer la ventana) la orden en cola sigue ahi y se cumple | `knockout cleanup (goal removed, stealth ...)` **sin** `order cleared` |
| C10h | Noquear y pausar el juego enseguida (antes de que tu personaje se mueva); esperar mas de 2 minutos en pausa y reanudar | Al reanudar tu personaje va a noquearlo con la orden: no hay noqueo instantaneo por respaldo durante la pausa ni la vigilancia se cierra por la ventana | `knockout via task` despues de reanudar; ningun `via fallback` ni `after land window passed` en la pausa |
| C10i | Noquear y, antes de que caiga, alejarte con otro personaje hasta que la zona de la jaula se descargue (o viajar lejos con la camara siguiendo a otro) | Sin cuelgue; al volver nada raro: la vigilancia se cerro sola | `[cage] knockout watch dropped (actor gone)` o un `knockout cleanup` normal |
| C10g | Noquear y, antes de que llegue, mandar a tu personaje a otro lado con una orden normal | En unos 3 s queda suelto (sin respaldo: el preso no cae) y cumple tu orden nueva | `knockout cleanup (...) after order abandoned`; sin `knockout via fallback` |

### 12.20 Los animales solo se doman con tu comida (cage-animals-submission T2)

Un animal se une solo si el jugador le dio de comer al menos una vez **y** su barriga llego a la barra. Las comidas se guardan en el save del animal (`total_recruit.fed_by_player`). Con `probeOnClick`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| F1 | Clic derecho y Hablar con un animal salvaje lleno que nunca alimentaste (Cabra, Goat Alpha) | **No** se une: "todavia no ha comido de tu mano"; la ventana ofrece DARLE COMIDA si llevas comida; la tarjeta dice "No comio de tu mano: dale comida" con la barriga llena | `Context menu: <animal> -> refuses (care) todavia no ha comido de tu mano` |
| F2 | DARLE COMIDA a ese animal | Come; la ventana se reabre y ahora se une ("comio de tu mano y te sigue") si la barriga sigue sobre la barra | `Fed <animal> with <item>. hunger a -> b (...); meals from the player 1` |
| F3 | Un animal con hambre (barriga bajo la barra) que nunca alimentaste | "todavia tiene hambre: le falta N% de barriga" y DARLE COMIDA; tras comer lo suficiente se une | `meals from the player N` |
| F4 | Animal en tu jaula, sin alimentar, mas de 6 h (espera facil) | No se une por tiempo: el mismo motivo que F1 | sin `easyWait` para el |
| F5 | Animal en tu jaula sano y lleno, sin alimentar | No hay "intimidado por tu poder" ni "Desconfia": el motivo es F1 | — |
| F6 | Animal que rescataste (te recuerda como salvador) y sin alimentar | No entra por rescate ni recibe el bonus por cuidarlo: pide comida (F1) | `[route] free-world` o `cage`, nunca `rescue` / `easy-healthy` |
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
| S5 | (otro NPC) PERDONAR | "Gracias... no lo olvidare"; sale corriendo ~80 m en direccion contraria a ti y se queda tranquilo; durante 5 minutos no ataca y **Hablar** sigue funcionando con el | `forgiven ...`, `calm for 30 s, running off`, `ran off; stays calm` |
| S6 | Esperar los 5 minutos de S5 | Vuelve a su rutina y a su velocidad normal; si era hostil, vuelve a ser hostil | `forgive window over after 300 s (time up)`, `forgive window over (from forgiven, hostile before yes)` |
| S7 | (otro NPC) RECHAZAR | Dice "Entonces nos volveremos a ver" y vuelve a ser hostil al instante; "<nombre>: lo rechazaste. No querra saber nada de ti en 72 h." (Balanced; en Cheat ~1 h, 8 H9) | `rejected ...`, `[defeat] <nombre>: defeats cleared`, `reject cooldown until game hour N (72 h)` |
| S8 | Tras S7, Hablar con el (o meterlo en tu jaula y Hablar) | Ventana de rechazo: "No quiere saber nada de ti"; la tarjeta dice lo mismo; tampoco ruega piedad si pierde un miembro | `Context menu: <nombre> -> refuses ... no quiere saber nada de ti` |
| S9 | Tras S7, guardar, salir, volver a cargar y Hablar con el | Sigue en enfriamiento hasta que pasen las 72 h de juego; despues se puede volver a vencer y a reclutar | `first character id read back from the save` |
| S10 | Cerrar la ventana de S3 con la X | Ninguna decision: se queda calmado donde esta durante 5 minutos y luego vuelve a lo suyo (como S6, sin correr) | `dismissed ...`, `calm for 300 s, in place` |
| S11 | Alejarte mas de 120 m mientras camina, o dejar pasar 60 s sin que llegue | Se cancela: vuelve a su rutina y, si era hostil, ataca de nuevo | `approach aborted: too far: ...` / `timeout: ...` |
| S12 | Pausar el juego mientras camina | El reloj de 60 s se congela; al despausar sigue | — |
| S13 | Tener la ventana de otro NPC abierta cuando llega | Se queda parado mirandote hasta que cierres la otra ventana; entonces abre la suya (si no pasaron los 60 s) | `arrived, waits for the window to be free` |
| S14 | Cargar un save mientras camina o con la ventana abierta | Nada queda colgado; el NPC del mundo cargado no recuerda el evento | `load begin: session state cleared (... submission event ... reject cooldowns ...)` |
| S15 | Vencer al mismo NPC otra vez despues de PERDONAR | Vuelve a acercarse (una vez por paliza, no una vez por sesion) | `submitted (...)` de nuevo |
| S16 | `submissionEventEnabled` apagado | Ningun NPC viene; Hablar sigue igual | sin `[submission]` |

### 12.22 La ventana de un preso: GOLPEAR / NOQUEAR, nunca IGNORAR / PERDONAR (fix tras T3)

Reporte del usuario (2026-10-03): un "Holy Warden" en tu jaula, Cautiverio 87%, "Sigue desafiante: dejalo con hambre o hierelo", y solo CERRAR. Regla: todo rechazo de un preso para el que el menu contextual ofreceria Golpear / Noquear (en tu jaula, despierto, `allowBeating`; `CageCommandsAllowed`) lleva GOLPEAR y NOQUEAR en la ventana. Quien no puede irse (jaula, cadenas, cargado) nunca ve IGNORAR ni PERDONAR. Mockup: `ui-mockup/readout.html` seccion 5, "Caged and defiant", "Caged and hungry", "Mercy plea from someone who cannot leave".

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| W1 | Hablar con un preso **despierto** y desafiante en tu jaula, con uno de los tuyos seleccionado | Motivo en rojo "...dejalo con hambre o hierelo"; botones GOLPEAR, "NOQUEAR NN%" (el mismo % que el menu contextual) y CERRAR, en una fila | — |
| W2 | GOLPEAR en W1 | La ventana se cierra; el seleccionado le da un golpe en el acto (como C3); el preso sigue en la jaula | `[cage] ... beat hit (...)` |
| W3 | NOQUEAR en W1 | La ventana se cierra; el seleccionado intenta el noqueo (como C4) | `[cage] ... knockout ordered ...` |
| W4 | W2 / W3 sin nadie seleccionado | "Selecciona a alguien de tu grupo para hacerlo" | — |
| W5 | El mismo preso con hambre, llevando comida | DARLE COMIDA, GOLPEAR, NOQUEAR en la primera fila y CERRAR solo a la derecha en una segunda | — |
| W5b | Un preso **esclavo** (o de arquetipo esclavo) con hambre en tu jaula, llevando comida | Se une sin pedir comida (como G3: la libertad va antes que el hambre): solo ACEPTAR / CANCELAR, sin DARLE COMIDA | — |
| W6 | Un preso de tu jaula que **si** se une (intimidado, alimentado y curado...) | Solo ACEPTAR / CANCELAR | — |
| W7 | `allowBeating` apagado (`submissionAllowBeating`) y W1 | Sin GOLPEAR ni NOQUEAR; el motivo dice "Sigue desafiante: dejalo con hambre" (no pide herirlo) | — |
| W8 | Preso inconsciente en tu jaula, o un preso en la jaula de otro | Sin GOLPEAR ni NOQUEAR (como C2) | — |
| W9 | Un preso de tu jaula lisiado que pide piedad (si llega a pasar) | ACEPTAR / RECHAZAR, **nunca** IGNORAR | — |
| W10 | Durante el evento de 12.21, levantar (cargar) al NPC antes de que llegue | Si la ventana se abre, ACEPTAR / RECHAZAR, **nunca** PERDONAR | — |

### 12.23 Quien quiere hablar, sobre su cabeza (head notice)

Pedido del usuario (2026-10-03): "el juego te avisa, pero si no lees el nombre no sabes quien quiere hablar". Quien te llama lleva un cartel sobre la cabeza: su nombre y "Quiere hablar contigo" (cautivo listo, `hail.cpp`) o "Se acerca para hablar contigo" (el sometido, `submission_director.cpp`). Dura `headNoticeSeconds` (90 por defecto, segundos reales, congelado en pausa) cuando pide algo al jugador ("Quiere hablar contigo", "Se acerca para hablar contigo", "Quiere ofrecerte ayuda"); un cartel de una linea que solo informa (miembro temporal: union, faltas, aviso, despedida; ayudante: inicio, aviso, despedida, traicion, "Sigo contigo") dura `headNoticeInfoSeconds` (25 por defecto). Arreglo del play test (fix/head-notice-lifetime): "Te acompanare 5 dias" "nunca se iba" con 300 s. Mockup: `ui-mockup/readout.html` seccion 2, "Someone wants to talk" y "Select the one who called". Con `probeOnClick` el log dice `[notice]`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| N1 | Esperar a que un preso de tu jaula llame (`hailFirstHours`; o en Cheat al minuto de juego, 8 H7) | Ademas del aviso en el registro, un cartel compacto sobre su cabeza: nombre en bronce, "Quiere hablar contigo" en ambar, borde ambar a la izquierda | `[notice] <nombre>: "Quiere hablar contigo" over the head [key ...] (prompt)` |
| N2 | Mover la camara | El cartel sigue la cabeza cada cuadro; fuera de pantalla se oculta y vuelve al volver | — |
| N3 | Abrir un menu contextual con el cartel visible | El cartel se oculta mientras el menu esta abierto | — |
| N4 | Seleccionar (clic izquierdo) a **ese** NPC | El cartel se va para siempre y aparece la tarjeta normal; nunca los dos a la vez | `[notice] dismissed by selection [key ...]` (con `probeOnClick`) |
| N5 | Con el cartel sobre A, seleccionar a otro NPC B | B tiene la tarjeta normal; el cartel de A sigue | — |
| N6 | Clic izquierdo o derecho en el suelo con el cartel visible | El cartel sigue (solo seleccionarlo o el tiempo lo quitan) | — |
| N7 | B seleccionado justo al lado de A, de modo que la tarjeta de B tape el cartel de A | El cartel de A se oculta mientras se tocarian; la tarjeta gana | — |
| N8 | No hacer nada `headNoticeSeconds` (90 s por defecto; bajar a 10 s en `headNoticeSeconds` para probar) | El cartel se va solo, a los ~90 s con el valor por defecto | — |
| N9 | Pausar el juego con el cartel visible | El tiempo del cartel se congela; al despausar sigue contando | — |
| N10 | Evento del sometido (12.21) | Al empezar a caminar: cartel "Se acerca para hablar contigo"; se va cuando abre la ventana o si se cancela el evento | `[notice] <nombre>: "Se acerca para hablar contigo" ...` |
| N11 | Que el NPC del cartel muera o se una al grupo | El cartel se va (en menos de 1 s) | — |
| N12 | Cargar un save con carteles visibles | No queda ningun cartel | — |
| N13 | Ventana de oferta abierta y el cartel caeria encima | Se corre de costado a la altura de la cabeza (como la tarjeta), u oculto sin lugar | — |
| N14 | Tomar un miembro temporal (12.25) y no hacer nada | "Te acompanare N dias" se va solo a los ~25 s (`headNoticeInfoSeconds`), no a los 90 | `[notice] <nombre>: "Te acompanare N dias" over the head [key ...] (squad) (info)` |
| N15 | Una falta o la despedida de un temporal; el inicio, aviso o despedida de un ayudante (12.26) | Cada linea dura ~25 s; la oferta del ayudante ("Quiere ofrecerte ayuda") dura ~90 s | `... (info)` para las lineas, `... (prompt)` para la oferta |
| N16 | `headNoticeInfoSeconds` a 5 s | Los carteles informativos se van a los ~5 s; "Quiere hablar contigo" sigue usando `headNoticeSeconds` | — |
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
| A11 | `allyTierEnabled` apagado con el aliado de A2 (Vanilla+ se retiro en v1) | Como antes: una tirada diaria, PROBABILIDAD, sin "-N%" | — |
| A12 | Preset 0 (Cheat) | Sin cambios: jaula y un momento | — |
| A13 | `allyTierEnabled` apagado, preset 1 | Todos los aliados vuelven a la tirada diaria | — |
| A14 | Bajar `allyTierDiscountScale` a 0.5 con relacion 28 | El descuento pasa a 14% | — |

### 12.25 El miembro temporal: avisos, maltrato y partida (ally-tiers-temp-helpers T2)

Un aliado Bueno o Neutral de nivel 80+ reclutado como en 12.24 A6. Preset 1 (Balanced). Defaults: 3 faltas, hambre 12 h seguidas bajo 25%, 12 h entre faltas del mismo tipo, aviso 12 h antes, 7 dias sin volver. Para no esperar dias de juego: `tempMemberHungryHours` = 1, `tempMemberStrikeCooldownHours` = 1, `allyTierTempMinDays` = `allyTierTempMaxDays` = 1, y la velocidad del juego al maximo. Mockup: `ui-mockup/readout.html` seccion 2, "Temporary member (squad notices)". Las faltas se loguean siempre (`[temp] strikes N (...)`).

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
| T12 | Un temporal de 4+ dias, bien tratado: dejar pasar los dias de juego (duel-join-chance D5) | Una vez por dia de juego, cartel informativo (~25 s) "Me quedan N dias" (el ultimo dia "Me queda 1 dia"); **no** el dia en que se unio, **no** el dia de "Me ire pronto"; si ese paso hubo una falta, sale en el chequeo siguiente (2 s) | `[temp] <nombre> daily notice: N day(s) left (day D)` |
| T13 | Guardar y cargar a mitad de un dia en que ya lo dijo | Lo repite una vez ese dia (el dia anunciado es de sesion); al dia siguiente, una vez | igual que T12 |
| T14 | `tempMemberDailyNotice` apagado | Nunca "Me quedan N dias"; los demas carteles igual | — |

### 12.26 El ayudante gratis (ally-tiers-temp-helpers T3)

Pedido del usuario (2026-10-03): aliados mas fuertes y de buena personalidad se ofrecen a acompanarte unos dias, gratis, sin que los puedas controlar (REQUISITOS R4b.6). Preset 1 (Balanced). **Mecanismo** (native-ai-packages T2): con `helperNativeBodyguard` encendido (por defecto) el ayudante recibe el contrato nativo de guardaespaldas del juego (`5090-gamedata.base`) con tu lider como empleador, como un mercenario contratado; si el contrato falla o esta apagado, el **respaldo B** lo escolta por codigo (repath cada 1 s, corre si esta lejos, ataca al blanco del lider). Los casos propios del contrato y del respaldo estan en 12.30 N1-N5. La escalera vieja BODYGUARD -> FOLLOW_PLAYER_ORDER -> walk ya no existe. Para probar sin esperar la tirada: `helperDebugKey` = 119 (F8) o `"helperDebugKey": 119` en `config.json`; seleccionar un NPC ajeno y apretar F8. Para no esperar dias: `helperDaysMin` = `helperDaysMax` = 1 y la velocidad al maximo. Mockup: `ui-mockup/readout.html` seccion 5 "Free helper offers to escort you" y seccion 2 "Free helper (offer and escort notices)". Todo se loguea como `[helper]`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| H1 | Tecla de prueba: seleccionar un aliado ajeno y apretar F8 | Camina hasta tu lider con el cartel "Quiere ofrecerte ayuda" y dice "Eh, espera. Puedo ayudarte."; al llegar, el popup con la etiqueta Ayuda: "<nombre> te ofrece acompanarte D dias, gratis. No podras darle ordenes.", PRECIO Gratis, ACEPTAR / RECHAZAR | `[helper] debug key on <nombre>: screen says <motivo> (...)` y `[helper] <nombre> offers D days (debug key), walks up from X m` |
| H2 | F8 sin nada seleccionado o con uno tuyo | Roller "[TotalRecruit] Selecciona un NPC que no sea tuyo." | `[helper] debug key: select an NPC that is not yours first` |
| H3 | Natural: quedarse cerca (<40 m) de un aliado Bueno o Neutral con nivel > el de tu lider + 10 | Con 15% por dia y por NPC se ofrece como en H1; si no toca, no vuelve a tirar ese dia | `[helper] <nombre>: no offer today (level L vs leader P, ...)` o `offers D days (roll: ...)` |
| H4 | ACEPTAR | Cartel "Te cubro las espaldas", roller "<nombre> te acompanara D dias. No acepta ordenes."; no aparece en tus retratos; te sigue a distancia de mercenario cuando caminas | `native bodyguard contract -> <lider> (N h) ok: ...; package ... [start]` y `escorts the player for D days (until hour H, native contract)`; con el respaldo: `..., code-driven)` |
| H5 | Seleccionarlo | Tarjeta con una sola linea verde "Ayudante: le quedan D dias" | — |
| H6 | Intentar darle ordenes (clic derecho en el suelo con el seleccionado) | No obedece: sigue escoltando a tu lider | — |
| H7 | Alejarse corriendo varios cientos de metros | Corre y te alcanza. Contrato nativo: lo hace el paquete del juego; si el contrato se pierde se reasigna una vez y, si vuelve a perderse, sigue el respaldo B. Respaldo B: corre a mas de 12 m y vuelve a caminar al alcanzarte | nativo: `contract gone, assigning it again` / `contract gone again, code-driven escort takes over`; respaldo: `escort runs to the leader (X m)`, `caught up (X m), walks` |
| H8 | Que tu lider pelee con bandidos | Pelea contra quien ataca a tu lider (nativo: el paquete de guardaespaldas; respaldo B: ataca al mismo blanco que tu lider) | respaldo: `fights for the leader against <enemigo>` |
| H9 | Pelea entre dos de tu grupo (sparring) o tu lider ataca a alguien de la faccion del ayudante | El ayudante no se mete | — |
| H10 | 12 h antes del final | Cartel "Me ire pronto" | `expiry warning (N h left)` |
| H11 | Dejar pasar el final | Cartel "Hasta aqui llego. Suerte", roller "<nombre> cumplio su palabra y sigue su camino."; vuelve a su rutina | `<nombre> stops helping: time up` |
| H12 | RECHAZAR en el popup | "Como quieras. Suerte en el camino.", roller "<nombre>: rechazaste su ayuda."; F8 o la tirada no lo hacen ofrecerse de nuevo por 3 dias (la tirada natural; F8 lo salta) | `offer rejected; no offer for 3 days` |
| H13 | Cerrar el popup con la X | Se va sin enfriamiento; no vuelve a preguntar ese dia | `offer dismissed (closed with no answer)` |
| H14 | Guardar a mitad del contrato, salir, cargar, acercarse | Cartel "Sigo contigo", vuelve a escoltar; la tarjeta dice los dias que quedaban | `re-armed after a load` |
| H15 | Ordenar a uno de tu grupo que lo ataque | Cartel "No vuelvo a ayudarte", roller "<nombre> dejo de ayudarte: tu grupo lo ataco."; deja de seguirte; 3 dias sin ofrecerse | `stops helping: attacked by the squad` |
| H16 | Noquearlo con tu grupo | Igual que H15 | `stops helping: knocked out by the squad` |
| H17 | Mientras un sometido camina hacia vos (12.x) | Ningun ayudante se ofrece hasta que termina, y al reves | — |
| H18 | `helperEnabled` apagado, o preset 0 (Cheat) | Nadie se ofrece (los contratos en curso siguen hasta su fin) | — |

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
| T3 | Seleccionar el aliado 80+ (clic izquierdo) con uno de tu grupo seleccionado antes | La tarjeta muestra "Duelos: W-L, si ganas X% se une" como segunda linea; cambiar de miembro seleccionado cambia el numero | — |
| T4 | Aliado de nivel 79, o un no aliado | Nunca RETAR A DUELO (Cheat si lo ofrece: 12.29 J7) | — |
| T5 | RETAR A DUELO sin nadie de tu grupo seleccionado (o el seleccionado inconsciente) | Roller "Selecciona a alguien de tu grupo para hacerlo"; nada se abre | — |
| T6 | RETAR A DUELO con tu luchador seleccionado | Se cierra la ventana de TotalRecruit y se abre la ventana **Dueling** de TotalSparring con ese luchador preseleccionado; roller "<nombre> acepta el reto: ..." | `[duel] <tuyo> challenges <nombre> [key ...]` |
| T7 | Con la ventana Dueling ya abierta, volver a Hablar y RETAR A DUELO | Roller "Ya hay un duelo abierto en TotalSparring."; el reto nuevo se borra | `TotalSparring did not open the duel ... challenge withdrawn` |
| T8 | Ganar el duelo (amistoso) | Balanced: se une solo si sale la chance (12.29); si se une, **para siempre** (no temporal: su tarjeta no dice "Temporal"), cartel "Me ganaste. Me uno a ti" sobre el y roller "<nombre> perdio el duelo y se une a tu grupo para siempre." | `[duel] bout duel outcome=1 ... -> won`, `[duel] <tuyo> won (wins N): join chance X%, roll R -> joined ...` y `lost the duel and joins for good` |
| T9 | Perder (o empatar) el duelo | Cartel "Vuelve cuando seas mas fuerte"; roller con las horas; RETAR A DUELO desaparece por 24 h de juego | `-> lost` y `won the duel: no challenge until hour N` |
| T10 | Despues de T9: guardar, salir, cargar, Hablar con el mismo NPC antes de 24 h | Sigue sin RETAR A DUELO (el enfriamiento viajo en el save); pasadas las 24 h vuelve | — |
| T11 | Sparring (no duelo) entre tu luchador y ese NPC despues de retarlo | No cuenta: ni se une ni enfriamiento | `[duel] bout spar ...` solo con `probeOnClick` |
| T12 | Retar y cancelar la ventana Dueling, o interrumpir el duelo | Nada pasa; el reto queda pendiente; un duelo terminado dentro de 6 h de juego todavia cuenta; despues de 6 h no | `-> ignored`; `challenge(s) expired unanswered` |
| T13 | Duelo a muerte ganado | Si murio: roller "<nombre> murio en el duelo." y nadie se une | `died in the duel: nobody to recruit` |
| T14 | La fila DUELOS contra el Fight Log de TotalSparring para ese par | Mismos ganados-perdidos (empates como tercer numero) | — |
| T15 | `duelChallengeEnabled` apagado | Nunca RETAR A DUELO | — |
| T16 | Salir de la partida / cerrar el juego con TotalSparring cargado | Sin crash; TotalRecruit se descarga limpio (el listener se da de baja) | `stopPlugin called.` sin errores de `[ts-bridge]` |

### 12.29 Ganar un duelo da una chance de que se una; Cheat tambien reta (duel-join-chance)

Diseno del usuario (2026-10-04). Preparacion como 12.28. Defaults: chance base 25, +20 por victoria (25/45/65/85/100), enfriamiento tras ganar sin union 24-72 h (nivel 80-99) o 24-120 h (100+), horas de juego. Para probar rapido: `duelWinCooldownMinHours` = 1 y las dos maximas = 1 (o `duelWinCooldown*Hours` = 1 en `config.json`); `duelJoinBaseChance` = 0 fuerza "no se une", = 100 fuerza "se une". Mockup: `ui-mockup/readout.html` seccion 5 "Ally 80+ can be challenged ...", "Cruel ally 80+ ...", "Cheat: an 80+ NPC ..." y seccion 1 "Ally 80+ you can challenge".

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| J1 | Balanced: Hablar con el aliado 80+ con un luchador que le gano 2 veces | Ventana: fila DUELOS "2-1" y debajo **SI LE GANAS** "65% de que se una"; tarjeta "Duelos: 2-1, si ganas 65% se une" | — |
| J2 | Sin historial legible (TotalSparring no responde) | Sin fila DUELOS ni SI LE GANAS; RETAR A DUELO sigue | — |
| J3 | Ganar un duelo con la chance en 0 (base 0, por victoria 0) | No se une. Cartel informativo "Buena pelea. Retame otra vez" (~25 s); roller "<nombre> perdio el duelo, pero no se une (0% de que se uniera). Podras retarlo otra vez en N h."; RETAR A DUELO desaparece | `[duel] <tuyo> won (wins N): join chance 0%, roll R -> stays (cooldown H h until hour T) vs <nombre> level L [key ...]` |
| J4 | Con defaults, nivel 80-99: la H de J3 | Entre 24 y 72 h; nivel 100+: entre 24 y 120 h | igual que J3 |
| J5 | Despues de J3: guardar, salir, cargar antes de la hora T | Sigue sin RETAR A DUELO (el enfriamiento viaja en el save, la misma fila que el de perder); pasada la hora vuelve, y la fila dice la chance de la victoria siguiente | — |
| J6 | Recargar un save hecho justo antes de ganar y ganar de nuevo con el mismo historial | La misma respuesta (se une o no) y el mismo enfriamiento: la tirada se siembra con el NPC y el numero de victorias | misma `roll R` |
| J7 | Preset Cheat: Hablar con un NPC no animal de nivel 80+ (TotalSparring presente) | La respuesta de Cheat de siempre (p. ej. "Metelo en una de tus jaulas...") mas RETAR A DUELO, DUELOS y SI LE GANAS "se une"; tarjeta "Duelos: W-L, si ganas se une"; nivel 79 o un animal: sin duelo | — |
| J8 | Cheat: ganar el duelo | Se une para siempre siempre (chance 100, sin enfriamiento) | `join chance 100%, roll 0 -> joined` |
| J9 | Perder o empatar (Balanced o Cheat) | Igual que 12.28 T9 (`duelChallengeCooldownHours`); en Cheat la espera es 1 minuto, redondeada a la proxima hora entera (8 H10) | `-> lost` |
| J10 | Cambiar `duelJoinBaseChance` o `duelJoinChancePerWin` en `config.json` y reiniciar | La fila y la tarjeta muestran la chance nueva | — |

### 12.30 Ayudante con contrato nativo y temporal que vuelve a casa (native-ai-packages T2/T3)

KenshiLib 0.5.0 vendorizado: el ayudante usa el paquete Bodyguard del juego (como un mercenario contratado) y el temporal que se va recibe el paquete del Wandering Squad. Preparacion como 12.25 / 12.26. Log `[helper]` y `[temp]` siempre.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| N1 | Aceptar un ayudante (12.26 H1/H4) con `helperNativeBodyguard` encendido | Te sigue a distancia de mercenario, corre si quedas lejos y pelea a quien te ataca | `[helper] <nombre> native bodyguard contract -> <lider> (N h) ok: ...; package ...` |
| N2 | Ayudante que venia en un pelotón con otros | Antes del contrato pasa a un pelotón propio (sus companeros no te siguen) | `in a platoon of N; moved to a platoon of its own (package ...)` |
| N3 | F8 sobre un ayudante ya activo | Loguea si es nativo o por codigo y le asigna el contrato nativo | `debug key on helper <nombre>: native|code-driven ...` |
| N4 | `helperNativeBodyguard` apagado (o el contrato falla) | Escolta por codigo (mecanismo B): repath cada 1 s, camina cerca, corre si esta a mas de 12 m, se ubica a ~3 m del lider y ataca al blanco del lider | `helperNativeBodyguard is off; code-driven escort` / `native contract failed; the code-driven escort takes over`, `escort runs|walks to the leader (X m)`, `fights for the leader against <enemigo>` |
| N5 | Fin del contrato (vencido o traicion) | El contrato nativo se termina y vuelve a su rutina | `native bodyguard contract removed (...) ok` y `stops helping: ...` |
| N6 | Un temporal cumple su tiempo y se va (12.25 T7) | Camina o viaja (a su casa, una ciudad), no se queda parado | `[temp] <nombre> native wander package <paquete> assigned (...)` |
| N7 | Si el paquete falla | Camina de vuelta al lugar donde se unio; llega a ~6 m o se rinde a los 240 s y vuelve a su IA | `native wander package failed: ...`, `[temp] <nombre> heading back to (x,z)`, luego `arrived` o `gave up` |

### 12.31 Arreglos de la prueba en juego (native-ai-packages, play-test fixes)

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| F1 | Con la tarjeta de un aliado 80+ a la vista, pelear un duelo con el y volver a seleccionarlo (o esperar) | La linea "Duelos: W-L..." cambia en ~2 s sin cambiar de seleccion (antes quedaba "0-0") | — |
| F2 | Ganar o perder un duelo (12.28 T8 / T9) | Los carteles "Me ganaste. Me uno a ti" / "Vuelve cuando seas mas fuerte" / "Buena pelea. Retame otra vez" duran ~25 s (informativos), no 90 | `[notice] ... (info)` |
| F3 | Comparar el "nivel N" del popup con el "Lv N" de la tarjeta de duelo de TotalSparring para el mismo NPC | Mismo numero (nivel general de habilidad, redondeado como TotalSparring); precios, tramos y chances se mueven con ese nivel | si no se puede leer: `[level] The overall skill level could not be read; using the rough ...` (una vez) |

### 12.32 Descanso de Noquear y estres por golpes (cage-beating-stress)

Con `"probeOnClick": true` y los valores por defecto (descansos de 1 h; base 0,08 por golpe y 0,15 por noqueo; ganancia 0,02..0,25; niveles 0,5..1,5; se desvanece en 24 h; en Balanced rendimiento 0,7 por golpe del dia y tope diario 0,35, S13-S18). En Balanced la linea `[stress]` termina en `[day D blow N, xM, today T% of cap C%]`; S1-S12 siguen valiendo para el primer golpe del dia. Mockup: `ui-mockup/readout.html`, "Menu on your prisoner, Golpear and Noquear resting" y la tarjeta "Caged bandit" (Golpes). La ganancia es `clamp(base x clamp(nivel lider / nivel preso, 0,5, 1,5) / aguante, 0,02, 0,25)` (aguante: miedoso 0,6, valiente 1,6, honorable 1,8, el resto 1).

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| S1 | Golpear (12.19 C3) a un preso despierto de tu jaula, de personalidad normal y del mismo nivel que tu lider | El golpe pega y suma 8% de estres por golpes; en la tarjeta aparece "Golpes 8" en su propia fila y el estres total sube 8 puntos | `[stress] <preso> beating stress +8% (blow, player N vs npc N, resolve 1.00) -> 8%` y en `[readout]` `(hunger .. wounds .. tired .. beaten 8)` |
| S2 | Repetir S1 con un preso miedoso y uno honorable (o anotar los que haya) | Miedoso: +13%; honorable: +4%; nunca menos de 2% ni mas de 25% | `beating stress +13% (... resolve 0.60)` / `+4% (... resolve 1.80)` |
| S3 | Noquear (12.19 C4) a un preso despierto de tu jaula y esperar a que caiga | **No** cuenta derrota (el numero de DERROTAS de la tarjeta no cambia); suma 15% (mismo nivel, normal) | `[defeat-hook] knockout fired for <preso>`, `[stress] <preso> beating stress +15% (knockout, ...)`, `[defeat] <preso> cage knockout: not a defeat (stress +15%)`; ningun `defeated by the player` |
| S4 | Justo despues de S3, cuando el preso se levante, clic derecho sobre el | "Noquear (1 h)" en lugar de "Noquear NN%"; al pulsarlo: "Podras volver a noquearlo en 1 h" y no se da ninguna orden; en la ventana NOQUEAR dice "NOQUEAR (1 h)" | sin `knockout ordered` |
| S5 | Dar Noquear y que el preso **no** caiga (cancelar con otra orden, 12.19 C10g) | Noquear no queda en descanso (solo descansa tras un noqueo que tumbo al preso) | `knockout cleanup (...) after order abandoned`; sin `cage knockout` |
| S6 | Golpear hasta noquear (12.19 C3b) | No es derrota; el golpe ya sumo su estres | `beating stress +X% (blow, ...)` y `[defeat] <preso> cage knockout: not a defeat (stress +0%, the blow added its own)` |
| S7 | Tras S3, mientras el preso sigue inconsciente, clic derecho varias veces sobre el | Tampoco cuenta por el clic derecho | `[defeat-skip] <preso>: already counted this knockout` (o un motivo de credito); ningun `defeated by the player` |
| S8 | Noquear a un NPC en el campo, fuera de una jaula (con el menu del juego o en pelea) | Cuenta como derrota igual que antes | `<nombre> defeated by the player (...; count N ...)` |
| S9 | Con la barra en, por ejemplo, 30%, esperar 6 horas de juego (a velocidad x3) | Baja 25 puntos (una barra llena se va en 24 h): queda en 5%; nunca baja de 0 | `[readout] ... beaten 5)` |
| S10 | Con estres por golpes, guardar, salir y cargar ese save | La barra vuelve con el valor del save, desvanecida segun las horas de juego que pasaron desde el ultimo golpe; cargar un save **anterior** a los golpes la deja en 0 | `[readout] ... beaten N)` |
| S11 | `submissionBeatStressBase` en 0 y golpear | El golpe no suma estres (base 0 apaga el termino; el minimo no aplica) | `beating stress +0% (blow, ...)` |
| S12 | Con la barra de golpes alta, comparar la chance de la tarjeta antes y despues de golpear | La chance sube con el estres (pesa como el resto del estres, con su tope `maxStressChance` y el aguante) | `[chance] ... stress=N(..%)` |
| S13 | Balanced (balanced-tuning T2), rendimiento decreciente: preso normal del nivel de tu lider; Golpear (S1), esperar el descanso, Noquear (S3), esperar, Golpear otra vez, todo el mismo dia de juego | Cada golpe del dia vale menos: +8%, +11% (15 x 0,7), +4% (8 x 0,49); nunca menos de 2% mientras quede tope | `[stress] <preso> beating stress +8% (...) -> 8% [day D blow 1, x1.00, today 8% of cap 35%]`, luego `+11% (knockout, ...) [day D blow 2, x0.70, today 19% of cap 35%]`, luego `+4% (blow, ...) [day D blow 3, x0.49, today 22% of cap 35%]` |
| S14 | Balanced, tope diario: seguir golpeando y noqueando al mismo preso el mismo dia de juego | Lo ganado en el dia no pasa de 35%; al llegar, los golpes siguientes suman +0% (siguen contando para el rendimiento) | `[day D blow N, ..., today 35% of cap 35%]` y `beating stress +0%` |
| S15 | Balanced, dia nuevo: tras S14, esperar a que cambie el dia de juego (medianoche) y golpear | El primer golpe del dia vuelve a valer completo (+8% en un normal del mismo nivel) | `[day D+1 blow 1, x1.00, today 8% of cap 35%]` |
| S16 | Balanced, save: a mitad de S13 o S14, guardar, salir y cargar ese save; golpear otra vez el mismo dia | Sigue la cuenta del dia (blow N+1, el "today" del save); un save de antes de T2 (sin las filas) cuenta como dia nuevo | `[day D blow N+1, ...]`; en un save viejo `[day D blow 1, x1.00, ...]` |
| S17 | `submissionBeatingDiminishFactor` 1.0 y `submissionBeatingDailyStressCap` 1.0 en `config.json` | Sin rendimiento decreciente: cada golpe vale como el primero (la barra igual topa en 100%) | `[day D blow N, x1.00, today ... of cap 100%]` |
| S18 | Cheat: golpear varias veces el mismo dia | Sin limites diarios: cada golpe suma lo de S1/S2 y la linea no lleva el corchete del dia | `beating stress +8% (...) -> N%` sin `[day ...]` |

### 12.33 Dos presets, esperas de Cheat de 1 minuto y horas de jaula guardadas (v1-presets-close)

Con `"probeOnClick": true`. Casos P (presets), W (esperas de Cheat), H (horas de jaula guardadas) y C (bonus por cuidarlo, jaula de Balanced sin atajos).

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| P1 | `preset` | Va de 0 a 1 y dice "0: Cheat, 1: Balanced"; no hay fila "Peso de Faccion" | — |
| P2 | Cerrar Kenshi, poner `"preset": 2` en el `config.json` desplegado, abrir | Corre Balanced (precios, aliados por nivel); el log lo dice una sola vez | `config.json preset 2 is not a preset (0: Cheat, 1: Balanced); running Balanced.` |
| P3 | Dejar una clave `"factionWeight": 0.75` en el `config.json` desplegado y abrir | Nada cambia: la clave se ignora, sin errores ni avisos; el precio de un hostil y el de un amigo no aliado salen de la misma curva | sin lineas de `factionWeight` |
| W1 | Cheat (`"preset": 0`): los casos 8 H2, H3, H7, H8, H9, H10 | Cada espera dura 1 minuto de juego; rechazo y duelo terminan en la proxima hora entera (como mucho ~1 h) | `[hold] clock started`, `called out to the player.`, `reject cooldown until game hour N (1 h)`, `no challenge until hour N` |
| W2 | Balanced: Golpear (12.32 S4), llamado (`hailFirstHours`, 12.23 N1), rechazo (12.21 S7), duelo perdido (12.28 T9) | Sin cambios: 1 h, 24 h, 72 h, 24 h | igual que antes |
| H1 | Encerrar a un preso, esperar 3 h de juego, guardar, salir de Kenshi, abrir y cargar ese save (cerca del preso) | Tras la carga **no** aparece un `clock started` nuevo para el: el reloj siguio desde el encierro original (en Balanced el reloj ya no decide nada desde P4; se nota en Cheat, H2) | sin `[hold] clock started: seen in your cage` para ese preso tras la carga |
| H2 | Cheat: encerrar a un preso, guardar enseguida (antes del minuto), cargar y esperar | Llama al completar el minuto contado desde el encierro original, no desde la carga | `called out to the player.` |
| H3 | Guardar con un preso encerrado, sacarlo de la jaula, guardar en otra ranura; cargar la segunda | No hay reloj: se reinicia al volver a encerrarlo | `[hold] clock started: seen in your cage` al volver a encerrarlo |
| H4 | Encerrar a un preso (reloj en la hora N), cargar un save **anterior** a la hora N en el que el mismo preso ya estaba encerrado | El reloj se reinicia en la hora del save viejo, nunca queda negativo ni congelado | `[hold] held=0.0h` y despues cuenta normal |
| C1 | Balanced: un preso de tu jaula sin heridas, comido y con la salud llena; clic derecho y Hablar | **No** se une solo: tira la chance normal; la tarjeta y la ventana muestran `LO CUIDASTE +N%` con N entre 5 y 20, y la chance ya lo incluye | `[route] cage` y `[chance] ... caredFor=+N ... -> M%` |
| C2 | Repetir C1 varias veces el mismo dia, y guardar/cargar y repetir | El mismo N cada vez (sorteo estable por NPC y por dia) | el mismo `caredFor=+N` |
| C3 | Esperar al dia de juego siguiente y repetir C1 | N puede cambiar (otro sorteo), siempre entre 5 y 20 | `caredFor=+N2` |
| C4 | Un preso herido, hambriento o con la salud no llena | Sin fila `LO CUIDASTE` y `caredFor=+0` | `[chance] ... caredFor=+0` |
| C5 | Un animal en tu jaula, sano y lleno | Sin bonus: lo deciden la comida del jugador y la barriga (12.20) | sin `caredFor` (ruta animal) |
| C6 | `submissionCaredForBonusMin` = "max" = 12; luego min 15 y max 3 | Primero siempre +12; despues siempre +15 (un maximo menor que el minimo cuenta como el minimo) | `caredFor=+12` / `caredFor=+15` |
| C7 | Balanced: dejar a un preso 6 h o mas en la jaula y hablarle | No se une por tiempo (12.3 E3) | ninguna linea `easy-mode needs` |
| C8 | Dejar `"easyHoursCaged": 6.0` en el `config.json` desplegado y abrir | Se ignora: sin errores ni avisos, y nadie se une por tiempo | sin lineas de `easyHoursCaged` |

### 12.34 Animales: duelo, 2/2 en Balanced y cura/derrota en Cheat (animal-duel-submission)

Con `"probeOnClick": true`, TotalSparring instalado y los valores por defecto (`duelAnimalsEnabled` on, `duelAnimalsFromLevel` 40, `submissionDefeatsToBreak` 2, `duelJoinBaseChance` 25 / `duelJoinChancePerWin` 20). "Animal libre" = sin dueno (fauna salvaje: "Wild Dogs", etc.). Mockup: `ui-mockup/readout.html`, tarjetas "Wild animal 40+ ...", "Wild animal beaten ..." y "Cheat: a wild animal your squad beat joins", ventanas "Wild animal 40+ can be challenged to a duel" y "Wild animal beaten twice yields". Contra un animal la ventana Dueling de TotalSparring ya no muestra apuesta (TotalSparring `master` `8782b2c`); si aparece la fila de apuesta, el TotalSparring instalado es anterior.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| A1 | Balanced: seleccionar un animal libre de nivel 40 o mas y abrir Hablar | La ventana conserva la respuesta de doma (p. ej. DARLE COMIDA) y agrega **RETAR A DUELO**, DUELOS y SI LE GANAS "25% de que se una"; la tarjeta trae "Duelos: 0-0, si ganas 25% se une" | `[readout] <animal> route=animal ...` |
| A2 | Balanced: un animal libre de nivel **menor que 40** | Sin RETAR A DUELO en la ventana ni linea de duelos en la tarjeta | sin `[duel]` |
| A3 | Balanced: un animal **con dueno** (bestia de carga de una faccion, perro de los Holy Nation Outlaws) de nivel 40+ | Sin RETAR A DUELO; la doma dice "ya tiene dueno" | `refuses`/`Refused <animal> (rechazo): ya tiene dueno ...` |
| A4 | `duelAnimalsEnabled` OFF (o `exclude.animals` true) y repetir A1 | Sin RETAR A DUELO (con `exclude.animals` el animal ni siquiera se puede hablar) | sin `[duel]` |
| A5 | A1 -> RETAR A DUELO con un personaje seleccionado | Se abre la ventana Dueling de TotalSparring; el roller dice "<animal> grune y se planta frente a ti: elige el duelo ..." (narrador, nunca su voz) | `[duel] <retador> challenges <animal> [key id:...]` |
| A6 | Balanced: ganar el duelo de A5 | Tirada al 25%: si sale, "<animal> agacha la cabeza y te sigue ..." y se une para siempre; si no, "<animal> se sacude y se aleja sin seguirte (25% ...). Podras retarlo otra vez en N h" (24-72 h). **Ningun cartel** sobre su cabeza en ningun caso | `[duel] <ganador> won (wins 1): join chance 25%, roll R -> joined vs <animal> ...` y `[duel] <animal> lost the duel and joins for good (animal) [key ...]`, `Recruited <animal>.`; o `-> stays (cooldown X h until hour N) vs <animal> level L` |
| A7 | Balanced: perder (o empatar) el duelo de A5 | "<animal> gana el duelo y se aleja sin volverse. Podras retarlo otra vez en 24 h"; sin cartel; el animal **no** queda hostil por el mod; el boton desaparece 24 h | `[duel] <animal> won the duel: no challenge until hour N` |
| A8 | Cheat (`"preset": 0`): repetir A1 y ganar el duelo | SI LE GANAS dice "se une"; al ganar se une siempre | `join chance 100%, roll 0 -> joined vs <animal>` |
| A9 | Balanced: noquear una vez a un animal libre en el campo (pelea normal) y seleccionarlo | La tarjeta suma **DERROTAS 1/2** junto a la barra de barriga; sigue la linea de doma | `<animal> defeated by the player (...; count 1, id:...)` y `[readout] ... defeats=1/2 ...` |
| A10 | Noquearlo una segunda vez y seleccionarlo (sin Hablar) | Ruta sometido: la tarjeta muestra la probabilidad ("N%: puede unirse si le hablas"), DERROTAS 2/2 y la barra de barriga | `[readout] <animal> route=animal chance=N% ... defeats=2/2` |
| A11 | Hablar con el animal de A10 | Tirada del dia: si sale, ventana con "Gratis", la probabilidad y ACEPTAR ("lo venciste demasiadas veces: agacha la cabeza y te sigue"); si no, "todavia no se somete: vencelo otra vez, o dale de comer" y el boton DARLE COMIDA si lo pide. Desde proactive-offer el animal tambien camina hasta ti a pedirlo (12.37 O16-O17) | `<animal> [route] field-yield [key id:...]`, `[chance] level=.. personality=<el valor de Normal> defeats=2(x..) situation=10 ...` (personalidad Normal aunque el motor diga otra, sin -25 de lealtad); al fallar `Refused <animal> (cuidados): todavia no se somete ...` |
| A12 | Un animal libre de A11 que fallo la tirada, alimentado de tu mano hasta la barra | Se une por la comida aunque la tirada del dia haya fallado ("comio de tu mano y te sigue") | sin `Refused`; ventana con ACEPTAR |
| A13 | Un animal **con dueno** noqueado 2+ veces en el campo | Nunca ruta sometido ni fila DERROTAS: sigue "ya tiene dueno: metelo en una jaula" | `[route] free-world` |
| A14 | Un animal en tu jaula con 2+ derrotas | Sigue la ruta de la jaula / doma (sin sometido) | `[route] cage` |
| A15 | Cheat: noquear a un animal libre una vez (pelea o duelo) y hablarle, fuera de la jaula | Se une al momento: "lo venciste: te reconoce como el mas fuerte y te sigue"; tarjeta "Lo venciste: te sigue" | `<animal> defeated by the player (...; count 1 ...)`; sin `Refused` |
| A16 | Cheat: curar con un personaje tuyo (primeros auxilios o doctor) a un animal libre herido y hablarle | Se une al momento: "lo curaste: confia en ti y te sigue"; tarjeta "Lo curaste: te sigue". La marca se escribe una sola vez aunque la cura dure varios segundos | `[heal] <animal> treated by <curador>: healed by the player [key id:...]` (una vez) |
| A17 | Tras A16 (sin reclutarlo), guardar, salir, cargar y hablarle | Sigue uniendose por la cura (la marca viaja en el save, `total_recruit.healed_by_player`) | sin `[heal]` nuevo; ventana con ACEPTAR |
| A18 | Cheat: darle de comer a un animal libre (§8 H11) | Igual que antes: "comio de tu mano y te sigue" | `Fed <animal> ...; meals from the player 1` |
| A19 | Cheat: un animal **con dueno** curado o noqueado, fuera de la jaula | Sigue pidiendo la jaula ("Metelo en una jaula") | `Refused <animal> (rechazo): metelo en una de tus jaulas ...` |
| A20 | Cheat: una **persona** noqueada o curada por ti, fuera de la jaula | Sin cambios: pide la jaula | `Refused <nombre> (rechazo): metelo en una de tus jaulas ...` |

### 12.35 Cheat: sirve cualquier jaula (cheat-any-cage)

Con `"probeOnClick": true`, Cheat (`"preset": 0`) y los valores por defecto (`cheatNeedsCage` on, `cheatAnyCage` on). "Jaula ajena" = jaula de otra faccion (esclavistas, prision de una ciudad), grilletes o cargado por otro (`SafeInPrison`).

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| X1 | Acercarse a un preso en una jaula ajena (que el barrido lo vea) | El reloj arranca sin hablarle | `<nombre> [hold] clock started: seen in a cage` |
| X2 | Hablarle enseguida (antes del minuto) | "esta dentro: dale un momento y te llamara"; la tarjeta muestra la ruta de captividad | `Refused <nombre> (...): esta dentro: dale un momento y te llamara` |
| X3 | Esperar un minuto de juego y hablarle | Se une sin condiciones (sigue encerrado: hay que abrir la jaula, y los guardias pueden reaccionar como a una fuga) | sin `Refused`; `Recruited <nombre>.` |
| X4 | `cheatAnyCage` OFF y repetir X2/X3 con otro preso ajeno | Pide tu jaula como antes ("metelo en una de tus jaulas") y el reloj no arranca por la jaula ajena | `Refused <nombre> (rechazo): metelo en una de tus jaulas ...`; sin `seen in a cage` |
| X5 | Cheat ON: un NPC libre en la calle | Pide una jaula, sin decir "tus jaulas" (duel-any-source U5, 12.38 D14) | `Refused <nombre> (rechazo): metelo en una jaula ...` (o otra linea de `cheat.needsAnyCage`) |
| X6 | Liberar al preso de X1 antes de hablarle (o que salga de la jaula) | El reloj se pierde; al volver a una jaula empieza de cero | `<nombre> [hold] clock dropped: no longer in a cage` |
| X7 | Tu propia jaula en Cheat | Igual que H2/H3; el log dice tu jaula | `<nombre> [hold] clock started: seen in your cage` |
| X8 | Abrir el menu de un preso en jaula ajena | Sin GOLPEAR / NOQUEAR: los comandos de jaula siguen siendo solo para tus jaulas | -- |
| X9 | Balanced (`"preset": 1`): un preso en jaula ajena | Sin cambios: sigue la respuesta de acceso de Balanced (jaula ajena) | sin `seen in a cage` |

### 12.36 La tarjeta dice cuanto lleva encerrado (prisoner-hold-time)

Con `"probeOnClick": true`. La linea va justo bajo la cabecera (nombre + etiqueta de ruta), tambien en tarjeta compacta, en tiempo de juego y redondeada hacia abajo. Texto en `messages.json` `readout.line.hold.elapsed` ("Encerrado: {time}"). Mockup: filas "Caged bandit" / "Caged prisoner you cared for" y la ultima, "Cheat: a prisoner held in another faction's cage", en `ui-mockup/readout.html`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| T1 | Balanced: meter a un preso en tu jaula y seleccionarlo enseguida | "Encerrado: 0 min" (o "1 min") bajo la cabecera | `[readout] <nombre> ... held=Encerrado: 0 min line=...` |
| T2 | Dejarlo 2 h 15 min de juego y seleccionarlo (la tarjeta se refresca cada 2 s) | "Encerrado: 2 h 15 min"; a las 2 h justas "Encerrado: 2 h" | `held=Encerrado: 2 h 15 min` |
| T3 | Mas de un dia | "Encerrado: 1 d 3 h" (sin minutos); a las 24 h justas "Encerrado: 1 d" | `held=Encerrado: 1 d 3 h` |
| T4 | Un NPC libre, un animal libre, un miembro temporal o un ayudante | Sin linea de encierro | `held=-` |
| T5 | Un preso que el barrido todavia no vio en tu jaula (recien encerrado fuera de rango) | Sin linea hasta el primer avistaje: la tarjeta no arranca el reloj | `held=-` y luego `[hold] clock started: seen in your cage` |
| T6 | Cheat con `cheatAnyCage` ON: un preso en jaula ajena tras X1 | "Encerrado: N min" | `held=Encerrado: ...` |
| T7 | Cheat con `cheatAnyCage` OFF, o Balanced: un preso en jaula ajena | Sin linea de encierro | `held=-` |
| T8 | Guardar y cargar con el preso aun en tu jaula | Conserva sus horas (P3): la linea sigue desde donde estaba | `held=Encerrado: ...` |


### 12.37 Oferta proactiva: viene a ofrecerse sin Hablar (proactive-offer)

Con `"probeOnClick": true`, TotalSparring instalado y los valores por defecto (`offerOnDuelWin` on, `offerOnHeal` on, `offerFailCalmSeconds` 20, `submissionEventEnabled` on). IGNORAR dura `submissionForgiveSeconds` (30 s por defecto; `deploy.sh` pasa 300 -> 30 solo si tu config sigue en el viejo default). Mockup: `ui-mockup/readout.html`, escena "Proactive offer" y ventanas "Submitted NPC asks to join" (ahora con IGNORAR), "Duel rival walks up and offers to join" y "Animal walks up and offers to join". El rival tiene que estar a 120 m o menos del lider cuando termina el duelo. Filtrar el log por `[offer]`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| O1 | Cheat (`"preset": 0`): ganar un duelo a una persona 80+ | En pocos segundos camina hasta tu lider, calmado, con "Se acerca para hablar contigo" sobre la cabeza y el roller "<nombre> se acerca para hablar contigo."; al llegar abre la ventana Sometido con **ACEPTAR / IGNORAR / RECHAZAR** (Gratis). Ya no se une al instante | `[duel] <ganador> won (wins N): join chance 100%, roll R -> the offer event brings <nombre> over [key ...]`, `[offer] request queued: <nombre> (duel win, join) [key ...]`, `[offer] serve: <nombre> (duel win, join, hostile no) -> walk up and offer`, `[offer] walk-up: <nombre> comes to offer (duel win)`, `[offer] consume: <nombre> (served)` |
| O2 | ACEPTAR en O1 | Se une; roller "<nombre>: se une a ti." | `[offer] outcome: <nombre> accepted: joined the squad` |
| O3 | Balanced: ganar un duelo y que la tirada **salga** | Igual que O1 (camina y ofrece) | `join chance N%, roll R -> the offer event brings ...` |
| O4 | Balanced: ganar un duelo y que la tirada **falle**, con el rival peleandote | La linea de siempre "perdio el duelo, pero no se une ... Podras retarlo otra vez en N h" y ademas "<nombre> no quiere unirse, pero deja de pelear por 20 s."; deja de atacar 20 s donde esta (sin huir) y despues vuelve a lo que hacia | `-> stays; the offer event calms it`, `[offer] request queued: <nombre> (duel win, calm)`, `[offer] serve: <nombre> (duel win, calm, hostile yes) -> calm in place`, `[offer] calm: <nombre>: the roll failed; hostile before yes`, `[offer] calm: <nombre> stops attacking for 20 s in place` |
| O5 | O4 con un rival que **no** te ataca | Solo la linea de siempre; ninguna calma ni linea extra | `[offer] serve: ... -> nothing (not hostile)`, `[offer] consume: <nombre> (a no from someone not hostile)` |
| O6 | Ganar el duelo a un animal libre 40+ (Cheat, o Balanced con tirada buena) | Camina hasta ti **sin cartel** sobre la cabeza y sin hablar; roller "<animal> se acerca con la cabeza gacha."; ventana con etiqueta **Animal**, "<animal> agacha la cabeza frente a ti: quiere seguirte.", Gratis, ACEPTAR / IGNORAR / RECHAZAR. ACEPTAR: "<animal> te sigue: ahora es parte de tu grupo." | `[offer] walk-up: <animal> comes to offer (duel win) (animal)`; `[submission] <animal> submitted (offer requested by duel win), walks up ...; animal` |
| O7 | IGNORAR (persona o animal) | Se aleja corriendo 80 m y no ataca durante `submissionForgiveSeconds`; persona: dice "Gracias..." y el roller "<nombre>: lo ignoraste y se aleja."; animal: solo "<animal> se aleja trotando." | `[offer] outcome: <nombre> ignored: runs off and stays calm`, `[submission] <nombre>: calm for 30 s, running off` (si el animal no corre: `running off FAILED (stays)`) |
| O8 | RECHAZAR | La ventana se cierra y el NPC vuelve a lo que hacia. **Sin** enfriamiento (hablarle enseguida no dice "no quiere saber nada de ti"), **sin** borrar derrotas (la tarjeta conserva DERROTAS) y sin linea en el roller. Si te atacaba antes del evento, vuelve a ser hostil | `[offer] outcome: <nombre> rejected: nothing changes (no defeats wiped, no cooldown)` (o `..., hostile again as before)`), `[submission] <nombre>: rejected (from holding, hostile before ...)` |
| O9 | Tras O8, esperar un minuto cerca sin hacer nada | No vuelve a ofrecerse hasta un gesto nuevo (otro duelo ganado, otra cura) | sin `[offer] walk-up` nuevo |
| O10 | Cheat: curar (primeros auxilios o doctor) con un personaje tuyo a un animal libre herido | Al estar de pie camina hasta ti y abre la ventana Animal, sin Hablar. La peticion se registra una vez aunque la cura dure varios segundos | `[offer] request queued: <animal> (heal, roll)` (una vez), `[offer] roll: <animal> -> joins (0%) ...`, `[offer] serve: <animal> (heal, roll, hostile ...) -> walk up and offer` |
| O11 | Seguir curandolo justo despues de que la oferta se sirvio o se rechazo | Ninguna peticion nueva mientras dura la misma cura (una nueva cura despues de 30 s sin curar si cuenta como gesto nuevo) | sin `[offer] request queued` repetido |
| O12 | Balanced: curar a un NPC hostil que no se uniria (tirada mala) | Al levantarse deja de atacar 20 s: "<nombre> no quiere unirse, pero deja de pelear por 20 s." | `[offer] roll: <nombre> -> no (N%) ...`, `[offer] serve: <nombre> (heal, roll, hostile yes) -> calm in place` |
| O13 | Curar a un vecino amistoso que no se une | Nada visible | `[offer] serve: ... -> nothing (not hostile)` |
| O14 | Curar a alguien y hablarle a mano antes de que venga | La ventana de Hablar de siempre; despues no camina a ofrecerse | `[offer] consume: <nombre> (the player talked by hand)` |
| O15 | Balanced: rescate (un NPC que se moria, curado por ti) | Igual que antes: al despertar ofrece por la ruta de rescate, sin segunda ventana | `[offer] consume: <nombre> (the rescue route offers when it wakes)` o `(the player talked by hand)` |
| O16 | Cheat: noquear una vez en el campo a un animal libre y alejarse a menos de 40 m | Al levantarse camina hasta ti y ofrece (el evento de sometido ya admite animales) | `[submission] <animal> submitted (...cheat.animal.defeated...), walks up ...; animal` |
| O17 | Balanced: una persona o un animal libre vencido 2 veces cuya tirada de rendicion **falla**, peleando | Deja de atacar 20 s; no se vuelve a tirar ese dia | `[submission] <nombre>: field yield not today (N%, day D)`, `[offer] calm: <nombre>: the field yield roll failed; hostile before yes` |
| O18 | Ganar un duelo con el rival a mas de 120 m del lider | Comportamiento viejo (Cheat se une al instante; Balanced tira) | `[offer] <nombre> is out of the event's reach: the duel win resolves as before` |
| O19 | `offerOnDuelWin` OFF y ganar un duelo | Comportamiento viejo (`-> joined vs <nombre>` en Cheat) | sin `[offer]` |
| O20 | `offerOnHeal` OFF y curar a alguien | Nadie camina a ofrecerse por la cura | sin `[offer] request` |
| O21 | Una peticion que nunca se puede servir (el paciente no se levanta en 5 min, o queda lejos) | Se descarta sola | `[offer] 1 request(s) expired unserved after 300 s (never up or never in reach)` |
| O22 | Hablar a mano con cualquiera, sin gesto previo | Igual que antes | sin `[offer]` salvo `consume` si habia una peticion |
| O23 | Guardar y cargar con un evento o una peticion en curso | El NPC recupera su ritmo; la peticion se olvida (hay que repetir el gesto) | `[submission] ...: unloaded (...)` |

### 12.38 Duelos: Cheat a cualquier nivel, duelos de TotalSparring que cuentan, victorias minimas y texto de cualquier jaula (duel-any-source)

Con `"probeOnClick": true`, TotalSparring instalado y los valores por defecto (`duelCheatPeopleFromLevel` 1, `duelCountExternalBouts` on, `duelMinWinsCheat` 1, `duelMinWinsBalanced` 1, `duelExternalBalancedFromLevel` 1, `cheatAnyCage` on). "Duelo externo" = abierto con el boton propio de TotalSparring, sin RETAR A DUELO. Mockup: estado de tarjeta "Ally you can challenge, under the minimum duel wins" y estado de ventana "Cheat: any person can be challenged ...".

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| D1 | Cheat (`"preset": 0`): abrir la ventana de una persona de nivel ~33 (p. ej. "Recluta Samurai") | RETAR A DUELO aparece junto a la respuesta de Cheat | -- |
| D2 | `duelCheatPeopleFromLevel` = 40 y repetir D1 | Sin RETAR A DUELO para el nivel 33 | -- |
| D3 | Balanced (`"preset": 1`): un aliado de nivel 33 | Sin RETAR A DUELO (sigue pidiendo aliado con niveles y 80+) | -- |
| D4 | Cheat: abrir un duelo desde la ventana de TotalSparring (sin RETAR A DUELO) contra un NPC ajeno y ganarlo | Igual que un reto de TotalRecruit ganado: camina a ofrecerse (o se une al instante si esta fuera de 120 m) | `[duel] external bout: <nombre> counts (won)`, `[duel] bout duel outcome=1 a=... (squad) b=... -> won`, `[duel] <tuyo> won (wins N): join chance 100%, roll 0 -> the offer event brings <nombre> over [key ...]` |
| D5 | Como D4 pero perderlo (o empatar) | "Vuelve cuando seas mas fuerte" y el enfriamiento de siempre (en Cheat, un minuto) | `[duel] external bout: <nombre> counts (lost)`, `-> lost`, `[duel] <nombre> won the duel: no challenge until hour H [key ...]` |
| D6 | Sparring o duelo desde TotalSparring entre dos miembros de tu grupo | No cuenta: nadie se une ni se enfria | `[duel] external bout: <id> ignored (squad pair)` (solo duelo; un sparring no escribe esta linea) |
| D7 | Balanced: duelo externo ganado contra alguien excluido (nombre en `exclude.characterNames` o faccion en `exclude.factions`) | Ignorado (U7: desde entonces un NPC reclutable cualquiera cuenta, ver D17) | `[duel] external bout: <nombre> ignored (not offered)` y `-> ignored` |
| D8 | Balanced: duelo externo ganado contra un aliado 80+ que queda noqueado | Cuenta (el tramo de aliado se lee en reposo, no por la ruta inconsciente): tirada de la chance de siempre | `[duel] external bout: <nombre> counts (won)`, `join chance 25%, roll R -> ...` |
| D9 | `duelCountExternalBouts` OFF y repetir D4 | Ignorado, como antes del cambio | `[duel] external bout: <nombre> ignored (switch off)`, `-> ignored` |
| D10 | RETAR A DUELO desde TotalRecruit y ganarlo | Sin cambios (el reto pendiente responde primero; ninguna linea `external bout`) | `-> won` sin `external bout` |
| D11 | `duelMinWinsBalanced` = 3; ganarle a un aliado 80+ por primera vez | No se une ni tira; cartel "Buena pelea. Ganame otra vez"; roller "<nombre> perdio el duelo, pero aun no se une. Victorias que faltan: 2."; sin espera: el boton sigue (D12) | `[duel] <tuyo> won (wins 1): join chance 0%, roll 0 -> needs 2 more win(s) vs <nombre> [key ...]` |
| D12 | Abrir la ventana y la tarjeta del NPC de D11 (1 victoria, minimo 3) | Tarjeta "Duelos: 1-0, faltan 2 victorias"; fila SI LE GANAS "Ganale 2 veces mas" (U6, D19) | -- |
| D13 | Ganarle la segunda vez (D11) | Tarjeta "Duelos: 2-0, si ganas 65% se une" (la proxima decide; la curva cuenta desde la primera victoria) y "Victorias que faltan: 1." en el roller | `... needs 1 more win(s) vs <nombre>` |
| D14 | Cheat, `cheatAnyCage` ON: hablar a una persona libre en la calle | "metelo en una jaula y hablara enseguida" (u otra linea neutra de `cheat.needsAnyCage`), nunca "tus jaulas"; tarjeta "Metelo en una jaula" | `Refused <nombre> (rechazo): metelo en una jaula ...` |
| D15 | `cheatAnyCage` OFF y repetir D14 | La linea de siempre: "metelo en una de tus jaulas ..." | `Refused <nombre> (rechazo): metelo en una de tus jaulas ...` |
| D16 | Cheat con "Duelo Cheat: victorias minimas" = 2: ganarle un duelo a una persona | No se une todavia: "Victorias que faltan: 1."; la segunda victoria recluta (o trae la oferta) | `join chance 0%, roll 0 -> needs 1 more win(s)` y luego `join chance 100%, roll 0 -> ...` |
| D17 | Balanced (U7), valores por defecto: duelo externo ganado contra un NPC comun no aliado de nivel 33 (p. ej. "Recluta Samurai") | Cuenta: tirada de la curva (25% la primera victoria) y la oferta proactiva si sale | `[duel] external bout: <nombre> counts (won)`, `-> won`, `[duel] <tuyo> won (wins 1): join chance 25%, roll R -> ...` |
| D18 | `duelExternalBalancedFromLevel` = 40 y repetir D17 | Ignorado (nivel 33 < 40); un aliado 80+ con tramo sigue contando (D8) | `[duel] external bout: <nombre> ignored (not offered)`, `-> ignored` |
| D19 | Balanced, "Duelo Balanced: victorias minimas" = 4 y 1 victoria: abrir la ventana del NPC con RETAR A DUELO (aliado 80+) | Fila SI LE GANAS "Ganale 3 veces mas" (U6; antes "0% de que se una"); la tarjeta "Duelos: 1-0, faltan 3 victorias" | -- |
| D20 | Balanced: tras D17, abrir la ventana de ese NPC de nivel 33 | Sin RETAR A DUELO (el boton sigue pidiendo aliado 80+): solo el duelo externo cuenta | -- |

### 12.39 Golpear: caminar hasta la jaula, queja, aviso y sonido (beat-feedback)

Golpear ya no pega desde cualquier lado sin que se note: el personaje **seleccionado** camina hasta el preso (la orden de caminar de `npc_motion`, una sola vez, nunca se repite encima de una orden tuya) y el golpe de 12.19 C3 entra recien al llegar a `submissionBeatReachMeters` (4 m; `submissionBeatReachMeters`, 1..15). Si ya esta cerca, pega en el acto. Al pegar: el preso se queja en voz alta ("¡Ugh!", "¡Agh!", "¡Ay!" o "¡Basta!", `cage.beatOuch`, siempre la misma para el mismo preso; un animal o alguien inconsciente no dice nada), el roller dice "<tuyo> golpea a <preso>" con el sonido de notificacion (`cage.beatLanded`) y suena un golpe y la queja sobre el preso (`submissionBeatHitSoundEnabled`, `submissionBeatHitSoundEnabled`; desde priority-one S1: el evento Wwise `Impact` con el switch `Impacts` = `submissionBeatHitSound` ("HtH_Impacts", el golpe sin arma) y la voz `VO_Get_Hit`, o `VO_Creature_Take_Damage` en un animal; ver 12.40 S1-S4). Si la caminata no termina en golpe, no hay golpe ni descanso, y el roller dice por que. Mockup: `ui-mockup/readout.html` seccion 2, "Menu on your prisoner" (callout de la caminata) y `ui-mockup/README.md` item 17. Con `probeOnClick`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| G1 | Seleccionar a uno de los tuyos a ~20 m de la jaula; clic derecho sobre el preso despierto, Golpear | Tu personaje camina hasta la jaula; al llegar se detiene, mira al preso y el golpe entra (la vida del torso baja como en 12.19 C3) | `[cage] <tuyo> -> <preso> beat walk started (20.0 m)`, luego `beat walk arrived (D m)` con D <= 4 y `beat hit (blunt X, part health A -> B)` |
| G2 | Lo mismo con tu personaje ya junto a la jaula (a menos de 4 m) | Golpe en el acto, sin caminar | `beat walk arrived (D m)` y `beat hit` sin `beat walk started` |
| G3 | En G1 / G2, mirar y escuchar al llegar | El preso dice "¡Ugh!" (o una de las otras tres) sobre su cabeza; el roller dice "<tuyo> golpea a <preso>" con el sonido de notificacion; suena un golpe sin arma y el quejido del preso (**verificar en juego**, 12.40 S1) | `[cage] beat hit sound Impact (Impacts=HtH_Impacts) played id N` y `[cage] beat hit sound VO_Get_Hit played id N` |
| G4 | Golpear a un preso muy herido hasta que **ese** golpe lo deje inconsciente (`submissionBeatingCooldownHours` en 0,5 para repetir; un preso ya caido no ofrece Golpear, 12.19 C2) | En el golpe que lo tumba: sin queja (ya no puede hablar); el aviso del roller y el sonido si | `beat hit` y `[defeat] <preso> cage knockout: not a defeat ...`, sin globo de texto |
| G5 | Golpear a un animal despierto en tu jaula | Golpe, aviso y sonido; **sin** globo de texto | `beat hit` |
| G6 | Golpear y, mientras camina, darle a tu personaje otra orden (moverse a otro lado o quedarse quieto) | En unos 4 s: no hay golpe, Golpear no queda en descanso, roller "<tuyo> dejo de acercarse a la jaula: no hubo golpe"; tu orden nueva se cumple (no se pisa) | `beat walk aborted (stalled)`, sin `beat hit` |
| G7 | Golpear desde un lugar del que no se puede llegar (otra planta sin camino, puerta cerrada) | Igual que G6 (se traba y aborta) | `beat walk aborted (stalled)` |
| G8 | `submissionBeatWalkSeconds` = 5 y Golpear desde ~60 m | A los 5 s de caminata (sin contar la pausa): sin golpe ni descanso, roller "<tuyo> no llego a la jaula a tiempo: no hubo golpe" | `beat walk aborted (timeout)` |
| G9 | Golpear y, mientras camina, liberar al preso o noquearlo con otro personaje | Sin golpe; roller "Ya no puedes hacerle eso" | `beat walk aborted (not allowed)` |
| G10 | Golpear y, mientras camina, que tu personaje caiga (noqueado en una pelea) | Sin golpe; roller "<tuyo> ya no puede golpearlo: no hubo golpe" | `beat walk aborted (actor down)` |
| G11 | Dos personajes tuyos: Golpear con uno y enseguida con el otro sobre el mismo preso, ambos lejos | El primero en llegar golpea; el segundo, al llegar o antes, recibe "Podras volver a golpearlo en 1 h" sin golpe | uno `beat hit`, otro `beat walk aborted (resting)` |
| G12 | Golpear dos veces seguidas con el mismo personaje mientras camina | Sigue una sola caminata (la nueva reemplaza a la vieja, sin aviso); un solo golpe al llegar | `beat walk aborted (replaced by a new Golpear)` y otro `beat walk started` |
| G13 | Golpear y cargar un save antes de que llegue | Nada queda pendiente; sin mensaje; al cargar nadie golpea | `beat walk aborted (at load)` |
| G14 | Golpear y pausar mientras camina; esperar mas de 30 s en pausa y reanudar | La caminata sigue y el golpe entra al llegar (la pausa no cuenta para el tiempo) | `beat walk arrived`, sin `timeout` durante la pausa |
| G15 | Ventana de un preso de tu jaula: GOLPEAR (12.22) lejos de la jaula | La ventana se cierra y pasa lo mismo que G1 / G3 | igual que G1 |
| G16 | `submissionBeatHitSoundEnabled` OFF y repetir G2 | Golpe, queja y aviso; sin sonido de golpe | sin `beat hit sound` |
| G17 | `config.json` del juego con `"submissionBeatHitSound": "NoExiste"` y repetir G2 | El quejido se oye igual; el golpe suena con el switch que el preso ya tenia o con el de fabrica (Light_Hit), o no suena (Wwise ignora un valor de switch desconocido) | `beat hit sound: SetSwitch Impacts=NoExiste failed ...` o `Impact (Impacts=NoExiste) played id N`; `VO_Get_Hit played id N` |

### 12.40 Prioridad 1: sonido y animacion de Golpear, duelo lejano, esperas de Cheat, animales golpeados, frases editables (priority-one)

Las correcciones de la prueba en juego de 12.39 y la lista "Prioridad 1". Con `probeOnClick` para ver las lineas del log.

**S1 Sonido de Golpear.** Los nombres de antes (`Heavy_Hit`, `Light_Hit`, `Play_*`) no son eventos de Wwise en los bancos de Kenshi (por eso no sonaba nada aunque `audioEvent` dijera que si). Ahora se postean eventos reales sobre el preso, por el Wwise del juego (`AK::SoundEngine::SetSwitch` + `PostEvent`, exportados por `kenshi_x64.exe`), y `PostEvent` devuelve si sono de verdad: `Impact` con el switch `Impacts` = `submissionBeatHitSound` (por defecto "HtH_Impacts", el golpe sin arma; tambien valen "Heavy_Hit", "Light_Hit"...; vacio = no se toca el switch) y la voz `VO_Get_Hit` (persona) o `VO_Creature_Take_Damage` (animal). Si el exe no exporta la funcion, cae a `audioValue` + `audioEvent` (sin confirmacion).

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| S1 | Golpear a un preso humano despierto (12.39 G2) | Se oye un punetazo y el quejido del preso | `beat hit sound Impact (Impacts=HtH_Impacts) played id N` y `beat hit sound VO_Get_Hit played id N` |
| S2 | Golpear a un animal enjaulado | Se oye el punetazo y el quejido del animal (no la voz humana) | `Impact (...) played id N` y `VO_Creature_Take_Damage played id N` |
| S3 | Golpear a un preso inconsciente | El punetazo; el quejido puede sonar (es la voz de golpe del juego, no la burbuja de texto, que sigue callada) | las mismas dos lineas |
| S4 | `config.json` desplegado con el viejo `"submissionBeatHitSound": "Heavy_Hit"` | Suena el golpe pesado del juego (es un valor real del switch `Impacts`): no hace falta migrarlo, pero "HtH_Impacts" es el punetazo | `Impact (Impacts=Heavy_Hit) played id N` |
| S5 | Si alguna linea dice `failed: PostEvent returned 0` | Anotarlo con el nombre: el evento no esta cargado en ese momento | `beat hit sound <evento> failed: PostEvent returned 0` |

**A Duelo ganado lejos.** El rival de un duelo terminado se busca en todo el alcance de la oferta (120 m del lider, `kOfferReachMeters`); antes solo a 80 m de tu grupo, y un duelo ganado con el rival entre 80 y 120 m no hacia nada.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| A1 | Ganar un duelo (Cheat o Balanced, `offerOnDuelWin` on) y, antes de que termine, alejar al lider a ~100 m del rival (o que el rival quede a 90-110 m al caer) | Igual que a corta distancia: camina a ofrecerse (o se calma si la tirada fallo) | `[offer] request: <nombre> (duel win ...)`; nunca `[duel] won against ..., who is not near the squad.` ni "tu rival ya no esta cerca" |
| A2 | Lo mismo con el rival a mas de 120 m del lider | Comportamiento viejo (12.37 O18) | `is out of the event's reach` (o no se encuentra si esta a mas de 120 m de todos los tuyos) |

**B Esperas de Cheat en minutos.** El enfriamiento de rechazo y el de duelo se guardan al minuto (antes en horas enteras: en Cheat duraban hasta la proxima hora). Toda espera de menos de una hora se lee en minutos; desde una hora, en horas como antes. Las lineas en minutos son claves nuevas (`cage.beatRestingMinutes`, `cage.knockoutRestingMinutes`, `cage.button.restHours` / `restMinutes`, `submissionEvent.rejectedNoticeMinutes`, `duel.lostMinutes`, `duel.staysMinutes`, `duel.animal.lostMinutes`, `duel.animal.staysMinutes`); un `messages.json` desplegado viejo las recibe con el deploy.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| B1 | Cheat: Golpear a un preso y abrir el menu enseguida | "Golpear (1 min)" (antes "(1 h)"); al minuto de juego vuelve "Golpear" | - |
| B2 | Cheat: clic en Golpear mientras descansa | "Podrás volver a golpearlo en 1 min" | - |
| B3 | Cheat: noquear (que caiga) y abrir el menu / la ventana | "Noquear (1 min)" y NOQUEAR (1 min) en la ventana | - |
| B4 | Cheat: RECHAZAR a un sometido | "<nombre>: lo rechazaste. No querra saber nada de ti en 1 min."; al minuto de juego acepta de nuevo (antes hasta ~1 h) | `reject cooldown until game minute N (0.02 h)` |
| B5 | Cheat: perder un duelo de reto | "... podras retarlo en 1 min"; al minuto se puede retar de nuevo | `no challenge until game minute N` |
| B6 | Balanced con los valores por defecto: Golpear y abrir el menu a los 20 min de juego | "Golpear (40 min)" (antes "(1 h)"); desde una hora, "(2 h)" como siempre. Rechazo y duelo terminan en la misma hora entera que antes | - |
| B7 | Cargar una partida guardada con una version anterior con un rechazo o un duelo perdido en curso | Sigue en enfriamiento hasta la misma hora entera; despues de guardar y cargar con esta version, igual | - |

**E Frases de apertura editables.** La primera linea de una persona sale de `refusal.*` (por que se niega) y `currency.*` (que pide) de `messages.json`; una lista elige una frase fija por personaje. Sin la clave, el texto de siempre. Un `messages.json` desplegado viejo conserva sus listas; las claves nuevas (`currency.unread`, `rescue*`, `ready`, `cats*`) llegan con el deploy.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| E1 | Hablar con varios bandidos libres (Balanced, piden cats) | "Todos tenemos un precio. El mío se paga en cats." (`currency.catsBandit`) | - |
| E2 | Insistir a alguien que ya dijo que no hasta que se harte | Una frase de `refusal.insistence`; el mismo NPC repite siempre la misma, otro puede decir otra | - |
| E3 | Hablar con un preso hambriento miedoso / honorable | Una de `currency.careFearful` / `currency.careKind` | - |
| E4 | Editar `refusal.grudge` en el `messages.json` desplegado ("PRUEBA") y hablar con alguien que te guarda rencor | Dice "PRUEBA" | - |
| E5 | Borrar `currency.ready` del desplegado y hablar con alguien que se une gratis | "Estoy listo para unirme a ti." (el respaldo), y al cerrar el juego el log nombra la clave faltante | `messages.json has no entry for: ... currency.ready` |

**D Golpear a un animal enjaulado ayuda a domarlo (Balanced).** El estres por golpes baja la barra de comida de un animal en tu jaula: `animalCageBeatingWeight` (0,5) x estres / resistencia, resistencia = 1 + nivel x 0,02 (`animalCageBeatingResolveBase`, `animalCageBeatingResolvePerLevel`); nunca mas del 90 % de la barra. La primera comida sigue haciendo falta. `animalCageBeatingCounts`, `animalCageBeatingWeight`, `animalCageBeatingResolveBase`, `animalCageBeatingResolvePerLevel`. Maqueta: `ui-mockup/readout.html`, estado "Animal in your cage, beaten (Balanced)".

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| D1 | Balanced: animal en tu jaula, alimentado una vez, con el estomago por debajo de la marca; Golpear 2-3 veces y mirar la tarjeta | La marca de "necesita" baja con cada golpe (y vuelve a subir a medida que el estres se desvanece, 24 h) | - |
| D2 | Seguir golpeando hasta que el estomago actual pase la marca bajada | Puede unirse ("listo"), sin mas comida | - |
| D3 | Lo mismo con un animal que nunca comio de tu mano | No se une: sigue pidiendo la primera comida | - |
| D4 | `animalCageBeatingCounts` OFF y repetir D1 | La marca no se mueve con los golpes | - |
| D5 | Cheat: animal en la jaula | Igual que antes: se une al minuto; los golpes no cambian nada | - |

**C Animacion de Golpear.** Al llegar a la jaula tu personaje lanza un punetazo: el clip `submissionBeatAnimClip` ("badpunch", la tecnica vanilla `ma badpunch`) pedido cada cuadro (`runSlaveAnim`) durante `submissionBeatAnimSeconds` (0,9 s) y terminado con `endSlaveAnim` en el personaje y su `AnimationClass`. El golpe, la queja, el sonido y el mensaje entran al final del amago (`submissionBeatWindupSeconds`, 0,35 s). `submissionBeatAnimEnabled`, `submissionBeatAnimSeconds`, `submissionBeatWindupSeconds`. Con `probeOnClick`.

| # | Que hacer | Esperado | Linea del log |
|---|---|---|---|
| C1 | Golpear a un preso (12.39 G1 o G2) | Al llegar, tu personaje lanza un punetazo; el golpe, el "¡Ugh!", el sonido y el mensaje llegan a mitad del gesto; despues vuelve a su pose normal (no queda congelado) | una vez por sesion `[cage] beat anim: clip 'badpunch' found playing`; `beat strike started (clip 'badpunch')`, `beat strike clip playing`, `beat strike landed`, `beat strike ended` |
| C2 | Si el log dice `NOT found playing` | El golpe entra en el acto (como antes); anotar si igual se vio el gesto | `beat anim: clip 'badpunch' NOT found playing (the blow lands at once)` |
| C3 | `submissionBeatAnimEnabled` OFF y repetir C1 | Golpe en el acto, sin gesto | `beat strike landed` sin `started (clip ...)` |
| C4 | `config.json` desplegado con `"submissionBeatAnimClip": "NoExiste"` y repetir C1 | Golpe en el acto; nada raro en el personaje | `NOT found playing` (si es el primero de la sesion), `beat strike landed` |
| C5 | Golpear y cargar una partida durante el gesto (o usar `submissionBeatAnimSeconds` 3 para tener tiempo) | Sin golpe, sin cuelgue; tras cargar nadie queda congelado | `beat strike aborted (at load)` |
| C6 | Durante el amago (subir `submissionBeatWindupSeconds` a 2-3 s), que el preso salga de la jaula o noquear a tu personaje | Sin golpe ni descanso; el roller dice por que (`cage.notAllowed` / `cage.beatAbort.actor`); el gesto termina | `beat strike aborted (not allowed)` / `(actor down)` |
| C7 | Golpear dos veces seguidas con el mismo personaje (Cheat: descanso de 1 min) | El segundo clic camina/golpea de nuevo cuando el descanso termina; un golpe en curso reemplazado no pega dos veces | `beat strike aborted (replaced by a new Golpear)` si el segundo llega durante el gesto |
