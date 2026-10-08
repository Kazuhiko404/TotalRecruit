# Hoja de balance (Balanced, config por defecto)

> **Generado, no editar a mano.** Programa: `tests/balance_sheet/main.cpp`, que llama a las funciones puras REALES de `src/` con `Config()` por defecto.
> Reglas de `src/` en el commit `aedc3b4`.
> Regenerar:
>
> ```bash
> "$MSBUILD" tests/balance_sheet/balance_sheet.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo
> tests/balance_sheet/x64/Release/balance_sheet.exe "$(git rev-parse --short HEAD)" > odd/balance/balance-sheet.md
> ```

Supuestos comunes: lider del grupo nivel 40; la tirada de unirse se sortea una vez por dia de juego (`DailyRoll`), asi que "dias 90%" = dias hasta un 90% de probabilidad acumulada de que se una manteniendo ese estado; sin relacion (la jaula no la usa), sin rencor, 0 derrotas salvo donde se dice.

## 1. Catalogo de terminos

`JoinChance` = nivel + relacion + personalidad + derrotas + estres + situacion + cuidado - rencor - unico, acotado a **[5, 95]** (`chance.floor`/`ceiling`).

| Termino | Funcion | Formula (defaults) | Min | Max |
|---|---|---|---|---|
| Nivel | `ChanceFromLevelGap` | base 35; NPC arriba: -1.5/nivel (piso 2); NPC abajo: +1.5/nivel (tope +40) | 2 | 75 |
| Relacion | `ChanceFromRelation` | relacion x 0.25 (no en la jaula ni en field-yield) | -25 | 25 |
| Personalidad | `ChanceFromPersonality` | `chance.personality[]` (tabla abajo) | -25 | 20 |
| Derrotas | `ChanceFromDefeats` | 12/derrota / resolve, tope 45 | 0 | 45 |
| Estres | `ChanceFromStress` | estres(0..1) x 30 / resolve, tope 30 | 0 | 30 |
| Situacion: jaula | `intimidationBonus` | fijo | 15 | 15 |
| Situacion: field-yield | `fieldYieldBonus` - lealtad | 10 (militar/ley: -25) | -15 | 10 |
| Situacion: rescate | `rescueBaseBonus` + curado x `rescueHealWeight` | 25 + 30 x curado(0..1) | 25 | 55 |
| Cuidado (jaula) | `CaredForBonus` | sorteo diario estable, solo sano+alimentado+vida plena | 5 | 20 |
| Rencor | `grudgePenalty` | fijo (honorable/valiente/amable: rechazo absoluto) | -20 | 0 |
| Unico | `uniquePenalty` | fijo | -25 | 0 |

### Personalidad y resolve

| Temperamento | Personalidad | Resolve (divide estres, derrotas y golpes) | Estres pleno vale | Grupo |
|---|---|---|---|---|
| normal | +0 | 1.0 | +30 | neutral |
| fearful | +20 | 0.6 | +30 | neutral |
| traitorous | +15 | 1.0 | +30 | greedy |
| dumb | +10 | 0.6 | +30 | neutral |
| smart | -5 | 1.0 | +30 | greedy |
| brave | -15 | 1.6 | +18 | neutral |
| honourable | -20 | 1.8 | +16 | good |
| maniacal | -25 | 1.0 | +30 | bad |
| cold/cruel | +0 | 1.0 | +30 | bad |
| warm/kind | +0 | 1.0 | +30 | good |

### Barra de estres (`StressTermsFromState`)

estres = clamp01(hambre x 0.50 + heridas x 0.35 + cansancio x 0.15 + golpes x 1.00). El termino de golpes entra **sin peso**: una barra de golpes llena satura sola la barra entera.

| Estado | hambre | heridas | cansancio | golpes | estres |
|---|---|---|---|---|---|
| fresco | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| hambriento (lleno 0) | 0.50 | 0.00 | 0.00 | 0.00 | 0.50 |
| herido (worstDamage 0.8) | 0.00 | 0.28 | 0.00 | 0.00 | 0.28 |
| agotado | 0.00 | 0.00 | 0.15 | 0.00 | 0.15 |
| hambre+herido+agotado | 0.50 | 0.28 | 0.15 | 0.00 | 0.93 |
| golpes 0.5 | 0.00 | 0.00 | 0.00 | 0.50 | 0.50 |
| golpes 1.0 | 0.00 | 0.00 | 0.00 | 1.00 | 1.00 |

