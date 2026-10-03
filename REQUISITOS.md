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
hasta que salga bien.

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
   veces; verlo de pie de nuevo rearma el siguiente.
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

**R4.5 — Los animales solo piden comida.** *(P1, ya cumplido)*
Sin cambios.

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
