# Hoja de balance (Balanced, config por defecto)

> **Generado, no editar a mano.** Programa: `tests/balance_sheet/main.cpp`, que llama a las funciones puras REALES de `src/` con `Config()` por defecto.
> Reglas de `src/` en el commit `3fb81b2`.
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
| Nivel | `ChanceFromLevelGap` | base 45; NPC arriba: -1.5/nivel (piso 2); NPC abajo: +1.5/nivel (tope +40) | 2 | 85 |
| Relacion | `ChanceFromRelation` | relacion x 0.25 (no en la jaula ni en field-yield) | -25 | 25 |
| Personalidad | `ChanceFromPersonality` | `chance.personality[]` (tabla abajo) | -25 | 20 |
| Derrotas | `ChanceFromDefeats` | 12/derrota / resolve, tope 45 | 0 | 45 |
| Estres | `ChanceFromStress` | estres(0..1) x 45 / resolve, tope 45 | 0 | 45 |
| Situacion: jaula | `intimidationBonus` | fijo | 15 | 15 |
| Situacion: field-yield | `fieldYieldBonus` - lealtad | 10 (militar/ley: -25) | -15 | 10 |
| Situacion: rescate | `rescueBaseBonus` + curado x `rescueHealWeight` | 25 + 30 x curado(0..1) | 25 | 55 |
| Cuidado (jaula) | `CaredForBonus` | sorteo diario estable, solo sano+alimentado+vida plena | 5 | 20 |
| Rencor | `grudgePenalty` | fijo (honorable/valiente/amable: rechazo absoluto) | -20 | 0 |
| Unico | `uniquePenalty` | fijo | -25 | 0 |

### Personalidad y resolve

| Temperamento | Personalidad | Resolve (divide estres, derrotas y golpes) | Estres pleno vale | Grupo |
|---|---|---|---|---|
| normal | +0 | 1.0 | +45 | neutral |
| fearful | +20 | 0.6 | +45 | neutral |
| traitorous | +15 | 1.0 | +45 | greedy |
| dumb | +10 | 0.6 | +45 | neutral |
| smart | -5 | 1.0 | +45 | greedy |
| brave | -15 | 1.6 | +28 | neutral |
| honourable | -20 | 1.8 | +25 | good |
| maniacal | -25 | 1.0 | +45 | bad |
| cold/cruel | +0 | 1.0 | +45 | bad |
| warm/kind | +0 | 1.0 | +45 | good |

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
| Nivel | 85 | 85 | 85 | 75 | 60 | 45 | 30 | 15 | 2 | 2 | 2 | 2 | 

## 2. Jaula: probabilidad diaria de unirse (Balanced)

Lider 40. Intimidacion +15 siempre. Columnas: **sano** = estres 0 sin bono (referencia; en el juego un preso sano, alimentado y a vida plena SIEMPRE tiene el bono de cuidado); **cuid. min/max** = sano + bono 5 / 20; **hambre** = lleno 0; **herido** = worstDamage 0.8; **H+H+C** = hambre + herido + agotado (estres 0.93); **golpes** = barra de golpes 1.0, por lo demas sano (sin bono: los golpes hieren); **quebrado** = todo junto (estres 1.0). Entre parentesis: dias hasta 90% acumulado.

