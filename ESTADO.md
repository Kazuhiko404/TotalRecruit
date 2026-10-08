# Estado del sistema

Qué está hecho, qué falta, y qué tan seguro es cada afirmación. Todo verificado
contra `src/` y contra el log de partidas reales, no de memoria.

> **Histórico, ver [odd/system-status.md](odd/system-status.md)** (foto vigente al
> 2026-10-06: qué hace hoy el mod, qué falta, avance por feature y riesgos).
> Este documento queda como el registro detallado de la ronda `rebalance-and-ui`
> y de las investigaciones; no se actualiza feature por feature. Hoy `main` es la
> única rama, con todas las rondas mergeadas y desplegadas (DLL en `3fb81b2`).
> Se corrigieron abajo las filas que el código ya contradecía (R1.1, R1.2, R2.3,
> R6.1, R6.2, modo fácil, líneas de log de §6, riesgos). §2.1, §3 y §6 son de la
> ronda `rebalance-and-ui` y no listan lo posterior. Nada tiene marca formal de QA.
> El 2026-10-07 se quitó el Mod Hub: la config vive solo en `config.json`, que el
> jugador edita a mano y el mod nunca escribe. Las filas del hub abajo quedan
> marcadas como retiradas.

Escrito tras la ronda `rebalance-and-ui` (T1-T9, rama `feat/rebalance-and-ui`, ya en
`main`). Todo lo nuevo de esa ronda estaba en **Compila + tests**: compila en
Release|x64 y sus reglas puras pasan `tests/rules_tests` (ver §2.1 y QA.md §12).

Este documento reemplaza la lectura cruzada de `REQUISITOS.md` (lo que falta),
`SPEC-ESTADO.md` (spec vs código), `COMPARATIVA.md` (contra otros mods) y
`PLAN.md` §8 (confirmado vs solo compila).

---

## 0. Cómo leer las columnas

Hay una diferencia que este proyecto ya pagó cara tres veces, así que tiene
columna propia:

| Nivel | Qué significa |
|---|---|
| **Jugado** | Se vio funcionar en una partida, con la línea del log que lo prueba |
| **Compila** | El código existe y linkea, pero nadie lo ejecutó en el juego |
| **Compila + tests** | Como *Compila*, y además la regla pura tiene pruebas automáticas (`tests/rules_tests`, sin motor). Prueba la lógica, no el juego |
| **Leído** | Verificado en headers o en la lib, sin ejecutar |
| **Idea** | Escrito en un documento, sin código |

Que algo compile no dice nada sobre el juego. `eatItem` devolvía `true` y no
alimentaba a nadie; `getOverallHealthRating` devuelve 0 en cualquier
inconsciente; `hunger` no es 0..1. Los headers dan el **tipo**, nunca el
**significado**.

---

## 1. Las seis investigaciones (REQUISITOS §7)

| # | Qué medía | Estado | Respuesta |
|---|---|---|---|
| **I1** | ¿Los animales tienen personalidad? | ✅ **Jugado** | **Sí, y varía.** Dos Canhuesos: `PT_HONORABLE` y `PT_TRAITOROUS`. No es default del FCS |
| **I2** | Rango real de `hunger` | ✅ **Jugado** | **0..3, donde 3 es lleno.** Confirmado por tres fuentes independientes |
| **I3** | Qué señal sirve de "vida restante" | ✅ **Leído** | `blood`/`getMaxBlood()` y `derivedFleshHealthPercent`. Nunca `getOverallHealthRating` |
| **I4** | ¿Escribir la marca de derrota altera la IA? | ✅ **Resuelta** | Ya no se escribe ni se lee la marca: el conteo vive en el save del personaje (`total_recruit.player_defeats`, recruit-gating T1; QA.md §9, REQUISITOS §7) |
| **I5** | ¿El conteo de intentos sobrevive la recarga? | ✅ **Leído** | **No** — sigue en memoria. El bug de identidad que destapó se arregló con el id estable (§2.1, T7); falta probarlo con una recarga real (QA 12.6 I4/I5) |
| **I6** | Qué controles acepta el hub | ⛔ **Retirado** | Retirado 2026-10-07: el Mod Hub ya no se usa; la config vive solo en `config.json` |

### Cómo se resolvió I2, que es el caso ejemplar

Tres fuentes que no se conocen entre sí dijeron lo mismo:

1. **Medición propia**: Bestia de Carga alimentada → `hunger=2.99`
2. **KEP** (`MedicalFix.cpp:307`): escribe `hunger = 3.0f` para reparar un estado médico
3. **CorpseFeeder** (`.ini`): `3.00 ~= 300 UI hunger`

En el medio me equivoqué: tomé la línea de KEP `clamp(hunger - 1.0, 0, 1)` como
escala del estómago. Esa línea es de **apariencia visual** — cuánto se le marcan
las costillas al modelo. Con ese techo un perro de `1.69` daba 69% lleno y se
domesticaba sin comer.

**La lección:** que otro plugin use un valor así prueba que sirve *ahí*, no que
signifique eso en general. Hay que mirar qué hace la función que lo contiene.

---

## 2. Requisitos: estado real

### CHEAT (§1)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R1.1 | Jaula obligatoria | ✅ **Compila + tests** | Cheat exige una jaula, y sirve **cualquiera** (`cheatAnyCage`, on por defecto: jaula ajena, grilletes, secuestro; cheat-any-cage, QA 12.35). `cheatNeedsCage` apaga la jaula |
| R1.2 | Un minuto y avisa | ✅ **Compila + tests** | En Cheat toda espera es como mucho un minuto de juego (`cheat_waits`, `kCheatWaitHours` = 1/60 h; `cheatHoursCaged` 0.02 solo cuenta si es menor); `DecideCheat` en `route_cheat.cpp` |
| R1.3 | Cero condiciones más | ✅ **Jugado** | Es lo que hace hoy |
| R1.4 | Umbral de amistad configurable, default 20 | ✅ **Compila** | `friendlyRelation`, default **20**. No aplica a Cheat |

### BALANCED — jaula (§2)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R2.1 | Nivel da chance, no permiso | ✅ **Jugado** | Las dos puertas eliminadas. El log muestra el desglose por término |
| R2.2 | Gap configurable | ✅ **Compila** | 7 controles: castigo/piso por nivel, bono/techo por ventaja, piso/techo global, intimidación |
| R2.3 | Castigo en jaula | ✅ **Compila + tests** | **Golpear**: el personaje camina hasta la jaula (`beat_walk`), tira un puñetazo (clip `badpunch`, `beat_strike`) y el golpe cae directo y acotado sobre el preso, que sigue en la jaula y no muere; queja, aviso y sonido Wwise. Descanso `submissionBeatingCooldownHours` (1 h). **Noquear**: orden nativa de noqueo, descanso 1 h. Los dos suman estrés por golpes. Caminata/mensaje probados informalmente; sonido y puñetazo sin probar (QA 12.39, 12.40) |
| R2.4 | Cooldown acumulativo y configurable | ✅ **Compila** | **Acumulativo**: horas × intentos de más, con techo. 4 controles. Sigue sin sobrevivir recarga (I5) |

### BALANCED — sumisión (§3)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R3.1 | Rogar por vida baja | ✅ **Jugado** | `blood/getMaxBlood()` (I3). Ahora con guarda de muerte: un cadáver no ruega |
| R3.2 | Contar veces inconsciente | ✅ **Jugado** | Hook a `knockout` + respaldo por estado. Confirmado: `Grunt Mason (2 this session)` |
| R3.3 | Piso persistente que sobreviva recarga | ✅ **Compila + tests** | Tres capas. T2: el piso nunca baja de `defeatsToBreak` (antes 1 < 2). Falta confirmar una recarga real (QA 12.1 F6) |
| R3.4 | Curar y volver a pegar | ✅ **Jugado** | `Grunt Mason` llegó a 2 derrotas |
| R3.5 | Personalidad decide cuánto aguanta | ✅ **Compila** | `ResolveMultiplier`: cobarde 0.6, valiente 1.6, honorable 1.8 |
| R3.6 | Quién puede comprar su vida | ✅ **Compila + tests** | `CanBuyTheirLife` (`mercy_rules`, T2) |
| R3.7 | Aceptar / Ignorar / Rechazar | ✅ **Compila + tests** | ACEPTAR en una súplica **se une gratis** (`AcceptMercy`, sin precio ni rechazo por honor, T2). IGNORAR sigue condicional |
| R3.8 | Pagar para que lo dejes ir | ✅ **Compila** | Ya estaba: `PayTribute` al ignorarlo, solo para quien puede comprar su vida |

