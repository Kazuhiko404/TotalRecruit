# Ajuste fino de Balanced: calibrar la jaula con la hoja de balance

Rama: `feat/balanced-tuning` (desde `main` @ `7bf8750`), mergeada en `main` (`b87d0b2`) y borrada el 2026-10-06; el trabajo que queda se hace en `main`. Generador de la hoja: `af1ff35`.
Ruta: por tarea (inline o escritor delegado), declarada en cada item del checklist al empezarlo.
Engram: proyecto `totalrecruit`, espejo `odd/balanced-tuning/tasks`.

## Objetivo
Que el preset Balanced se sienta lore-friendly y parejo: cuanto tarda en quebrarse un preso depende de quien es (personalidad, nivel, unico), y ninguna herramienta de la jaula (golpear, cuidar, esperar) domina a las demas. Cada cambio de numeros se mide contra `odd/balance/balance-sheet.md` antes y despues.

## Contexto: estado actual (2026-10-06)
`main` es la unica rama y esta desplegada (DLL commiteado en `3fb81b2`). Estado completo en `odd/system-status.md`. Desde que se escribio este documento:
- **T5 hecho** por priority-one B (`9b0466a`): rechazo y duelo se guardan al minuto de juego; las esperas de menos de 1 h se leen en minutos ("Golpear (1 min)").
- **T7 casi hecho**: duelo con animales libres 40+ en los dos presets (`fe90e80`), field-yield 2/2 para animales libres en Balanced (`6d956a1`), cura/derrota en Cheat (`588491c`) y los golpes bajan la barra de comida de un animal enjaulado en Balanced (priority-one D, `65c6652`). Falta: resolucion por especie (hoy por nivel).
- **Hoja regenerada** sobre las reglas de `3fb81b2` (`b8ca98c`): las secciones 1-4 no cambiaron respecto de `7bf8750` (la evidencia de T1-T4 y T6 sigue valiendo tal cual); la seccion 5 ahora muestra las esperas de Cheat al minuto y la nueva seccion 6, la barra del animal enjaulado golpeado.
- El usuario juega Cheat: T1-T4 y T6 solo tocan Balanced.

Estado de la v1 cuando se escribio (`main` @ `7bf8750`, DLL md5 `79a7eba70868acbc1830096b52f9f053`):
- **Cheat** = jaula + 1 minuto de juego y te llama; todas sus esperas duran 1 minuto (`cheat_waits`, `kCheatWaitHours`).
- **Balanced** = motor completo (`JoinChance`, tirada diaria `DailyRoll`).
- **Vanilla+ eliminado**: un `preset` fuera de rango (un 2 guardado) carga Balanced.
- **Atajos de jaula eliminados**: fuera la espera facil (`easyHoursCaged`) y "sano gracias a ti" (`Route_EasyHealthy`).
- **Bonus por cuidarlo** 5..20 %, sorteado por NPC y por dia (`DailySeed` + `kCaredForBonusSalt`; `submissionCaredForBonusMin/Max`).
- **Estres por golpes**: Golpear 8 %, Noquear 15 %, x factor de nivel / resolucion, acotado 2..25 %, se desvanece en 24 h de juego, guardado con el personaje.
- **Los noqueos de la jaula son estres, no derrota.**
- **Descansos** Golpear y Noquear: 1 h cada uno (`submissionBeatingCooldownHours`, `submissionKnockoutCooldownHours`).
- **Horas en jaula persistidas** (`total_recruit.hold_start_hour`).

Referencias: `odd/system-status.md`, `REQUISITOS.md` §2 (jaula) y §3 (sumision), `QA.md` (§4, §8, 12.19, 12.32, 12.33), `odd/balance/balance-sheet.md`, `odd/tasks/v1-presets-close.md`, `odd/tasks/cage-beating-stress.md`.