### Termino de nivel por diferencia (NPC - lider)

| Diferencia | -60 | -40 | -27 | -20 | -10 | +0 | +10 | +20 | +30 | +40 | +50 | +60 | 
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Nivel | 75 | 75 | 75 | 65 | 50 | 35 | 20 | 5 | 2 | 2 | 2 | 2 | 

## 2. Jaula: probabilidad diaria de unirse (Balanced)

Lider 40. Intimidacion +15 siempre. Columnas: **sano** = estres 0 sin bono (referencia; en el juego un preso sano, alimentado y a vida plena SIEMPRE tiene el bono de cuidado); **cuid. min/max** = sano + bono 5 / 20; **hambre** = lleno 0; **herido** = worstDamage 0.8; **H+H+C** = hambre + herido + agotado (estres 0.93); **golpes** = barra de golpes 1.0, por lo demas sano (sin bono: los golpes hieren); **quebrado** = todo junto (estres 1.0). Entre parentesis: dias hasta 90% acumulado.

| Temperamento | Unico | Nivel NPC | sano | cuid. min | cuid. max | hambre | herido | H+H+C | golpes | quebrado |
|---|---|---|---|---|---|---|---|---|---|---|
| fearful | no | 20 (-20) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) |
| fearful | no | 40 (+0) | 70 (2) | 75 (2) | 90 (1) | 94 (1) | 83 (2) | 95 (1) | 95 (1) | 95 (1) |
| fearful | no | 60 (+20) | 40 (5) | 45 (4) | 60 (3) | 64 (3) | 53 (4) | 70 (2) | 70 (2) | 70 (2) |
| fearful | no | 90 (+50) | 37 (5) | 42 (5) | 57 (3) | 61 (3) | 50 (4) | 67 (3) | 67 (3) | 67 (3) |
| normal | no | 20 (-20) | 80 (2) | 85 (2) | 95 (1) | 95 (1) | 88 (2) | 95 (1) | 95 (1) | 95 (1) |
| normal | no | 40 (+0) | 50 (4) | 55 (3) | 70 (2) | 65 (3) | 58 (3) | 77 (2) | 80 (2) | 80 (2) |
| normal | no | 60 (+20) | 20 (11) | 25 (9) | 40 (5) | 35 (6) | 28 (8) | 47 (4) | 50 (4) | 50 (4) |
| normal | no | 90 (+50) | 17 (13) | 22 (10) | 37 (5) | 32 (6) | 25 (9) | 44 (4) | 47 (4) | 47 (4) |
| brave | no | 20 (-20) | 65 (3) | 70 (2) | 85 (2) | 74 (2) | 70 (2) | 82 (2) | 83 (2) | 83 (2) |
| brave | no | 40 (+0) | 35 (6) | 40 (5) | 55 (3) | 44 (4) | 40 (5) | 52 (4) | 53 (4) | 53 (4) |
| brave | no | 60 (+20) | 5 (45) | 10 (22) | 25 (9) | 14 (16) | 10 (22) | 22 (10) | 23 (9) | 23 (9) |
| brave | no | 90 (+50) | 5 (45) | 7 (32) | 22 (10) | 11 (20) | 7 (32) | 19 (11) | 20 (11) | 20 (11) |
| honourable | no | 20 (-20) | 60 (3) | 65 (3) | 80 (2) | 68 (3) | 64 (3) | 75 (2) | 76 (2) | 76 (2) |
| honourable | no | 40 (+0) | 30 (7) | 35 (6) | 50 (4) | 38 (5) | 34 (6) | 45 (4) | 46 (4) | 46 (4) |
| honourable | no | 60 (+20) | 5 (45) | 5 (45) | 20 (11) | 8 (28) | 5 (45) | 15 (15) | 16 (14) | 16 (14) |
| honourable | no | 90 (+50) | 5 (45) | 5 (45) | 17 (13) | 5 (45) | 5 (45) | 12 (19) | 13 (17) | 13 (17) |
| fearful | si | 20 (-20) | 75 (2) | 80 (2) | 95 (1) | 95 (1) | 88 (2) | 95 (1) | 95 (1) | 95 (1) |
| fearful | si | 40 (+0) | 45 (4) | 50 (4) | 65 (3) | 69 (2) | 58 (3) | 75 (2) | 75 (2) | 75 (2) |
| fearful | si | 60 (+20) | 15 (15) | 20 (11) | 35 (6) | 39 (5) | 28 (8) | 45 (4) | 45 (4) | 45 (4) |
| fearful | si | 90 (+50) | 12 (19) | 17 (13) | 32 (6) | 36 (6) | 25 (9) | 42 (5) | 42 (5) | 42 (5) |
| normal | si | 20 (-20) | 55 (3) | 60 (3) | 75 (2) | 70 (2) | 63 (3) | 82 (2) | 85 (2) | 85 (2) |
| normal | si | 40 (+0) | 25 (9) | 30 (7) | 45 (4) | 40 (5) | 33 (6) | 52 (4) | 55 (3) | 55 (3) |
| normal | si | 60 (+20) | 5 (45) | 5 (45) | 15 (15) | 10 (22) | 5 (45) | 22 (10) | 25 (9) | 25 (9) |
| normal | si | 90 (+50) | 5 (45) | 5 (45) | 12 (19) | 7 (32) | 5 (45) | 19 (11) | 22 (10) | 22 (10) |
| brave | si | 20 (-20) | 40 (5) | 45 (4) | 60 (3) | 49 (4) | 45 (4) | 57 (3) | 58 (3) | 58 (3) |
| brave | si | 40 (+0) | 10 (22) | 15 (15) | 30 (7) | 19 (11) | 15 (15) | 27 (8) | 28 (8) | 28 (8) |
| brave | si | 60 (+20) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) |
| brave | si | 90 (+50) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) |
| honourable | si | 20 (-20) | 35 (6) | 40 (5) | 55 (3) | 43 (5) | 39 (5) | 50 (4) | 51 (4) | 51 (4) |
| honourable | si | 40 (+0) | 5 (45) | 10 (22) | 25 (9) | 13 (17) | 9 (25) | 20 (11) | 21 (10) | 21 (10) |
| honourable | si | 60 (+20) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) |
| honourable | si | 90 (+50) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) |

