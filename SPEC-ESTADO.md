# Especificación y estado — los cuatro modos

Comparación entre la especificación escrita a mano y lo que el código hace hoy.
Todo el estado está verificado contra `src/`, no de memoria; donde hay un número
concreto está citado.

**Cómo leer esto:** el documento es de trabajo. Las secciones `NOTA` están para
que las llenes, y las preguntas del final bloquean la implementación — no la
empiezo hasta que estén respondidas.

Leyenda: ✅ hecho · 🟡 parcial · ❌ falta

---

## 1. CHEAT / EASY

### 1.1 Reclutar en jaula

| Especificación | Estado | Detalle |
|---|---|---|
| Agarrar cualquier NPC y meterlo en una jaula | 🟡 | Funciona, pero no es obligatorio |
| Esperar X horas configurable (por defecto 24h) | ❌ | `Evaluate` retorna en su primera línea cuando el preset es Cheat (`rules.cpp:1068`): se une al instante, sin jaula y sin esperar |
| El sistema avisa cuando se puede reclutar | ✅ | `hail.cpp`, programado sobre el reloj de juego |
| Hablar y se une | ✅ | |
| Aplica a todos, animales y únicos incluidos | ✅ | |

**El hueco de fondo:** la especificación conserva el *ritual* (jaula más tiempo)
y solo quita las condiciones. El código actual no conserva nada: Cheat es un
atajo total. Son dos cosas distintas, y la de la especificación es mejor —
mantiene el gesto y elimina el precio.

> **NOTA (para llenar):**
>
>

### 1.2 Reclutar amistoso

| Especificación | Estado | Detalle |
|---|---|---|
| Misma facción o buena relación → pedirle que se una | 🟡 | Anda |
| Umbral de relación configurable (ej. 50) | ❌ | Hardcodeado en **25** (`rules.cpp:1261`) |
| Animales piden comida y se unen, sin tope, en el mundo abierto | ✅ | |

> **NOTA (para llenar):**
>
>

### 1.3 Reclutar por rescate

| Especificación | Estado | Detalle |
|---|---|---|
| Salvar de una jaula o de morir, curar, hablar → se une | ✅ | `LT_MY_LIFESAVER` persiste en la partida |
| Igual para animales | ✅ | |

> **NOTA (para llenar):**
>
>

---

## 2. NORMAL / BALANCED

### 2.1 Reclutar en jaula

| Especificación | Estado | Detalle |
|---|---|---|
| Vencerlo primero | ✅ | Estar en la jaula ya lo prueba |
| Meterlo en jaula y esperar X horas configurable | ✅ | `easyHoursCaged`, `hoursCagedForProud` |
| Aviso cuando está listo | ✅ | |
| Condiciones según personalidad: cats, comida, o someterlo | ✅ | `ChooseCurrency` (`rules.cpp:653`) |
| **La diferencia de nivel da chance, en todos los NPC y bestias** | 🟡 | El nivel es **puerta binaria**, no chance: +15 → no te escucha; −40 → se somete. La chance por nivel existe solo para aliados |

> **NOTA (para llenar):**
>
>

### 2.2 Reclutar amistoso

| Especificación | Estado | Detalle |
|---|---|---|
| Umbral de relación configurable | ❌ | Hardcodeado en 25 |
| Precio según nivel y relación, con descuentos por relación | ✅ | `factionWeight`, solo en Vanilla+ |
| **Honorables: chance por relación y por diferencia de nivel** | 🟡 | La chance existe solo en la rama de aliados; un honorable en jaula no la usa |
| Si el gap es mayor a X configurable, rechazo cortés | 🟡 | Rechaza, pero el **15** no es configurable |
| Si insistís, deja de hablarte, con cooldown configurable | 🟡 | Funciona (`IsIgnoringPlayer`), pero las horas **no** son configurables |
| Animales: nivel, personalidad y edad, todo configurable | 🟡 | Nivel ✅ y edad ✅ (`CharacterAnimal::age` / `lifespanInDays`). **Personalidad de animales: no la leemos.** Las comidas no son configurables |

