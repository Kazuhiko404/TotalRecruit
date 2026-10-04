# TotalRecruit — Lista de tareas

Última actualización: 2026-10-04 (`main` @ `3554d19`, desplegado).

La foto completa del sistema (qué hace hoy el mod, qué falta, ideas) es
[odd/system-status.md](odd/system-status.md). Este archivo es solo la lista corta de lo que
**queda**. La configuración vive en el Mod Hub de Emkej (`src/mod_hub_integration.cpp`); no existe
un panel propio de ajustes.

---

## Abierto

- [ ] **Probar en juego el duelo con chance y el aviso diario** — QA.md 12.29 y 12.25 T12-T14
  (desplegado en `3554d19`, falta la prueba del usuario).
- [ ] **QA en partida** — [QA.md](QA.md) §12, en el orden de prioridad de su cabecera. Nada tiene
  marca formal de pasado. Lo de mayor riesgo: 12.6 I4/I5 (id estable tras recargar y relanzar).
- [ ] **Rebalanceo**: el nivel ahora es el nivel general de habilidad de TotalSparring (`7727771`);
  precios, tramos, chance, regateo, ayudante y doma se movieron con él. Revisar en juego.
- [ ] **R6.3** — secciones del hub (`register_setting_section`). Falta confirmar el layout del
  struct con el volcado de slots antes de llamarlo; llamar a ciegas crashea.
- [ ] **Puente de diálogo FCS** — `dialogue_bridge.cpp` compila pero nunca se ejecutó: hace falta
  un paquete de diálogo real en el FCS con la línea `totalrecruit accept`.
- [ ] **`messages.json` sin leer**: las secciones `refusal.*` y `currency.*` no las lee nadie.
  Cablearlas cambia lo que dicen los NPC: es una decisión de contenido, no un refactor.
- [ ] **Tramos de reputación literales** `75 / 25 / 5 / -5` (`relation_rules.cpp`): solo falta el nombre
  de config.
- [ ] **Anillo `mercy` <-> `offer_popup`**: el único anillo de includes que queda.
- [ ] **Persistir en el save** el reloj de encierro y los intentos de charla (hoy en memoria).

## Hecho desde 2026-10-01 (detalle en odd/system-status.md)

- [x] **I4** — resuelta: el mod ya no escribe ni lee la marca de derrota; el conteo vive en el save
  del personaje (`total_recruit.player_defeats`, recruit-gating T1; QA.md §9, REQUISITOS §7).
- [x] Precio, tarjeta al seleccionar, Hablar en hueco libre, ventana única, regateo, tirada diaria,
  Golpear / Noquear en jaula, animales con tu comida, evento del sometido, carteles sobre la cabeza,
  aliados por nivel, miembros temporales, ayudante gratis con contrato nativo, partida nativa del
  temporal, duelo vía TotalSparring con chance de unión, KenshiLib 0.5.0. Todo desplegado.

## Decisiones tomadas (para no reabrirlas)

- El precio **no** usa el nivel del jugador (ya domina la chance). Ver el documento de la feature.
- Modo fácil: 6 horas planas para todos, un atajo de respaldo (`easyHoursCaged`).
- Insistir a un prisionero en jaula no corta la charla (QA.md 12.7 N1).

## Entrega

`origin/main` es un commit squash ajeno (`6257bd6`): un push normal de `main` se rechaza. Cómo
entregar lo decide el usuario.