## Problema
La hoja de balance (generada desde las reglas reales de `src/` en `7bf8750`) muestra que:
- quebrar a un preso (estres pleno, hasta +45) vale mucho mas que cuidarlo (+5..20);
- Golpear + Noquear saturan la barra de estres en horas, sin costo real mas alla de perder el bonus de cuidado;
- desde que se quito la espera facil, el tiempo en la jaula no suma nada por si mismo;
- los dias hasta unirse no siguen una tabla objetivo explicita por temperamento;
- quedan restos de Cheat (enfriamientos en horas enteras, etiquetas "(1 h)" para esperas de 1 minuto).

## Por que
La jaula de Balanced cobra tiempo de juego y decisiones (R2.x). Si una sola herramienta resuelve todo en una tarde, el resto del sistema sobra y no se siente Kenshi.

## Principios (del usuario)
- Lore-friendly y balanceado.
- Aleatoriedad con margenes min/max, pero estable: sembrada por dia (`DailySeed`), nunca re-tirable con clics.
- Todo numero configurable (clave en `config.json`).
- Modulos aislados (SOLID), sin parche sobre parche.
- Maqueta en `ui-mockup/` antes de cambiar cualquier UI; las maquetas quedan al dia.
- Nunca pisar el `config.json` desplegado: `scripts/deploy.sh` solo agrega claves.
- Test-first con `tests/rules_tests` (RED antes de implementar).
- Regenerar `odd/balance/balance-sheet.md` antes y despues de cada cambio de numeros (comando en `DEVELOPING.md`).
- Conventional Commits; docs en espanol, codigo en ingles.

## Alcance
T1..T7 abajo. Solo reglas puras de Balanced (`src/*_rules.cpp`), sus claves de config, mensajes y docs. Cheat no cambia salvo T5.

## Restricciones
- Toolset v100: sin C++11.
- Lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos solo POD (C2712).
- No se toca el `config.json` desplegado; las plantillas solo reciben claves NUEVAS. Sin deploy ni push sin pedido del usuario.
- No se quita comportamiento ya validado por el usuario por paridad o prolijidad.
- Toda regla nueva entra al `.vcxproj` de `tests/balance_sheet` y la hoja gana la fila o tabla que la muestra.

## Checklist

### T1 Calibracion: tabla objetivo
Ruta: inline (dos defaults + pruebas de caracterizacion; sin regla nueva).
- [x] Tabla objetivo acordada con el usuario el 2026-10-07: "balanced, ni dificil ni super facil". Dias de juego hasta 90 % acumulado, nivel igual al lider:

  | Temperamento | Sano, sin trabajar | Cuidado | Quebrado |
  |---|---|---|---|
  | cobarde | 2 | 1-2 | 1 |
  | normal | 4 | 3 | 2 |
  | valiente | 7 | 5 | 3 |
  | honorable | 9 | 6 | 4 |
  | unico honorable | - | - | ~10 |

- [x] Hallazgo que cambio la palanca: con base 45 + intimidacion 15, un normal sano ya estaba en 60 (3 dias) sin tocarlo, asi que bajar solo resolucion/estres no alcanzaba sin anular el estres. La palanca es la base, no el estres.
- [x] Cambio (`8b9081d`): `chanceLevelBase` 45 -> 35 y `submissionMaxStressChance` 45 -> 30 (defaults en `src/config.h` y en la plantilla `TotalRecruit/config.json`). Aguante y castigo de unico sin cambios. El `config.json` desplegado del usuario NO se toca (sigue en 45/45; juega Cheat).
- [x] Pruebas: RED 13 checks en `chance_rules`; GREEN tras el cambio; caracterizaciones de `route_decisions` actualizadas a la base 35 (field-yield 79 -> 69, aliado 65 -> 55, jaula 60 -> 50). `rules_tests` 63/63, 0/73382 fallos. DLL compila.
- [x] Hoja regenerada sobre `8b9081d` (seccion 2, nivel +0, sano / cuidado medio / quebrado): fearful 2/2/1, normal 4/3/2, brave 6/4/4, honourable 7/5/4, unico honourable 45/13/10. Todo dentro de +-1 dia salvo honorable sano (7 contra 9, mas facil). Valiente quebrado 4 contra 3: dentro de +-1.
- Efecto lateral (aceptado, apunta a T6): toda ruta de Balanced comparte la base. Field-yield normal con 2 derrotas 79 -> 69; aliado tiers-off 65 -> 55. `QA.md` 2.1 y `REQUISITOS.md` R2.1 actualizados.
- Nota para T3: con estres pleno 30, cuidar ya casi iguala a quebrar en valiente/honorable (ventaja +6/+4); el bono maximo +20 vale 66 % del estres pleno de un normal.

