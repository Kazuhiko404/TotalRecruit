# Requisitos — lo que falta construir

Lógica y reglas, sin código. Sale de [SPEC-ESTADO.md](SPEC-ESTADO.md) más el
brainstorm: cada requisito aquí viene de una nota o de una respuesta, no de una
idea suelta.

Donde una comprobación contra los headers cambia lo que se puede pedir, está
dicho. Donde algo sigue sin verificar, también.

Leyenda de prioridad: **P0** bloquea a otros · **P1** núcleo · **P2** mejora ·
**P3** cuando lo demás esté asentado.

---

## 0. Reglas que aplican a todo

**R0.1 — Un solo motor de chance.** Relación, diferencia de nivel, personalidad
y un aporte propio de cada ruta se combinan en un único cálculo que consultan
las cuatro rutas. Nada de cuatro fórmulas parecidas. *(P1 decidido: opción a)*

**R0.2 — Piezas chicas y comprobables.** Cada entrada del cálculo es su propia
regla, aislada, para que un número equivocado se pueda atribuir a un solo lugar.
Clean code y SOLID: funciones compactas, reutilizables, fáciles de testear.

**R0.3 — Nada se decide dos veces.** Un mismo NPC consultado dos veces seguidas
da el mismo resultado. Sin dados por click: el jugador no debe poder insistir
hasta que salga bien. La tirada se siembra con el NPC **y el día de juego**
(`floor(horas de juego / 24)`): dentro del mismo día no cambia por más clics o
recargas, y al día siguiente se vuelve a sortear. "Volvé mañana" significa algo,
y un 37% es un 37% a lo largo del tiempo, no un sí o un no para siempre.

**R0.4 — Todo número es configurable.** Sin excepciones nuevas. Preferentemente
en el panel del hub, porque ahí el cambio es inmediato; el archivo de config
exige reiniciar y reimportar, y eso hace que un valor no se toque nunca.

**R0.5 — Todo texto vive en el archivo de mensajes.** Cada regla nueva trae sus
líneas. Si falta una clave, el log la nombra al cerrar — ese aviso es parte del
ciclo de trabajo, no un extra.

**R0.6 — Todo rechazo nombra la salida.** No basta con decir que no; hay que
decir qué lo cambiaría.

---

## 1. CHEAT — quitar el precio, conservar el gesto

**R1.1 — La jaula es obligatoria.** *(P1)*
Hoy Cheat recluta al instante a cualquiera, en cualquier estado. Debe exigir que
el NPC esté en una jaula del jugador. Sin jaula, no hay oferta.

**R1.2 — Un minuto de juego, y avisa.** *(P1)*
Metido en la jaula, pasa un tiempo corto —del orden de un minuto de juego— y el
NPC llama al jugador para decir que quiere hablar y unirse. No son horas: en
Cheat el tiempo es un gesto, no un precio.

**R1.3 — Cero condiciones más.** *(P1)*
Ni cats, ni comida, ni sumisión, ni nivel, ni relación. Cheat es "cualquiera,
incluidos animales y únicos", y eso no cambia.

**R1.4 — Umbral de amistad configurable, por defecto 20.** *(P1)*
La ruta amistosa deja de usar un 25 fijo. Se configura, y arranca en 20.

---

## 2. BALANCED — la jaula

**R2.1 — El nivel da chance, no permiso.** *(P1)*
Hoy el nivel es una puerta: pasado cierto hueco no te escucha, y con ventaja
enorme se somete solo. Debe convertirse en una probabilidad que participe del
motor de R0.1, para todos los NPC y todas las bestias.

*Pendiente de investigación:* qué hueco de nivel produce qué probabilidad, y si
la curva actual (−6 puntos por nivel de diferencia) se siente bien en juego o
castiga demasiado rápido.

**R2.2 — El gap que corta la charla, configurable.** *(P1)*
El límite a partir del cual el NPC se niega con cortesía deja de estar fijo en
15.

**R2.3 — Castigo dentro de la jaula.** *(P2)*
Idea a evaluar, salida de la nota: cada X horas el jugador puede golpear al
prisionero, quitándole una cantidad de vida y acercándolo a la sumisión. Es la
forma de quebrar a un honorable o a un único de nivel alto sin esperar días.

