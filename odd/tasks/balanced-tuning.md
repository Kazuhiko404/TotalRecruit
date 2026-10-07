# Ajuste fino de Balanced: calibrar la jaula con la hoja de balance

Rama: `feat/balanced-tuning` (desde `main` @ `7bf8750`; primer commit `80ca99b`, generador de la hoja de balance).
Ruta: por tarea (inline o escritor delegado), declarada en cada item del checklist al empezarlo.
Engram: proyecto `totalrecruit`, espejo `odd/balanced-tuning/tasks`.

## Objetivo
Que el preset Balanced se sienta lore-friendly y parejo: cuanto tarda en quebrarse un preso depende de quien es (personalidad, nivel, unico), y ninguna herramienta de la jaula (golpear, cuidar, esperar) domina a las demas. Cada cambio de numeros se mide contra `odd/balance/balance-sheet.md` antes y despues.

## Contexto: estado de la v1
`main` @ `7bf8750`, desplegado (DLL md5 `79a7eba70868acbc1830096b52f9f053`):
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
- Todo numero configurable (`config.json` + Mod Hub).
- Modulos aislados (SOLID), sin parche sobre parche.
- Maqueta en `ui-mockup/` antes de cambiar cualquier UI; las maquetas quedan al dia.
- Nunca pisar el `config.json` desplegado: `scripts/deploy.sh` solo agrega claves.
- Test-first con `tests/rules_tests` (RED antes de implementar).
- Regenerar `odd/balance/balance-sheet.md` antes y despues de cada cambio de numeros (comando en `DEVELOPING.md`).
- Conventional Commits; docs en espanol, codigo en ingles.

## Alcance
T1..T7 abajo. Solo reglas puras de Balanced (`src/*_rules.cpp`), sus claves de config, Mod Hub, mensajes y docs. Cheat no cambia salvo T5.

## Restricciones
- Toolset v100: sin C++11.
- Lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos solo POD (C2712).
- No se toca el `config.json` desplegado; las plantillas solo reciben claves NUEVAS. Sin deploy ni push sin pedido del usuario.
- No se quita comportamiento ya validado por el usuario por paridad o prolijidad.
- Toda regla nueva entra al `.vcxproj` de `tests/balance_sheet` y la hoja gana la fila o tabla que la muestra.

## Checklist

### T1 Calibracion: tabla objetivo
- [ ] Acordar con el usuario la tabla objetivo (unica pregunta de producto). Propuesta, en dias de juego hasta 90 % acumulado con el preso trabajado (hambre/heridas/golpes), nivel igual al lider:

  | Temperamento | Objetivo |
  |---|---|
  | cobarde (fearful) | ~1 dia |
  | normal | ~2-3 dias |
  | valiente / honorable | ~5-7 dias |
  | unico honorable | ~1-2 semanas |

- [ ] Contrastar con la hoja. Evidencia (seccion 2, "dias 90%" entre parentesis, nivel +0):
  - fearful: quebrado 95 (1). Ya en objetivo.
  - normal: sano 60 (3), quebrado 95 (1). Quebrado queda por debajo del objetivo (1 contra 2-3).
  - brave: sano 45 (4), quebrado 73 (2). Objetivo 5-7.
  - honourable: sano 40 (5), quebrado 65 (3). Objetivo 5-7.
  - honourable unico: sano 15 (15), quebrado 40 (5). Objetivo 7-14.
- [ ] Contrastar los terminos fijos con la hoja (seccion 1):
  - **Diferencia de nivel**: base 45, +-1.5 por nivel, rango 2..85; a -27 ya vale 85 y a +30 ya vale 2 (tope y piso). Es el termino de mayor rango (83 puntos) contra personalidad -25..20.
  - **Personalidad**: fearful +20 ... maniacal -25; resolucion 0.6 (fearful, dumb) a 1.8 (honourable).
  - **Piso/techo** de `JoinChance`: [5, 95]. Muchas filas tocan el techo 95 al dia 1 (fearful +0 hambre, normal +0 H+H+C).
  - **Unicos**: -25 fijo. brave/honourable unicos a +50 quedan en 5 (45) en TODAS las columnas.
  - **Intimidacion**: +15 fijo en la jaula.
- Propuesta: ajustar primero los multiplicadores de resolucion y el tope de estres (`ChanceFromStress`, 45 / resolve) hasta que la columna "quebrado" calce con la tabla; despues, si hace falta, la pendiente de nivel (1.5/nivel). Claves existentes, sin regla nueva. Si un ajuste necesita un termino nuevo, va en T2-T4.

### T2 Golpear domina
- [ ] Evidencia (seccion 3, lider 40, un golpe cada vez que termina el descanso de 1 h):
  - normal nivel 40: +0.080 por golpe, barra a 1.0 en 25 sesiones solo Golpear, en **6** con Golpear+Noquear; meseta 1.00 (+45).
  - fearful nivel 40: 11 sesiones solo Golpear, 3 con Golpear+Noquear.
  - brave nivel 40: meseta solo Golpear 0.64 (+18), pero 10 sesiones a 1.0 con Golpear+Noquear; honourable nivel 40: meseta 0.24 (+6), 12 sesiones con Golpear+Noquear.
  - Umbral de saturacion: ganancia > 0.0417 por hora (lo que se desvanece en 1 h).
  - El termino de golpes entra al estres con peso 1.00 (hambre 0.50, heridas 0.35, cansancio 0.15): una barra de golpes llena satura sola el estres.
  - Columna "golpes" (seccion 2, nivel +0): normal 95 (1), brave 73 (2), honourable 65 (3): igual que "quebrado".
  - Unico costo que muestra la hoja: Golpear hiere, asi que se pierde el bonus de cuidado. Sin efecto en relacion ni opinion.
