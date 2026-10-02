> **HISTÓRICO — superado por [ESTADO.md](ESTADO.md).** Este documento describe el estado de
> septiembre y no se mantiene. Lo vigente (qué existe, qué se jugó y qué solo compila) está en
> `ESTADO.md`; las pruebas pendientes en `QA.md` §12.

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
> Agarrar cualquier NPC y meterlo en una jaula  > que funcione y que si sea obligatorio
>  Esperar X horas configurable > esta bien, dejarlo asi que no sea necesario esperar nada, que al meter al npc a la jaula despues de 1 min del game te salga la notificacion de que quiere hablar y unirse

### 1.2 Reclutar amistoso

| Especificación | Estado | Detalle |
|---|---|---|
| Misma facción o buena relación → pedirle que se una | 🟡 | Anda |
| Umbral de relación configurable (ej. 50) | ❌ | Hardcodeado en **25** (`rules.cpp:1261`) |
| Animales piden comida y se unen, sin tope, en el mundo abierto | ✅ | |

> **NOTA (para llenar):**
> Umbral de relación configurable (ej. 50) aca que se configure, el defecto que sea 20
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
> **La diferencia de nivel da chance, en todos los NPC y bestias** investigar esto
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
> Umbral de relación configurable > configurar
> **Honorables: chance por relación y por diferencia de nivel** > si esto esta bien, al honorable y otros unicos y nivel alto hay que hacerlos morir de hambre o pegarles, etc, aca podemos poner una idea de 
que cada x horas te permita pegarle dentro de la jaula y le quite x cantidad de vida para que ayude a sumision, es buena idea para que la analizemos
> Si el gap es mayor a X configurable, rechazo cortés  > ok tenemos que configurar
>las horas **no** son configurables > configurar > tambien cuantas veces hasta que te deje de hablar > formula horas * cant, si le hablas 5 veces seguidas * 1h > 5h cooldown si es que se puede, hay que validad si el sistema recuerda esto
> Animales: nivel, personalidad y edad, todo configurable > ok , hay que confiigurar tambien el minimo de barra de hambre que tenemos que llenar para poder reclutar, por ejemplo si le damos de comer a un cachorro con llenarle no se pongamos 15% ya te pide unir, quiza animales mas fuerte sea ams comida, algo asi, podemos hacer un loop > barra actual > le das comida > aumento arriba el umbral ? si -> se une, no -> volver amostrar para darle mas comida.

### 2.3 Reclutar por rescate

| Especificación | Estado | Detalle |
|---|---|---|
| **Chance según relación, personalidad, gap de nivel y cuánto curaste** | ❌ | Hoy es sí o sí: el tag basta y se une |
| **Puede pedir plata o comida para pagarte** | ❌ | No pide nada |
| Animales solo piden comida | ✅ | |

Esta es la ruta más lejos de la especificación. Hoy el rescate es la puerta
gratis del mod; la especificación la quiere con condiciones.

> **NOTA (para llenar):**
> tenemos que entonces diseñar esto y que quede bien
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

**DECIDIDO -- quien puede comprar su vida y quien no:**

| Personalidad | Al rogar por su vida |
|---|---|
| `PT_FEARFUL`, `PT_TRAITOROUS`, `PT_DUMB` | Ofrece pago u objeto, o unirse |
| `PT_COLD_CRUEL`, `PT_MANIACAL` | Ofrece pago -- no tiene honor que perder |
| `PT_HONORABLE`, `PT_BRAVE` | **Se une o muere peleando.** Sin pago |

Un honorable comprando su salida contradice lo unico que lo define, y
abarataria las dos rutas a la vez.