### BALANCED — rescate (§4)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R4.1 | La deuda no obliga | ✅ **Compila** | Usa `JoinChance` con `situation=45` (`rules.cpp:1409`) |
| R4.2 | Si no se une, te paga | ✅ **Compila** | `PayPartingGift` transfiere lo que lleva. Se cobra **una sola vez**, marcado con `LT_I_SCREWED_THIS_GUY` (persiste en la partida) |
| R4.3 | La deuda no vence | ✅ **Compila** | `LT_MY_LIFESAVER` persiste en la partida |
| R4.4 | Cuánto curaste importa | ✅ **Compila** | `rescueHealWeight` × fracción restaurada. Usa sangre si está inconsciente |
| R4.5 | Animales solo piden comida | ✅ **Jugado** | |

### Animales (§5)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R5.1 | Domesticar por barra de hambre | ✅ **Jugado** | `HungerToFullness01` + `TamingFullnessNeeded`. Log: *"le falta 13% de barriga"* |
| R5.2 | Cachorros: poco | ✅ **Compila** | `cubFullness` |
| R5.3 | Adultos: más | ✅ **Compila** | Escala por madurez y nivel |
| R5.4 | Personalidad animal | ✅ **Compila** | Entra vía `ResolveMultiplier`: un animal desconfiado pide más comida. Configurable |
| R5.5 | Todo configurable | ✅ **Jugado** | 10 valores en `config.json` |
| — | Doméstico vs salvaje (pedido tuyo) | ✅ **Jugado** | `Goat Pet` de Nómadas rechazado. Ver §4 |

### Config e interfaz (§6)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R6.1 | Todo lo nuevo configurable | ✅ **Compila + tests** | Toda clave nueva va a `config.json`, con su clamp en `LoadConfig`. Quedan literales: tramos de reputación 75/25/5/-5 y el 3.0 de barriga llena |
| R6.2 | Preferir el hub | ⛔ **Retirado** | Retirado 2026-10-07: el Mod Hub ya no se usa; la config vive solo en `config.json` |
| R6.3 | Agrupar el panel | ⛔ **Retirado** | Retirado 2026-10-07: el Mod Hub ya no se usa; la config vive solo en `config.json` |
| R6.4 | Documentar lo que no va al panel | ⛔ **Retirado** | Retirado 2026-10-07: el Mod Hub ya no se usa; la config vive solo en `config.json` |

---

### 2.1 Ronda `rebalance-and-ui` (T1-T9)

Todo en **Compila + tests**. Nada de esto se jugó: cada fila apunta a su caso en QA.md §12.

