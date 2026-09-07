# Estado del sistema

Qué está hecho, qué falta, y qué tan seguro es cada afirmación. Todo verificado
contra `src/` y contra el log de partidas reales, no de memoria.

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
| **I4** | ¿Escribir la marca de derrota altera la IA? | 🔬 **Instrumentada** | El mod ya escribe la marca y loguea `[I4 before-mark]` / `[I4 after-mark]` con la intención de la IA. Falta jugar y comparar |
| **I5** | ¿El conteo de intentos sobrevive la recarga? | ✅ **Leído** | **No** — sigue en memoria. El bug de identidad que destapó ya está arreglado (§3) |
| **I6** | Qué controles acepta el hub | ✅ **Leído** | Mucho más de lo que usamos. Ver §5 |

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
| R1.1 | Jaula obligatoria | ✅ **Compila** | Cheat exige jaula del jugador. `cheatNeedsCage` lo apaga |
| R1.2 | Un minuto y avisa | ✅ **Compila** | `cheatHoursCaged` = 0.02 (un minuto de juego) |
| R1.3 | Cero condiciones más | ✅ **Jugado** | Es lo que hace hoy |
| R1.4 | Umbral de amistad configurable, default 20 | ✅ **Compila** | `friendlyRelation`, default **20**. No aplica a Cheat |

### BALANCED — jaula (§2)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R2.1 | Nivel da chance, no permiso | ✅ **Compila** | **Las dos puertas eliminadas.** El nivel pesa en `JoinChance` y nunca decide solo |
| R2.2 | Gap configurable | ✅ **Compila** | 7 controles: castigo/piso por nivel, bono/techo por ventaja, piso/techo global, intimidación |
| R2.3 | Castigo en jaula | ✅ **Compila** | Botón **GOLPEARLO**. Usa `knockoutForceTimer`, no daño real: no puede matarlo. Cooldown configurable |
| R2.4 | Cooldown acumulativo y configurable | ✅ **Compila** | **Acumulativo**: horas × intentos de más, con techo. 4 controles. Sigue sin sobrevivir recarga (I5) |

### BALANCED — sumisión (§3)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R3.1 | Rogar por vida baja | ✅ **Compila** | Dispara en el knockout usando `blood/getMaxBlood()` (I3). Umbral configurable |
| R3.2 | Contar veces inconsciente | ✅ **Compila** | Hook a `MedicalSystem::knockout`. Solo cuenta si `lastGuyWhoDefeatedMe` es tuyo |
| R3.3 | Piso persistente que sobreviva recarga | ✅ **Compila** | Tres capas: marca `LT_SQUAD_LOST_TO_ME_ONCE` (persiste) + conteo de sesión + condición. Pierde precisión, nunca progreso |
| R3.4 | Curar y volver a pegar | ✅ **Compila** | Sale del contador: curarlo lo despierta, y volver a vencerlo suma otra derrota |
| R3.5 | Personalidad decide cuánto aguanta | ✅ **Compila** | `ResolveMultiplier`: cobarde 0.6, valiente 1.6, honorable 1.8 |
| R3.6 | Quién puede comprar su vida | ✅ **Compila** | `CanBuyTheirLife` (`mercy.cpp:88`) |
| R3.7 | Aceptar / Ignorar / Rechazar | ✅ **Compila** | Botón "IGNORAR" condicional (`mercy.cpp:223`) |
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
| R5.5 | Todo configurable | ✅ **Jugado** | 10 valores en config y hub |
| — | Doméstico vs salvaje (pedido tuyo) | ✅ **Jugado** | `Goat Pet` de Nómadas rechazado. Ver §4 |

### Config e interfaz (§6)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R6.1 | Todo lo nuevo configurable | ✅ **Compila** | **~75 controles.** No quedan números hardcodeados en las rutas |
| R6.2 | Preferir el hub | ✅ **Jugado** | 43+ controles |
| R6.3 | Agrupar el panel | 🔬 **Instrumentada** | El hub da 160 bytes contra los 72 que declaramos. El log ahora vuelca los slots; llamar a ciegas crashea |
| R6.4 | Documentar lo que no va al panel | ⚠️ **Mal escrito** | I6 lo desmiente: `register_text_setting` existe, sí puede ir al panel |

---

## 3. Bugs encontrados y arreglados en esta ronda

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

---

## 4. Lo que aportaron los otros mods

Revisar mods ajenos resolvió cosas que los headers no podían.

