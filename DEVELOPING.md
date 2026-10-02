# Desarrollo — TotalRecruit

Guía para trabajar en el plugin: compilar, desplegar, saber dónde mirar y qué
hacer cuando algo falla.

El [README.md](README.md) es para el jugador. Este es para quien toca el código.

---

## 1. El ciclo, completo

**Cada cambio son tres pasos, y ninguno es opcional: compilar → copiar → verificar.**

```bash
cd /f/Proyectos/Kenshi-mods/TotalRecruit

MSBUILD="/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"
"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo

DST="/d/SteamLibrary/steamapps/common/Kenshi/mods/TotalRecruit"
cp x64/Release/TotalRecruit.dll TotalRecruit.dll
cp x64/Release/TotalRecruit.dll "$DST/TotalRecruit.dll"

md5sum x64/Release/TotalRecruit.dll "$DST/TotalRecruit.dll"
```

Los dos hashes tienen que ser **idénticos**. Si no lo son, estás probando el
build anterior.

### Las tres rutas

| Qué | Dónde |
|---|---|
| Salida del compilador | `x64/Release/TotalRecruit.dll` |
| Copia del repo (la que se commitea) | `TotalRecruit.dll` |
| **El juego** | `d:\SteamLibrary\steamapps\common\Kenshi\mods\TotalRecruit\` |

La carpeta desplegada necesita cuatro archivos:

```
d:\SteamLibrary\steamapps\common\Kenshi\mods\TotalRecruit\
├── TotalRecruit.dll     <- cambia en cada build
├── TotalRecruit.mod     <- cabecera de 46 bytes, no se toca
├── RE_Kenshi.json       <- le dice a RE_Kenshi qué DLL cargar
└── config.json          <- copiar desde TotalRecruit/config.json cuando cambie
```

**Ojo con el `config.json` fuente**: vive en `TotalRecruit/config.json` — la
subcarpeta, no la raíz del proyecto. Si agregás una clave, copiala también:

```bash
cp TotalRecruit/config.json "$DST/config.json"
```

### Por qué la verificación no se saltea

**Kenshi bloquea el DLL mientras corre.** Con el juego abierto, el `cp` falla
con `Device or resource busy` — y si no mirás la salida, seguís probando el
binario viejo y depurando un bug que ya arreglaste. Ya pasó.

Si aparece ese error: cerrá Kenshi (no alcanza con volver al menú principal),
copiá de nuevo, y recién ahí abrí el juego.

---

## 2. Requisitos del build

- Visual Studio con **Platform Toolset v100** — no es negociable, el SDK está
  compilado contra esa ABI
- Boost 1.60
- Variables de entorno `KENSHILIB_DIR` y `BOOST_INCLUDE_PATH`
- **Release | x64 únicamente.** Debug no linkea contra KenshiLib.

Los warnings `C4482` y `C4091` vienen de los headers de KenshiLib, no del
código propio. Son ruido esperado.

---

## 3. Dónde mirar el código

`src/` tiene ~18 módulos. Cada `.h` arranca con un comentario que explica por
qué existe — leelo antes que el `.cpp`.

### Entrada

| Archivo | Qué hace |
|---|---|
| `TotalRecruit.cpp` | `startPlugin` / `stopPlugin`. Instala y desinstala todo. |
| `common.h` | Includes compartidos. **El orden importa** (Ogre vs. MyGUI). |
| `config.cpp/.h` | Lee y guarda `config.json` con una sola tabla de claves (`ConfigFields`): `SaveConfig` **mezcla** en el archivo existente, no lo reescribe. |
| `json_merge.cpp/.h` | **Puro.** El merge de texto JSON: reemplaza valores de claves conocidas en su lugar, agrega las que faltan, deja intacto el resto (comentarios, claves ajenas, orden). |
| `atomic_file.cpp/.h` | Escritura atómica (archivo temporal + reemplazo); lo usa `SaveConfig`. Se prueba contra el disco real en `rules_tests`. |
| `log.cpp/.h` | Todo va al log de RE_Kenshi con prefijo. |

### La decisión

| Archivo | Qué hace |
|---|---|
| `rules.cpp/.h` | **El motor.** Qué quiere un NPC antes de unirse. Empezá acá. Lee el juego y arma los hechos. |
| `route_rules.cpp/.h` | **Puro.** Qué ruta contesta primero (`ChooseEntryRoute`). El orden de precedencia vive solo acá. |
| `chance_rules.cpp/.h` | **Puro.** El motor de chance (`JoinChance`, `ChanceFrom*`, el roll estable). |
| `price_rules.cpp/.h` | **Puro.** La curva de precio, los multiplicadores (talento, personalidad, facción) y el precio final: redondeo, mínimo y tope (`FinalPriceInCats`). |
| `money_rules.cpp/.h` | **Puro.** Cobrar y mover cats entre bolsas (`ChargePrice`, `TransferCats`): todo o nada, sin acuñar ni perder cats si un paso falla. |
| `cats_purse.cpp/.h` | La bolsa respaldada por el motor (`CharacterPurse`): el único lugar que conoce `getMoney` / `takeMoney` / `Ownerships::addMoney`. |
| `char_key.h`, `temperament.h` | **Puros.** Identidad de personaje y el enum de personalidad sin motor. `temperament_map.cpp` hace el único mapeo desde `PT_*`. |
| `recruit_core.cpp/.h` | Resuelve el target del click, valida y ejecuta el reclutamiento. |
| `rescue.cpp` | Ruta A — lo salvaste de morir. |
| `surrender.cpp` | Ruta C — se rinde. Declara el método de `AI` localmente. |
| `feeding.cpp` | Darle comida de tu mochila. |

### Lo que ve el jugador

| Archivo | Qué hace |
|---|---|
| `recruit_overlay.cpp` | El botón junto al menú contextual (dos hooks). |
| `offer_popup.cpp` | La ventana de decisión. Acá vive el retrato. |
| `hail.cpp` | Los cautivos avisan, programado sobre el reloj de juego. |
| `dialogue_authoring.cpp` | Escribe las líneas y las burbujas. |
| `mercy.cpp` | Ruta piedad: al perder un miembro, tres respuestas. |
| `mod_hub_integration.cpp` | Ajustes en el panel de Mod Hub (`src/emc/` es su API). |
| `spawn_panel.cpp` / `spawn_core.cpp` | Panel de spawn. |

### Soporte

| Archivo | Qué hace |
|---|---|
| `cursor_hint.cpp` | El globo de chat vanilla sobre NPCs hablables. |
| `frame_tick.cpp` | Callback por frame, hookeado a `ForgottenGUI::update`. |
| `probe.cpp` | Diagnóstico. Con `probeOnClick` loguea todo del NPC clickeado. |
| `dialogue_bridge.cpp` | Puente a diálogo FCS. **Compila pero nunca se ejecutó.** |
| `game_data.cpp` | Listados de escuadras y personajes. |

Los módulos marcados **Puro** no incluyen headers del motor: reciben valores
planos y los structs de `config.h`. Eso es lo que permite compilarlos en el
ejecutable de tests (sección 7).

### Los documentos antes que el código

1. **`../KENSHI-MODDING-NOTES.md` §11** — las trampas del SDK. Cada una costó
   un crash o una sesión de debug. Leelo antes de tocar nada.
2. **`PLAN.md`** — el diseño y las APIs verificadas. La **§8** separa lo
   confirmado jugando de lo que solo compila.
3. Los headers en `../KenshiLib_Examples_deps/kenshilib/Include/kenshi/` — la
   única fuente de verdad sobre qué existe.

---

## 4. Cuando algo falla

### Primero: el log

```
d:\SteamLibrary\steamapps\common\Kenshi\RE_Kenshi_log.txt
```

**No es `kenshi.log`.** Ese es otro archivo y no tiene nada nuestro.

Para diagnosticar, poné `"probeOnClick": true` en `config.json`: loguea traits,
estado médico y el veredicto de cada click.

### El mod no carga

1. ¿El md5 del desplegado coincide con el del build?
2. ¿Los exports están mangled?

```bash
DUMPBIN="/c/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe"
"$DUMPBIN" //EXPORTS x64/Release/TotalRecruit.dll | grep -i plugin
```

Tiene que salir `?startPlugin@@YAXXZ` y `?stopPlugin@@YAXXZ`. Si salen pelados
(`startPlugin` a secas), alguien puso `extern "C"` y RE_Kenshi no los va a
encontrar nunca.

### El juego crashea

Sospechá de un `virtual` de motor declarado en código nuestro. Es la primera
trampa de §11 y **la única que se lleva el proceso**: construís una vtable
propia, el método queda en el slot 0, y la llamada salta a otra función. El
guard SEH no ayuda — no es una lectura inválida, es una llamada válida a la
función equivocada.

Solución: llamar al thunk `_NV_` (no virtual, exportado, mismo código) o leer
el campo en su offset documentado.

### `C2011: redefinición de tipo`

Dos headers del SDK definen el mismo tipo sin guardas. Conocidos: `ContextMenu`,
`BuildingDesignation` (`Platoon.h` / `Building/Building.h`) y `CharacterMessage`
(`Character.h` / `AI/AI.h`).

Renombrar con macro **no funciona** — los enumeradores chocan igual con `C2365`.
Lo que sí: no incluir el header y declarar a mano el método no virtual que
necesitás, **a scope global** (dentro de un namespace mangla distinto y no
linkea). Ver `surrender.cpp`.

### `C2712: no se puede usar __try en funciones que requieren desenrollado`

MSVC cuenta *cada* objeto C++ del frame entero, incluidos temporales implícitos
de MyGUI. Cuentan también un `hand&` devuelto por un getter y cualquier
`std::string` — hasta uno construido dentro de un `LogInfo`.

Solución: cada guard SEH en su propia función con frame **solo POD**. El patrón
está por todos lados: `LoadPortrait` hace el trabajo, `GuardedLoadPortrait` pone
el `TR_TRY`, `SafeLoadPortrait` reporta afuera.

### `C2022: demasiado grande para el carácter`

Toolset v100 lee el fuente como Latin-1, así que los acentos van como escapes
UTF-8 en hex (`\xc3\xad` es `í`). El escape se come el dígito hex que le sigue:

```cpp
"Todav\xc3\xada no."      // C2022
"Todav\xc3\xad" "a no."   // bien: cortado con concatenación
```

La salida más simple: escribir el texto sin acentos.

### Un símbolo no linkea

Que el header lo declare no garantiza que exista en la lib. Verificá:

```bash
cd /f/Proyectos/Kenshi-mods/KenshiLib_Examples_deps/kenshilib/Libraries
"$DUMPBIN" //LINKERMEMBER:1 KenshiLib.lib | grep -o "?[A-Za-z_]*@ClaseQueSea@@[^ ]*" | sort -u
```

En el nombre mangled, `QEAA` = miembro **no virtual** — seguro de llamar.

Dato que vale oro: los comentarios `// private` de los headers son
documentación del fuente original, **no C++ real**. Varias clases tienen un solo
`public:` que cubre todo, así que esos métodos se pueden llamar igual. Así se
encontró `PortraitManager::setImageWidgetNPC`.