- [ ] Propuesta (modulo puro nuevo o extension de `beating_stress_rules`, test-first):
  - **Rendimiento decreciente** por golpe del mismo dia de juego: la ganancia n-esima del dia se multiplica por `beatingDiminishFactor` (p.ej. 0.7, rango 0.3..1.0).
  - **Tope diario** de estres por golpes: `beatingDailyStressCap` (p.ej. 0.35, rango 0.1..1.0).
  - **Penalizacion de relacion/opinion** al golpear: `beatingRelationPenaltyMin/Max` (p.ej. 2..6 por golpe), x1.5 para honorable y valiente (`beatingRelationPenaltyPrideMult`). Nota: la jaula hoy no usa la relacion en `JoinChance`; decidir si la penalizacion pesa en la tirada de la jaula o solo despues (miembro reclutado, rencor).
  - Considerar bajar el peso del termino de golpes (hoy 1.00) a la par de hambre (0.50): `stressWeightBeaten`.

### T3 Quebrar pesa mucho mas que cuidar
- [ ] Evidencia (seccion 2, "Cuidar contra quebrar", nivel igual al lider):
  - normal: cuidado medio (+12) 72 contra quebrado 95, ventaja de quebrar +23; dias 90 %: 2 cuidando, 1 quebrando.
  - normal unico: 47 contra 80, ventaja +33; 4 contra 2 dias.
  - honourable unico: 27 contra 40, ventaja +13; 8 contra 5 dias.
  - "El bono de cuidado maximo (+20) vale 44% de lo que vale el estres pleno de un normal (+45)."
- [ ] Propuesta: bonus por cuidarlo que **crece con dias seguidos cuidado** (racha guardada con el personaje, se corta si un dia no esta sano + alimentado + vida plena). Bonus = sorteo estable del dia entre min y max + `caredForStreakPerDay` x dias de racha, tope `caredForStreakMax`. Valores iniciales a calibrar con T1, p.ej. +3/dia hasta +15, de modo que una semana de cuidado iguale aprox. al estres pleno de un normal. Todo sembrado con `DailySeed`; nada re-tirable.

### T4 El tiempo en jaula ya no cuenta
- [ ] Evidencia: el catalogo de `JoinChance` (seccion 1) no tiene termino de tiempo; la hoja no muestra ningun numero por dias encerrado. Los dias de las tablas salen solo de repetir la tirada diaria con el mismo estado.
- [ ] Propuesta: termino de **resignacion** por dia encerrado, leido del reloj de retencion persistido (`total_recruit.hold_start_hour`): `+resignationPerDay` por dia completo (p.ej. 2, rango 0..5), tope `resignationMax` (p.ej. 15, rango 0..30), dividido por la resolucion como estres y derrotas. Modulo puro `resignation_rules`; fila nueva en la hoja y en la tarjeta ("Resignado +N%") con maqueta primero.

### T5 Restos de Cheat
- [ ] Evidencia (seccion 5): `kCheatWaitHours` = 0.0167 h; rechazo del sometido y duelo perdido empiezan a las 10.25 h y terminan a las 11 h (45 min despues), porque se guardan en horas enteras; los descansos muestran "Golpear (1 h)" y "Noquear (1 h)" para una espera de 1 minuto.
- [ ] Propuesta: guardar los enfriamientos de rechazo y duelo en sub-hora (decimas de hora, como `hold_start_hour`), y etiquetar en minutos cuando la espera es menor a 1 h ("Golpear (1 min)"). Claves de `messages.json` nuevas; maqueta del boton primero.

### T6 Otros desbalances visibles en la hoja
- [ ] Revisar con el usuario, solo con numeros de la hoja:
  - **Moneda "care" une aunque la tirada falle** (seccion 2, "Que pasa cuando la tirada de la jaula FALLA"): honourable y warm/kind no unicos, sanos y alimentados, "se une: si" en todos los arquetipos. En la practica salta la columna "dias 90%" de honourable (cuidado 52, 4 dias). Confirmar si es intencional despues de P4.
  - **Esclavos**: moneda "freedom" une siempre en no unicos, sea cual sea la tirada.
  - **Nivel satura rapido**: a -20 niveles fearful y normal ya estan en 90-95 sano al dia 1; a +30 el termino ya vale el piso 2.
  - **Unicos fuertes inalcanzables en la jaula**: brave/honourable unicos a +50 quedan en 5 (45) en todas las columnas.
  - **Field-yield casi seguro** con 2 derrotas: normal 91, fearful 95 (seccion 4); maniacal 66 supera a brave 62 pese a personalidad -25 (resolucion 1.0 contra 1.6).

### T7 Animales: reclutar con duelo (respeto) o sometiendolos
- [ ] Pedido del usuario: un animal tambien se puede reclutar ganandole un duelo (se gana su respeto) o sometiendolo a golpes, no solo con comida.
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
- Todo numero nuevo tiene clave en `config.json` (solo agregada) y fila en Mod Hub; `rules_tests` verde; DLL compila.

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
- `80ca99b`: generador de la hoja de balance (`tests/balance_sheet`), hoja en `odd/balance/balance-sheet.md` sobre las reglas de `7bf8750`.
- Este documento y `odd/balance/START-PROMPT.md` creados. Ningun numero cambiado todavia.
- `133d08b`: T7 agregado (animales por duelo o sometimiento), pedido del usuario.

## Proximo paso
T1: confirmar la tabla objetivo con el usuario; despues T2-T4, luego T5, luego T6, luego T7 (animales: duelo y sometimiento).
