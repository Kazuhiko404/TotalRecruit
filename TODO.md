# TotalRecruit — Lista de Tareas y Estado

Última actualización: 2026-09-06

---

## 1. Completado recientemente

- [x] **Panel de Configuración Ampliado y en Español** (`src/settings_panel.cpp`):
  - Dimensiones del panel agrandadas a `0.44f x 0.72f` (era `0.28f x 0.48f`), solucionando solapamientos de sliders y etiquetas.
  - Traducción al español de todos los títulos, opciones y sliders.
  - Persistencia automática de ajustes al cerrar con `SaveConfig(GetModDirectory())`.
- [x] **Flujo de Negociación y Soborno Dinámico** (`src/rules.cpp`):
  - NPCs libres con personalidades sobornables calculan precio dinámico según nivel del NPC vs líder.
  - Rechazo inmediato por desprecio si la diferencia de nivel es > 15 (`npcLevel - playerLevel > 15`).
  - Personalidades honorables/buenas nunca aceptan Cats (piden comida si están hambrientos o rehúsan).
  - Límite de insistencia: 3 intentos en menos de 2h de juego provocan 4h de enfriamiento donde el NPC ignora al jugador.
- [x] **Pacificación Inmediata en Reclutamiento y Despertar** (`src/recruit_core.cpp`, `src/rescue.cpp`):
  - Función `SafePacify`: llama `endCombatMode()`, `clearTempEnemyStatus(playerLeader)` y `reThinkCurrentAIAction()`.
  - Evita que personajes recién reclutados o curados/rescatados vuelvan a atacar inmediatamente al jugador.
- [x] **Sonido Nativo de Notificación de Kenshi** (`src/recruit_core.cpp`, `src/offer_popup.cpp`):
  - Reproduce `ou->playNotification(NULL)` al aparecer la ventana de oferta (`ShowOffer`) y al emitirse notificaciones en pantalla (`PostMessage`).
- [x] **Retratos de NPCs en Popup de Oferta** (`src/offer_popup.cpp`):
  - Resuelto mediante `PortraitManager::setImageWidgetNPC(Character*, MyGUI::ImageBox*)` para pintar la cara de personajes no pertenecientes a la facción del jugador.

---

- [x] **Corrección Crítica de Comida (Bug Traga-Armaduras)** (`src/feeding.cpp`, `src/rules.h`, `src/recruit_core.cpp`):
  - Fix: `feeding.cpp` comparaba `item->getItemType() == ITEM_FOOD`. Como `itemType::ARMOUR == 3` y `ItemFunction::ITEM_FOOD == 3`, ¡alimentaba al NPC con armaduras del jugador! Corregido a `item->itemFunction == ITEM_FOOD`.
  - Se agregó `verdict.wantsFood = condition.starving` para evitar que heridos no hambrientos exijan comida.
- [x] **Popup de Oferta con Texto Responsivo** (`src/offer_popup.cpp`):
  - Reemplazado skin `Kenshi_EditBox` por `Kenshi_WordWrapEmpty` permitiendo ajuste y visualización completa del texto sin cortes.
- [x] **Detección Rigurosa de Inconsciencia y Coma por Desangrado** (`src/rules.cpp`, `src/recruit_core.cpp`):
  - Kenshi no marca `isUnconcious()` durante un coma de recuperación por desangrado. Ampliado `SafeReadCondition` con `isInBloodlossTrauma()`, `knockoutTimer > 0.0f` y lectura pura de salud en jaulas (`medical->partHead`, `medical->partWorstTorso`) ya que la jaula fuerza animación despierta.
  - Corregido *fallthrough* en `SpeakOpeningLine` donde `Currency_None` saltaba a `Currency_Cats`, provocando que NPCs en coma dijeran "Tenemos un precio...".
- [x] **Sometimiento por Fuerza y Nivel Abrumador en Jaula** (`src/rules.cpp`):
  - Si el líder supera al cautivo por más de 40 niveles (`playerLevel - npcLevel >= 40`), el NPC en jaula se rinde y somete por intimidación inmediata, evitando bloqueos por honor.
  - Fix: el límite de insistencia ahora solo aplica a NPCs libres, para que los prisioneros no se harten de que les hables.
- [x] **Integración Nativa con Mod Hub (Emkej's Mod Core)** (`src/mod_hub_integration.cpp`):
  - Reemplazado menú de configuración standalone (`settings_panel.cpp`) por integración limpia en tiempo de ejecución con `Emkejs-Mod-Core.dll`.
  - El mod ahora se configura directo desde el "Mod Hub" del juego con sliders y checkboxes manejados externamente, sin romper la independencia (no hay dependencias de enlazado fuerte).
- [x] **Puente C++ para Diálogos Nativos FCS** (`src/dialogue_bridge.cpp`):
  - Lógica para interceptar `"totalrecruit accept"` en FCS construida y lista.

---

## 2. En Progreso / Tareas Pendientes (El Usuario)

- [ ] **Autoría del Paquete FCS (Dialogue Package)**:
  - El puente C++ está listo, pero el juego nunca lo va a disparar hasta que el usuario abra el Forgotten Construction Set (FCS) y arme un paquete de diálogo real con los nodos que el C++ espera.
- [ ] **Ajustes de Balance en Juego**:
  - Probar con distintos NPCs libres y prisioneros los nuevos umbrales de soborno dinámico y tiempos de ignorar tras insistencia.