### Cuidar contra quebrar (mismo NPC, nivel igual al lider)

| Temperamento | Unico | cuidado medio (+12) | quebrado | ventaja de quebrar | dias 90% cuidando | dias 90% quebrando |
|---|---|---|---|---|---|---|
| fearful | no | 82 | 95 | +13 | 2 | 1 |
| normal | no | 62 | 80 | +18 | 3 | 2 |
| brave | no | 47 | 53 | +6 | 4 | 4 |
| honourable | no | 42 | 46 | +4 | 5 | 4 |
| fearful | si | 57 | 75 | +18 | 3 | 2 |
| normal | si | 37 | 55 | +18 | 5 | 3 |
| brave | si | 22 | 28 | +6 | 10 | 8 |
| honourable | si | 17 | 21 | +4 | 13 | 10 |

El bono de cuidado maximo (+20) vale 66% de lo que vale el estres pleno de un normal (+30).

### Que pasa cuando la tirada de la jaula FALLA (`DecideCage`, tabla de monedas)

Preso sano, alimentado, vida plena, nivel igual al lider, bolsillo conocido en 0 cats, clave cuya tirada falla. `se une` = se une igual aunque la tirada fallo.

| Temperamento | Arquetipo | Unico | Moneda | Se une | Motivo |
|---|---|---|---|---|---|
| fearful | other | no | cats | no | pide {cats} cats y tienes {purse} |
| fearful | bandit | no | cats | no | pide {cats} cats y tienes {purse} |
| fearful | military | no | submission | no | sigue desafiante: déjalo con hambre o hérelo |
| fearful | slave | no | freedom | **si** | seguirá a quien lo liberó |
| normal | other | no | cats | no | pide {cats} cats y tienes {purse} |
| normal | bandit | no | cats | no | pide {cats} cats y tienes {purse} |
| normal | military | no | submission | no | sigue desafiante: déjalo con hambre o hérelo |
| normal | slave | no | freedom | **si** | seguirá a quien lo liberó |
| brave | other | no | submission | no | sigue desafiante: déjalo con hambre o hérelo |
| brave | bandit | no | cats | no | pide {cats} cats y tienes {purse} |
| brave | military | no | submission | no | sigue desafiante: déjalo con hambre o hérelo |
| brave | slave | no | freedom | **si** | seguirá a quien lo liberó |
| honourable | other | no | care | **si** | alimentado y curado: se unirá |
| honourable | bandit | no | care | **si** | alimentado y curado: se unirá |
| honourable | military | no | care | **si** | alimentado y curado: se unirá |
| honourable | slave | no | freedom | **si** | seguirá a quien lo liberó |
| warm/kind | other | no | care | **si** | alimentado y curado: se unirá |
| warm/kind | bandit | no | care | **si** | alimentado y curado: se unirá |
| warm/kind | military | no | care | **si** | alimentado y curado: se unirá |
| warm/kind | slave | no | freedom | **si** | seguirá a quien lo liberó |
| cold/cruel | other | no | cats | no | pide {cats} cats y tienes {purse} |
| cold/cruel | bandit | no | cats | no | pide {cats} cats y tienes {purse} |
| cold/cruel | military | no | submission | no | sigue desafiante: déjalo con hambre o hérelo |
| cold/cruel | slave | no | freedom | **si** | seguirá a quien lo liberó |
| fearful | other | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| fearful | bandit | si | cats | no | pide {cats} cats y tienes {purse} |
| fearful | military | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| fearful | slave | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| normal | other | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| normal | bandit | si | cats | no | pide {cats} cats y tienes {purse} |
| normal | military | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| normal | slave | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| brave | other | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| brave | bandit | si | cats | no | pide {cats} cats y tienes {purse} |
| brave | military | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| brave | slave | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| honourable | other | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| honourable | bandit | si | cats | no | pide {cats} cats y tienes {purse} |
| honourable | military | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| honourable | slave | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| warm/kind | other | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| warm/kind | bandit | si | cats | no | pide {cats} cats y tienes {purse} |
| warm/kind | military | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| warm/kind | slave | si | submission | no | no hay dinero que compre lo que cree: el hambre y las heridas harán el trabajo |
| cold/cruel | other | si | cats | no | pide {cats} cats y tienes {purse} |
| cold/cruel | bandit | si | cats | no | pide {cats} cats y tienes {purse} |
| cold/cruel | military | si | cats | no | pide {cats} cats y tienes {purse} |
| cold/cruel | slave | si | cats | no | pide {cats} cats y tienes {purse} |