| Temperamento | Unico | Nivel NPC | sano | cuid. min | cuid. max | hambre | herido | H+H+C | golpes | quebrado |
|---|---|---|---|---|---|---|---|---|---|---|
| fearful | no | 20 (-20) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) |
| fearful | no | 40 (+0) | 80 (2) | 85 (2) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) |
| fearful | no | 60 (+20) | 50 (4) | 55 (3) | 70 (2) | 87 (2) | 71 (2) | 95 (1) | 95 (1) | 95 (1) |
| fearful | no | 90 (+50) | 37 (5) | 42 (5) | 57 (3) | 74 (2) | 58 (3) | 82 (2) | 82 (2) | 82 (2) |
| normal | no | 20 (-20) | 90 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) |
| normal | no | 40 (+0) | 60 (3) | 65 (3) | 80 (2) | 82 (2) | 72 (2) | 95 (1) | 95 (1) | 95 (1) |
| normal | no | 60 (+20) | 30 (7) | 35 (6) | 50 (4) | 52 (4) | 42 (5) | 71 (2) | 75 (2) | 75 (2) |
| normal | no | 90 (+50) | 17 (13) | 22 (10) | 37 (5) | 39 (5) | 29 (7) | 58 (3) | 62 (3) | 62 (3) |
| brave | no | 20 (-20) | 75 (2) | 80 (2) | 95 (1) | 89 (2) | 82 (2) | 95 (1) | 95 (1) | 95 (1) |
| brave | no | 40 (+0) | 45 (4) | 50 (4) | 65 (3) | 59 (3) | 52 (4) | 71 (2) | 73 (2) | 73 (2) |
| brave | no | 60 (+20) | 15 (15) | 20 (11) | 35 (6) | 29 (7) | 22 (10) | 41 (5) | 43 (5) | 43 (5) |
| brave | no | 90 (+50) | 5 (45) | 7 (32) | 22 (10) | 16 (14) | 9 (25) | 28 (8) | 30 (7) | 30 (7) |
| honourable | no | 20 (-20) | 70 (2) | 75 (2) | 90 (1) | 82 (2) | 77 (2) | 93 (1) | 95 (1) | 95 (1) |
| honourable | no | 40 (+0) | 40 (5) | 45 (4) | 60 (3) | 52 (4) | 47 (4) | 63 (3) | 65 (3) | 65 (3) |
| honourable | no | 60 (+20) | 10 (22) | 15 (15) | 30 (7) | 22 (10) | 17 (13) | 33 (6) | 35 (6) | 35 (6) |
| honourable | no | 90 (+50) | 5 (45) | 5 (45) | 17 (13) | 9 (25) | 5 (45) | 20 (11) | 22 (10) | 22 (10) |
| fearful | si | 20 (-20) | 85 (2) | 90 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) | 95 (1) |
| fearful | si | 40 (+0) | 55 (3) | 60 (3) | 75 (2) | 92 (1) | 76 (2) | 95 (1) | 95 (1) | 95 (1) |
| fearful | si | 60 (+20) | 25 (9) | 30 (7) | 45 (4) | 62 (3) | 46 (4) | 70 (2) | 70 (2) | 70 (2) |
| fearful | si | 90 (+50) | 12 (19) | 17 (13) | 32 (6) | 49 (4) | 33 (6) | 57 (3) | 57 (3) | 57 (3) |
| normal | si | 20 (-20) | 65 (3) | 70 (2) | 85 (2) | 87 (2) | 77 (2) | 95 (1) | 95 (1) | 95 (1) |
| normal | si | 40 (+0) | 35 (6) | 40 (5) | 55 (3) | 57 (3) | 47 (4) | 76 (2) | 80 (2) | 80 (2) |
| normal | si | 60 (+20) | 5 (45) | 10 (22) | 25 (9) | 27 (8) | 17 (13) | 46 (4) | 50 (4) | 50 (4) |
| normal | si | 90 (+50) | 5 (45) | 5 (45) | 12 (19) | 14 (16) | 5 (45) | 33 (6) | 37 (5) | 37 (5) |
| brave | si | 20 (-20) | 50 (4) | 55 (3) | 70 (2) | 64 (3) | 57 (3) | 76 (2) | 78 (2) | 78 (2) |
| brave | si | 40 (+0) | 20 (11) | 25 (9) | 40 (5) | 34 (6) | 27 (8) | 46 (4) | 48 (4) | 48 (4) |
| brave | si | 60 (+20) | 5 (45) | 5 (45) | 10 (22) | 5 (45) | 5 (45) | 16 (14) | 18 (12) | 18 (12) |
| brave | si | 90 (+50) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) |
| honourable | si | 20 (-20) | 45 (4) | 50 (4) | 65 (3) | 57 (3) | 52 (4) | 68 (3) | 70 (2) | 70 (2) |
| honourable | si | 40 (+0) | 15 (15) | 20 (11) | 35 (6) | 27 (8) | 22 (10) | 38 (5) | 40 (5) | 40 (5) |
| honourable | si | 60 (+20) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 8 (28) | 10 (22) | 10 (22) |
| honourable | si | 90 (+50) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) | 5 (45) |

### Cuidar contra quebrar (mismo NPC, nivel igual al lider)

| Temperamento | Unico | cuidado medio (+12) | quebrado | ventaja de quebrar | dias 90% cuidando | dias 90% quebrando |
|---|---|---|---|---|---|---|
| fearful | no | 92 | 95 | +3 | 1 | 1 |
| normal | no | 72 | 95 | +23 | 2 | 1 |
| brave | no | 57 | 73 | +16 | 3 | 2 |
| honourable | no | 52 | 65 | +13 | 4 | 3 |
| fearful | si | 67 | 95 | +28 | 3 | 1 |
| normal | si | 47 | 80 | +33 | 4 | 2 |
| brave | si | 32 | 48 | +16 | 6 | 4 |
| honourable | si | 27 | 40 | +13 | 8 | 5 |

El bono de cuidado maximo (+20) vale 44% de lo que vale el estres pleno de un normal (+45).

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

ganancia = clamp(base x factor de nivel / resolve, 0.02, 0.25); base Golpear 0.08, Noquear 0.15; factor = lider/NPC acotado a [0.5, 1.5]; descanso Golpear 1 h, Noquear 1 h; una barra llena se va en 24 h (-0.0417/h). Simulacion: un golpe (y/o un noqueo) cada vez que el descanso termina, 72 h seguidas, lider 40.

