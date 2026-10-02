# TotalRecruit — Lista de tareas

Última actualización: 2026-10-01 (ronda `rebalance-and-ui`, T1-T10).

Lo hecho está en [ESTADO.md](ESTADO.md); este archivo es solo lo que **queda**.
La configuración vive en el Mod Hub de Emkej (`src/mod_hub_integration.cpp`); no existe
un panel propio de ajustes.

---

## Abierto

- [ ] **QA en partida** — [QA.md](QA.md) §12, en el orden de prioridad de su cabecera.
  Nada de la ronda T1-T9 se jugó todavía.
- [ ] **I4** — ¿escribir la marca de derrota altera la IA? Instrumentada (`[I4 before-mark]` /
  `[I4 after-mark]`), falta observar en juego (QA.md §9).
- [ ] **R6.3** — secciones del hub (`register_setting_section`). Falta confirmar el layout del
  struct con el volcado de slots antes de llamarlo; llamar a ciegas crashea.
- [ ] **Puente de diálogo FCS** — `dialogue_bridge.cpp` compila pero nunca se ejecutó: hace falta
  un paquete de diálogo real en el FCS con la línea `totalrecruit accept`.
- [ ] **`messages.json` sin leer**: las secciones `refusal.*` y `currency.*` no las lee nadie;
  las líneas de apertura de los NPC son literales en `npc_speech.cpp` (igual que `DescribeOffer`,
  `HailLine` y los mensajes de piedad y golpiza). Cablearlas cambia lo que dicen los NPC:
  es una decisión de contenido, no un refactor.
- [ ] **Tramos de reputación literales** `75 / 25 / 5 / -5` (`relation_rules.cpp`): solo falta el nombre
  de config.
- [ ] **Anillo `mercy` <-> `offer_popup`**: la piedad abre la ventana y la ventana responde llamando a
  `AcceptMercy` / `GrantMercy`. Es el único anillo de includes que queda.
- [ ] **Afinar la tarjeta**: puntería del hover (radio de 48 px), ancla sobre la cabeza y que las líneas
  más largas quepan en 264 px (QA.md 12.8).
- [ ] **Persistir en el save** el conteo de derrotas y el reloj de encierro (hoy en memoria; el reloj
  reinicia al cargar a propósito). Revisar solo después de que pasen QA I4/I5.

## Decisiones tomadas (para no reabrirlas)

- El precio **no** usa el nivel del jugador (ya domina la chance). Ver el documento de la feature.
- Modo fácil: 6 horas planas para todos, un atajo de respaldo (`easyHoursCaged`).
- Insistir a un prisionero en jaula no corta la charla (QA.md 12.7 N1).

## Entrega

`origin/main` es un commit squash ajeno (`6257bd6`) con el mismo árbol que `4a23201`: un push normal se
rechaza. Cómo entregar la rama `feat/rebalance-and-ui` lo decide el usuario.