## 3. Estres por golpes (Golpear / Noquear)

ganancia = clamp(base x factor de nivel x rendimiento / resolve, 0.02, 0.25), despues tope diario; base Golpear 0.08, Noquear 0.15; factor = lider/NPC acotado a [0.5, 1.5]; rendimiento = 0.70^(n-1) para el golpe n-esimo del dia de juego (Golpear y Noquear cuentan juntos); tope diario 0.35 de barra por preso y dia de juego (solo Balanced; Cheat sin limites diarios); descanso Golpear 1 h, Noquear 1 h; una barra llena se va en 24 h (-0.0417/h). Simulacion: un golpe (y/o un noqueo) cada vez que el descanso termina, 72 h seguidas (3 dias de juego), lider 40.

| Temperamento | Nivel NPC | factor | +Golpear | +Noquear | sesiones a 1.0 (solo Golpear) | meseta solo Golpear | a las 24 h solo Golpear | sesiones a 1.0 (Golpear+Noquear) | meseta Golpear+Noquear | maximo Golpear+Noquear | sin limites diarios: sesiones a 1.0 (G+N) | estres pleno vale |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| fearful | 20 | 1.50 | 0.200 | 0.250 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.35 (+17) | 3 | +30 |
| fearful | 27 | 1.48 | 0.198 | 0.250 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.35 (+17) | 3 | +30 |
| fearful | 40 | 1.00 | 0.133 | 0.250 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.31 (+15) | 3 | +30 |
| fearful | 60 | 0.67 | 0.089 | 0.167 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.27 (+13) | 5 | +30 |
| fearful | 80 | 0.50 | 0.067 | 0.125 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.19 (+9) | 7 | +30 |
| normal | 20 | 1.50 | 0.120 | 0.225 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.31 (+9) | 4 | +30 |
| normal | 27 | 1.48 | 0.119 | 0.222 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.31 (+9) | 4 | +30 |
| normal | 40 | 1.00 | 0.080 | 0.150 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.24 (+7) | 6 | +30 |
| normal | 60 | 0.67 | 0.053 | 0.100 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.14 (+4) | 9 | +30 |
| normal | 80 | 0.50 | 0.040 | 0.075 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.10 (+2) | 14 | +30 |
| brave | 20 | 1.50 | 0.075 | 0.141 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.22 (+4) | 6 | +18 |
| brave | 27 | 1.48 | 0.074 | 0.139 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.22 (+4) | 6 | +18 |
| brave | 40 | 1.00 | 0.050 | 0.094 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.13 (+2) | 10 | +18 |
| brave | 60 | 0.67 | 0.033 | 0.063 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.08 (+1) | 18 | +18 |
| brave | 80 | 0.50 | 0.025 | 0.047 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.06 (+1) | 32 | +18 |
| honourable | 20 | 1.50 | 0.067 | 0.125 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.19 (+3) | 7 | +16 |
| honourable | 27 | 1.48 | 0.066 | 0.123 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.19 (+3) | 7 | +16 |
| honourable | 40 | 1.00 | 0.044 | 0.083 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.11 (+1) | 12 | +16 |
| honourable | 60 | 0.67 | 0.030 | 0.056 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.07 (+1) | 22 | +16 |
| honourable | 80 | 0.50 | 0.022 | 0.042 | nunca | 0.00 (+0) | 0.00 (+0) | nunca | 0.00 (+0) | 0.05 (+0) | 44 | +16 |