| Temperamento | Nivel NPC | factor | +Golpear | +Noquear | sesiones a 1.0 (solo Golpear) | meseta solo Golpear | a las 24 h solo Golpear | sesiones a 1.0 (Golpear+Noquear) | estres pleno vale |
|---|---|---|---|---|---|---|---|---|---|
| fearful | 20 | 1.50 | 0.200 | 0.250 | 7 | 1.00 (+45) | 0.96 (+45) | 3 | +45 |
| fearful | 27 | 1.48 | 0.198 | 0.250 | 7 | 1.00 (+45) | 0.96 (+45) | 3 | +45 |
| fearful | 40 | 1.00 | 0.133 | 0.250 | 11 | 1.00 (+45) | 0.96 (+45) | 3 | +45 |
| fearful | 60 | 0.67 | 0.089 | 0.167 | 21 | 1.00 (+45) | 0.96 (+45) | 5 | +45 |
| fearful | 80 | 0.50 | 0.067 | 0.125 | 39 | 1.00 (+45) | 0.60 (+44) | 7 | +45 |
| normal | 20 | 1.50 | 0.120 | 0.225 | 13 | 1.00 (+45) | 0.96 (+43) | 4 | +45 |
| normal | 27 | 1.48 | 0.119 | 0.222 | 13 | 1.00 (+45) | 0.96 (+43) | 4 | +45 |
| normal | 40 | 1.00 | 0.080 | 0.150 | 25 | 1.00 (+45) | 0.92 (+41) | 6 | +45 |
| normal | 60 | 0.67 | 0.053 | 0.100 | nunca | 0.88 (+39) | 0.28 (+12) | 9 | +45 |
| normal | 80 | 0.50 | 0.040 | 0.075 | nunca | 0.04 (+1) | 0.00 (+0) | 14 | +45 |
| brave | 20 | 1.50 | 0.075 | 0.141 | 29 | 1.00 (+28) | 0.80 (+22) | 6 | +28 |
| brave | 27 | 1.48 | 0.074 | 0.139 | 30 | 1.00 (+28) | 0.78 (+21) | 6 | +28 |
| brave | 40 | 1.00 | 0.050 | 0.094 | nunca | 0.64 (+18) | 0.20 (+5) | 10 | +28 |
| brave | 60 | 0.67 | 0.033 | 0.063 | nunca | 0.03 (+0) | 0.00 (+0) | 18 | +28 |
| brave | 80 | 0.50 | 0.025 | 0.047 | nunca | 0.02 (+0) | 0.00 (+0) | 32 | +28 |
| honourable | 20 | 1.50 | 0.067 | 0.125 | 39 | 1.00 (+25) | 0.60 (+15) | 7 | +25 |
| honourable | 27 | 1.48 | 0.066 | 0.123 | 40 | 1.00 (+25) | 0.58 (+14) | 7 | +25 |
| honourable | 40 | 1.00 | 0.044 | 0.083 | nunca | 0.24 (+6) | 0.07 (+1) | 12 | +25 |
| honourable | 60 | 0.67 | 0.030 | 0.056 | nunca | 0.03 (+0) | 0.00 (+0) | 22 | +25 |
| honourable | 80 | 0.50 | 0.022 | 0.042 | nunca | 0.02 (+0) | 0.00 (+0) | 44 | +25 |

El umbral para saturar a 1 golpe por hora es una ganancia > 0.0417 (lo que se desvanece en 1 h). Una barra llena tarda 24 h en irse si se deja de golpear. Golpear ademas hiere, asi que el preso golpeado pierde el bono de cuidado y suma el termino de heridas.

## 4. Otras rutas

### Field-yield (vencido en el campo, sin jaula; se abre con 2 derrotas)

Situacion +10 (militar/ley: -15). Nivel igual al lider. Recien vencido = worstDamage 0.8 (estres 0.28).

| Temperamento | 2 derrotas | 3 derrotas | 4 derrotas | 4 derr. +20 niveles | 2 derr. leal | 4 derr. leal |
|---|---|---|---|---|---|---|
| fearful | 95 | 95 | 95 | 95 | 95 | 95 |
| normal | 91 | 95 | 95 | 82 | 66 | 87 |
| brave | 62 | 69 | 77 | 47 | 37 | 52 |
| honourable | 55 | 62 | 68 | 38 | 30 | 43 |
| maniacal | 66 | 78 | 87 | 57 | 41 | 62 |

### Rescate (situacion 25 + 30 x curado)

Nivel igual al lider, sin derrotas, sano al despertar (estres 0), relacion desconocida salvo la ultima columna.

| Temperamento | curado 0 (+25) | curado 0.5 (+40) | curado 1 (+55) | curado 1, NPC +20 | curado 1, relacion -30 |
|---|---|---|---|---|---|
| fearful | 90 | 95 | 95 | 90 | 95 |
| normal | 70 | 85 | 95 | 70 | 93 |
| brave | 55 | 70 | 85 | 55 | 78 |
| honourable | 50 | 65 | 80 | 50 | 73 |
| maniacal | 45 | 60 | 75 | 45 | 68 |

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