| Fuente | Qué aportó | Cómo se usó |
|---|---|---|
| **KEP** (`MedicalFix.cpp`) | `hunger = 3.0f` al reparar; `hunger < 0` inválido | Techo y piso de la escala |
| **KEP** (`AppearanceExtension.cpp`) | `clamp(hunger - 1.0, 0, 1)` | **Trampa**: es apariencia visual, no escala. Me costó un bug |
| **CorpseFeeder** (`.ini`, sin fuentes) | `3.00 ~= 300 UI hunger`, `nutrition_per_body=0.40`, `hunger_gain_enabled` | Confirmó el techo, dio la escala de UI, y **probó que el patrón correcto es escribir el hambre uno mismo** |
| **KEP** (`InformationPanel`, `StatsTool`) | Lee y escribe `myPersonality` sin distinguir animal | Primera pista para I1 |
| **Emkejs-Mod-Core** (`.dll`, sin fuentes) | Strings de `register_*` | I6 entera: secciones, select, texto libre |
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
| `register_setting_section` en el hub | **R6.3** — agrupar el panel |
| `register_select_setting` | Desplegable para el preset |
| `register_text_setting` | Desmiente R6.4: IDs de diálogo y exclusiones **sí** pueden ir al panel |
| `register_bool_condition_rule` | Settings que se habilitan según otro |
| `derivedFleshHealthPercent` + `healthAsPercent()` | **R3.1** — porcentaje de vida real |
| `blood` / `getMaxBlood()` | Ídem, y funciona en inconscientes |
| Personalidad animal (I1) | **R5.4** — meterla al cálculo |

⚠️ **Advertencia sobre el hub:** nuestro `mod_hub_api.h` declara 9 funciones (72
bytes) contra las ~17 del hub real (160 bytes, confirmado en el log). El orden
del struct es **hipótesis, no hecho**. Llamar un puntero en el offset equivocado
se lleva el proceso — hay que confirmar el layout antes de usar los nuevos.

---

## 6. Qué queda pendiente, por prioridad

### Cómo validar lo nuevo, en una partida

Con `probeOnClick: true` el log dice lo suficiente para cerrar cada punto sin
adivinar. Buscá estas líneas:

| Qué querés validar | Qué buscar en el log |
|---|---|
| **I4** — ¿el tag altera la IA? | `[I4 before-mark]` y `[I4 after-mark]` en la misma línea de tiempo. Si `intendedAggression`, `distress` o `intendsToAttack` cambian entre las dos, el tag mueve la IA |
| **R2.1/R2.2** — el nivel ya no es puerta | `[chance] level=X relation=Y personality=Z defeats=N(xM) situation=S -> P%` en cada decisión |
| **R3.2/R3.3** — conteo de derrotas | `defeated by the player (N this session, floor+session=M)` |
| **R3.1** — rogar por vida baja | En esa misma línea: `blood=0.18 nearDeath=yes` |
| **R6.3** — slots del hub | `Mod Hub slots (19 of 160 bytes): [0]=fn [1]=fn ...` |
| **Alimentar** | `Fed X. hunger 1.70 -> 2.10 (credited by plugin)` |
| **Regalo del rescate** | `X paid the player N cats for the rescue` |

Para I4 concretamente: pegale a un NPC hasta dejarlo inconsciente, mirá las dos
líneas `[I4 ...]`, y después observalo un rato. Si su facción no cambia de trato
y no huye distinto, el tag es seguro y **R3.3 queda confirmado**.

### El orden de REQUISITOS §8

*Todo lo de esta lista está implementado.* Lo que queda:

1. **Jugarlo.** Nada de lo nuevo pasó de **compila** a **jugado** todavía.
2. **I4** — dos clicks y un rato de observación, con el log ya puesto.
3. **R6.3** — confirmar el layout del hub con el volcado de slots, y recién ahí
   llamar a `register_setting_section`.

### Riesgos conocidos de esta ronda

- **El botón extra es uno solo.** Si el prisionero tiene hambre y además se le
  puede pegar, gana la comida. Es una limitación de la ventana, no un olvido.
- **`knockoutForceTimer` no está probado en juego.** Linkea y es no-virtual,
  pero nadie lo llamó todavía; si no deja inconsciente a nadie, R2.3 no hace nada.
- **El conteo de derrotas depende de `lastGuyWhoDefeatedMe`.** Si el motor no lo
  llena como espero, el contador queda en cero y no se rompe nada — pero R3.2
  tampoco funciona. El log lo dice de inmediato.

---

## 7. Deuda de documentación

| Documento | Problema |
|---|---|
| `COMPARATIVA.md` | §3.1 y §3.2 ya resueltos, sigue diciendo que faltan |
| `REQUISITOS.md` R6.4 | I6 lo desmiente: el hub sí acepta texto libre |
| `REQUISITOS.md` §7 | Cinco de seis investigaciones cerradas |
| `PLAN.md` §8 | No incluye lo de esta ronda |
| `KENSHI-MODDING-NOTES.md` §11 | ✅ Corregido: se agregó `eatItem` no alimenta animales, y que los `.ini` de mods sin fuentes pueden ser documentación |
