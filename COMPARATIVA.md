> **HISTÓRICO — superado por [ESTADO.md](ESTADO.md).** Este documento describe el estado de
> septiembre y no se mantiene. Lo vigente (qué existe, qué se jugó y qué solo compila) está en
> `ESTADO.md`; las pruebas pendientes en `QA.md` §12.

# TotalRecruit frente a los cuatro mods de reclutamiento

Análisis contra `../Kenshi_Guia_Completa_Mods_Reclutamiento.md`, que documenta
Recruit ANYONE, Recruit Prisoners, Recruit Prisoners for Cats y TameBeasties.

Todo lo que se afirma de TotalRecruit está verificado contra el código, no
contra la memoria. Los archivos y líneas están citados para que se pueda
comprobar.

---

## 1. La diferencia de fondo

La guía dedica su capítulo 9 entero a explicar por qué hacen falta parches:

> *"Recruit Prisoners no necesariamente sabe automáticamente que ese NPC debe
> tener sus diálogos de reclutamiento. Por eso puede necesitar un patch."*

Y para animales lo dice sin rodeos: TameBeasties responde *"no conozco esta
criatura"*.

La causa es una sola: **esos mods preguntan si el NPC tiene el diálogo
instalado.** Un `DIALOGUE_PACKAGE` solo existe en los personajes que venían con
el mod, así que cualquier NPC de otro mod queda fuera hasta que alguien escriba
el parche.

TotalRecruit pregunta otra cosa: **¿esto es un `Character`?** El motor puebla
`NPCType` y `myPersonality` para todos, incluidos los de mods que no existían
cuando se escribió el plugin. Está confirmado jugando: el log muestra
`NPCType=OT_MILITARY personality=PT_DUMB` en NPCs de mods ajenos.

El capítulo 9 de la guía no nos aplica. No porque seamos más listos, sino
porque preguntamos otra cosa.

---

## 2. Lo que tenemos mejor

### 2.1 Cobertura sin parches

| | Los cuatro mods | TotalRecruit |
|---|---|---|
| NPC vanilla | sí | sí |
| NPC de otro mod | **necesita parche** | sí |
| Criatura de otro mod | **necesita parche** | sí |
| Mods que aún no existen | imposible | sí |

Esto no es una mejora incremental: es la razón de existir del proyecto.

### 2.2 Un solo sistema en vez de cuatro

La tabla del capítulo 7 de la guía reparte el coste entre mods distintos:

| Mod | Coste |
|---|---|
| Recruit ANYONE | generalmente no |
| Recruit Prisoners | variable según diálogo |
| Recruit Prisoners for Cats | sí, en cats |
| TameBeasties | confianza |