### T2 Golpear domina
Ruta: escritor delegado (modulo puro nuevo + pegamento del motor + filas del save + config + generador de la hoja + docs: varios archivos no triviales).
- [x] Evidencia (seccion 3, lider 40, un golpe cada vez que termina el descanso de 1 h):
  - normal nivel 40: +0.080 por golpe, barra a 1.0 en 25 sesiones solo Golpear, en **6** con Golpear+Noquear; meseta 1.00 (+45).
  - fearful nivel 40: 11 sesiones solo Golpear, 3 con Golpear+Noquear.
  - brave nivel 40: meseta solo Golpear 0.64 (+18), pero 10 sesiones a 1.0 con Golpear+Noquear; honourable nivel 40: meseta 0.24 (+6), 12 sesiones con Golpear+Noquear.
  - Umbral de saturacion: ganancia > 0.0417 por hora (lo que se desvanece en 1 h).
  - El termino de golpes entra al estres con peso 1.00 (hambre 0.50, heridas 0.35, cansancio 0.15): una barra de golpes llena satura sola el estres.
  - Columna "golpes" (seccion 2, nivel +0): normal 95 (1), brave 73 (2), honourable 65 (3): igual que "quebrado".
  - Unico costo que muestra la hoja: Golpear hiere, asi que se pierde el bonus de cuidado. Sin efecto en relacion ni opinion.
- [x] Propuesta (modulo puro nuevo o extension de `beating_stress_rules`, test-first). Hecho lo de abajo marcado; el resto descartado por el padre:
  - **Rendimiento decreciente** por golpe del mismo dia de juego: la ganancia n-esima del dia se multiplica por `beatingDiminishFactor` (p.ej. 0.7, rango 0.3..1.0).
  - **Tope diario** de estres por golpes: `beatingDailyStressCap` (p.ej. 0.35, rango 0.1..1.0).
  - ~~**Penalizacion de relacion/opinion** al golpear~~ (descartado por el padre: la jaula no usa relacion; el tope diario basta): `beatingRelationPenaltyMin/Max` (p.ej. 2..6 por golpe), x1.5 para honorable y valiente (`beatingRelationPenaltyPrideMult`). Nota: la jaula hoy no usa la relacion en `JoinChance`; decidir si la penalizacion pesa en la tirada de la jaula o solo despues (miembro reclutado, rencor).
  - ~~Considerar bajar el peso del termino de golpes (hoy 1.00) a la par de hambre (0.50): `stressWeightBeaten`~~ (descartado por el padre: la jaula no usa relacion; el tope diario basta; el peso sigue en 1.00).
