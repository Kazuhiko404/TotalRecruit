# TotalRecruit — Lista de tareas

Última actualización: 2026-10-06. `main` es la única rama; todo mergeado y desplegado (DLL en `3fb81b2`).
Foto completa (qué hace el mod, avance por feature, riesgos): [odd/system-status.md](odd/system-status.md).
La configuración vive en el Mod Hub de Emkej (`src/mod_hub_integration.cpp`, 183 controles).

---

## Avance

- Reglas, tarjeta, precio y estructura (rebalance-and-ui) — `38c7643`
- Precio, tarjeta al seleccionar, Hablar en hueco libre y al soltar — `f21474e`
- Derrotas en el save, field-yield, ventana única — `0c9f671`
- Tirada diaria y regateo — `3bf2e03`
- Golpear / Noquear en jaula, animales con tu comida, sometido que viene, carteles — `4b9cb9e`
- Aliados por nivel, miembro temporal, ayudante gratis — `ac17ed7`; tarjeta oculta en diálogo — `c6a3f2a`
- Duración de carteles 90 s / 25 s — `eac73f6`
- Duelo vía TotalSparring, ayudante con contrato nativo, partida nativa del temporal — `ac6ed10` / `fe8e490`
- Chance por duelo ganado y aviso diario del temporal — `3554d19`
- Golpe directo, arreglos del módulo de presos, estrés por golpes, cierre v1 (Cheat/Balanced, esperas de 1 min, horas de jaula guardadas) — `7bf8750`
- Insistencia solo por rechazos reales; en Cheat un animal alimentado se une — `c15675c`
- Animales: duelo 40+, 2/2 en Balanced, cura/derrota en Cheat — `588491c`
- Cheat: sirve cualquier jaula — `a2dacdd`; horas encerrado en la tarjeta — `a256a76`
- Oferta proactiva (viene a ofrecerse tras duelo o cura) — `5e93243`
- Duelos: Cheat a cualquier nivel, duelos de TotalSparring que cuentan, victorias mínimas — `1bb0de4`
- Golpear camina a la jaula, queja, aviso — `320983c` (probado informal: caminata, mensaje y notificación OK)
- Prioridad 1: sonido Wwise real `f7b719d`, duelo a 120 m `fc754c7`, esperas de Cheat en minutos `9b0466a`, frases de `messages.json` `20069d8`, golpes bajan la barra del animal enjaulado `65c6652`, puñetazo `badpunch` `fb536ff`
- Hoja de balance — `af1ff35`; compila y regenerada — `b8ca98c`

## Abierto

### 1. QA en juego (nada tiene marca formal de pasado)
- [ ] **12.39 + 12.40**: sonido del golpe, puñetazo, esperas en minutos, animal enjaulado golpeado,
  frases editables, duelo a 120 m. Es lo que el usuario está jugando (Cheat).
- [ ] **12.6 I4/I5**: id estable tras recargar y relanzar (el mayor riesgo).
- [ ] **12.28**: puente de duelos (única casilla sin marcar, `odd/tasks/ts-duel-bridge.md:25`).
- [ ] El resto de [QA.md](QA.md) §12, en el orden de odd/system-status.md.

### 2. Ajuste de Balanced (solo Balanced; el usuario juega Cheat) — odd/tasks/balanced-tuning.md
- [ ] T1 tabla objetivo de días hasta unirse (necesita el visto bueno del usuario).
- [ ] T2 rendimiento decreciente y tope diario de los golpes.
- [ ] T3 racha de días cuidado.
- [ ] T4 resignación por días encerrado.
- [ ] T6 revisar: cuidado/libertad siempre unen, nivel satura, únicos fuertes inalcanzables,
  field-yield casi seguro.
- [ ] T7 lo que queda: resistencia por especie (hoy por nivel). T5 hecho (prioridad 1 B).

### 3. Después
- [ ] Reglas y precios por personaje único.
- [ ] Bolsa de comida para animales (como TameBeasties).
- [ ] Secciones del Mod Hub (R6.3): bloqueado hasta tener el volcado de slots.

### Descartado / limpieza (sin valor para el jugador)
Puente de diálogo FCS (apagado a propósito, `dialogue_bridge.cpp:235`), config INI por capas,
tramos de reputación y regateo configurables, guardar intentos de charla, scroll del popup,
include mutuo `mercy` <-> `offer_popup`, el "would be copied" de `deploy.sh`, `--set-if` de
`add_missing_keys.py` que solo migra números, claves muertas desplegadas `easyHoursCaged` /
`factionWeight` (se pueden borrar a mano), lecturas por offset del ayudante nativo.

### Riesgos
- Sonido y puñetazo sin probar en juego.
- Si `getAnimationPlaying_animName` no ve el clip, el golpe cae en el acto (como antes).
- Miembro temporal por FCS quedaría permanente (no aplica con el puente apagado).
- Solapa con "THE Recruit Anyone" si están los dos.

## Decisiones tomadas (para no reabrirlas)

- El precio **no** usa el nivel del jugador (ya domina la chance).
- Balanced sin atajos de jaula: se quitó la espera fácil (`easyHoursCaged`) y "sano gracias a ti";
  cuidarlo da un bonus diario de 5 a 20.
- Dos presets: Cheat y Balanced (Vanilla+ retirado).
- Cheat: toda espera dura como mucho 1 minuto de juego; sirve cualquier jaula.
- Golpear es un golpe directo después de caminar y tirar el puñetazo: no saca al preso de la jaula.
- Los noqueos de la jaula suman estrés, no derrotas.
- Insistir a un prisionero en jaula no corta la charla (QA.md 12.7 N1).

## Entrega

`origin/main` es un commit squash ajeno (`6257bd6`): un push normal de `main` se rechaza. Cómo
entregar lo decide el usuario.