### Desde Git Bash: la doble barra

`dumpbin` necesita **dos** barras en los flags:

```bash
"$DUMPBIN" //EXPORTS archivo.dll        # bien
"$DUMPBIN" /EXPORTS  archivo.dll        # LNK1181: Git Bash lo hace ruta Windows
```

Y el `cd` **no persiste** entre llamadas: cada comando arranca donde estaba.
Usá rutas absolutas o encadená con `&&`.

---

## 5. Cómo buscar en las fuentes

Buscar bien acá es la mitad del trabajo. El SDK no tiene documentación: los
headers son lo que hay, y la respuesta a "¿esto se puede hacer?" casi siempre
está en un plugin que alguien ya escribió.

**Usá `rg` (ripgrep), no `grep`.** Respeta `.gitignore`, no entra a `.git/` y
recorre las 14 fuentes en segundos.

### El mapa: qué hay en `f:\Proyectos\Kenshi-mods\`

**El SDK — la verdad sobre qué existe**

| Ruta | Qué es |
|---|---|
| `KenshiLib_Examples_deps/kenshilib/Include/kenshi/` | **160 headers.** El motor entero. Empezá siempre acá. |
| `KenshiLib_Examples_deps/kenshilib/Include/kenshi/gui/` | GUI: `PortraitManager`, `DatapanelGUI`, `ForgottenGUI`, `ContextMenu` |
| `KenshiLib_Examples_deps/kenshilib/Include/kenshi/util/` | `hand`, `lektor`, y los tipos que aparecen en todas las firmas |
| `KenshiLib_Examples_deps/kenshilib/Include/kenshi/AI/`, `Building/`, `combat/` | Subsistemas (ojo: `AI.h` y `Building.h` tienen tipos duplicados, §4) |
| `KenshiLib_Examples_deps/kenshilib/Libraries/KenshiLib.lib` | Los símbolos que **realmente** linkean |

