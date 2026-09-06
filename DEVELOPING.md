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
| `config.cpp/.h` | Lee `config.json`, aplica defaults y clamps. |
| `log.cpp/.h` | Todo va al log de RE_Kenshi con prefijo. |

### La decisión

| Archivo | Qué hace |
|---|---|
| `rules.cpp/.h` | **El motor.** Qué quiere un NPC antes de unirse. Empezá acá. |
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
| `settings_panel.cpp` | Panel de ajustes en `DatapanelGUI` nativo. |
| `spawn_panel.cpp` / `spawn_core.cpp` | Panel de spawn. |

### Soporte

| Archivo | Qué hace |
|---|---|
| `frame_tick.cpp` | Callback por frame, hookeado a `ForgottenGUI::update`. |
| `probe.cpp` | Diagnóstico. Con `probeOnClick` loguea todo del NPC clickeado. |
| `dialogue_bridge.cpp` | Puente a diálogo FCS. **Compila pero nunca se ejecutó.** |
| `game_data.cpp` | Listados de escuadras y personajes. |

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

## 5. Verificar de verdad

Que compile no prueba nada sobre el juego. La §8 de `PLAN.md` mantiene la
separación entre *confirmado jugando* y *solo compila*, y vale la pena
mantenerla honesta.

Los headers dan el **tipo**, nunca el **rango**. Dos ejemplos que ya costaron
sesiones: `hunger` no es 0..1 (un NPC bien alimentado lee ~3), y
`getOverallHealthRating()` devuelve 0 para cualquier inconsciente, por intacto
que esté.

Commiteá después de cada cambio que funcione.