- [x] Hecho (`aedc3b4`, solo Balanced; Cheat sin cambios): modulo puro `src/beating_daily_rules.cpp/.h`.
  - **Orden por golpe** (`DailyBeatingGain`, comentado en el encabezado): 1) rendimiento `submissionBeatingDiminishFactor`^(n-1) (0.9 tras el reajuste, rango 0.3..1.0) sobre la ganancia cruda, **antes** del acotado 0.02..0.25 (`BeatingStressGainScaled`, extension limpia de `beating_stress_rules`; `BeatingStressGain` = escala 1): todo golpe sigue sumando al menos el minimo y el maximo sigue acotando un noqueo fresco; 2) tope diario `submissionBeatingDailyStressCap` (0.6 tras el reajuste, rango 0.1..1.0) **despues** del acotado: lo unico que baja un golpe del minimo, hasta 0. Golpear y Noquear cuentan juntos para n; un golpe que el tope dejo en 0 igual cuenta.
  - Dia = `DayIndexFromHours` (el de `DailySeed`). Registro por preso (dia, golpes, ganado) en `BeatingDayLedger`, detras del mismo lock que la barra (`beating_stress.cpp`); se guarda con el personaje en tres filas nuevas de `saved_ints` (`total_recruit.beating_day` = dia + 1, `beating_day_blows`, `beating_day_gained` en milesimas). Save viejo sin filas = dia nuevo; un registro de otro dia (o de un reloj que volvio atras) = dia nuevo. `load_boundary` lo vacia con la barra (`ForgetAllBeatingStress`).
  - Log con `probeOnClick` en Balanced: `[stress] <preso> beating stress +X% (...) -> Y% [day D blow N, xM, today T% of cap C%]`.
  - Claves nuevas solo agregadas a la plantilla `TotalRecruit/config.json`, clamp en `LoadConfig` con las constantes del modulo. Sin control de hub (el Mod Hub ya no existe).
- [x] Pruebas: RED 38 checks en `beating_daily_rules` (62 suites, 61 pasan, 38/73355 fallos) con los cuerpos vacios; GREEN tras implementar: 62/62 suites, 0/73355 fallos. DLL compila.
- [x] Hoja regenerada sobre `aedc3b4` (seccion 3, lider 40, nivel 40; columnas nuevas "meseta" y "maximo" de Golpear+Noquear y "sin limites diarios" para comparar):

  | Temperamento | Antes: sesiones a 1.0 solo Golpear / G+N, meseta solo Golpear | Despues: sesiones a 1.0 solo Golpear / G+N | Despues: maximo G+N (chance) | Despues: meseta G+N a las 72 h |
  |---|---|---|---|---|
  | fearful | 11 / 3, 1.00 (+30) | nunca / nunca | 0.31 (+15) | 0.00 |
  | normal | 25 / 6, 1.00 (+30) | nunca / nunca | 0.24 (+7) | 0.00 |
  | brave | nunca / 10, 0.64 (+12) | nunca / nunca | 0.13 (+2) | 0.00 |
  | honourable | nunca / 12, 0.24 (+4) | nunca / nunca | 0.11 (+1) | 0.00 |

  Criterio de aceptacion cumplido: Golpear+Noquear no saturan la barra de un normal de igual nivel ni en un dia ni en tres. La meseta a las 72 h es 0 porque, golpeando cada hora, el tope del dia se alcanza en las primeras horas y el resto del dia se desvanece (0.0417/h contra golpes de 0.02).
- [x] Reajuste del padre (`6a08e0e`): con 0.7 / 0.35 y la barra desvaneciendose en 24 h, golpear dejaba de servir (normal +7 en su pico). Se probaron en la hoja combinaciones de desvanecimiento 24-72 h, rendimiento 0.7-1.0 y tope 0.35-0.7; se eligio 0.9 / 0.6 sin tocar las 24 h (comportamiento ya existente). Maximo G+N, nivel 40: fearful 0.56 (+27), normal 0.47 (+14), brave 0.30 (+5), honourable 0.25 (+4). Golpear solo lleva a mitad de camino; golpes + hambre quiebran. Prueba de defaults RED -> GREEN; las pruebas de mecanica fijan 0.7 / 0.35 explicitos (`MechanicsSub`). `rules_tests` 62/62. Hoja regenerada sobre `6a08e0e`.
- Nota para T3: la columna "golpes" de la seccion 2 sigue suponiendo una barra de golpes 1.0, que en Balanced ya no se alcanza golpeando (maximo ~0.6 por dia mas lo que quede del anterior); la ventaja de quebrar sobre cuidar es ahora menor de lo que muestra esa columna. Revisar esa columna o su supuesto en T3.