El resto de `KenshiLib_Examples_deps/` es Boost: ~11.400 de los 11.567 archivos.
**Excluílo siempre** o cada búsqueda devuelve basura.

**Plugins de referencia — cómo se hace en la práctica**

| Ruta | Archivos | Para qué sirve |
|---|---|---|
| `KenshiExtensionPlugin/` (KEP) | 110 | El más completo. Diálogo, jaulas, paneles, hooks. |
| `Shutori_Re_Kenshi_Source/` | 92 | UI grande y `PrisonerUtil.cpp` (1359 líneas de jaulas). |
| `re_kenshi-personal-projects/` | 44 | **Trae `wiki/` y `docs/`** — ver abajo. |
| `RE_Kenshi_Plugins_Source/` | 8 | Ocho plugins chicos y completos. Buen molde para empezar. |
| `KenshiLib_Examples/` | 8 | Ejemplos oficiales. `Dialogue/Dialogue.cpp` es la plantilla del hook. |
| `UnlimitedRecruitV2/` | 16 | El padre de este mod. |
| `Loot-Scoot-Execute/`, `LimbRemoval/` | 6 / 1 | Chicos. Anclan overlays al menú contextual — por eso no usamos ese espacio. |
| `KenshiLua/` | 936 + 13 lua | Bindings Lua. **`docs/BindingsReference.md` documenta APIs en prosa.** |

