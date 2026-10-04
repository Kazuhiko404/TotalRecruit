# Golpear y Noquear: descanso de 1 h y estres acumulado

Rama: `fix/cage-beat-direct-hit` (sobre `ddce78e`).
Ruta: escritor delegado (un solo escritor acotado; 2+ archivos no triviales por tarea).

## Objetivo
Los dos comandos de la jaula (Golpear y Noquear) descansan por preso un tiempo configurable (1 h por defecto), y los noqueos que provocan ya no cuentan como derrota: suben un estres acumulado, propio del preso, que se guarda con la partida y se desvanece con las horas de juego.

Palabras del usuario: "agrega a config el cooldown de golpear y noquear, ponelo default 1h, 4h es mucho. Los noqueos que no cuenten como derrota pero que suban el estrés un % calculado con alguna fórmula con min y max dependiendo los niveles y la personalidad del npc, tanto golpear como el KO."

## Problema
- Golpear descansa 6 h (`submissionBeatingCooldownHours`); Noquear no descansa nunca.
- Cada noqueo acreditado a Golpear o Noquear suma una derrota (`ClaimOrderedKnockoutCredit` -> `RecordDefeat`): un preso se "quiebra" noqueandolo en bucle dentro de la jaula, y la ruta de rendicion en el campo (`defeatsToBreak`) se alimenta de la jaula.
- El estres es solo instantaneo (hambre, heridas, cansancio): una paliza no deja huella mas alla de la herida.

## Por que
La jaula se trabaja con hambre y castigo sostenido (R2.3); las derrotas son vencerlo de verdad (R3.2). Un noqueo en la jaula es castigo, no una victoria.

## Alcance
- S1 descansos: `beatingCooldownHours` 1 h por defecto; `knockoutCooldownHours` nuevo (1 h, `submissionKnockoutCooldownHours`, Mod Hub "Sumision: horas entre noqueos"). Noquear descansa por preso tras una orden que lo tumbo; boton "Noquear (1 h)", el click dice la espera. Ledger generalizado (`CageCooldownLedger`), solo de sesion como el de Golpear. Maqueta primero.
- S2 los noqueos de la jaula no son derrotas: el credito se reclama igual (`ResolveCageKnockout`), pero va al estres; el episodio se marca contado para que el clic derecho no lo cuente por la puerta de atras. Los noqueos del campo cuentan igual que antes.
- S3 estres por golpes: regla pura `beating_stress_rules` (ganancia con base, factor de niveles, temperamento, min y max; desvanecimiento lineal; ledger), lado del motor `beating_stress` (guardado en el estado de guardado del personaje, dos filas de `saved_ints`). Cuarto termino del estres (`beaten`), en la linea del readout y en la tarjeta.
- S4 docs: REQUISITOS R2.3 / R3.2, DEVELOPING.md, QA.md, `odd/system-status.md`, comentarios de `config.h`, claves nuevas de `messages.json`.

## Restricciones
- Toolset v100: sin C++11.
- Lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos solo POD (C2712).
- No se toca el `config.json` desplegado; las plantillas solo reciben claves NUEVAS. Sin deploy, sin push.
- Modulos aislados (SOLID), sin parche sobre parche.

## Checklist
- [ ] S0 maqueta: Noquear en espera ("Noquear (1 h)") y el cuarto termino del estres (Golpes) en `ui-mockup/readout.html`, `ui-mockup/js/*`, `ui-mockup/README.md`. Ruta: escritor delegado.
- [ ] S1 descansos de Golpear (1 h) y Noquear (1 h nuevo), regla pura + motor + Mod Hub + tests. Ruta: escritor delegado.
- [ ] S2 noqueos de la jaula: no son derrota, van al estres. Ruta: escritor delegado.
- [ ] S3 estres acumulado por golpes: regla pura + tests, motor, guardado, cuarto termino, readout y tarjeta. Ruta: escritor delegado.
- [ ] S4 docs. Ruta: escritor delegado.

## Criterios de aceptacion
- Golpear y Noquear descansan 1 h por preso (configurable); el boton muestra la espera y el click la dice.
- Un noqueo de Golpear o Noquear sobre un preso de la jaula no cambia el conteo de derrotas; log `[defeat] <nombre> cage knockout: not a defeat (stress +X%)`.
- Un noqueo en el campo (sin comando) cuenta como antes.
- Cada golpe que pega y cada noqueo de Noquear suman estres con la formula; el valor se desvanece en `beatingStressFadeHours` horas de juego y sobrevive a una recarga.
- El estres por golpes se ve en la linea del readout (`beaten N`) y en la tarjeta (Golpes N).
- rules_tests pasa entero; TotalRecruit.dll compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- `scripts/deploy.sh --dry-run`: config.json / messages.json solo agregan claves.

## Progreso
- Documento creado antes de tocar el codigo.

## Siguiente paso
S0 maqueta.
