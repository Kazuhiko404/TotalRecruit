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
Orden real: S0, S1, S3, S2, S4 (S2 necesita el modulo de estres de S3).

- [x] S0 maqueta: Noquear en espera ("Noquear (1 h)") y el cuarto termino del estres (Golpes, en su propia fila) en `ui-mockup/readout.html`, `ui-mockup/js/{readout,data,popup_data}.js`, `ui-mockup/README.md`, filas nuevas de Mod Hub. Ruta: escritor delegado. Evidencia: `node --check` de los JS y de los scripts en linea. Commit `aa30d1e`.
- [x] S1 descansos: `beatingCooldownHours` 1 h; `knockoutCooldownHours` nuevo (1 h, `submissionKnockoutCooldownHours`, Mod Hub "Sumision: horas entre noqueos" 0,5..48). `BeatCooldownLedger` -> `CageCooldownLedger` (uno por comando), `RestLabel` / `RestHoursShown`, `KnockoutCaption`, `KnockoutReady`, `CageCommandFacts::knockoutHoursLeft`, `CageCommand_BeatResting` / `CageCommand_KnockoutResting`, `cage.knockoutResting`. El credito ahora tiene clase (`OrderedCreditKind`, `CreditWindowMs`) y `ResolveCageKnockout` reemplaza a `ClaimOrderedKnockoutCredit`: un credito de Noquear reclamado (el preso cayo) arranca su descanso. Ruta: escritor delegado. Evidencia: RED = rules_tests no compila (C2065 `CageCooldownLedger`, C3861 `RestLabel`); GREEN = 46/46 suites, 0/64442 fallidos; DLL compila. Commit `292e552`.
- [x] S3 estres por golpes: `beating_stress_rules` (puro: `BeatingStressGain`, `BeatingLevelFactor`, `DecayedBeatingStress`, `BeatingStressLedger`, forma del save) y `beating_stress` (motor: lecturas, lock, `saved_ints` `total_recruit.beating_stress` / `_hour`, `ForgetAllBeatingStress` en la carga); `SafeTemperament` en `engine_reads`; `StressTerms::beaten` y `StressFromState(..., beatingStress, ...)`; `CoreFacts::beatingStress`; tarjeta (segmento `kSoftDanger`, leyenda "Golpes" en su fila), linea `[readout] ... beaten N`; Golpear suma estres en cada golpe que pega; 7 claves de config con 5 sliders. `SplitStress` redondea en vez de truncar. Ruta: escritor delegado. Evidencia: RED = C1083 `beating_stress_rules.h`, C2660 `StressFromState` 5 args; GREEN = 47/47 suites, 0/66768 fallidos; DLL compila. Commit `5a6a637`.
- [x] S2 noqueos de la jaula: `IsCageKnockout` / `CageKnockoutAddsStress` (puro); el hook no llama a `RecordDefeat` para un noqueo de la jaula, marca el episodio contado (`NoteCageKnockout`) y loguea `[defeat] <nombre> cage knockout: not a defeat (stress +X%)`; Noquear suma su estres al reclamarse. `RecordDefeat` pierde el parametro `creditedToPlayer` (nadie lo usa ya). Ruta: escritor delegado. Evidencia: RED = C3861 `IsCageKnockout`; GREEN = 47/47 suites, 0/66774 fallidos; DLL compila. Commit `5a4bee6`.
- [x] S4 docs: REQUISITOS R2.3 (Estado nuevo) y R3.2 (nota), DEVELOPING.md (filas `defeats`, `cage_command_rules`, `cage_commands`, `saved_ints`, `beating_stress_rules`, `beating_stress`, `readout_model`), QA.md (12.13 K4b/K6, 12.19 C3c/C3e/C5/C5b, 12.32 nueva), `odd/system-status.md`, `config.h`. Ruta: escritor delegado. Commit `f62eb36`.

Decisiones:
- Nivel del jugador = el del lider del escuadron (el mismo que compara `JoinChance`), no el del actor.
- El noqueo de Golpear no suma estres extra: el golpe ya sumo el suyo (`stress +0%, the blow added its own`).
- Base 0 o menos apaga el termino (el minimo no aplica); un desvanecimiento de 0 o menos no desvanece.
- El save guarda el valor tal como se escribio y su hora (milesimas / decimas de hora); el desvanecimiento se aplica al leer. Un reloj anterior a la hora guardada no baja ni sube la barra.
- Un noqueo de la jaula no abre la suplica de R3.1 (antes podia, por la derrota): Golpear no deja casi sin sangre.
- `submissionBeatingCooldownHours` se agrego a la plantilla con 1.0; el config desplegado no la tenia, asi que rige el nuevo valor por defecto.

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
- S0..S4 hechos en `fix/cage-beat-direct-hit`: `aa30d1e`, `292e552`, `5a6a637`, `5a4bee6`, `f62eb36`. Sin deploy, sin push.
- `scripts/deploy.sh --dry-run`: config.json agrega 9 claves, messages.json 2 (`cage.knockoutResting`, `readout.beaten`), 0 valores cambiados.

## Siguiente paso
Deploy (lo hace el padre) y QA en juego: 12.32 S1-S12, 12.19 C3c/C3e/C5, 12.13 K6.