**Mods instalados en el juego — la otra mitad del mapa**

Hay mods que no viven en el repo sino en la carpeta del juego, y algunos son
referencia valiosa aunque vengan sin fuentes:

```
d:\SteamLibrary\steamapps\common\Kenshi\mods\
```

| Mod | Qué tiene |
|---|---|
| `SentientSands/` | **Release compilado, sin fuentes C++.** Diálogo con LLM. La lógica abierta está en Python (`Kayak/`, `server/`) y su base de conocimiento en texto plano. Fuentes: `github.com/Starswimmer/Kayak` (ver su `PROJECT_LINKS.md`). |

SentientSands importa a Kenshi por **`ContextMenu` y `Dialogue`** — los mismos
dos sistemas que hookea TotalRecruit. Si algo raro pasa con el menú contextual
o las conversaciones, es el primer sospechoso de conflicto.

Que un mod venga sin fuentes no lo vuelve inútil: `dumpbin //IMPORTS` sobre su
DLL dice qué APIs del motor toca, y eso ya responde si pisa lo mismo que vos.

```bash
MOD="/d/SteamLibrary/steamapps/common/Kenshi/mods/OtroMod/OtroMod.dll"
"$DUMPBIN" //IMPORTS "$MOD" | rg -o "ContextMenu|Dialogue|ForgottenGUI|PortraitManager" | sort -u
```

**Documentación en prosa — lo escaso, y por eso valioso**

| Ruta | Qué tiene |
|---|---|
| `re_kenshi-personal-projects/wiki/` | `For-Plugin-Authors.md`, `Conditions.md`, `FCS-Schema-Reference.md`, `Testing-Seams-and-Adapters.md` |
| `KenshiLua/docs/` | `BindingsReference.md`, `UnboundRefrence.md` — qué se pudo exponer y qué no |
| `ReKenshi_KenshiLib_Swagger/ReKenshi_KLIB_Navigator.html` | **Navegador del SDK en un HTML de 4 MB.** Abrilo en el browser para explorar clases visualmente. |
| `../KENSHI-MODDING-NOTES.md` | Nuestras propias trampas. §11 es obligatoria. |

### Las cuatro búsquedas que resuelven casi todo

**A. ¿Existe este método? ¿Qué firma tiene?**

```bash
cd /f/Proyectos/Kenshi-mods
rg -n "createPortraitImage" KenshiLib_Examples_deps/kenshilib/Include/kenshi/
```

El header da nombre, tipos, RVA y offset de vtable. **No da el rango de los
valores** — eso solo lo da el juego corriendo (§6).

**B. ¿Alguien ya lo usó? ¿Cómo?**

Esta es la más valiosa, y la que más se olvida. Excluí el SDK para ver solo
código de plugins reales:

```bash
rg -l "PortraitManager" --glob '!KenshiLib_Examples_deps/**' .
```

Si tres plugins llaman a algo de la misma forma, esa forma funciona. Si nadie
llama a un método que parece obvio, preguntate por qué — puede ser una de las
APIs que linkean y no hacen nada (§4).

**C. ¿Qué puedo hacer con esta clase?**

```bash
sed -n '/^class PortraitManager/,/^};/p'   KenshiLib_Examples_deps/kenshilib/Include/kenshi/gui/PortraitManager.h
```

Leé la clase **entera**, no solo el método que buscabas. El vecino de al lado
suele ser la respuesta: así apareció `setImageWidgetNPC`, dos líneas debajo del
`createPortraitImage` que no funcionaba.