### T3 Quebrar pesa mucho mas que cuidar
Ruta: escritor delegado (modulo puro nuevo + pegamento del motor + filas del save + config + tarjeta + generador de la hoja + docs: varios archivos no triviales).
- [x] Evidencia (hoja de `7bf8750`; antes de T1) (seccion 2, "Cuidar contra quebrar", nivel igual al lider):
  - normal: cuidado medio (+12) 72 contra quebrado 95, ventaja de quebrar +23; dias 90 %: 2 cuidando, 1 quebrando.
  - normal unico: 47 contra 80, ventaja +33; 4 contra 2 dias.
  - honourable unico: 27 contra 40, ventaja +13; 8 contra 5 dias.
  - "El bono de cuidado maximo (+20) vale 44% de lo que vale el estres pleno de un normal (+45)."
- [x] Propuesta: bonus por cuidarlo que **crece con dias seguidos cuidado** (racha guardada con el personaje, se corta si un dia no esta sano + alimentado + vida plena). Bonus = sorteo estable del dia entre min y max + `caredForStreakPerDay` x dias de racha, tope `caredForStreakMax`. Valores iniciales a calibrar con T1, p.ej. +3/dia hasta +15, de modo que una semana de cuidado iguale aprox. al estres pleno de un normal. Todo sembrado con `DailySeed`; nada re-tirable.

- [x] Maqueta primero (`0118ce0`): la fila de la tarjeta "Lo cuidaste +21% (racha 3 dias)" (estado `caredForStreak` de `ui-mockup/js/data.js`, `caredRow` de `ui-mockup/js/readout.js`, tabla de `readout.html`, item 6 del README de la maqueta).
- [x] Hecho (`e4b8b5b`, solo Balanced; Cheat sin cambios): modulo puro `src/care_streak_rules.cpp/.h` y el lado del motor `src/care_streak.cpp/.h`.
  - **Racha** = dias de juego mantenidos seguidos **antes** de hoy (`CareStreakDays`): un dia se mantiene si cada avistaje de ese dia lo encontro sano + alimentado + vida plena (`IsFullyCaredFor`). Un avistaje que no cumple vuelve la racha a 0 en el momento; un dia sin ningun avistaje la corta (en la duda, esperar: la regla del reloj de encierro). Dia = `DayIndexFromHours` (el de `DailySeed`): nada se re-tira con clics.
  - **Bonus** = sorteo estable del dia (sin cambios) + `CareStreakBonus` = `submissionCaredForStreakPerDay` (3, rango 0..10) x dias, tope `submissionCaredForStreakMax` (15, rango 0..40; racha plena a los 5 dias). Solo con cuidado de hoy y clave (`route_cage`); los dias van en `ChanceInputs::careStreakDays` (tarjeta y log, `JoinChance` no los lee).
  - **Avistajes**: el barrido de `hail.cpp` (cada preso en tu jaula, `NoteCareState`, lee `ReadFullyCaredFor`) y cada decision de la jaula (`GatherCageExtra` en modo decidir); el peek de la tarjeta solo lee.
  - **Save**: tres filas nuevas de `saved_ints` (`total_recruit.care_streak_day` = dia + 1, `care_streak_days`, `care_streak_kept`). Save viejo sin filas = racha 0. `load_boundary` lo vacia (`ForgetAllCareStreaks`).
  - **Tarjeta**: la fila "LO CUIDASTE" dice "+21% (racha 3 dias)" con racha (clave nueva `readout.caredForStreak` en la plantilla `messages.json`; el valor empieza en 2/5 del ancho para que entre). Log: `[chance] ... caredFor=+N(streak Dd)`; con `probeOnClick`, `<preso> [care] streak A -> B days (day D, cared for | not cared for: reset)`.
  - Claves nuevas solo agregadas a la plantilla `TotalRecruit/config.json`, clamp en `LoadConfig` con las constantes del modulo.