Requisitos si se acepta:
- Intervalo entre golpes y daño por golpe, configurables.
- Debe reflejarse en la sumisión, no ser una acción sin consecuencia.
- Debe tener un techo: no puede convertirse en matarlo por accidente.
- Necesita una razón visible en la interfaz — es una acción del jugador, no un
  efecto oculto.

*Verificado:* el motor expone aplicación de daño, así que es alcanzable. Falta
decidir si se limita a la jaula y cómo se presenta.

*Estado (2026-10-03, cage-animals-submission T1):* se limita a tu jaula y se
presenta como dos botones junto a Hablar, **Golpear** y **Noquear**, que da el
personaje seleccionado. El desmayo forzado de 60 s se retiró.

*Estado (2026-10-04, cage-beat-direct-hit):* **Golpear** es un golpe directo del
personaje seleccionado, en el acto y sin órdenes de combate: el daño es el del
motor para ese atacante (`getTotalAttackDamageFor`), aplicado solo como
contundente (no sangra) al torso del preso. El techo es explícito: el golpe
nunca deja esa parte por debajo de -0,5 x su vida máxima, así que puede noquear
pero nunca matar, y el preso se queda en la jaula. La orden nativa
`BEAT_CAGE_OCCUPANT` se retiró: el actor nunca la tomaba y su respaldo armaba una
pelea que sacaba al preso de la jaula. **Noquear** sigue siendo la orden del
propio juego (`STEALTH_KNOCKOUT`): si sale lo decide el motor. Se confirma en
juego (`QA.md` 12.19).

*Estado (2026-10-04, prisoner-module-fixes F1):* **Noquear actúa una vez.** La
orden deja en el actor una meta persistente (`addGoal`) que nadie quitaba: la IA
la seguía persiguiendo, volvía a entrar en sigilo y noqueaba al preso cada vez
que se levantaba. Ahora la vigilancia sigue después de que el actor toma la
orden, hasta que el preso cae (o muere) o pasan 2 minutos, y en todo final
(preso caído, ventana vencida, respaldo, una orden nueva, una carga) se suelta al
actor: se quita la meta, la orden si sigue en cola, el sigilo solo si la orden lo
encendió, y la IA repiensa. Las partidas ya trabadas se curan solas: cada 3 s se
quita la meta de noqueo de un personaje del jugador si ninguna vigilancia la
tiene y su objetivo ya está caído o es un preso de tu jaula (un noqueo que el
jugador ordena con el menú del juego es una orden, no esta meta, y no se toca).

*Estado (2026-10-04, prisoner-module-fixes F5):* la limpieza ya no borra órdenes
del jugador. KenshiLib no tiene una llamada para quitar una sola orden, así que
la orden de noqueo se suelta solo cuando es la única en cola y apunta a nuestro
preso; una orden en cola con Shift o una más nueva se respetan (la meta quitada
y el repensar sueltan al actor igual). El curado periódico nunca toca órdenes y
solo quita una meta cuyo objetivo es un preso de tu jaula (caído no basta). Si el
actor abandona la orden (otra tarea, sin orden ni meta de noqueo, 3 s seguidos),
la vigilancia termina limpia y sin respaldo. *(F9)* Los tiempos de la vigilancia
no corren con el juego en pausa.