**D. ¿Qué enum/constante necesito?**

Casi todo vive en dos archivos:

```bash
rg -n "EV_GET_UP|DA_JOIN_SQUAD|DC_IS_IMPRISONED"   KenshiLib_Examples_deps/kenshilib/Include/kenshi/Enums.h   KenshiLib_Examples_deps/kenshilib/Include/kenshi/Dialogue.h
```

`Enums.h` tiene eventos (`EV_`), condiciones (`DC_`), arquetipos (`OT_`) y
estados. `Dialogue.h` tiene las acciones (`DA_`).

### Recetas que ahorran horas

**Buscar sin ahogarse en Boost** — poné esto en tu `.bashrc`:

```bash
alias kgrep='rg --glob "!KenshiLib_Examples_deps/boost_1_60_0/**" --glob "!.git/**"'
```

**Solo headers del motor:**

```bash
rg -n "PATRON" --glob '*.h' KenshiLib_Examples_deps/kenshilib/Include/kenshi/
```

**Solo plugins, sin SDK ni docs:**

```bash
rg -n "PATRON" --glob '*.cpp' --glob '!KenshiLib_Examples_deps/**' .
```

**Encontrar un archivo por nombre:**

```bash
rg --files | rg -i "prisoner"
```

**Ver un enum completo:**

```bash
H=KenshiLib_Examples_deps/kenshilib/Include/kenshi
sed -n '/enum CharacterPerceptionTags_LongTerm/,/};/p' "$H/Character.h"
```

**Confirmar que un símbolo linkea** (ver §4 para el detalle):

```bash
LIB=KenshiLib_Examples_deps/kenshilib/Libraries/KenshiLib.lib
"$DUMPBIN" //LINKERMEMBER:1 "$LIB" | rg -o "\?[A-Za-z_]*@PortraitManager@@[^ ]*" | sort -u
```

### El orden que funciona

1. **`rg` en los headers** — ¿existe? ¿qué firma?
2. **`rg` en los plugins** — ¿alguien lo usó? Copiá esa forma.
3. **Leé la clase entera** — el vecino suele ser la respuesta.
4. **`dumpbin` en la lib** — ¿linkea de verdad?
5. **Recién ahí, código.**

Saltearse el paso 2 es el error caro. El SDK tiene APIs que compilan, linkean y
no hacen nada; los plugins que ya corren son la única prueba de cuáles sirven.

Un ejemplo real de esto: buscando `PortraitManager` en plugins aparece que
Shutori lo usa en tres archivos, siempre con `setImageWidget(handle, ...)` —
el camino del escuadrón, porque dibuja a sus propios luchadores. Nadie había
resuelto el caso del NPC ajeno, y por eso no había ejemplo que copiar. Esa
búsqueda de diez segundos explica por qué el retrato costó lo que costó.

---

## 6. Verificar de verdad

Que compile no prueba nada sobre el juego. La §8 de `PLAN.md` mantiene la
separación entre *confirmado jugando* y *solo compila*, y vale la pena
mantenerla honesta.

Los headers dan el **tipo**, nunca el **rango**. Dos ejemplos que ya costaron
sesiones: `hunger` no es 0..1 (un NPC bien alimentado lee ~3), y
`getOverallHealthRating()` devuelve 0 para cualquier inconsciente, por intacto
que esté.

Commiteá después de cada cambio que funcione.

---

## 7. Tests (reglas puras)

`tests/rules_tests` is a standalone console exe: it compiles the engine-free
rule modules from `src/` (`*_rules.cpp` and friends) with the same v100
toolset as the plugin, and links no KenshiLib. It never loads the game.

```bash
cd /f/Proyectos/Kenshi-mods/TotalRecruit

MSBUILD="/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"
"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo
tests/rules_tests/x64/Release/rules_tests.exe     # exit code 0 = green
```

Adding a rule test:

1. Put the rule in an engine-free module under `src/` (plain arguments, the
   config structs passed in, no `GetConfig()`, no `Character*`) and add the
   `.cpp` to **both** `TotalRecruit.vcxproj` and `tests/rules_tests/rules_tests.vcxproj`.
2. Write `tests/rules_tests/<module>_tests.cpp` with a `RunXxxTests()` function,
   declare it in `test_support.h`, and list it in `kSuites` in `main.cpp`.
3. Red first: run the exe, watch the new test fail for the right reason, then
   implement. Anything that has to read the game (hooks, UI, `Safe*` readers)
   stays out of the tests and is covered by `QA.md` instead.