| Cambio | Tarea | Estado | Detalle | QA |
|---|---|---|---|---|
| Arnés de pruebas `tests/rules_tests` | T1 | ✅ **Compila + tests** | Exe de consola sin motor; 19 suites, 36664 checks verdes. Módulos puros compartidos con el DLL | S1 |
| Orden de rutas | T2 | ✅ **Compila + tests** | El rencor imbatible va antes que ceder en el campo; en jaula las derrotas alimentan la chance y los fallbacks siguen vivos | 12.1 F1-F6 |
| ACEPTAR una súplica = unirse gratis | T2 | ✅ **Compila + tests** | `AcceptMercy`; clave nueva `mercy.joinedNotice` | 12.1 F7-F8 |
| Piso de derrotas persistido | T2 | ✅ **Compila + tests** | `PersistedDefeatFloor` nunca baja de `defeatsToBreak` | 12.1 F6 |
| Súplica limpiada en cada cierre | T3 | ✅ **Compila + tests** | `PleaTracker` por `CharKey`, ya no un `Character*` crudo; `Close()` olvida la súplica | 12.2 M1-M4 |
| ~~Modo fácil 12h → 6h, plano~~ | T4 | Retirado | La espera fácil (`easyHoursCaged`) se quitó en v1-presets-close P4; la clave desplegada queda y se ignora | 12.3 |
| Reloj de encierro | T4 | ✅ **Compila + tests** | `HoldClock`: arranca al enjaular, se suelta al salir, reinicia si el reloj de juego retrocede | 12.3 |
| Curva de precio recalibrada | T5 | ✅ **Compila + tests** | `catsPerLevel 90, catsCeiling 28000, steepness 0.09, midpoint 31`; mínimo al final, redondeo a 10, `catsHardCap` 150000, `talentMaxMultiplier` 1.5 | 12.4 P1-P9 |
| Cobro todo-o-nada, reembolso, transferencias | T5 | ✅ **Compila + tests** | `money_rules` + `cats_purse`; regalo y tributo con una sola convención | 12.4 |
| Escritor de config que mezcla | T6 | ⛔ **Retirado** | 2026-10-07: el mod ya no escribe `config.json` (se quitó junto con el Mod Hub) | 12.5 C1 |
| Id estable de personaje en el save | T7 | ✅ **Compila + tests** | `total_recruit.character_id`; `CharKey` = id o handle; reinicio de sesión al empezar una carga. **La parte del motor (hooks) no está probada** | 12.6 I1-I10 |
| `Evaluate` dividido por ruta | T8 | ✅ **Compila + tests** | `route_*.cpp`, `character_facts`, `reason_catalog`; `rules.cpp` 2221 → 189 líneas | 12.7 |
| Motivos por clave, textos en `messages.json` | T8 | ✅ **Compila + tests** | Bloque `reason` nuevo (29 claves); el tono del rechazo sale de la clave | 12.7 Z1-Z4 |
| 19 números sueltos a config | T8 | ✅ **Compila + tests** | Defaults = los literales viejos | 12.7 C1-C4 |
| Anillo de includes `defeats`/`mercy`/`recruit_core` roto | T8 | ✅ **Compila** | `SetNearDeathListener` | 12.7 R2 |
| Golpiza `GOLPEARLO` acreditada al jugador | T8 | ✅ **Compila + tests** | `CreditDefeatToPlayer` | 12.7 A1-A3 |
| Tarjeta de estrés y chance | T9 | ✅ **Compila + tests** | Vista pura probada; la UI MyGUI, la puntería del hover y que el texto quepa **no** están probados | 12.8 U1-U21 |
| `EvaluatePeek` | T9 | ✅ **Compila + tests** | La misma decisión sin efectos (sin reloj, sin id, sin tirada) | 12.8 U17 |

**Nota sobre git.** `origin/main` es un commit squash ajeno (`6257bd6`) con el mismo
árbol que `4a23201`, así que un `git push` normal de esta rama será rechazado.
Entregarla (rebase, force, PR) es decisión del usuario; no se empujó nada.

---

## 3. Bugs encontrados y arreglados en la ronda anterior

| Bug | Cómo se veía | Causa real |
|---|---|---|
| Animal rechazado por honor | *"su honor le impide aceptar sobornos"* a una Bestia de Carga | El bloque de NPC libre **nunca preguntaba si era animal**. `ChooseCurrency` sí ramifica, pero está después del `return` |
| Animal se domesticaba sin comer | Un Canhueso hambriento se unía al instante | **Mío**: puse el techo de `hunger` en 2.0 tomando una línea de apariencia visual de KEP |
| Umbral de hambre invertido | Nada visible (default 0 lo desactiva) | `hunger >= threshold` marcaba famélico al **más saciado** |
| Diez comidas, hambre clavado | *"Fed Bog Dog"* ×10 y seguía pidiendo 13% | **`eatItem` no alimenta animales.** `CharacterAnimal` sobreescribe `foodUpdate` y usa `eatItemInMouth` |
| Probe inalcanzable | 3 sesiones sin conseguir una medición | El probe colgaba del botón "Hablar", que no aparece en inconscientes ni excluidos |
| Lógica duplicada | — | Los animales en jaula tenían su propia copia de la aritmética de comidas |
| **Regalo prometido y no entregado** | El rescatado decía *"te deja algo en pago"* y no daba nada | `owesGift` se marcaba y **nadie lo leía**. Ahora `PayPartingGift` transfiere con `Ownerships::addMoney` |
| **Cooldown heredado de un muerto** | Invisible hasta que pasa | Los mapas usaban `Character*` crudo como clave. Ahora usan `CharKey{index, serial}` del `hand` |
| **Cadáveres rogando por su vida** | Ventana de piedad sobre dos muertos | `blood` negativo (−0.42, −0.95) pasaba el umbral de "vida baja". Faltaba preguntar si ya estaba muerto |
| **Un animal hablando en español** | Bestia de Carga abriendo la ventana de piedad | `mercy.cpp` no filtraba animales en ningún lado |
| **Piedad sobre inconscientes** | El veredicto decía "espera a que despierte" y la ventana salía igual | `Evaluate` y `OfferMercy` son dos caminos y solo uno chequeaba |
| **Precio a alguien tirado en el suelo** | Cotizó 347k a un KO | `proneState=PS_KO` con `isLiterallyUnconcious=no`. El predicado del motor no cubre PS_KO |
| **El plea sin nombre ni variantes** | *"Piedad. Ya no puedo pelear."* siempre igual | Era una constante hardcodeada, fuera de messages.json |
| **El botón X no cerraba** | La cruz del título no hacía nada | Nadie escuchaba `eventWindowButtonPressed` |
| **Texto del popup cortado** | La frase terminaba a media línea | El alto se **estimaba** (48 chars/línea). Ahora MyGUI mide con `getTextSize()` |
| **La sumisión era inalcanzable** | *"esperá que despierte"*, siempre | Todo el código de sumisión vivía dentro de la rama de jaula |
| **Testigos de I4 inútiles** | 5 campos idénticos antes y después | Esos campos leen `0` en **42 de 42** muestras. Un testigo que nunca varía no prueba nada |