`ChooseCurrency` ([rules.cpp:454](src/rules.cpp#L454)) **es esa columna entera**,
elegida por NPC en vez de por mod instalado:

- esclavo → **libertad**
- hambriento o herido → **cuidados**
- honorable o bondadoso → **cuidados** (nunca dinero)
- bandido, comerciante, civil → **cats**
- militar o ley → **sumisión**
- animal → **cuidados**

Lo que la guía resuelve eligiendo mod, nosotros lo resolvemos leyendo al
personaje.

### 2.3 Convivencia

El capítulo 8 desaconseja combinar Recruit Prisoners con for Cats (el propio
autor lo dice), y también ANYONE con Prisoners: *"diálogos duplicados, opciones
que no aparecen, un sistema sobrescribiendo al otro"*.

TotalRecruit es un overlay sobre el menú contextual, con nombres de widget
propios y anclado **al lado** del menú y no debajo, donde LimbRemoval y
Loot-Scoot-Execute ya ponen los suyos. Convive con los cuatro.

### 2.4 Rutas que no pasan por la jaula

Toda la guía asume un único flujo, sin excepción:

```
capturar → jaula → despertar → diálogo → reclutar
```

Tres de nuestras cuatro rutas no tocan una jaula:

- **Rescate** — lo encontraste desangrándose y lo curaste. El motor lo recuerda
  como `LT_MY_LIFESAVER`, que **persiste en la partida**, así que la deuda
  sobrevive aunque te vayas antes de que despierte.
- **Rendición** — lo venciste y cede antes de morir.
- **Piedad** — perdió un miembro (`EV_LOST_ARM` / `EV_LOST_LEG`) y suplica.

Ninguno de los cuatro mods tiene esto, y no es un descuido suyo: el flujo de
jaula es lo único que el diálogo FCS puede expresar cómodamente.

### 2.5 Rechazos que explican, y que dicen cómo salir

Un diálogo FCS no puede explicarse a sí mismo: si la condición no se cumple, el
jugador ve otra línea, o ninguna. La guía lo refleja en su capítulo 10, donde el
procedimiento ante un diálogo que no aparece es *sacar de la jaula, volver a
meter, guardar, recargar, importar* — a ciegas.

Nosotros devolvemos siempre un motivo, y desde esta semana también **la salida**:

> *"sirve con lealtad a su facción: gana reputación, o encárcelalo y somételo"*

### 2.6 Tres respuestas donde los demás tienen una

La ruta de piedad ofrece **Aceptar / Perdonar / Rechazar**. Perdonar suelta el
agro y lo deja irse: no ganás nada. Es la única opción del sistema que existe
solo porque es coherente, no porque beneficie al jugador.

### 2.7 Textos editables

`messages.json` permite reescribir, traducir o ampliar cualquier línea sin
recompilar, con *fallback por clave*: una traducción a medias cuesta sabor,
nunca un recluta. Los cuatro mods requieren FCS para cambiar una palabra.

---

## 3. Lo que ellos tienen mejor

> **Desactualizado.** Los puntos 3.1 y 3.2 ya se resolvieron: hay curva de
> precios calibrada contra los valores publicados de *for Cats*, y el precio lee
> 39 estadísticas reales agrupadas por oficio, sombra y combate. Queda vigente
> solo 3.3. Ver [ESTADO.md](ESTADO.md) §4.

Acá conviene ser honesto: hay tres cosas donde la guía describe algo superior a
lo nuestro.

### 3.1 La escala de precios está un orden de magnitud por debajo

Los rangos de Recruit Prisoners for Cats (capítulo 5.4) contra los nuestros:

| Perfil | For Cats | TotalRecruit hoy |
|---|---|---|
| NPC débil | 1.000 – 6.000 | mínimo **250** |
| Bandido modesto | 6.000 – 25.000 | ~400 – 1.200 |
| Combatiente 40-50 | 30.000 – 50.000 | ~2.000 |
| Único muy fuerte | 100.000 – 150.000 | ~4.000 |

El log de la partida real lo confirma: `pide 1280 cats` para un guardia de
nivel 30.

Con esos números el dinero no es un precio, es vuelto. La idea central de for
Cats — *"capturar a alguien no debería significar que puedes obtener gratis un
personaje muy fuerte"* — no llega a cumplirse con nuestra escala.

**Es configurable sin recompilar**: `catsPerLevel` y `catsMinimum` son claves de
`config.json`.

### 3.2 El precio no lee las estadísticas reales

Capítulo 5.7, y es la mejor idea de la guía:

> *"Un precio puede parecer alto o bajo por estadísticas que no son obvias, como
> combate o sigilo... no hay que asumir que dos NPCs que parecen iguales
> necesariamente cuestan lo mismo."*

`PriceInCats` ([rules.cpp:429](src/rules.cpp#L429)) usa `getRoughLevel()`, que es
un **promedio**. Dos NPCs de nivel 30 cuestan lo mismo aunque uno sea un
asesino y el otro un panadero armado.

`CharStats::getStat(StatsEnumerated, bool)` está exportado y disponible
(`CharStats.h:81`), y `MELEE_ATTACK`, `STEALTH_KILL` y compañía están en
`Enums.h`. Es alcanzable, y daría precios que **cuentan una historia**.

### 3.3 Precios propios para personajes concretos

La guía menciona que algunos líderes y únicos tienen precios propios. Nosotros
tenemos `isUnique()` y `unique_extra_hours`, pero no una tabla por nombre.
`character->data->stringID` da el registro exacto de FCS, así que una tabla en
`messages.json` o en el config sería directa.

---

## 4. Lo que nos falta y ellos tampoco tienen

- **El puente FCS sigue sin ejercitarse.** Está escrito y compila, pero ningún
  paquete de diálogo lo ha disparado. Mientras tanto la ventana propia hace el
  trabajo, así que no es un bloqueo — pero tampoco está probado.
- **Bolsa de comida para animales.** TameBeasties incluye una (capítulo 6.4). Un
  animal reclutado hay que gestionarlo, y ahí no ayudamos.
- **Import y datos.** El capítulo 10 entero (guardar, recargar, importar) no nos
  aplica: no tocamos datos del juego. Vale anotarlo como ventaja, no como falta.

---

## 5. Resumen

**Mejor que ellos:** cobertura universal sin parches, un solo sistema con cuatro
monedas, convivencia con todo, tres rutas sin jaula, rechazos que explican y
nombran la salida, textos editables sin FCS.

**Peor que ellos:** la escala de precios es demasiado barata, el precio no
distingue a un asesino de un panadero, y no hay precios por personaje.

**Sin probar:** el puente FCS.

La lectura corta: el sistema está mejor construido, y **peor calibrado**. Lo
primero costó meses y no se puede copiar; lo segundo son dos números en un panel
y una lectura de estadísticas.

---

## 6. Si hubiera que priorizar

1. **Subir la escala de cats.** Sin recompilar, desde `config.json`. Es el cambio con
   mejor relación esfuerzo/efecto de toda esta lista.
2. **Precio por estadísticas reales.** Convierte el precio en información sobre
   el personaje, que es lo que for Cats hace bien.
3. **Tabla de precios por `stringID`.** Solo cuando las dos anteriores estén
   asentadas.
4. **Ejercitar el puente FCS.** Depende de que exista un paquete en FCS.