> **NOTA (para llenar):**
>Chance de que pida unirse al estar cerca de morir  > aca podemos revisar cuanta vida le queda, por ejemplo configurable 5%-10%-30%, etc
> O tras vencerlo X veces > podrias averiguar si esto se puede contar cuantas veces lo dejas inconcidente por ejemplo. y que sea configurable
> hay que diseñar  Curarlo y pegarle de nuevo para que se someta y La personalidad afecta las veces y la chance 
> tambien puede estar la opcion de la personalidad que te pida dinero y te perdone, por ejemplo.

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
> A esta bien, pero siempre seguir clean code y SOLID, funciones flexibles, compactas, reusables, etc, facil de testear y analizar por bugs
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
> hagamos un brainstorm a partir de las respuestas que puse
>
> **DECIDIDO: (c) -- el rescatado ofrece, nunca cobra.**
>
> Releyendo la especificacion, "puede pedir plata o comida para pagarte" no es
> un precio: es el NPC pagandote a vos. Eso cambia el signo de toda la ruta.
>
> - Se une si puede.
> - Si no puede, te da lo que tiene: comida, cats, un objeto.
> - Nunca te cobra por haberlo salvado.
>
> Asi el rescate sigue siendo la unica ruta donde el jugador no ejerce poder, y
> deja de ser gratis sin volverse una transaccion. De fondo queda la opcion (b):
> LT_MY_LIFESAVER persiste, asi que quien no se una hoy sigue en deuda.
>
> Lo mismo para la sumision (seccion 3): un vencido que ruega puede ofrecer un
> pago u objeto en lugar de unirse -- y eso le da peso a Ignorar, porque dejas
> de ganar algo concreto. Pero solo los cobardes: un honorable se une o muere.

### P3 — ¿Cheat mantiene el ritual?

Hoy Cheat se une al instante. La especificación pide jaula y 24h sin
condiciones.

- **(a) Con ritual** (jaula + horas, cero condiciones).
- **(b) Instantáneo** como hoy.
- **(c) Las dos**: Cheat instantáneo y Easy con ritual — serían **cuatro**
  presets.

> **RESPUESTA:**
> b
>

### P4 — ¿Los animales tienen personalidad?

La especificación la pide. **Todavía no lo verifiqué**: `getStateBroadcast()`
existe para todo `Character`, pero no sé si el motor le pone `myPersonality`
distinta de `PT_NONE` a una bestia.

Es una comprobación de cinco minutos con `probeOnClick` sobre un animal. ¿La
hago antes de que decidas, o preferís que la lógica animal use solo nivel y
edad?

> **RESPUESTA:**
> revisemos primero pero la logica base ya se puede agregar
>

### P5 — ¿"Perdonar" o "Ignorar"?

Misma consecuencia, distinta historia. **Perdonar** dice algo del jugador;
**Ignorar** dice que le da igual. La especificación usa Ignorar.

¿Renombro, o quedan los dos como acciones separadas?

> **RESPUESTA:**
> Ignorar me parece mejor
>
> **DECIDIDO: "Ignorar".** Con el pago de la P2 el nombre se gana su lugar:
> ignorar deja de ser solo dejarlo ir, es renunciar a lo que ofrecia.

### P6 — Sumisión por golpes: ¿qué contamos?

La especificación pide "vencerlo X veces" y "curarlo y volver a pegarle".

El motor tiene `LT_DEFEATED_MY_SQUAD_ONCE` y `lastGuyWhoDefeatedMe`, pero **no
un contador de derrotas**. Habría que llevarlo en el plugin, y eso vive solo en
memoria: **una recarga lo reinicia**.

- **(a) Contador propio**, aceptando que la recarga lo borra.
- **(b) Por estado**, no por conteo: hambre y heridas, como ya hace la rama de
  sumisión — sin contador y a prueba de recargas.

> **RESPUESTA:**
> veamos una solucion inteligente con un fallback o algo asi
>

### P7 — ¿Todos los números al hub?

Son unos 8-10 valores nuevos. El panel ya lleva 36.

- **(a) Todos al hub.**
- **(b) Los importantes al hub, el resto solo en `config.json`.**

> **RESPUESTA:**
>  tenemos que ver como optimizar todo esto, esta bien que sea flexible pero veamos como ordenar, ideal agregar todo configurable ya sea al panel hub o config, el tema esque enel panel hub los cambios son automaticos
el config tienes que relogear y importar, no es lo ideal.
>