---

## 4. Lo que aportaron los otros mods

Revisar mods ajenos resolvió cosas que los headers no podían.

| Fuente | Qué aportó | Cómo se usó |
|---|---|---|
| **KEP** (`MedicalFix.cpp`) | `hunger = 3.0f` al reparar; `hunger < 0` inválido | Techo y piso de la escala |
| **KEP** (`AppearanceExtension.cpp`) | `clamp(hunger - 1.0, 0, 1)` | **Trampa**: es apariencia visual, no escala. Me costó un bug |
| **CorpseFeeder** (`.ini`, sin fuentes) | `3.00 ~= 300 UI hunger`, `nutrition_per_body=0.40`, `hunger_gain_enabled` | Confirmó el techo, dio la escala de UI, y **probó que el patrón correcto es escribir el hambre uno mismo** |
| **KEP** (`InformationPanel`, `StatsTool`) | Lee y escribe `myPersonality` sin distinguir animal | Primera pista para I1 |
| **Emkejs-Mod-Core** (`.dll`, sin fuentes) | Strings de `register_*` | I6 (retirada 2026-10-07 con el Mod Hub) |
| **Guía de los 4 mods** | Precios publicados de *for Cats* | Calibración de la curva de precios |

**Lo más valioso fue un `.ini` de un mod sin fuentes.** CorpseFeeder documenta en
prosa la escala interna del hambre. Vale revisar los config de los mods
instalados, no solo su código.

### Contra los cuatro mods de reclutamiento

`COMPARATIVA.md` está **desactualizado**: sus §3.1 y §3.2 (precios baratos, no
lee stats) ya se resolvieron — hay curva calibrada y precio por talento sobre 39
stats. Lo que sigue vigente:

| | Ellos | Nosotros |
|---|---|---|
| NPC de otro mod | necesita parche | **funciona** |
| Rutas sin jaula | ninguna | **tres** (rescate, rendición, piedad) |
| Rechazo explicado | no | **sí, y nombra la salida** |
| Precios por `stringID` | algunos | **no** |
| Bolsa de comida animal | TameBeasties sí | **no** |

---

## 5. Hallazgos sin usar todavía

Cosas verificadas que están listas para aprovechar:

| Hallazgo | Qué habilita |
|---|---|
| `derivedFleshHealthPercent` + `healthAsPercent()` | **R3.1** — porcentaje de vida real |
| `blood` / `getMaxBlood()` | Ídem, y funciona en inconscientes |
| Personalidad animal (I1) | **R5.4** — meterla al cálculo |

Los hallazgos del Mod Hub (`register_*`, layout del struct) se retiraron el
2026-10-07 junto con el hub.

---

## 6. Qué queda pendiente, por prioridad

### Cómo validar lo nuevo, en una partida

Con `probeOnClick: true` el log dice lo suficiente para cerrar cada punto sin
adivinar. Buscá estas líneas:

| Qué querés validar | Qué buscar en el log |
|---|---|
| **R2.1/R2.2** — el nivel ya no es puerta | `[chance] level=X relation=Y personality=Z defeats=N(xM) situation=S -> P%` en cada decisión |
| **R3.2/R3.3** — conteo de derrotas | `<nombre> defeated by the player (<veredicto>; count N, <key>)` |
| **R3.1** — rogar por vida baja | En esa misma línea: `blood=0.18 nearDeath=yes dead=no` |
| **Alimentar** | `Fed X. hunger 1.70 -> 2.10 (credited by plugin)` |
| **Regalo del rescate** | `X paid the player N cats for the rescue` |