*Estado (2026-10-04, prisoner-module-fixes F6):* **intervalo entre golpizas.**
Tras un golpe que pegó, Golpear descansa sobre ese preso
`submissionBeatingCooldownHours` horas de juego (Mod Hub "Sumision: horas entre
golpizas", 6 por defecto; por preso y solo de sesión, como los episodios de
derrota). El botón sigue en el bloque y en la ventana y muestra la espera
("Golpear (4 h)", horas redondeadas hacia arriba); un clic solo la dice ("Podrás
volver a golpearlo en 4 h"). Cada clic vuelve a leer los hechos: una ventana o un
menú viejo no actúa sobre un preso liberado, caído o fuera de la jaula ("Ya no
puedes hacerle eso"). Noquear no descansa. *(F7)* El golpe nunca sube la vida:
una parte que ya está en el piso o debajo no recibe golpe ni se toca. *(F8)* El
crédito de un noqueo por Golpear dura 5 s alrededor del golpe (y solo si el golpe
pegó); el de Noquear sigue en 2 minutos.

*Estado (2026-10-04, prisoner-module-fixes F4):* con un preso dentro de la jaula
el menú del juego abre dos columnas (el personaje y la jaula: ABRIR CON LLAVE,
DESMANTELAR, PONER EN LIBERTAD). El bloque Hablar / Golpear / Noquear se ubica
fuera de las dos; con una sola columna queda donde siempre.

**R2.4 — Cooldown de insistencia acumulativo y configurable.** *(P1)*
Hoy funciona pero con horas fijas. Debe pasar a:
- Horas base, configurables.
- Cantidad de intentos antes de cortar, configurable.
- **Acumulativo:** hablar cinco veces seguidas da cinco veces el castigo.

*Pendiente de investigación:* hay que confirmar que el conteo de intentos
sobrevive una recarga. Si no sobrevive, aplica el patrón de R3.3.

---

## 3. BALANCED — sumisión

**R3.1 — Rogar por la vida al estar cerca de morir.** *(P1)*
Hoy solo dispara al perder un miembro. Debe disparar también por vida baja, con
el umbral configurable — del orden de 5%, 10% o 30% de vida restante.

*Advertencia verificada:* la medida de "salud general" del motor devuelve cero
para cualquier inconsciente, sin importar cuán intacto esté. No se puede usar
tal cual como porcentaje de vida. Hay que elegir otra señal y **verificarla
jugando** antes de fijar umbrales.

**R3.2 — Contar cuántas veces lo dejaste inconsciente.** *(P1)*
La cantidad de veces debe ser configurable y participar de la chance de
sumisión.

*Verificado:* el motor tiene una llamada de noqueo interceptable, así que contar
es posible.

**R3.3 — El conteo sobrevive la recarga.** *(P1; implementado en recruit-gating T1: el conteo vive en el save del propio personaje)*
Un contador propio vive en memoria y una recarga lo borra. La solución es la de
TotalSparring:

1. **Conteo en el save del personaje:** el mod ya guarda un id propio
   (`total_recruit.character_id`) dentro del estado de guardado de cada
   personaje; al lado guarda `total_recruit.player_defeats` con cuántas veces
   el jugador lo derrotó. Cargar una partida restaura **exactamente** el conteo
   de esa partida (una partida vieja trae su conteo viejo).
2. **Una derrota por noqueo:** un noqueo cuenta una vez aunque el motor lo
   anuncie varias veces o el jugador haga clic derecho sobre el cuerpo muchas
   veces; verlo de pie de nuevo rearma el siguiente. Dentro de los 5 s (tiempo
   real) que siguen a una derrota contada nada cuenta otra vez, aunque el motor
   lo lea de pie: el gancho llega dos veces por noqueo, ~1,2 s aparte, y las dos
   veces lo lee de pie (prisoner-module-fixes F2).
3. **Respaldo por estado:** hambre y heridas actuales, que no dependen de ningún
   conteo.

*Descartado:* la marca `LT_SQUAD_LOST_TO_ME_ONCE` del motor como piso. El motor
la escribe solo, así que un guardia nunca vencido aparecía con `2/2` derrotas y
se rendía en el campo. El mod ya no la lee ni la escribe.

**R3.4 — Curar y volver a pegar.** *(P2)*
Curar a un vencido y derrotarlo de nuevo debe acercarlo a la sumisión más que
dejarlo tirado. Es el bucle que la nota pide, y encaja con R2.3 y R3.2.

**R3.5 — La personalidad decide cuánto aguanta.** *(P1)*
Cuántas derrotas hacen falta y con qué probabilidad cede depende del
temperamento. Un cobarde cede antes que un valiente.

*Rendirse en el campo (recruit-gating T2):* la ruta `field-yield` habla antes
que el mundo libre, así que hace las mismas preguntas de acceso primero: el
prisionero de otra facción y el que está harto de tu insistencia no son tuyos
para hablarles, vencidos o no. La lealtad militar o de la ley no cierra la ruta
(una paliza es el único argumento que escuchan) pero pesa en la tirada:
`submissionFieldYieldLoyaltyPenalty`, 25 por defecto, 0..100.

**R3.6 — Quién puede comprar su vida.** *(P1, decidido; implementado en T2, `mercy_rules`)*

| Temperamento | Al rogar por su vida |
|---|---|
| Miedoso, traicionero, corto de luces | Ofrece pago u objeto, o unirse |
| Cruel, maníaco | Ofrece pago — no tiene honor que perder |
| Honorable, valiente | **Se une o muere peleando.** Sin pago |

Un honorable comprando su salida contradice lo único que lo define.

**R3.7 — Tres respuestas, y la tercera cuesta algo.** *(P1; implementado en T2: ACEPTAR se une gratis, sin precio ni rechazo por honor)*
Aceptar, Ignorar, Rechazar. **Ignorar** —no "Perdonar"— porque con R3.6 el
jugador renuncia a algo concreto: pierde el pago que le estaban ofreciendo. El
NPC pierde el agro y se va. **Rechazar** mantiene el agro y la pelea sigue.

**R3.8 — Pedir dinero y perdonarte.** *(P3)*
Variante de la nota: en lugar de unirse o pagar, un NPC de cierto temperamento
podría pagar **para que lo dejes ir**, y eso mejorar la relación con su facción.
Convierte la sumisión en algo que se puede jugar sin reclutar a nadie.

---

## 4. BALANCED — rescate

Es la brecha más grande, y la ruta de la que el mod está más orgulloso: la única
donde el jugador no ejerce poder. **Hay que diseñarla bien, no solo llenar el
hueco.**

**R4.1 — La deuda existe, pero no obliga.** *(P1, decidido)*
Hoy quien fue salvado se une siempre. Debe pasar a una probabilidad que
considere relación, temperamento, diferencia de nivel y **cuánto se lo curó**.

**R4.2 — Si no puede seguirte, te paga.** *(P1, decidido)*
El que no se une **entrega lo que lleva**: cats, comida, un objeto. Nunca cobra
por haber sido salvado. Esto es lo que mantiene la ficción: el NPC pagando al
jugador, no al revés.

**R4.3 — La deuda no vence.** *(P1)*
La marca de "me salvaste la vida" persiste en la partida. Quien hoy se niega
sigue en deuda: si el jugador sube de nivel o mejora la relación, un encuentro
posterior puede cambiar la respuesta. **La negativa no cierra la puerta.**

**R4.4 — Cuánto curaste importa.** *(P2)*
Vendar a alguien y dejarlo a medias no vale lo mismo que dejarlo entero. La
cantidad de cuidado recibido debe pesar en la chance.

*Estado (2026-10-04, prisoner-module-fixes F3):* la tarjeta de lectura y el menú
dan el mismo veredicto de rescate. El vistazo de la tarjeta tira con el id que el
NPC ya tiene (sin crear uno); antes no tiraba y decía "No puede seguirte: te paga
con lo suyo" mientras el clic decía que se unía.

**R4.5 — Los animales solo piden comida.** *(P1, ya cumplido)*
Sin cambios.

---

## 4b. BALANCED — aliados por nivel

*(2026-10-03, ally-tiers-temp-helpers T1.)* Un aliado (relación ≥
`friendlyRelation`) ya no es una sola tirada diaria: en Balanced deciden su
nivel y su grupo de personalidad. Vanilla+ y Cheat no cambian (Vanilla+ conserva
la tirada; Cheat responde antes de leer nada). `allyTierEnabled` en falso vuelve
a la tirada también en Balanced.

**Grupos de personalidad** (`src/personality_groups.cpp`, una sola función):

| Grupo | Temperamentos |
|---|---|
| Bueno | Honorable, Cálido y bondadoso |
| Neutral | Normal, Corto de luces, Miedoso, Valiente |
| Ambicioso | Traicionero, Astuto |
| Malo (cruel / psicópata) | Frío y cruel, Maníaco |

**R4b.1 — El trato por nivel del NPC.** *(P1, hecho)* Cada nivel de corte
pertenece al tramo que empieza: 20 ya es "20-49", 80 ya es "80-99", 100 ya es
"100+".

| Nivel del NPC | Grupo | Trato |
|---|---|---|
| 1-19 | Bueno | Gratis, con la tirada de aliado de hoy |
| 1-19 | Los demás | Paga cats, descuento hasta 20% |
| 20-49 | Todos | Paga cats, descuento hasta 20% |
| 50-79 | Todos | Paga cats, descuento hasta 10% |
| 80-99 | Ambicioso | Paga cats, descuento hasta 10% |
| 80+ | Bueno / Neutral | Miembro temporal (si el nivel es cercano) |
| 80+ | Malo | Se niega ("su crueldad no se pone a tus órdenes") |
| 100+ | Ambicioso | Se niega ("vale más de lo que podrías pagarle") |

Quien paga **no tira**: pagar es la condición, como cualquier precio.

**R4b.2 — Descuento por amistad.** *(P1, hecho)*
`descuento = min(máximo del tramo, max(0, relación) / 100 × allyTierDiscountScale)`.
Entra al precio por `PriceInputs.factionMultiplier = 1 − descuento`, solo para
aliados de Balanced en el mundo abierto (`character_facts.cpp`); en Balanced ese
multiplicador vale 1.0 para todo lo demás, así que nada se cuenta dos veces. El
regateo, la bolsa y el recargo por ofensa trabajan sobre el precio ya rebajado.

**R4b.3 — Miembro temporal (80+).** *(P1, T1 y T2 hechos; la partida nativa
probada de manera informal, QA 12.25 / 12.30 pendiente)* Bueno o Neutral, de nivel 80 o más, y a lo sumo `allyTierCloseLevelGap`
(15) niveles por encima del líder; si no, "eres demasiado débil para que te
acompañe". Se queda entre `allyTierTempMinDays` y `allyTierTempMaxDays` días
(3..10), sorteados una vez por día con la semilla del NPC.

- **Lo que se guarda.** Al unirse: la hora de vencimiento (ahora + días × 24) y
  las faltas, como enteros en el save del personaje, y la facción de la que vino
  (leída antes de reclutarlo) como texto. Sobreviven a recargar.
- **Maltrato.** Una falta por cada una de estas, cada tipo a lo sumo una vez cada
  `tempMemberStrikeCooldownHours` (12): hambre seguida `tempMemberHungryHours`
  (12 h) bajo `tempMemberHungerThreshold` (25% de estómago, o el motor dice
  hambriento); alguien de tu grupo lo ataca; alguien de tu grupo lo noquea. Con
  `tempMemberMaxStrikes` (3) faltas se va antes de tiempo.
- **Siempre avisa, sobre su cabeza.** Al unirse "Te acompañaré N días"; en cada
  falta "No me trates así" (hambre: "Tengo hambre…"); en la falta anterior a la
  última "Si sigues así me voy"; `tempMemberExpiryWarningHours` (12 h) antes del
  final "Me iré pronto"; una vez por día de juego "Me quedan N días" ("Me queda 1
  día" el último; no el día en que se unió ni el de "Me iré pronto";
  `tempMemberDailyNotice`); al irse "Me voy. Fue un honor" o, maltratado, "Me largo",
  más una línea en el roller con su nombre. La tarjeta del miembro seleccionado
  dice "Temporal: le quedan N días".
- **La partida.** La secuencia de despido de KEP: vuelve a su facción (si no se
  puede, la de KEP por defecto o la vacía), en un pelotón propio creado con la
  plantilla vanilla "Wandering Squad" (`5006-squad.mod`). Con KenshiLib 0.5.0 ese
  pelotón recibe el paquete de IA nativo de la plantilla (el reset de blackboard
  de KEP, `AiPackages::ResetToTemplate`): `2675-gamedata.base` "Wandering from
  town to town", así que camina o viaja de ciudad en ciudad en vez de quedarse
  parado (log `[temp] <nombre> native wander package <paquete> assigned`).
  **Respaldo:** si el paquete no se puede asignar, camina de vuelta al lugar donde
  se unió (guardado en el save como `total_recruit.temp_join_spot`, "x,z";
  `src/leave_homing.*`): reordena cada 4 s, llega a ~6 m o se rinde a los 240 s y
  vuelve a su IA (`[temp] <nombre> heading back to (x,z)`, luego `arrived` o
  `gave up`). Después no quiere saber nada de ti por
  `tempMemberRejoinCooldownDays` (7) días (el mismo enfriamiento del rechazo del
  sometido), así que no se lo puede volver a tomar enseguida.
- **Límites.** Si lo reclutas por un diálogo FCS (vacío por defecto), entra como
  miembro permanente. La caminata de respaldo es de sesión: tras recargar no se
  retoma.

**R4b.6 — Ayudante gratis.** *(P1, T3 hecho; contrato nativo probado de manera informal, QA 12.26 / 12.30 pendiente)*
Pedido del usuario (2026-10-03): "que haya una chance de que NPCs aliados de
nivel más alto y buena personalidad se ofrezcan a ayudarte unos días, pero que no
los puedas controlar; parecido a los mercenarios de Kenshi, pero gratis".

- **Quién.** Solo en Balanced (`helperEnabled`). Un aliado (relación ≥
  `friendlyRelation`), Bueno o Neutral, con nivel mayor al de tu líder +
  `helperMinLevelLead` (10), a 40 m o menos, de pie, sin pelear, sin jaula, no
  tuyo, sin enfriamiento. Una tirada por día y por NPC con
  `helperOfferChancePerDay` (15%).
- **La oferta.** Camina hasta tu líder como el sometido (nunca dos eventos a la
  vez), con el cartel "Quiere ofrecerte ayuda". Al llegar, el popup: "<nombre> te
  ofrece acompañarte N días, gratis. No podrás darle órdenes." ACEPTAR /
  RECHAZAR. N entre `helperDaysMin` y `helperDaysMax` (2..5). RECHAZAR: no se
  vuelve a ofrecer por `helperRejectCooldownDays` (3) días (se guarda con el
  personaje). La X no es respuesta: se va y no pregunta más ese día.
- **Durante.** Sigue en su facción: no aparece en tus retratos y no acepta
  órdenes. Te sigue como un mercenario contratado y pelea contra quien ataca a tu
  líder (ver **Mecanismo**).
  Carteles: "Te cubro las espaldas" al aceptar, "Me iré pronto"
  `helperExpiryWarningHours` (12 h) antes del final. Su tarjeta dice "Ayudante: le
  quedan N días". El vencimiento se guarda: tras cargar, cuando vuelve a estar
  cerca, sigue ("Sigo contigo").
- **El final.** Vencido: "Hasta aquí llego. Suerte" y vuelve a su rutina. Si tu
  grupo lo ataca o lo noquea: "No vuelvo a ayudarte", se va y empieza el
  enfriamiento. Muerto o reclutado (Hablar): termina sin más.
- **Mecanismo** (native-ai-packages T2, KenshiLib 0.5.0). Con
  `helperNativeBodyguard` (encendido por defecto) recibe el contrato nativo de
  guardaespaldas del juego (`5090-gamedata.base`, `SIG_BODYGUARD`) con tu líder
  como empleador, por las horas que le quedan (`src/ai_packages.*`); si estaba en
  un pelotón con otros, antes pasa a uno propio. Si el contrato se pierde (por
  ejemplo al cargar) se le asigna otra vez una sola vez. Al terminar se cierra el
  contrato y su pelotón recupera sus paquetes. **Respaldo B** (contrato apagado o
  fallido): escolta por código, reordena cada 1 s, camina cerca, corre si está a
  más de 12 m, se ubica a ~3 m del líder y ataca al blanco del líder. Detalle en
  `src/helper.h`.
- **Prueba.** `helperDebugKey` (código de tecla de Windows, 119 = F8; 0 =
  apagado): con un NPC seleccionado, se ofrece al instante. Todo se loguea como
  `[helper]`.

**R4b.4 — Siempre se explica.** *(P1, hecho)* El popup nombra el tramo ("Aliado
(nivel 34): te cobra 1.840 cats, con 14% de descuento por amistad", PRECIO con
"-14%", "te acompañará 6 días como miembro temporal") y la lectura sobre el NPC
también ("Aliado: 1.840 cats con descuento"). Las negativas llevan su motivo.

**R4b.5 — Todo configurable.** *(P1, hecho)* Claves `allyTier*` en
`config.json` y en el Mod Hub: los niveles de corte, los máximos de cada tramo,
la escala, la distancia de nivel y los días. El miembro temporal, `tempMember*`:
faltas para irse, umbral y horas de hambre, horas entre faltas, aviso antes de
irse y días sin volver. El ayudante, `helper*`: activo, chance por día, niveles de
ventaja, días mínimos y máximos, días tras rechazo, aviso antes de irse y la
tecla de prueba. El duelo, `duelChallenge*`: activo, horas tras perder y horas
que espera el reto.

**R4b.7 — Retar a duelo a un aliado 80+.** *(P1, hecho; probado de manera informal, QA 12.28 / 12.29 pendiente)*
Pedido del usuario (2026-10-03): "el duelo para el tramo 80+: retarlo a duelo
para que se una. TotalSparring ya tiene la lógica del duelo (decide amistoso o a
muerte); solo hay que llevar la cuenta después del duelo y comunicar los
plugins".
- **Cuándo.** Balanced, aliado con tramo (`Verdict::allyTiered`), nivel
  `allyTierEliteFromLevel` (80) o más, no animal, `duelChallengeEnabled`,
  TotalSparring cargado, escuchando y listo, y sin enfriamiento de duelo
  perdido. Un solo predicado: `CanOfferDuel` (`duel_challenge_rules.h`).
- **La ventana.** La respuesta del tramo queda igual (miembro temporal, precio
  del ambicioso, o la negativa del cruel / líder débil) y se agrega **RETAR A
  DUELO**, más la fila **DUELOS** con el historial cara a cara de TotalSparring
  del miembro seleccionado contra el NPC ("2-1"; empates como tercer número). La
  tarjeta del NPC muestra "Duelos: 2-1" como segunda línea. El historial se lee
  al abrir la ventana o al cambiar el par seleccionado, nunca por cuadro.
- **El clic.** El retador es el miembro seleccionado (la misma regla y el mismo
  "Selecciona a alguien de tu grupo para hacerlo" de Golpear / Noquear). Se anota
  el reto pendiente, se cierra la ventana y se abre la ventana Dueling de
  TotalSparring con ese retador. Ocupada, rechazada o no lista: una línea en el
  roller y el reto se borra.
- **El resultado** (lo avisa TotalSparring al terminar cada pelea). Duelo ganado
  por tu lado contra el NPC retado: en Balanced es una chance (diseño del usuario
  2026-10-04): `min(100, duelJoinBaseChance (25) + duelJoinChancePerWin (20) ×
  (victorias − 1))`, con las victorias cara a cara que cuenta TotalSparring (la
  nueva incluida); la tirada se siembra con el NPC y el número de victorias, así
  que recargar no la cambia. Si sale, se recluta **para siempre** (no temporal),
  cartel "Me ganaste. Me uno a ti" y línea en el roller; si no, cartel "Buena
  pelea. Retame otra vez", línea con la chance y las horas, y un enfriamiento
  sembrado entre `duelWinCooldownMinHours` (24) y
  `duelWinCooldownMaxHoursUnder100` (72) para nivel 80-99 o
  `duelWinCooldownMaxHours` (120) para 100+, guardado en la misma fila. La
  ventana (fila **SI LE GANAS**) y la tarjeta muestran la chance de la próxima
  victoria. En Cheat el duelo se ofrece a cualquier persona de nivel 80+ (no
  animal) y ganar siempre recluta. Perdido o empate:
  cartel "Vuelve cuando seas más fuerte" y `duelChallengeCooldownHours` (24)
  horas sin el botón, guardadas con el personaje
  (`total_recruit.duel_cooldown_until`). Sparring, sin decidir (cancelado,
  interrumpido) o con otro: no cuenta; sin decidir deja el reto pendiente hasta
  `duelChallengeExpiryHours` (6). Si murió en un duelo a muerte, se dice y nadie
  se une.
- **Sin TotalSparring** el botón no aparece y los tramos funcionan como antes.

---

## 5. Animales

**R5.1 — Domesticar por barra de hambre, no por comidas.** *(P1)*
Reemplaza el conteo actual de comidas. El bucle pedido:

1. Se lee cuánta hambre tiene ahora.
2. El jugador le da de comer.
3. ¿Superó el umbral que este animal necesita?
   - **Sí** → se ofrece a unirse.
   - **No** → sigue mostrando que quiere más, y el jugador puede seguir.

**Advertencia verificada, y cambia el diseño:** el hambre del motor **no es un
porcentaje**. Un animal bien alimentado marca alrededor de 3, no 1. Así que
"llenarle el 15% de la barra" **no se puede expresar como está escrito** hasta
saber el máximo real. Antes de fijar umbrales hay que medir en juego qué valores
toma el hambre en un animal famélico y en uno saciado.

El umbral debe ser configurable y depender del animal:

*Nota (2026-10-03, cage-animals-submission T2):* la barra sola no alcanza. Un
animal salvaje está lleno por su cuenta (Cabra y Goat Alpha decían "CAN JOIN"
sin haber comido nunca), así que ahora se une solo si **el jugador le dio de
comer al menos una vez** y además la barriga llegó al umbral. Las comidas que le
dio el jugador se guardan con el personaje (`total_recruit.fed_by_player`, en
comidas). Vale igual en la jaula: la espera fácil, la intimidación, el rescate y
"sano gracias a ti" no doman a un animal.

**R5.2 — Cachorros y débiles: una comida.** *(P1, ya cumplido)*
Cría, o cualquier animal de nivel bajo, se une con muy poco. No tiene otra
opción ni motivo para desconfiar.

**R5.3 — Adultos y fuertes: más comida y más nivel.** *(P1, ya cumplido)*
Escala con madurez y nivel, con descuento si el jugador es claramente más fuerte
—que en términos del juego significa "puede seguir alimentándome".

**R5.4 — Personalidad animal, si existe.** *(P2)*
*Sin verificar.* No sabemos si el motor le asigna temperamento a las bestias. Se
comprueba activando el registro de diagnóstico y haciendo click en un animal. Si
existe, entra en el motor de chance como en cualquier NPC; si no, la lógica
queda con nivel, madurez y hambre, que ya es más de lo que hace cualquier otro
mod.

**R5.5 — Todo configurable.** *(P1)*
Umbral de hambre, peso de la madurez, peso del nivel, descuento por ventaja.

---

## 6. Configuración e interfaz

**R6.1 — Todo lo nuevo, configurable.** *(P1)*
Sin números escondidos en el código.

**R6.2 — Preferir el panel del hub.** *(P1)*
Ahí el cambio es inmediato. El archivo exige reiniciar y reimportar, y un valor
que cuesta eso no se toca nunca.

**R6.3 — El panel necesita orden.** *(P1)*
Ya lleva más de treinta controles en una lista plana, y esto agrega diez o más.
Hace falta agrupar por ruta: general, jaula, amistoso, rescate, sumisión,
animales, precios.

*Verificado:* el hub instalado ofrece más tipos de control de los que usamos
—entre ellos listas de opciones y filas de acción— y una versión más nueva de su
interfaz que la que declaramos. Un desplegable para el preset y agrupadores para
el resto son alcanzables sin inventar nada.

**R6.4 — Los que no se pueden llevar al panel, documentados.** *(P2)*

*Corregido por I6:* el hub **sí** acepta texto libre (`register_text_setting`), así que los
identificadores de diálogo y las listas de exclusión **pueden** ir al panel; este requisito
partía de que no. Lo que queda solo en el archivo es menos de lo que decía. Falta confirmar
el layout del struct antes de llamar esas funciones (ver [ESTADO.md](ESTADO.md) §5).

---

## 7. Investigaciones antes de implementar

Cada una decide el diseño de un requisito. Ninguna lleva mucho, y todas evitan
construir sobre una suposición.

**Cinco de seis están cerradas.** El detalle y la evidencia están en
[ESTADO.md](ESTADO.md) §1.

| # | Qué medir | Bloquea | Respuesta |
|---|---|---|---|
| I1 | Si el motor asigna temperamento a los animales | R5.4 | ✅ **Sí, y varía** entre individuos de la misma especie |
| I2 | Qué valores toma el hambre entre famélico y saciado | R5.1 | ✅ **0..3, donde 3 es lleno.** Tres fuentes independientes |
| I3 | Qué señal sirve de "vida restante" | R3.1 | ✅ `blood`/`getMaxBlood()` o `derivedFleshHealthPercent` |
| I4 | Si escribir la marca de derrota altera la conducta de la IA | R3.3 | ✅ **Resuelta: ya no se escribe ni se lee.** El conteo vive en el save del personaje (recruit-gating T1) |
| I5 | Si el conteo de intentos de charla sobrevive una recarga | R2.4 | ✅ **No**, y la clave es un puntero crudo — bug latente |
| I6 | Qué tipos de control acepta realmente el panel del hub | R6.3 | ✅ Secciones, select, texto libre y más |

---

## 8. Orden sugerido

1. **Las investigaciones I1 a I6.** Baratas, y evitan rehacer.
2. **El motor de chance (R0.1 a R0.3).** Todo lo demás se apoya en él.
3. **Configurables (R6.1, R6.2) y los umbrales sueltos (R1.4, R2.2, R2.4).**
   Cambios chicos, efecto inmediato, se prueban jugando.
4. **Cheat con jaula (R1.1 a R1.3).** Autocontenido.
5. **Rescate (R4.1 a R4.4).** Es el que más criterio necesita.
6. **Sumisión (R3.1 a R3.7).** Depende del motor y de I3, I4.
7. **Animales por hambre (R5.1, R5.5).** Depende de I2.
8. **Orden del panel (R6.3).** Cuando ya estén todos los controles.
9. **Extras (R2.3, R3.4, R3.8).** Cuando el resto se sienta bien.