"+Golpear" y "+Noquear" son el primer golpe del dia. "Meseta" es la barra tras la ultima sesion de las 72 h y "maximo" la mas alta tras una sesion: con el tope diario un preso gana a lo sumo 0.35 por dia de juego, asi que la barra solo pasa de ese valor cuando lo que quedaba del dia anterior aun no se desvanecio. La ultima columna de sesiones es la regla sin limites diarios (Cheat, y Balanced antes de T2). El umbral para saturar a 1 golpe por hora sin limites es una ganancia > 0.0417 (lo que se desvanece en 1 h). Una barra llena tarda 24 h en irse si se deja de golpear. Golpear ademas hiere, asi que el preso golpeado pierde el bono de cuidado y suma el termino de heridas.

## 4. Otras rutas

### Field-yield (vencido en el campo, sin jaula; se abre con 2 derrotas)

Situacion +10 (militar/ley: -15). Nivel igual al lider. Recien vencido = worstDamage 0.8 (estres 0.28).

| Temperamento | 2 derrotas | 3 derrotas | 4 derrotas | 4 derr. +20 niveles | 2 derr. leal | 4 derr. leal |
|---|---|---|---|---|---|---|
| fearful | 95 | 95 | 95 | 93 | 93 | 95 |
| normal | 77 | 89 | 95 | 68 | 52 | 73 |
| brave | 50 | 57 | 65 | 35 | 25 | 40 |
| honourable | 42 | 49 | 55 | 25 | 17 | 30 |
| maniacal | 52 | 64 | 73 | 43 | 27 | 48 |

### Rescate (situacion 25 + 30 x curado)

Nivel igual al lider, sin derrotas, sano al despertar (estres 0), relacion desconocida salvo la ultima columna.

| Temperamento | curado 0 (+25) | curado 0.5 (+40) | curado 1 (+55) | curado 1, NPC +20 | curado 1, relacion -30 |
|---|---|---|---|---|---|
| fearful | 80 | 95 | 95 | 80 | 95 |
| normal | 60 | 75 | 90 | 60 | 83 |
| brave | 45 | 60 | 75 | 45 | 68 |
| honourable | 40 | 55 | 70 | 40 | 63 |
| maniacal | 35 | 50 | 65 | 35 | 58 |

### Duelo ganado (80+, TotalSparring)

chance = min(100, 25 + 20 x (victorias - 1)); si no se une, enfriamiento 24-72 h (80-99) o 24-120 h (100+).

| Victoria n | chance | unido para la n-esima (acumulado) |
|---|---|---|
| 1 | 25 | 25% |
| 2 | 45 | 59% |
| 3 | 65 | 86% |
| 4 | 85 | 98% |
| 5 | 100 | 100% |
| 6 | 100 | 100% |