> **NOTA (para llenar):**
>
>

### 2.3 Reclutar por rescate

| Especificación | Estado | Detalle |
|---|---|---|
| **Chance según relación, personalidad, gap de nivel y cuánto curaste** | ❌ | Hoy es sí o sí: el tag basta y se une |
| **Puede pedir plata o comida para pagarte** | ❌ | No pide nada |
| Animales solo piden comida | ✅ | |

Esta es la ruta más lejos de la especificación. Hoy el rescate es la puerta
gratis del mod; la especificación la quiere con condiciones.

> **NOTA (para llenar):**
>
>

---

## 3. SUMISIÓN (solo Balanced y superior)

| Especificación | Estado | Detalle |
|---|---|---|
| Chance de que pida unirse al estar cerca de morir | 🟡 | Dispara por **perder un miembro** (`EV_LOST_ARM` / `EV_LOST_LEG`), no por estar cerca de morir |
| O al arrancarle miembros | ✅ | Es justo el disparador |
| O tras vencerlo X veces | ❌ | No contamos golpes ni derrotas |
| Aceptar / Ignorar / Rechazar | ✅ | Tres botones (`mercy.cpp`) |
| Ignorar → pierde agro y se va o se desmaya ahí | 🟡 | Eso hace hoy el botón **Perdonar** |
| Rechazar → mantiene el agro, podés matarlo | ✅ | |
| Curarlo y pegarle de nuevo para que se someta | ❌ | Sin implementar |
| La personalidad afecta las veces y la chance | ❌ | La línea de súplica es deliberadamente igual para todos |

Ojo con el vocabulario: hoy hay tres botones —**ACEPTAR / PERDONAR /
RECHAZAR**— y la especificación los nombra **Aceptar / Ignorar / Rechazar**.
"Perdonar" y "Ignorar" describen la misma consecuencia (se va sin agro) pero
cuentan cosas distintas sobre el jugador.

> **NOTA (para llenar):**
>
>

---

## 4. El patrón detrás de los huecos

Tres cosas atraviesan todas las secciones, y conviene verlas juntas antes de
tocar código:

**A. Números hardcodeados que la especificación quiere configurables**

| Qué | Hoy | Dónde |
|---|---|---|
| Umbral de relación amistosa | 25 | `rules.cpp:1261` |
| Gap de nivel que corta la charla | 15 | `rules.cpp` rama de libres |
| Horas de cooldown por insistencia | fijo | `IsIgnoringPlayer` |
| Comidas para domesticar | 1..5 calculado | `MealsToTame` |
| Bandas de relación (75/25/5/-15/-30) | fijas | `RelationBand` |

**B. La chance por nivel vive en un solo lugar**

`AllyJoinChance` solo corre en la rama de aliados. La especificación la quiere
en jaula, en amistoso, en rescate y en sumisión. Hoy, en el resto, el nivel es
una puerta que se abre o no.

**C. El rescate no evalúa nada**

Es la brecha más grande de la especificación entera, y también la ruta de la que
el mod está más orgulloso. Cambiarla toca la ficción, no solo los números.

---

## 5. Lo que hay hoy y la especificación no menciona

Vale tenerlo a la vista para decidir si se queda, se mueve o se saca:

- **Precio por talento.** Lee 39 stats reales y agrupa en oficio (×1.6), sombra
  (×1.3) y combate (×1.0). Un asesino y un panadero de nivel 30 ya no cuestan
  igual.
- **Curva de precio calibrada.** Cuadrático suave abajo más sigmoide arriba,
  contra los precios publicados de Recruit Prisoners for Cats.
- **Únicos partidos por creencia.** Los venales pagan (×2); los fanáticos solo
  ceden por sumisión.
