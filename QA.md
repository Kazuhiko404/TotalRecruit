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
| S2 | Abrir el panel de Mod Hub | ~78 controles de TotalRecruit, ninguno rechazado | Ningún `Mod Hub rejected` |
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

---

## 3. Sumisión (R3.1 a R3.5)

### 3.1 Contar derrotas

| # | Qué hacer | Esperado | Línea del log |
|---|---|---|---|
| D1 | Dejar inconsciente a un NPC hostil | Se cuenta una derrota | `defeated by the player (1 this session, floor+session=1)` |
| D2 | Que **otro** NPC noquee a un tercero | **No** cuenta | Ninguna línea `defeated by the player` |
| D3 | Que noqueen a alguien tuyo | **No** cuenta | Ninguna línea |
| D4 | Curarlo, dejarlo inconsciente otra vez | Cuenta 2 (R3.4) | `(2 this session, floor+session=2)` |
| D5 | Golpearlo dos veces en el mismo instante | Cuenta **una** sola | El contador no salta de 1 a 3 |

**D2 y D3 son los filtros.** Si cuentan, el mod está sumando peleas ajenas.

### 3.2 El piso persiste (R3.3)

| # | Qué hacer | Esperado | Falla si |
|---|---|---|---|
| D6 | Vencerlo 3 veces, guardar, salir, entrar, click | `floor+session=1` — perdió precisión | Da `0`: la marca no persistió |
| D7 | Tras D6, mirar el probe | `memoryTags: SQUAD_LOST_TO_ME` | No aparece el tag |

El diseño acepta perder precisión (3 → 1). Lo que **nunca** puede pasar es
volver a 0.

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

**Esta es la que cierra el último punto pendiente.**

1. Elegí un NPC hostil de una facción cuyo trato conozcas.
2. Dejalo inconsciente **por primera vez**.
3. Buscá en el log las dos líneas seguidas:

```
[I4 before-mark] intendedAggression=... aggroTowards=... distress=... intendsToAttack=... tempNeutral=...
[I4 after-mark]  intendedAggression=... aggroTowards=... distress=... intendsToAttack=... tempNeutral=...
```

| Resultado | Qué significa |
|---|---|
| **Los cinco valores iguales** | La marca no toca la IA → **R3.3 confirmado** |
| **Alguno cambia** | La marca mueve la IA → hay que buscar otro tag |

4. Después dejalo despertar y **observalo 10 minutos**: ¿huye distinto? ¿su
   facción te trata distinto? ¿ataca a quien antes ignoraba?

Si nada cambia, I4 queda cerrada y las seis investigaciones también.

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