I4 ya no tiene líneas propias: el mod no escribe la marca de derrota (§1), así que
no hay nada que observar.

### Siguiente paso: QA en partida

El orden de prioridad esta en **QA.md §12** (arriba de todo):

1. Humo **S1-S4**: carga, config leído, probe, texto faltante.
2. **12.6 I4/I5**: el id estable sobrevive una recarga y un relanzamiento. Es lo que mas
   riesgo tiene (hooks de `Character` sin verificar contra el exe).
3. **12.8 U1/U2/U6/U13/U17**: la tarjeta (puntería, números iguales al clic, sin efectos).
4. **12.4 P1-P9**: el dinero.
5. El resto de §12 (F, M, E, C, A/Z/N).

Pendientes abiertos fuera de QA: ejercitar el puente de
diálogo FCS. I4 quedó resuelta (§1). Ver TODO.md y odd/system-status.md.

### Lo grande de la ronda anterior: la sumisión sin jaula

El log lo destapó sin ambigüedad: **19 personajes, todos con
`inSomething=IN_NOTHING`, y cero líneas de `[chance]`**. El jugador venció al
mismo hombre cuatro veces y recibió *"inconsciente: espera a que despierte"*
todas las veces.

La causa era de ubicación, no de números: **todo lo de sumisión estaba cableado
dentro de la rama de jaula**, y el chequeo de inconsciente cortaba 150 líneas
antes. La única ruta que la spec describe *sin* jaula era la única que no podía
ejecutarse.

Ahora la sumisión tiene camino propio, **antes** de ese chequeo. Y el cambio
conceptual: estar noqueado **no descalifica** una rendición, **es la prueba** de
que la hubo.

| Sin jaula, herido, mismo nivel | 2 derrotas | 3 | 4 |
|---|---|---|---|
| Cobarde / Normal | 95% | 95% | 95% |
| Valiente | 66% | 73% | 81% |
| Honorable | 58% | 65% | 71% |

### La barra de estrés

Idea del usuario, y arregla un problema de fondo: **un contador no tiene reloj**
— vencer a alguien hace un mes argumenta igual que hace una hora, y nada de lo
que el jugador *deja de hacer* se desgasta.

Los tags del motor son **booleanos**: no se puede guardar "estrés = 47" en un
personaje. Pero no hace falta — el estrés **ya existe repartido** en `hunger`,
`worstDamage` y `restedState`, que ya persisten en la partida y ya se recuperan
solos.

Así que se **deriva**, no se almacena. Le das de comer y baja. Lo curás y baja.
Lo dejás y sube sola. Mantener a alguien con hambre pasa a ser una decisión que
hay que sostener, no una casilla que queda marcada.

### Riesgos conocidos de la ronda anterior

- ~~El botón extra es uno solo~~: superado; el preso de tu jaula tiene su bloque
  Hablar / Golpear / Noquear y la ventana trae GOLPEAR / NOQUEAR.
- ~~`knockoutForceTimer` no está probado~~: obsoleto; Golpear ya no lo usa (golpe
  directo, cage-beat-direct-hit) y Noquear es la orden nativa.
- **El conteo de derrotas depende de `lastGuyWhoDefeatedMe`.** Si el motor no lo
  llena como espero, el contador queda en cero y no se rompe nada — pero R3.2
  tampoco funciona. El log lo dice de inmediato.

---

## 7. Deuda de documentación

| Documento | Problema |
|---|---|
| `COMPARATIVA.md` | §3.1 y §3.2 ya resueltos, sigue diciendo que faltan |
| `SPEC-ESTADO.md`, `COMPARATIVA.md` | Marcados como históricos (banner arriba); la verdad vigente es este documento |
| `REQUISITOS.md` R6.2-R6.4, I6 | Retirados 2026-10-07 con el Mod Hub |
| `REQUISITOS.md` §7 | Cinco de seis investigaciones cerradas |
| `PLAN.md` §8 | No incluye lo de esta ronda |
| `KENSHI-MODDING-NOTES.md` §11 | ✅ Corregido: se agregó `eatItem` no alimenta animales, y que los `.ini` de mods sin fuentes pueden ser documentación |