- [x] Pruebas: RED con los cuerpos vacios: 31 checks en `care_streak_rules`, `route_decisions` y `readout_model` (60/63 suites, 31/73424 fallos); GREEN tras implementar: 63/63 suites, 0/73424 fallos. DLL compila.
- [x] Hoja regenerada sobre `e4b8b5b`. Los "dias 90%" de la seccion 2 ahora se calculan dia por dia (`CageDaysTo90`), con la racha creciendo en las columnas de cuidado; las demas columnas no cambian. "Cuidar contra quebrar" gana la columna **una semana** (sorteo medio +12 + racha plena +15 = +27):

  | Temperamento | Unico | cuidado medio | una semana | quebrado | ventaja de quebrar (antes, sobre cuidado medio) | ventaja ahora (sobre una semana) | dias 90% cuidando antes / despues |
  |---|---|---|---|---|---|---|---|
  | fearful | no | 82 | 95 | 95 | +13 | +0 | 2 / 2 |
  | normal | no | 62 | 77 | 80 | +18 | +3 | 3 / 3 |
  | brave | no | 47 | 62 | 53 | +6 | -9 | 4 / 4 |
  | honourable | no | 42 | 57 | 46 | +4 | -11 | 5 / 4 |
  | fearful | si | 57 | 72 | 75 | +18 | +3 | 3 / 3 |
  | normal | si | 37 | 52 | 55 | +18 | +3 | 5 / 5 |
  | brave | si | 22 | 37 | 28 | +6 | -9 | 10 / 7 |
  | honourable | si | 17 | 32 | 21 | +4 | -11 | 13 / 8 |

  Criterio de aceptacion cumplido: una semana de cuidado de un normal (77) queda a 3 puntos de quebrarlo (80). Nivel +0, dias de sano / cuidado medio (tabla de arriba) / quebrado: fearful 2/2/1, normal 4/3/2, brave 6/4/4, honourable 7/**4**/4 (antes 7/5/4). Quebrado y sano sin cambios.
- Desvio a revisar (T6 o con el usuario): honorable cuidado queda en 4 dias contra el objetivo 6 (fuera de +-1; antes 5, en el borde) y en valiente y honorable cuidar una semana ya vale mas que quebrarlos (-9 / -11), porque el cuidado no se divide por la resolucion y el estres si. Coherente con el lore (el honorable responde al cuidado), pero la palanca para volver a 5-6 dias seria el sorteo (`submissionCaredForBonusMin/Max`) o una racha mas lenta, no T3.
- La columna "golpes" de la seccion 2 sigue suponiendo una barra de golpes 1.0 (nota de T2): sin cambio en T3.

### T4 El tiempo en jaula ya no cuenta
- [ ] Evidencia: el catalogo de `JoinChance` (seccion 1) no tiene termino de tiempo; la hoja no muestra ningun numero por dias encerrado. Los dias de las tablas salen solo de repetir la tirada diaria con el mismo estado.
- [ ] Propuesta: termino de **resignacion** por dia encerrado, leido del reloj de retencion persistido (`total_recruit.hold_start_hour`): `+resignationPerDay` por dia completo (p.ej. 2, rango 0..5), tope `resignationMax` (p.ej. 15, rango 0..30), dividido por la resolucion como estres y derrotas. Modulo puro `resignation_rules`; fila nueva en la hoja y en la tarjeta ("Resignado +N%") con maqueta primero.

### T5 Restos de Cheat
- [x] Hecho por priority-one B (`9b0466a`, `cooldown_minutes` + `wait_format`). Hoja regenerada (seccion 5): rechazo y duelo perdido empezados a las 10.25 h terminan en el minuto 616 (1 min despues); las etiquetas dicen " (1 min)", "Golpear (1 min)", "Noquear (1 min)". Antes: terminaban a las 11 h (45 min despues) y decian "(1 h)".
- [x] Propuesta original (cumplida con minutos de juego en lugar de decimas de hora): guardar los enfriamientos de rechazo y duelo en sub-hora (decimas de hora, como `hold_start_hour`), y etiquetar en minutos cuando la espera es menor a 1 h ("Golpear (1 min)"). Claves de `messages.json` nuevas; maqueta del boton primero.