- **Cortes por relación hostil.** Bajo −15 los honorables no tratan; bajo −30 ni
  los codiciosos aceptan plata.
- **`messages.json`.** Todos los textos editables, con fallback por clave.
- **Marcador de habla vanilla.** `Dialogue::hasDialogue_Fast`.

---

## 6. Preguntas que bloquean la implementación

Sin estas respuestas, cualquier cosa que escriba es adivinar.

### P1 — ¿Un motor de chance compartido, o parche por ruta?

Casi todo lo anterior es lo mismo pedido cuatro veces: **una chance calculada
desde relación, diferencia de nivel y personalidad**, que las cuatro rutas
consulten.

- **(a) Motor compartido.** Una función, cuatro llamadas. Coherente y más fácil
  de balancear después, pero es un refactor del núcleo y hay que probar las
  cuatro rutas juntas.
- **(b) Hueco por hueco.** Cada cambio se prueba solo, riesgo bajo por paso,
  pero termina en cuatro fórmulas parecidas que se van a desincronizar.

**Mi recomendación: (a).** El patrón ya se repite tres veces; una cuarta copia
lo vuelve permanente.

> **RESPUESTA:**
>
>

### P2 — El rescate: ¿cuánto deja de ser gratis?

La especificación dice chance por relación, personalidad, gap y **cuánto
curaste**, y que puede pedir plata o comida "para pagarte".

Ese último punto invierte la ficción actual: hoy el rescate es la única ruta
donde el jugador **no** ejerce poder, y `LT_MY_LIFESAVER` es la deuda.

- **(a) Como dice la especificación**: chance y posible precio.
- **(b) Chance sí, precio no**: puede negarse, pero no te cobra por haberlo
  salvado.
- **(c) Como está**: quien te sacó del barro es tu gente.

> **RESPUESTA:**
>
>

### P3 — ¿Cheat mantiene el ritual?

Hoy Cheat se une al instante. La especificación pide jaula y 24h sin
condiciones.

- **(a) Con ritual** (jaula + horas, cero condiciones).
- **(b) Instantáneo** como hoy.
- **(c) Las dos**: Cheat instantáneo y Easy con ritual — serían **cuatro**
  presets.

> **RESPUESTA:**
>
>

### P4 — ¿Los animales tienen personalidad?

La especificación la pide. **Todavía no lo verifiqué**: `getStateBroadcast()`
existe para todo `Character`, pero no sé si el motor le pone `myPersonality`
distinta de `PT_NONE` a una bestia.

Es una comprobación de cinco minutos con `probeOnClick` sobre un animal. ¿La
hago antes de que decidas, o preferís que la lógica animal use solo nivel y
edad?

> **RESPUESTA:**
>
>

### P5 — ¿"Perdonar" o "Ignorar"?

Misma consecuencia, distinta historia. **Perdonar** dice algo del jugador;
**Ignorar** dice que le da igual. La especificación usa Ignorar.

¿Renombro, o quedan los dos como acciones separadas?

> **RESPUESTA:**
>
>

### P6 — Sumisión por golpes: ¿qué contamos?

La especificación pide "vencerlo X veces" y "curarlo y volver a pegarle".

El motor tiene `LT_DEFEATED_MY_SQUAD_ONCE` y `lastGuyWhoDefeatedMe`, pero **no
un contador de derrotas**. Habría que llevarlo en el plugin, y eso vive solo en
memoria: **una recarga lo reinicia**.

- **(a) Contador propio**, aceptando que la recarga lo borra.
- **(b) Por estado**, no por conteo: hambre y heridas, como ya hace la rama de
  sumisión — sin contador y a prueba de recargas.

> **RESPUESTA:**
>
>

### P7 — ¿Todos los números al hub?

Son unos 8-10 valores nuevos. El panel ya lleva 36.

- **(a) Todos al hub.**
- **(b) Los importantes al hub, el resto solo en `config.json`.**

> **RESPUESTA:**
>
>