### Aliados por nivel (relacion >= 20)

Cortes: medio 20, alto 50, elite 80, leyenda 100; descuento = min(tope, relacion/100 x 1.0); temporal si NPC - lider <= 15; temporal 3-10 dias.

| Tramo | tope | desc. relacion 20 | desc. relacion 50 | desc. relacion 100 |
|---|---|---|---|---|
| bajo (<20) | 20% | 20% | 20% | 20% |
| medio (20-49) | 20% | 20% | 20% | 20% |
| alto (50-79) | 10% | 10% | 10% | 10% |
| elite (80-99) | 10% | 10% | 10% | 10% |
| leyenda (100+) | 0% | 0% | 0% | 0% |

| Nivel NPC (lider 40, relacion 50) | good | neutral | greedy | bad |
|---|---|---|---|---|
| 10 | free | paid -20% | paid -20% | paid -20% |
| 30 | paid -20% | paid -20% | paid -20% | paid -20% |
| 60 | paid -10% | paid -10% | paid -10% | paid -10% |
| 85 | refuse | refuse | paid -10% | refuse |
| 105 | refuse | refuse | refuse | refuse |

| Nivel NPC (lider 80, relacion 50) | good | neutral | greedy | bad |
|---|---|---|---|---|
| 10 | free | paid -20% | paid -20% | paid -20% |
| 30 | paid -20% | paid -20% | paid -20% | paid -20% |
| 60 | paid -10% | paid -10% | paid -10% | paid -10% |
| 85 | temporary | temporary | paid -10% | refuse |
| 105 | refuse | refuse | refuse | refuse |

## 5. Restos de Cheat (esperas de 1 minuto)

`kCheatWaitHours` = 0.0167 h. El rechazo y el duelo se guardan como el MINUTO de juego en que terminan (`CooldownUntilMinute`, priority-one B): una espera de 1 h o mas termina en la hora entera redondeada hacia arriba (como antes), una menor de 1 h en el minuto redondeado hacia arriba. Las etiquetas muestran minutos bajo 1 h (`ShowWait`) y horas desde 1 h.

| Espera | Cheat (h) | empieza 10.25 h, termina | etiqueta |
|---|---|---|---|
| rechazo del sometido | 0.0167 | minuto 616 (1 min despues) | " (1 min)" |
| duelo perdido | 0.0167 | minuto 616 (1 min despues) | " (1 min)" |
| descanso Golpear | 0.0167 | (ledger en horas de juego) | "Golpear (1 min)" |
| descanso Noquear | 0.0167 | (ledger en horas de juego) | "Noquear (1 min)" |

Comparacion Balanced: el rechazo (72 h) empezado a las 10.25 h termina en el minuto 4980 (hora 83.00); Golpear en descanso pleno se lee "Golpear (1 h)".

## 6. Animal enjaulado: los golpes bajan la barra de comida (Balanced, priority-one D)

`DecideCagedAnimal` (`route_animal.cpp`): resolve = max(0.1, 1.00 + nivel x 0.020); descuento = clamp(0.50 x estres / resolve, 0, 0.90); barra = barra x (1 - descuento). La primera comida de mano del jugador sigue siendo obligatoria. Barra base = `TamingFullnessNeeded` de un adulto (madurez 1.0) frente al lider 40, sin temperamento. Estres por Golpear con temperamento normal (seccion 3).

| Nivel animal | resolve | +Golpear | barra base | estres 0.25 | estres 0.5 | estres 1.0 |
|---|---|---|---|---|---|---|
| 5 | 1.10 | 0.120 | 70% | 62% (-11%) | 54% (-23%) | 38% (-45%) |
| 20 | 1.40 | 0.120 | 85% | 77% (-9%) | 70% (-18%) | 55% (-36%) |
| 40 | 1.80 | 0.080 | 100% | 93% (-7%) | 86% (-14%) | 72% (-28%) |
| 60 | 2.20 | 0.053 | 100% | 94% (-6%) | 89% (-11%) | 77% (-23%) |
| 90 | 2.80 | 0.040 | 100% | 96% (-4%) | 91% (-9%) | 82% (-18%) |

Celdas: barra de estomago que debe alcanzar (descuento aplicado). Con `animalCageBeatingCounts` apagado el descuento es 0.