### T6 Otros desbalances visibles en la hoja
- [ ] Revisar con el usuario, solo con numeros de la hoja:
  - **Moneda "care" une aunque la tirada falle** (seccion 2, "Que pasa cuando la tirada de la jaula FALLA"): honourable y warm/kind no unicos, sanos y alimentados, "se une: si" en todos los arquetipos. En la practica salta la columna "dias 90%" de honourable (cuidado 52, 4 dias). Confirmar si es intencional despues de P4.
  - **Esclavos**: moneda "freedom" une siempre en no unicos, sea cual sea la tirada.
  - **Nivel satura rapido**: a -20 niveles fearful y normal ya estan en 90-95 sano al dia 1; a +30 el termino ya vale el piso 2.
  - **Unicos fuertes inalcanzables en la jaula**: brave/honourable unicos a +50 quedan en 5 (45) en todas las columnas.
  - **Field-yield casi seguro** con 2 derrotas: normal 91, fearful 95 (seccion 4); maniacal 66 supera a brave 62 pese a personalidad -25 (resolucion 1.0 contra 1.6).

### T7 Animales: reclutar con duelo (respeto) o sometiendolos
> Casi todo hecho (2026-10-06): duelo con animales libres 40+ (`fe90e80`, `0717f2e`), field-yield 2/2 de animales libres en Balanced (`6d956a1`), cura/derrota en Cheat (`588491c`), golpes en la jaula bajan la barra de comida en Balanced (`65c6652`, hoja seccion 6). TotalSparring acepta un animal como lado B y ya no ofrece apuesta contra animales (TotalSparring `8782b2c`). Queda abierto: resolucion por especie (hoy `animalCageBeatingResolveBase` + nivel x `animalCageBeatingResolvePerLevel`). El "Hoy" de abajo es el estado de cuando se escribio.

- [x] Pedido del usuario: un animal tambien se puede reclutar ganandole un duelo (se gana su respeto) o sometiendolo a golpes, no solo con comida.
- [ ] Hoy: el animal se doma solo por el estomago con comida del jugador (`DecideAnimal`, `route_cage.cpp` antes de cualquier argumento humano); el duelo excluye animales (`duel_challenge_rules.cpp:13` `!facts.isAnimal`, y `recruit_core.cpp:1120` en Cheat); field-yield excluye animales (`route_rules.cpp` `!f.animal`); Golpear/Noquear en jaula si aplican a un animal pero su estres no entra en la decision del animal.
- [ ] Propuesta, dos vias nuevas en Balanced que conviven con la comida (cada una en su modulo puro, con tirada estable `DailySeed` y min/max configurables):
  - **Duelo / respeto**: RETAR A DUELO tambien para animales (sin el corte de nivel 80+ o con uno propio, p.ej. `animalDuelMinLevel`); ganar da una chance de unirse `animalDuelJoinBase` + `animalDuelJoinPerWin` por victoria previa (estilo `duel_join_rules`), con enfriamiento tras perder. Verificar primero que TotalSparring (puente `TS_*`) admite un animal como lado B; si no, documentarlo y pedir la parte de TotalSparring al usuario (no tocar ese repo sin permiso).
  - **Sometimiento**: las derrotas que el jugador le inflige (noqueos en el campo) y el estres de golpes en la jaula suman a la decision del animal: termino `animalSubmissionPerDefeat` (tope) y el estres de golpes ya existente, divididos por una resolucion por especie o por nivel (los animales no tienen temperamento humano: definir la resolucion, p.ej. por nivel o por clase de criatura).
- [ ] Coherencia lore: un animal salvaje respeta la fuerza; uno domestico/ajeno sigue excluido como hoy. Animal domado por duelo o sometimiento entra igual que el domado con comida.
- [ ] Hoja: filas nuevas en `tests/balance_sheet` para las dos vias; tarjeta y popup con maqueta primero ("Te respeta", "Sometido").
- [ ] Cheat no cambia: animal en jaula + 1 min y se une.

