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
| **I4** | ¿Escribir la marca de derrota altera la IA? | ❌ **Pendiente** | **La única sin resolver.** Requiere jugar y observar conducta |
| **I5** | ¿El conteo de intentos sobrevive la recarga? | ✅ **Leído** | **No.** Y hay un bug latente: la clave es un `Character*` crudo |
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
| R1.1 | Jaula obligatoria | ❌ **Idea** | `Evaluate` retorna en la primera línea con preset Cheat (`rules.cpp:1270`) |
| R1.2 | Un minuto y avisa | ❌ **Idea** | |
| R1.3 | Cero condiciones más | ✅ **Jugado** | Es lo que hace hoy |
| R1.4 | Umbral de amistad configurable, default 20 | ❌ **Idea** | Sigue hardcodeado en **25** (`rules.cpp:1520`) |

### BALANCED — jaula (§2)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R2.1 | Nivel da chance, no permiso | 🟡 **Parcial** | `JoinChance` existe y corre en aliados y rescate. Pero quedan puertas duras: `>15` no escucha (`1583`), `>=40` se somete (`1667`) |
| R2.2 | Gap configurable | ❌ **Idea** | El **15** sigue fijo |
| R2.3 | Castigo en jaula | ❌ **Idea** | Sin implementar |
| R2.4 | Cooldown acumulativo y configurable | ❌ **Idea** | **3 intentos** y **4 horas** fijos (`1851-1853`). Además I5: no sobrevive recarga |

### BALANCED — sumisión (§3)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R3.1 | Rogar por vida baja | ❌ **Idea** | Solo dispara por perder miembro. **I3 ya dio la señal a usar** |
| R3.2 | Contar veces inconsciente | ❌ **Idea** | |
| R3.3 | Piso persistente que sobreviva recarga | ❌ **Idea** | **Bloqueado por I4** |
| R3.4 | Curar y volver a pegar | ❌ **Idea** | |
| R3.5 | Personalidad decide cuánto aguanta | ❌ **Idea** | |
| R3.6 | Quién puede comprar su vida | ✅ **Compila** | `CanBuyTheirLife` (`mercy.cpp:88`) |
| R3.7 | Aceptar / Ignorar / Rechazar | ✅ **Compila** | Botón "IGNORAR" condicional (`mercy.cpp:223`) |
| R3.8 | Pagar para que lo dejes ir | ❌ **Idea** | P3, no urgente |

### BALANCED — rescate (§4)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R4.1 | La deuda no obliga | ✅ **Compila** | Usa `JoinChance` con `situation=45` (`rules.cpp:1409`) |
| R4.2 | Si no se une, te paga | ⚠️ **Roto** | `owesGift = true` se marca y **nadie lo lee**. El NPC dice que te deja algo y no te da nada |
| R4.3 | La deuda no vence | ✅ **Compila** | `LT_MY_LIFESAVER` persiste en la partida |
| R4.4 | Cuánto curaste importa | ❌ **Idea** | |
| R4.5 | Animales solo piden comida | ✅ **Jugado** | |

### Animales (§5)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R5.1 | Domesticar por barra de hambre | ✅ **Jugado** | `HungerToFullness01` + `TamingFullnessNeeded`. Log: *"le falta 13% de barriga"* |
| R5.2 | Cachorros: poco | ✅ **Compila** | `cubFullness` |
| R5.3 | Adultos: más | ✅ **Compila** | Escala por madurez y nivel |
| R5.4 | Personalidad animal | 🟡 **Desbloqueado** | I1 confirmó que existe. Aún no entra al cálculo |
| R5.5 | Todo configurable | ✅ **Jugado** | 10 valores en config y hub |
| — | Doméstico vs salvaje (pedido tuyo) | ✅ **Jugado** | `Goat Pet` de Nómadas rechazado. Ver §4 |

### Config e interfaz (§6)

| Req | Qué pide | Estado | Detalle |
|---|---|---|---|
| R6.1 | Todo lo nuevo configurable | 🟡 **Parcial** | Lo de animales sí. Los umbrales viejos (25/15/40/3/4h) no |
| R6.2 | Preferir el hub | ✅ **Jugado** | 43+ controles |
| R6.3 | Agrupar el panel | ❌ **Idea** | **I6 confirmó que `register_setting_section` existe** |
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

### Bloquea o rompe

1. **R4.2 — `owesGift` no hace nada.** El NPC promete pago y no entrega. Es la
   ruta de la que el mod está más orgulloso, prometiendo algo que no cumple.
2. **I5 — clave de puntero crudo.** Un NPC que muere deja la dirección colgada;
   una dirección reusada hereda el cooldown de un muerto. Ya existe hoy.

### El orden de REQUISITOS §8

3. **R2.1 completo** — quedan dos puertas duras (`>15`, `>=40`) que deberían ser
   chance.
4. **Umbrales sueltos configurables** — R1.4 (25→20), R2.2 (15), R2.4 (3 y 4h).
   Cambios chicos, efecto inmediato.
5. **Cheat con jaula** — R1.1 a R1.3. Autocontenido.
6. **Sumisión** — R3.1 (I3 ya dio la señal), R3.2, R3.5. R3.3 espera a I4.
7. **R5.4** — meter la personalidad animal al cálculo.
8. **R6.3** — agrupar el panel, ahora que I6 confirmó que se puede.

### Cuando lo demás esté asentado

9. R2.3 (castigo en jaula), R3.4 (curar y repegar), R3.8, R4.4.
10. **I4** — la única investigación abierta. Solo bloquea R3.3.

---

## 7. Deuda de documentación

| Documento | Problema |
|---|---|
| `COMPARATIVA.md` | §3.1 y §3.2 ya resueltos, sigue diciendo que faltan |
| `REQUISITOS.md` R6.4 | I6 lo desmiente: el hub sí acepta texto libre |
| `REQUISITOS.md` §7 | Cinco de seis investigaciones cerradas |
| `PLAN.md` §8 | No incluye lo de esta ronda |
| `KENSHI-MODDING-NOTES.md` §11 | **Falta `eatItem` no alimenta animales** — es una trampa nueva del SDK, del mismo tipo que las que ya están |