## Criterios de aceptacion
- Tabla objetivo de T1 acordada y escrita en este documento.
- La hoja regenerada despues de cada tarea calza con la tabla objetivo (columna "quebrado" y "cuidado" dentro de +-1 dia del objetivo, nivel +0).
- Golpear+Noquear no saturan la barra en menos de un dia de juego en un normal de igual nivel (T2).
- Cuidar una semana seguida vale aprox. lo mismo que quebrar (T3).
- El tiempo encerrado suma un termino acotado y visible (T4).
- En Cheat los enfriamientos terminan al minuto y las etiquetas dicen minutos (T5).
- Todo numero nuevo tiene clave en `config.json` (solo agregada) y su clamp en `LoadConfig`; `rules_tests` verde; DLL compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- Hoja de balance, antes y despues de cada cambio de numeros: `"$MSBUILD" tests/balance_sheet/balance_sheet.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo && tests/balance_sheet/x64/Release/balance_sheet.exe "$(git rev-parse --short HEAD)" > odd/balance/balance-sheet.md`
- `scripts/deploy.sh --dry-run`: config.json / messages.json solo agregan claves.

## QA pendiente para cerrar la v1
En juego, en este orden (ver `odd/system-status.md` y `QA.md`):
1. **12.6 I1-I10** (I4/I5): id estable tras recargar y relanzar. Riesgo mas alto: los ganchos de `Character` no estan verificados.
2. **12.33** P1-P3, W1-W2, H1-H4, C1-C8: dos presets, esperas de Cheat, horas de jaula guardadas, jaula sin atajos.
3. **§8 H7-H10**: esperas de Cheat de 1 minuto (llamado, rechazo, duelo perdido).
4. **12.32**: descanso de Noquear y estres por golpes.
5. **12.19** (C3c-C3h, C5, C10-C10i): Golpear y Noquear.
6. **12.3 E3/E8**, **12.8 U3**, **12.1 F3**, **12.13 K4b**.
7. **12.29** y **12.25 T12-T14**: chance por duelo ganado y aviso diario.

Pasos de cierre: QA en juego -> merge de `feat/balanced-tuning` a `main` -> push (decide el usuario).

## Progreso
- `af1ff35`: generador de la hoja de balance (`tests/balance_sheet`), hoja en `odd/balance/balance-sheet.md` sobre las reglas de `7bf8750`.
- Este documento y `odd/balance/START-PROMPT.md` creados. Ningun numero cambiado todavia.
- `1b310c7`: T7 agregado (animales por duelo o sometimiento), pedido del usuario.
- 2026-10-06: rama mergeada en `main` (`b87d0b2`) y borrada. T5 hecho por priority-one B; T7 casi hecho (ver arriba). `b8ca98c`: la hoja compila contra los enfriamientos al minuto, suma la seccion 6 (animal enjaulado golpeado) y se regenero sobre `3fb81b2`.
- 2026-10-07: T1 hecho (`8b9081d`): tabla objetivo acordada, base de nivel 35 y estres pleno 30; hoja regenerada.
- 2026-10-07: T2 hecho (`aedc3b4`): rendimiento decreciente 0.7 y tope diario 0.35 del estres por golpes en Balanced, guardados con el personaje; hoja regenerada (seccion 3); QA 12.32 S13-S18.
- 2026-10-07: T3 hecho (`0118ce0` maqueta, `e4b8b5b` regla + motor + save + tarjeta): racha de cuidado +3/dia hasta +15, guardada con el personaje; hoja regenerada (seccion 2, una semana de cuidado de un normal 77 contra quebrado 80); QA 12.33 C9-C15.

## Proximo paso
Mod Hub quitado (`odd/tasks/remove-mod-hub.md`, 2026-10-07): las claves nuevas van solo a `config.json`. T2 y T3 hechos; sigue **T4** (resignacion por dias encerrado), luego T6 (con el desvio de honorable cuidado de T3) y lo que queda de T7.
