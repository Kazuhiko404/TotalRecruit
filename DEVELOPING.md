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

## 1b. Deploy con un solo comando: `scripts/deploy.sh`

```bash
scripts/deploy.sh --dry-run   # no construye ni escribe nada: muestra que haria
scripts/deploy.sh             # lo real (Kenshi tiene que estar cerrado)
```

Lo que hace, en orden:

1. **Se niega a correr si `kenshi_x64.exe` esta abierto** (`tasklist | grep -i kenshi_x64`, exit 1). No se busca
   `kenshi` a secas: `KenshiPatcher.exe` tambien coincide. Con `--dry-run` solo avisa.
2. Compila `TotalRecruit.vcxproj` y `tests/rules_tests` (Release|x64) y corre `rules_tests.exe`: si un test falla, no despliega.
3. Copia `x64/Release/TotalRecruit.dll` a la raiz del repo, a la carpeta del paquete `TotalRecruit/` (la que se commitea) y a
   `...\Kenshi\mods\TotalRecruit\`; copia `TotalRecruit/gui/` recursivo. `RE_Kenshi.json` y `TotalRecruit.mod` se copian solo si
   faltan en el juego. Una instalacion nueva crea la carpeta.
4. **`config.json` y `messages.json` del juego nunca se pisan.** Guarda `*.bak-<fecha-hora>` y corre
   `scripts/add_missing_keys.py`, que **solo agrega las claves que faltan** (recursivo: tambien dentro de `reason`, `readout`, `exclude`...) y
   deja todo valor existente. La unica excepcion de valor es `easyHoursCaged`: pasa a 6 solo si el desplegado es exactamente 12 (el default
   viejo); cualquier otro valor puesto a mano se respeta. Si no hay archivo desplegado, copia el del repo tal cual. Al escribir,
   el archivo se reserializa con indentacion de 2 espacios (se pierden las lineas en blanco de formato, no valores ni claves).
5. Compara el md5 de `x64/Release`, la raiz, el paquete y el juego, y falla si no coinciden.

Necesita `python` (o `python3`) en el PATH solo para el paso 4. La carpeta del juego se puede cambiar con `TR_GAME_DIR`.

### Las tres rutas

| Qué | Dónde |
|---|---|
| Salida del compilador | `x64/Release/TotalRecruit.dll` |
| Copia del repo (la que se commitea) | `TotalRecruit.dll` |
| **El juego** | `d:\SteamLibrary\steamapps\common\Kenshi\mods\TotalRecruit\` |

La carpeta desplegada necesita estos archivos (`scripts/deploy.sh` los pone):

```
d:\SteamLibrary\steamapps\common\Kenshi\mods\TotalRecruit\
├── TotalRecruit.dll     <- cambia en cada build
├── TotalRecruit.mod     <- cabecera de 46 bytes, no se toca
├── RE_Kenshi.json       <- le dice a RE_Kenshi qué DLL cargar
├── config.json          <- del usuario: solo se le agregan claves faltantes
├── messages.json        <- idem
└── gui\images	r_white.png
```

La tarjeta de lectura (T9) dibuja sus barras con `TotalRecruit/gui/images/tr_white.png`: copiá esa carpeta `gui` a la desplegada. Sin ella el mod anda igual (usa el skin `WhiteSkin` y lo dice en el log), pero con un mod de skin (DarkUI, PowerModUI) las barras se ven oscuras.

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

### KenshiLib 0.5.0, vendorizado

TotalRecruit compila contra **KenshiLib 0.5.0**, copiado en
`deps/kenshilib-0.5.0/` (el `README.md` de esa carpeta indica tag, commit y md5).
`TotalRecruit.vcxproj` toma de ahí `Include/`, el header forzado
`OgreUnordered.h` y `Libraries/KenshiLib.lib`. Las librerías de Ogre y MyGUI
siguen saliendo de `$(KENSHILIB_DIR)\Libraries`.

- Por qué: el juego instalado ya corre KenshiLib 0.5.0 (su `KenshiLib.dll` es
  idéntico al del release v0.5.0), y solo 0.5.0 trae `AI/Blackboard.h`
  (paquetes de IA nativos: `replaceAIPackage`, `addPackages`, ...).
- `KENSHILIB_DIR` sigue en 0.4.0 y lo comparten los otros mods: no se toca.
- Los archivos de `deps/` son idénticos a los de upstream (`deps/** -text` en
  `.gitattributes`). No se editan; para actualizar se reemplaza la carpeta entera.
- Después de cambiar de versión, verificá que cada import del DLL exista en el
  `KenshiLib.dll` instalado: `dumpbin -imports x64/Release/TotalRecruit.dll`
  contra `dumpbin -exports` del DLL del juego. Un import que falta = el mod no carga.

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
| `rules.cpp/.h` | **El despachador.** `Evaluate`: junta los hechos, pregunta qué ruta contesta y devuelve el veredicto. No decide nada por su cuenta. Empezá acá. Antes de toda ruta (el preset Cheat incluido) un NPC rechazado en el evento del sometido contesta `Reason_RejectCooldown` (`DecideRejectCooldown` sobre `ReadRejectCooldown` de `character_facts`); `mercy` mira lo mismo antes de suplicar. `EvaluatePeek` es la misma pregunta como espectador (sin reloj de encierro, sin id, sin tirada, sin log): lo usa la tarjeta. |
| `character_facts.cpp/.h` | **Lo único que lee el motor para decidir.** `GatherCoreFacts` (una vez por pregunta) y un `Gather*Extra` por ruta; nada más lee el juego en el camino de la decisión. `ReadHaggleTerms` junta lo que pide un regateo (`HaggleTerms` en `entry_facts.h`: la misma clave y día de `MintRollKey`, si hoy está ofendido, nivel y temperamento del NPC, mejor stat del líder que habla). En modo vistazo el rescate tambien tira: `PeekRollKey` arma la tirada con el id que ya existe (`SafeFindCharKey`, como `ReadHaggle`; nunca crea uno), asi la tarjeta y el clic dicen lo mismo de un rescate (prisoner-module-fixes F3). `MintRollKey` y `PeekRollKey` solo difieren en como leen la clave: las dos arman con `RollFactFrom`. |
| `entry_facts.h`, `verdict.h`, `archetype.h` | **Puros.** Los hechos planos (`CoreFacts` y los extras por ruta), el veredicto (`Verdict`, `Currency`, `ReasonKey`) y el arquetipo sin motor. |
| `route_decisions.h` + `route_gate/cheat/rescue/yield/animal/free/cage/cats.cpp` | **Puros.** Una función por ruta, de hechos planos a `Verdict`. `route_cats` es el precio que comparten el mundo libre y la jaula. `DecideAccessRefusal` (jaula ajena, insistencia) y `ServesStandingOrganisation` (militar, ley) viven en `route_free` y los usa también `field-yield` (`route_yield`): una sola pregunta de acceso para las dos rutas que hablan fuera de tu jaula. Para cambiar lo que una ruta decide, es acá y en su test. |
| `personality_groups.cpp/.h` | **Puro.** Los diez temperamentos leídos como cuatro grupos (`GroupOf`): Bueno (Honorable, WarmKind), Neutral (Normal, Dumb, Fearful, Brave), Ambicioso (Traitorous, Smart), Malo (ColdCruel, Maniacal). El único mapeo; REQUISITOS §4b lo documenta. Tests: `personality_groups_tests.cpp`. |
| `ally_tier_rules.cpp/.h` | **Puro.** Aliados por nivel en Balanced (`DecideAllyTier`): de nivel del NPC, nivel del líder, grupo y relación a `Free` / `Paid` (con `discountFraction`) / `Temporary` / `Refuse` y su motivo. Cada nivel de corte pertenece al tramo que empieza (`AllyTierOf`: 20, 50, 80, 100 de `AllyTiers` en `config.h`). Descuento `min(tope del tramo, relación/100 × discountScale)` (`FriendshipDiscount`), `AllyPriceMultiplier` = 1 − descuento solo para `Paid`, `DiscountPercent` redondeado, y los días del miembro temporal (`TemporaryDaysFromSeed` sobre `DailySeed(..., kTempDaysSalt)`). `route_free` decide con `DecideBalancedAllyTier` y `character_facts` (`AllyFriendshipMultiplier`, solo en `GatherFreeExtra`) pone el descuento en `PriceInputs.factionMultiplier`: los dos llaman a la misma función, así el "-14%" del popup es el del precio. Tests: `ally_tier_rules_tests.cpp`, y los casos de ruta en `route_decisions_tests.cpp`. |
| `temp_member_rules.cpp/.h` | **Puro.** La estadía de un miembro temporal (80+): `TempExpiryHour` (ahora + días × 24), `TempDaysLeft` (redondeo hacia arriba), `IsTempMemberHungry` y `StepTempMember`: faltas por hambre (`tempMemberHungryHours` seguidas bajo `tempMemberHungerThreshold`, o el motor dice hambriento), por un golpe del grupo y por un noqueo del grupo (solo el borde hacia el noqueo), cada tipo a lo sumo una vez cada `tempMemberStrikeCooldownHours`; el aviso en la falta que lo deja a `tempMemberMaxStrikes - 1`, el "me ire pronto" `tempMemberExpiryWarningHours` antes, y la partida (`TempLeave_Mistreated` gana sobre `TempLeave_Expired`). El aviso diario (`tempMemberDailyNotice`): `ShouldAnnounceDaysLeft` (una vez por dia de juego, nunca el dia en que se unio, nunca el dia del "me ire pronto" -- `TempExpiryWarningFallsOnDay` --, nunca en un paso que ya dice otra cosa) y `StepTempMember` lo marca en `TempMemberStep::dailyNotice` / `dailyDays`; el dia de union y el ultimo dia anunciado son de sesion (tras cargar se repite una vez ese dia). `TempMemberBook`: quién es miembro por `CharKey`, con el puente al save (0 para quien no es miembro). Tests: `temp_member_rules_tests.cpp`. |
| `temp_member.cpp/.h` | El lado del motor del miembro temporal. `CompleteOffer` lee la facción de origen (`FactionStringId`) **antes** de reclutar y llama `OnRecruitedTemporarily(Character*, días, facción)`: guarda vencimiento, faltas (filas `total_recruit.temp_expiry_hour` / `temp_strikes` de `saved_ints`) y facción (`total_recruit.temp_home_faction` de `saved_strings`), y levanta "Te acompanare N dias" (con el dia de union, `DayIndexFromHours`, para que el aviso diario no salga ese dia; tambien guarda el lugar donde se unio, `total_recruit.temp_join_spot`). `TempMemberTick` (cada 2 s desde `frame_tick`) recorre el grupo, lee hambre (`SafeHunger`), golpes (`SafeAttackedByPlayer`) y noqueos (`SafeIsKnockedOut` + `SafeDefeatedByPlayer`, no si fue de hambre), y aplica `StepTempMember`: carteles de grupo (`RaiseSquadHeadNotice`) en cada falta, el aviso, "me ire pronto" y una vez por dia "Me quedan N dias" (`tempMember.notice.daily`, "Me queda 1 dia" con `dailyOne`); al irse, `LeaveSquad`, cartel de despedida, linea en el roller y el enfriamiento de rechazo (`SetRejectCooldownUntil`, `tempMemberRejoinCooldownDays`) para que no se lo pueda volver a tomar enseguida. `TempMemberDaysLeft` / `SelectedTempMember` alimentan la tarjeta. Tabla con lock; `load_boundary` la vacia (`ForgetAllTempMembers`). El camino FCS (`ConsumePendingRecruit`) no lo llama. |
| `squad_leave.cpp/.h` | Sacar a alguien del grupo, nativo: port del dismiss de KEP (`CharacterExtension.cpp:72-108`). Facción de origen por string id (si no, `1233-gamedata.base`, si no la vacía; nunca una del jugador), `unselectPlayerCharacter`, `_NV_setFaction(facción, NULL)`, `createNewEmptyActivePlatoon("5006-squad.mod" = "Wandering Squad" vanilla, permanente, donde está)` + `_NV_addActiveObject`, limpiar el sistema de tareas (`taskSystemAI` leído en su offset 0x20 porque `AI/AI.h` choca con `Character.h`) y `reThinkCurrentAIAction`. Con KenshiLib 0.5.0 hace tambien el reset de blackboard de KEP (`AiPackages::ResetToTemplate`: el paquete del Wandering Squad, logueado `[temp] <nombre> native wander package <paquete> assigned`); si falla, `leave_homing` lo hace caminar al lugar donde se unio (native-ai-packages T3). |
| `helper_rules.cpp/.h` | **Puro.** El ayudante gratis (T3). `ScreenHelperCandidate`: activo, Balanced, no tuyo, no animal, de pie, sin jaula, no ocupado (ya ayudante), sin pelear, a 40 m o menos del lider, aliado (`friendlyRelation`), Bueno o Neutral (`HelperPersonalityAllowed`, `personality_groups`), nivel > lider + `helperMinLevelLead`, sin enfriamiento, sin tirada hoy; cada salto con su motivo (`HelperSkipText`). `HelperOfferRoll` (semilla % 10000 < chance × 10000, `DailySeed` con `kHelperOfferSalt`) y `HelperDaysFromSeed` (`helperDaysMin..Max`). `StepHelper`: muerto, reclutado, noqueado por el grupo, atacado por el grupo, vencido (en ese orden) y el aviso `helperExpiryWarningHours` antes; el vencimiento es `TempExpiryHour`. La escalera de escolta (`NextFarChecks` / `NextEscortRung`): BODYGUARD, FOLLOW_PLAYER_ORDER, la orden de caminar del walk-up; sube tras 3 chequeos seguidos a mas de 15 m fuera de combate. `ShouldAssist`: el blanco del lider mientras pelea, nunca uno del grupo, el mismo, su faccion o alguien caido. `HelperBook`: contratos, enfriamiento de rechazo y dia de tirada por `CharKey`, con el puente al save. Tests: `helper_rules_tests.cpp`. |
| `helper.cpp/.h` | El lado del motor del ayudante, desde `frame_tick` (`HelperTick`). **Primero el contrato nativo** (native-ai-packages T2, `helperNativeBodyguard`, por defecto encendido): `AiPackages::AssignBodyguard` con el paquete Bodyguard (5090) y el lider como empleador, como un mercenario contratado; si esta en un pelotón con otros, antes `DetachToOwnPlatoon`; se lee de vuelta y se loguea `[helper] <nombre> native bodyguard contract -> <lider> (N h) ok|failed: ...`. Cada chequeo, `NativeWatchStep`: si el contrato desaparece se reasigna una vez y despues cae al mecanismo B. **Mecanismo B (escolta por codigo)**, apagado el nativo o si falla: repath cada 1 s (`PlanEscort`: camina cerca, corre lejos, a un anillo de 3 m del lider, `EscortOffsetMeters`) y pelea el blanco del lider con la secuencia del fight_starter (`NpcMotion::EngageFor`, `ShouldAssist` / `ShouldReissueAssist`). Oferta: barrido cada 5 s (sin ventana, con el slot de `approach_driver` libre), una tirada por NPC por dia; camina con el walk-up del sometido, cartel "Quiere ofrecerte ayuda", al llegar `BuildHelperOfferView` (ACEPTAR / RECHAZAR; la X no es respuesta). Fin: vencido ("Hasta aqui llego"), traicionado ("No vuelvo a ayudarte" + enfriamiento), muerto o reclutado; el contrato nativo se termina con `AiPackages::EndContract` y siempre `ResumeRoutine`. Filas `total_recruit.helper_expiry_hour` y `helper_reject_until` de `saved_ints`; tras cargar, el primero a 100 m del lider se re-arma ("Sigo contigo"). Tecla de prueba `helperDebugKey` (codigo virtual de Windows, `GetAsyncKeyState`, solo con el juego al frente): el NPC seleccionado se ofrece ya; sobre un ayudante, loguea y asigna el contrato nativo. Tabla con lock; `load_boundary` llama `ForgetHelpers`. Log `[helper]` siempre. |
| `ai_packages.cpp/.h` | El unico lugar que toca un blackboard (`Platoon::blackboard`, `AI/Blackboard.h` de KenshiLib 0.5.0), con las secuencias de KenshiExtensionPlugin probadas en juego: `AssignBodyguard` (halt + clearOrders, `_setContractJob(5090, horas, hand(empleador))` y `replaceAIPackage` solo si el contrato no quedo activo; lee de vuelta `contractJob`, `missionEmployer` en 0x48 y el paquete actual en 0x68, porque `AIPackage.h` no compila junto a `Blackboard.h`), `EndContract` (`endContractJob` + el "reset AI" de KEP), `ResetToTemplate` (el dismiss de KEP sobre un pelotón nuevo) y `DetachToOwnPlatoon`. Todo con guarda SEH y chequeo de NULL; devuelve `Outcome {ok, package, why}` y no loguea. |
| `leave_homing_rules.cpp/.h` | **Puro.** La vuelta a casa de respaldo de un temporal que se fue sin paquete: el lugar donde se unio como "x,z" (`FormatJoinSpot` / `ParseJoinSpot`, guardado en `saved_strings` como `total_recruit.temp_join_spot`) y `StepHoming` (re-emite la orden cada 4 s, llega a 6 m, se rinde a los 240 s). Tests: `leave_homing_rules_tests.cpp`. |
| `leave_homing.cpp/.h` | El lado del motor: `StartLeaveHoming` da la orden de caminar (`NpcMotion::MoveToPoint`), `LeaveHomingTick` (frame tick) la repite y al llegar o rendirse devuelve la rutina (`ResumeRoutine`). Log `[temp] <nombre> heading back to (x,z)`, luego `arrived` o `gave up`. Solo sesion. |
| `approach_driver.cpp/.h` | El walk-up compartido, extraido de `submission_director` para que el ayudante no lo copie: el slot de un evento a la vez (`ClaimApproachSlot` / `ReleaseApproachSlot`: sometido y ayudante nunca a la vez), el reloj de segundos reales congelado en pausa (`EventClock`), y el caminante (`WalkUp`: NPC + clave, `BeginWalkUp`, `ReassertCalm`, `StepWalkUp` = `ApproachStep` + repath / esperar parado / llegar y mirar al lider). Llegar y abortar los decide cada director. |
| `duel_challenge_rules.cpp/.h` | **Puro**, con lock. El reto a duelo de un NPC 80+ (ts-duel-bridge, duel-join-chance). `CanOfferDuel`: el unico predicado de RETAR A DUELO (`duelChallengeEnabled`, TotalSparring disponible, `Verdict::allyTiered` en Balanced o `cheatPreset` en Cheat, no animal, nivel >= `allyTiers.eliteFromLevel` con el nivel de `SafeNpcLevel`, sin enfriamiento). `DuelChallengeBook`: reto pendiente por `CharKey` del NPC (`{retador, hora}`) y enfriamiento; `Resolve` de un `DuelBoutRecord` (la copia de un `TS_BoutEvent`): duelo, lado A tuyo gano, B = el retado -> `Won` (con `winner` = lado A; si se une lo decide `duel_join_rules`); B gano o empate -> `Lost` + enfriamiento `ceil(ahora + duelChallengeCooldownHours)`; sparring, sin decidir, par ajeno o lado ilegible -> `Ignored` (sin decidir deja el reto). `ExpirePending` (`duelChallengeExpiryHours`), `FormatDuelRecord` ("2-1", "2-1-1"). Instancia de proceso con lock (`RecordDuelChallenge`, `ResolveDuelBout`, `SetDuelCooldownUntil` para el enfriamiento de una victoria sin union, ...) y el puente al save `total_recruit.duel_cooldown_until` (una sola fila para los dos enfriamientos). Tests: `duel_challenge_rules_tests.cpp`. |
| `duel_join_rules.cpp/.h` | **Puro.** Que significa ganar un duelo (duel-join-chance). `DuelJoinChance`: `min(100, duelJoinBaseChance + duelJoinChancePerWin * (victorias - 1))`, con la victoria nueva contada (25, 45, 65, 85, 100). `DecideDuelWin`: Cheat se une siempre; Balanced tira `DailySeed(npc, victorias, kDuelJoinRollSalt) % 100 < chance` (misma semilla tras recargar: no se re-tira) y si no se une, un enfriamiento uniforme en `[duelWinCooldownMinHours, duelWinCooldownMaxHoursUnder100]` (nivel 80-99) o `[.., duelWinCooldownMaxHours]` (100+), con `kDuelWinCooldownSalt`; un maximo menor que el minimo es el minimo. `DuelNextWinOdds`: lo que diria la proxima victoria, para la fila SI LE GANAS y la linea de la tarjeta. Tests: `duel_join_rules_tests.cpp`. |
| `duel_record_cache_rules.cpp/.h` | **Puro.** Cuando la tarjeta vuelve a leer el historial de duelos (`DuelRecordNeedsRead`): par (NPC, miembro seleccionado) nuevo o una pelea nueva (`TsBoutGeneration`) desde su copia; nunca en el refresco de 2 s. Tests: `duel_record_cache_rules_tests.cpp`. |
| `level_rules.cpp/.h` | **Puro.** El unico nivel del mod: `RoundLevel` redondea el nivel general de habilidad (0-100) igual que TotalSparring (mitad lejos de cero), para que el popup diga el mismo nivel que la tarjeta de duelo. Tests: `level_rules_tests.cpp`. |
| `npc_level.cpp/.h` | `SafeNpcLevel`: `CharStats::getOverallSkillLevel_0_100()` con `RoundLevel`; si no se puede leer, `getRoughLevel()` (logueado una vez como `[level]`). Alimenta todo nivel: chance, precios, tramos de aliados, temporales, el corte 80+ del duelo, regateo, ayudante, doma y el "nivel N" del popup. |
| `ts_bridge.cpp/.h` | El puente a la API publica de TotalSparring (`src/third_party/ts_api.h`, copia textual; no se linkea nada). Engancha perezoso desde `frame_tick` (`TsBridgeTick`, cada 2 s, nunca en `startPlugin`): `GetModuleHandleA("TotalSparring.dll")` o el export `TS_GetApi` buscado con Toolhelp32 (el patron de `mod_hub_integration`), version 1, tamaño y punteros de la tabla, y un listener de peleas; sin TotalSparring tras 15 intentos, ausente por la sesion (logueado una vez). `TsAvailable` (enganchado + escuchando + `IsReady`), `TsDuelStats` (`GetDuelStats`, solo en eventos: lee SQLite), `TsOpenDuel` (`OpenDuelPopup` -> `TsOpenResult`). El callback solo copia: `CharKey` de a y b con `SafeFindCharKey` y si a es tuyo, a una cola con lock (32); `TsTakeBoutEvents` la vacia en el cuadro siguiente; `TsBoutGeneration` sube con cada pelea copiada (TotalSparring ya la escribio en su historial) y la tarjeta la usa para releer el historial (`duel_record_cache_rules`). Toda llamada a la tabla con guarda SEH y su codigo revisado. `ShutdownTsBridge` (primero en `stopPlugin`) da de baja el listener solo si el modulo de TotalSparring sigue cargado. Log `[ts-bridge]`. |
| `body_rules.cpp/.h`, `relation_rules.cpp/.h` | **Puros.** Qué significan las lecturas del cuerpo (herido, hambriento, curado) y los tramos de reputación. |
| `reason_catalog.cpp/.h` | **Puro.** Cada motivo (`ReasonKey`): su clave en `messages.json`, su texto interno y el tono de rechazo. Agregar un motivo = una fila acá + una línea en `messages.json`. |
| `reason_text.cpp/.h` | Convierte la clave del veredicto en la línea del jugador (`Say` + `Fill`). |
| `animal_rules.cpp/.h` | **Puro.** La barra de domesticación (madurez, nivel, temperamento). |
| `npc_speech.cpp/.h` | Lo que dice el NPC: línea de apertura, `HailLine`, y las palabras de la ventana de reclutamiento (`OfferWording`: bloque `popup` de `messages.json`, con el castellano interno si falta una clave). El tono de un rechazo sale de la clave del motivo, nunca del texto. |
| `engine_reads.cpp/.h` | Las lecturas `Safe*` compartidas (líder, facción, reloj de juego, memoria del motor, jaula, nombre, tarea actual `SafeCurrentTask`: vive acá porque `Tasker.h` y `AI/AITaskSystem.h` definen los dos `taskPriority`). Tambien `SafeDefeatedByPlayer` (quien lo noqueo es tuyo; lo usan `defeats` y `temp_member`), `SafeAttackedByPlayer` (`getAllAttackers`), `SafeHunger`, `FactionStringId`, `SafeSelectedCharacter` y `SafeAttackTargetOf` (a quien ataca alguien, `getAttackTarget`; el ayudante pelea ese blanco). **Si necesitás una lectura que ya existe en otro archivo, va acá, no una copia.** |
| `hold_tracking.cpp/.h`, `insistence.cpp/.h`, `rescue_memory.cpp/.h`, `pacify.cpp/.h` | El reloj de encierro, la insistencia (R2.4), la deuda del rescate y la pacificación: estado por-sesión o memoria del motor, cada uno en su archivo. `pacify` tiene ademas la calma por personaje del evento del sometido (`SafeCalmToward`: `endCombatMode`, `attackTarget(0)`, y por cada miembro del grupo `clearTempEnemyStatus` + `rememberCharacter(ST_NONE)`, sin re-pensar), `SafeHostileToward` (como estaba antes), `SafeMarkHostile` (`ST_TEMPORARY_ENEMY` hacia el lider) y `SafeInCombat`. |
| `route_rules.cpp/.h` | **Puro.** Qué ruta contesta primero (`ChooseEntryRoute`, sobre los hechos de `MakeRouteFacts`). El orden de precedencia vive solo acá. |
| `chance_rules.cpp/.h` | **Puro.** El motor de chance (`JoinChance`, `ChanceFrom*`, el roll estable). La tirada del veredicto es diaria: `RollsToJoin(haveKey, key, dayIndex, pct)` = `DailyRoll(key, dayIndex, kJoinRollSalt, pct)`; `DayIndexFromHours` = `floor(horas/24)`. El día lo pone `MintRollKey` (`character_facts.cpp`) desde `SafeGameHours`, el mismo reloj que el de encierro, en `RollFact.dayIndex`; `RollFactFrom(haveKey, key, haveClock, horas)` arma el `RollFact` (sin clave o sin reloj no hay clave) para `MintRollKey` y `PeekRollKey`. Otra tirada del mismo día usa otra sal (`kHaggleRollSalt`). |
| `price_rules.cpp/.h` | **Puro.** La curva de precio (calibrada a precios tipo vanilla: nivel 13 con nombre y talento ~2.500 cats), los multiplicadores (talento, personalidad, facción), la prima conjunta talento x único x carácter con tope `premiumMaxMultiplier` (`PremiumMultiplier`) y el precio final: redondeo, mínimo y tope (`FinalPriceInCats`). |
| `haggle_rules.cpp/.h` | **Puro.** El regateo de un precio en cats: límites del slider (`HaggleSliderBounds`, `ClampOffer`; mínimo `haggleMinFraction` del pedido redondeado a 10, paso 10), la probabilidad de aceptar (`AcceptOdds`: 100 a precio pleno; si no `100 - descuento% * haggleDiscountWeight + HaggleLeverage + HagglePersonalityShift`, entre 0 y 95), la tirada (`ResolveHaggle`, la misma semilla diaria que la de unirse con `kHaggleRollSalt`) y el precio del ofendido (`OffendedAsk`, +`haggleOffenceMarkup` redondeado a 10; lo aplica solo `ApplyCatsOffer` en `route_cats.cpp`). Las posiciones del slider: `HaggleStepCount`, `OfferAtStep`, `StepOfOffer` (0 = el mínimo, la última = el pedido aunque no sea múltiplo de 10). Tests: `haggle_rules_tests.cpp`. |
| `haggle_ledger.cpp/.h` | **Puro.** Quién se ofendió y qué día (`HaggleLedger`, una tabla por sesión en `SessionHaggleLedger()`): `IsHaggleLocked(key, día)`, `OffendedMarkup(key, día, prices)`. Solo memoria, nada al save; `load_boundary` la vacía. `character_facts.cpp` la lee con `SafeFindCharKey` (sin crear id) en `HaggleFact` de `FreeExtra`/`CageExtra`, y lee la fuerza del que habla con `SafeSpeakerBestStat`. |
| `money_rules.cpp/.h` | **Puro.** Cobrar y mover cats entre bolsas (`ChargePrice`, `TransferCats`): todo o nada, sin acuñar ni perder cats si un paso falla. |
| `cats_purse.cpp/.h` | La bolsa respaldada por el motor (`CharacterPurse`): el único lugar que conoce `getMoney` / `takeMoney` / `Ownerships::addMoney`. |
| `char_key.h`, `temperament.h` | **Puros.** La clave de personaje (`CharKey`: id persistido, o handle de respaldo) y el enum de personalidad sin motor. `temperament_map.cpp` hace el único mapeo desde `PT_*` y `OT_*`. |
| `identity_registry.h` | **Puro.** Qué objeto vivo lleva qué id, conflictos de ids duplicados y la elección id-o-handle (`ChooseKey`). |
| `character_identity.cpp/.h` | Los tres hooks de `Character` (`_NV_serialise`, `_NV_loadFromSerialise`, `_DESTRUCTOR`) que guardan un UUID propio (`total_recruit.character_id`) dentro del save, y a su lado cada entero por personaje de `saved_ints.h` (un solo bucle de lectura/escritura `idata`, no una copia por campo). Port de `TotalSparring/src/shared/game/fighter_identity`. |
| `saved_ints.cpp/.h` | La tabla de enteros por personaje que viajan en el save junto al id: clave `idata` + `saved(id)` + `restore(id, valor)`. Hoy `total_recruit.player_defeats` (`defeats.h`), `total_recruit.fed_by_player` (`feeding_ledger.h`) y `total_recruit.reject_cooldown_until` (`submission_tracker.h`). Tambien `total_recruit.temp_expiry_hour` y `total_recruit.temp_strikes` (`temp_member.h`), y `total_recruit.helper_expiry_hour` y `total_recruit.helper_reject_until` (`helper.h`), y `total_recruit.duel_cooldown_until` (`duel_challenge_rules.h`). Un conteo nuevo que tenga que sobrevivir a una carga = una fila aca, nada en `character_identity`. Se escribe siempre (0 incluido); `restore` solo si la clave esta; los dos pueden correr fuera del hilo del juego. |
| `saved_strings.cpp/.h` | El gemelo de `saved_ints` para textos (`sdata`), por el mismo puente que el id: clave + `saved(id)` + `restore(id, valor)`. Hoy `total_recruit.temp_home_faction` (`temp_member.h`, la faccion a la que vuelve un miembro temporal) y `total_recruit.temp_join_spot` ("x,z", donde se unio: la vuelta de respaldo de `leave_homing`). Mismo contrato: se escribe siempre ("" incluido). |
| `feeding_ledger.cpp/.h` | Cuantas comidas le dio el jugador a cada animal (unidad: comidas, una por `FeedCaptive` exitoso sobre un animal, tope 1000), por `CharKey`, con lock. Se guarda por `saved_ints`; `load_boundary` lo vacia. `character_facts` lo lee sin crear id (`AnimalExtra::fedByPlayer`). Tests: `feeding_ledger_tests.cpp`. |
| `defeat_rules.cpp/.h` | **Puro.** Quién se lleva el crédito de un noqueo (`DecideDefeatCredit`). |
| `defeat_episode_rules.cpp/.h` | **Puro.** Una derrota por noqueo, la regla "visto de pie" de `TotalSparring/src/spar/domain/phantom_ko_rules`: cuenta si está caído y nada contó en este episodio; verlo de pie lo rearma; el hook que lo vio caer desde de pie abre un episodio nuevo. Ventana de eco (`kKnockoutEchoWindowMs` = 5 s, tiempo real, `GetTickCount`): cualquier avistamiento dentro de esa ventana desde la ultima derrota contada es el mismo episodio, diga lo que diga la lectura de pie (el hook llega dos veces ~1,2 s aparte y las dos leen "de pie"; prisoner-module-fixes F2); un rearme por verlo de pie no la corta. Tests: `defeat_episode_rules_tests.cpp`. |
| `defeat_ledger.cpp/.h` | **Puro.** Cuántas derrotas por `CharKey`. `character_identity.cpp` (vía la tabla de `saved_ints`) lo escribe en el save del personaje (`idata["total_recruit.player_defeats"]`, siempre junto al id, 0 incluido) y lo devuelve al cargar; `load_boundary` lo vacía al comenzar la carga. Tests: `defeat_ledger_tests.cpp`. |
| `char_key_reader.cpp/.h` | `SafeCharKey` (crea el id) y `SafeFindCharKey` (solo lee). Si el id no se puede obtener cae al handle, logueado una vez. `StillSameCharacter(puntero, clave)`: el puntero solo mientras la clave leida de el sigue siendo esa (si no, NULL; la lectura tiene guarda): un puntero guardado entre cuadros (el walk-up, la vigilancia de Noquear) nunca se usa solo. |
| `load_boundary.cpp/.h` | Hookea `SaveManager::loadGame/importGame` y, al comenzar una carga, limpia toda tabla por-sesión y el registro de ids (`ResetSessionState`). |
| `recruit_core.cpp/.h` | Resuelve el target del click, valida y ejecuta el reclutamiento. `PresentOffer` arma el `OfferInput` (con `HaggleInputFor` para un precio en cats, `CageInputFor` para GOLPEAR / NOQUEAR y `RemedyFor`: el remedio es solo comida; `HelpCaptive` solo alimenta) y abre `ShowPricedOffer`; `CompleteOffer` paga el pedido; `CompleteHaggle` (OFRECER) reevalúa, tira `ResolveHaggle` con la clave y el día de `MintRollKey`: aceptado cobra la oferta (`PayAndRecruit`, `money.paid` dice el monto), ofendido anota `SessionHaggleLedger().RecordOffence` y reabre la ventana bloqueada con el precio subido. Log `[haggle] <nombre> ask=.. offer=.. odds=..% -> accepted|offended` con `probeOnClick`. El reto a duelo (seccion propia al final): `DuelOfferedFor` arma `DuelOfferFacts` y pregunta `CanOfferDuel`; `PresentOffer` llena `OfferInput::duel` y lee el historial una vez al abrir; `ChallengeToDuel` (RETAR A DUELO) toma el retador con `SelectedSquadActor`, anota el reto y llama `TsOpenDuel` (si no se abre, lo borra y lo dice); `DuelChallengeTick` engancha TotalSparring, vence retos viejos cada 5 s y resuelve las peleas de la cola: ganada -> busca al NPC por `CharKey` cerca de tu grupo (80 m), lee las victorias del ganador con `TsDuelStats` y el nivel con `SafeNpcLevel`, y pregunta `DecideDuelWin`: se une -> `Recruit` permanente + `RaiseSquadHeadNotice` "Me ganaste. Me uno a ti"; no se une -> `SetDuelCooldownUntil` + cartel informativo "Buena pelea. Retame otra vez" + roller con la chance y las horas (`duel.stays`); perdida -> cartel "Vuelve cuando seas mas fuerte". `DuelNextWinOddsNow` da la fila SI LE GANAS y la linea de la tarjeta. Log `[duel] <ganador> won (wins N): join chance X%, roll R -> joined|stays (cooldown H h until hour T) vs <nombre> ...`. |
| `defeats.cpp/.h` | El lado del motor de las derrotas: el hook de `MedicalSystem::knockout` (lee si estaba de pie **antes** del noqueo nativo), el clic derecho (`NoteIfDown`) y lo que alimenta "visto de pie" (`NoteSeenStanding`, desde el barrido de `hail` y la tarjeta). Un noqueo que el jugador ordeno con Golpear / Noquear cuenta como suyo aunque el motor no nombre ganador: el hook lo reclama con `ClaimOrderedKnockoutCredit` (`cage_commands`). `ClearDefeats` las borra (cuenta y episodio) cuando el jugador rechaza a un sometido. Ya no lee ni escribe la marca `LT_SQUAD_LOST_TO_ME_ONCE` del motor. No conoce a `mercy`: avisa por un listener que `startPlugin` conecta (`SetNearDeathListener`), para que la cadena sea `mercy` → `recruit_core` → `defeats` y no un anillo. |
| `cage_command_rules.cpp/.h` | **Puro.** Las decisiones de Golpear / Noquear: si aplican (`CageCommandsAllowed`: en tu jaula, despierto, `allowBeating`; el mismo predicado decide el menu contextual y los botones GOLPEAR / NOQUEAR de la ventana, `offer_view`), el bloque de botones (`ComposeContextBlock`: `[Hablar]` o `[Hablar, Golpear, Noquear]`), lo que puede hacer un clic con los hechos releidos en ese momento (`JudgeCageClick`: ya no aplica / Golpear en descanso / actuar), el intervalo entre golpizas de R2.3 (`BeatCooldownLedger`: horas de juego desde el ultimo golpe que pego, por preso, solo de sesion como los episodios de derrota; `BeatReady`; `BeatLabel` / `BeatRestHoursShown`: "Golpear (4 h)" con las horas redondeadas hacia arriba), el golpe de Golpear (`BeatBlunt`: todo el daño de ataque del motor para ese atacante como contundente, `kBeatDefaultBlunt` = 15 si no se lee; `PickBeatPart`: la primera parte de torso, si no la primera fatal; `CapBeatBlunt` / `BeatHealthFloor`: la parte nunca baja de -0,5 x su vida maxima; `BeatHeldHealth`: si el motor bajo de mas se vuelve al piso, pero nunca por encima de como el golpe encontro la parte: un golpe nunca cura), la vigilancia de la orden nativa de Noquear (`JudgeOrderWatch`: cualquier cambio de tarea es "tomada"; sin cambio en `kOrderWatchTimeoutMs` = 1,5 s corre el respaldo; es la primera fase de `knockout_watch_rules`), el puente de credito (`OrderedKnockoutCredit`, cada entrada con su ventana: un golpe que pego se reclama una vez dentro de `kBeatCreditWindowMs` = 5 s, un noqueo ordenado dentro de `kOrderedKnockoutWindowMs` = 2 min; un desmayo posterior, por hambre por ejemplo, no es del golpe), la etiqueta "Noquear 72%" (`KnockoutLabel`) y la habilidad 0..100 a 0..1 (`SkillFraction`). Tests: `cage_command_rules_tests.cpp`. |
| `cage_commands.cpp/.h` | El lado del motor de Golpear / Noquear. `ReadCageCommandFacts` lee los hechos (jaula propia, caido o muerto, `allowBeating`, las horas que le quedan de descanso a Golpear sobre ese preso) para el menu, la ventana y cada clic; un golpe que pego arranca el descanso (`submission.beatingCooldownHours`, `BeatCooldownLedger`; la carga lo vacia); `OrderSelectedToBeat` / `OrderSelectedToKnockout` releen los hechos al clic (`JudgeCageClick`) y devuelven `CageCommand_NotAllowed` ("cage.notAllowed") o `CageCommand_Resting` ("cage.beatResting", con las horas) sin hacer nada; `ReadKnockoutOdds` da el % del motor. El actor es el personaje seleccionado (`SelectedSquadActor`). `IssueBeat` = un golpe directo, en el acto y sin ninguna orden: lee `CharStats::getTotalAttackDamageFor(preso)` y aplica ese daño solo como contundente (sin corte ni perforacion: no sangra) con `HealthPartStatus::applyDamage` sobre el torso del preso (`PickBeatPart` sobre `MedicalSystem::anatomy`), con el tope de `CapBeatBlunt`; si el motor bajo mas de la cuenta, la parte se deja en el piso (`BeatHeldHealth`); una parte que ya estaba en el piso o debajo no recibe golpe y no se toca. Puede noquear, nunca mata, y el preso no sale de la jaula. La orden nativa `BEAT_CAGE_OCCUPANT` nunca la tomaba el actor y el respaldo de pelea de TotalSparring sacaba al preso de la jaula: se retiraron los dos (`fix/cage-beat-direct-hit`). `IssueKnockout` = `addOrder(NULL, STEALTH_KNOCKOUT, preso, false, true, posicion)` + `addGoal`. `checkPlayerOrderForProblems` se pregunta y se loguea, no decide (su polaridad no esta probada en el juego): decide la vigilancia, desde `frame_tick` (`TickCageCommands`), solo para Noquear. Respaldo de Noquear: `MedicalSystem::knockout(SkillFraction(STAT_ASSASSINATION))`. Nunca habla con el jugador: devuelve `CageCommandResult` y quien llama (overlay, botones GOLPEAR / NOQUEAR de `offer_popup`) dice `CageCommandProblem`. Noquear actua una vez: la vigilancia sigue despues de "tomada" hasta que el preso cae (o muere), el actor abandona la orden (`kKnockoutAbandonMs` seguidos fuera de ella) o vence `kKnockoutLandWindowMs` (`knockout_watch_rules`), y todo final (caido, abandonada, ventana, respaldo, orden nueva, carga) suelta al actor; la vigilancia guarda las claves del actor y del preso y los vuelve a resolver cada tick (`StillSameCharacter`): un actor que ya no resuelve (zona descargada, liberado) cierra la vigilancia sin limpieza (`KnockoutStep_ActorGone`), un preso que ya no resuelve cuenta como ilegible y el actor se suelta con `CleanUpKnockoutOrder` (`knockout_order_cleanup`); `TickCageCommands` tambien llama a `SweepStrayKnockoutGoals`. Log con `probeOnClick`: `[cage] <actor> -> <preso> beat hit (blunt X, part health A -> B)`, `[cage] <actor> -> <preso> knockout via task|fallback` y `[cage] <actor> knockout cleanup (goal removed, stealth off|kept) after <motivo>`; fallas siempre. `load_boundary` suelta al actor vigilado y lo vacia. |
| `knockout_watch_rules.cpp/.h` | **Puro.** Noquear actua una vez. `NextKnockoutWatchStep`: con el preso caido, muerto o ilegible termina (limpieza) en cualquier fase; esperando que la tome decide `JudgeOrderWatch` (esperar / tomada / respaldo); tomada, termina antes (sin respaldo) si el actor abandona la orden (`ActorAbandonedKnockout`: la tarea no es el noqueo, no queda orden de noqueo en cola y la meta actual no es el noqueo; `NextAbandonedMs` acumula la racha, `kKnockoutAbandonMs` = 3 s) y si no espera hasta `kKnockoutLandWindowMs` (= `kOrderedKnockoutWindowMs`, 2 min) y despues limpia igual. Los tiempos de la vigilancia (el plazo para tomarla, la ventana, la racha de abandono) son su propio reloj: `WatchFrameMs` suma el tiempo real del cuadro, nada con el juego en pausa y como mucho `kWatchMaxFrameMs` = 1 s por cuadro (una pausa no fuerza el respaldo ni vence la ventana). `ShouldDropKnockoutOrder`: KenshiLib 0.5.0 no tiene una llamada para quitar una sola orden (solo `clearOrders`), asi que la orden se suelta solo si `clearOrders` quita exactamente la nuestra: una sola orden en cola y es el noqueo sobre nuestro preso. `ShouldEndStealthAfterKnockout`: el sigilo se apaga solo si la orden lo encendio (no estaba en sigilo al darla y ahora si). `ShouldClearStrayKnockoutGoal`: una meta `STEALTH_KNOCKOUT` sin vigilancia se quita solo si su objetivo se conoce (es la meta actual y nombra a quien) y es un preso de tu jaula (caido no basta: un noqueo del menu del juego es del juego). `StrayGoalSweepDue` / `kStrayGoalSweepMs` = 3 s. Tests: `knockout_watch_rules_tests.cpp`. |
| `knockout_order_cleanup.cpp/.h` | El lado del motor de "actua una vez" (su propia unidad: necesita `AI/AITaskSystem.h`, que no convive con `Tasker.h`). `ReadStealthMode` (al dar la orden). `ReadKnockoutPursuit` (para la vigilancia: orden de noqueo en cola, meta actual = noqueo). `CleanUpKnockoutOrder`: `OrdersReceiver::removeGoal(STEALTH_KNOCKOUT)`; `clearOrders` solo si `ShouldDropKnockoutOrder` (lee `hasPlayerOrder`, `orders.isOnlyOne()` y la primera orden con `getFirstOrder` + `TaskMatch(Tasker*)`: clave y `subject`; `TaskMatch` vive en un marco sin guarda, C2712), nunca a ciegas; `setStealthMode(false)` segun `ShouldEndStealthAfterKnockout`, `reThinkCurrentAIAction` (el par agregar/parar de TotalPredator, `RawStopOrders`). `SweepStrayKnockoutGoals`: cada `kStrayGoalSweepMs` recorre `playerCharacters`; para quien tiene la meta lee el objetivo de `currentGoal` y, si `ShouldClearStrayKnockoutGoal`, quita **solo la meta** (nunca ordenes) y repiensa (el sigilo se deja como esta). Log con `probeOnClick`: `[cage] <actor> knockout cleanup (goal removed[, order cleared], stealth off|kept) ...`, con `stray goal for <objetivo>` en el curado. |
| `rescue.cpp` | Ruta A — lo salvaste de morir. |
| `surrender.cpp` | Ruta C — se rinde. Declara el método de `AI` localmente. |
| `submission_rules.cpp/.h` | **Puro.** El evento del sometido (un NPC vencido camina hasta vos y pide unirse): estados `Idle -> Approaching -> Holding (ventana) -> Forgiven (huye, calmado) -> Done`, el reloj del evento (`AdvanceEventClock`: segundos reales, congelado en pausa, maximo 1 s por cuadro, como TotalSparring), el filtro del barrido (`ScreenCandidate`, del dato mas barato al mas caro), que dice el vistazo al veredicto (`JudgePeek`: rendicion = dispara, field-yield abierto = una tirada con clave, el resto = nada), "una vez por paliza" (`MayAskAgain`), la calma rota (`NextCombatStrikes` / `CalmBroken`: 3 re-afirmaciones seguidas en combate), `HoldStep` (mientras la ventana es suya) y `ForgivenStep`, que se restaura por cada final (`PlanFor`: aceptar recluta; perdonar huye y queda calmado; la X queda calmado en el lugar; rechazar borra derrotas, enfria y vuelve hostil; abortar o terminar la ventana devuelve lo que habia), el punto de huida (`FleePoint`) y el enfriamiento del rechazo (`RejectCooldownUntilHour`, `RejectCooldownActive`, `DecideRejectCooldown`: el unico predicado que corta toda ruta con `Reason_RejectCooldown`). Tests: `submission_rules_tests.cpp`. |
| `approach_rules.cpp/.h` | **Puro.** Port de `TotalSparring/src/duel/challenge/approach_rules` (`ApproachStep`: invalido, combate, llegar, muy lejos, tiempo, repath cada 1 s) y `world_scale.h` (10 unidades = 1 m). Cambio: una ventana abierta no aborta, el NPC llega y espera parado (`Approach_Stand`). Tests: `submission_rules_tests.cpp`. |
| `submission_tracker.cpp/.h` | **Puro**, con lock. Lo que el evento recuerda por `CharKey`: a quien ya se le pregunto y con cuantas derrotas, de quien fallo la tirada hoy, y el enfriamiento del rechazo (hora de juego en que termina), que viaja en el save como `total_recruit.reject_cooldown_until` por `saved_ints`. `load_boundary` lo vacia. Tests: `submission_tracker_tests.cpp`. |
| `submission_director.cpp/.h` | El lado del motor del evento (port de `challenge_director` de TotalSparring), desde `frame_tick` (`SubmissionTick`). Un evento a la vez. Barrido cada 2 s (sin ventana ni suplica abierta): personajes a 40 m del lider, filtrados por `ScreenCandidate`, despues `EvaluatePeek` y, para un field-yield abierto, una sola `Evaluate` con clave (una tirada fallida no se repite ese dia). Mientras camina: calma cada 1 s solo sobre ese NPC (`SafeCalmToward`) y la orden de moverse re-emitida (`npc_motion`). Llega a 3 m y abre `BuildSubmissionOfferView`; la ventana marca su NPC en un `PleaTracker` (`IsSubmissionPending`). Respuestas: `AcceptSubmission` / `ForgiveSubmission` / `RejectSubmission` / `DismissSubmission` (la X). Nunca toca `FactionRelations`. `ForgetSubmission` en la carga le devuelve ritmo y rutina. El walk-up, su reloj y el slot de un evento a la vez (compartido con el ayudante) viven en `approach_driver`. Log `[submission]` siempre. |
| `npc_motion.cpp/.h` | Mover un NPC ajeno, todo portado de TotalSparring `duel/challenge`: `MoveTo` / `MoveToPoint` (`setDestination HIGH_PRIORITY` + `addOrder MOVE_CUS_ORDERED`), `StopWalking`, `BeginStand` / `Stand` (re-emitir la orden al lugar donde llego + `lookatPosition`), `ResumeRoutine` (`clearOrders` + `reThinkCurrentAIAction`) y correr (`BeginRun` / `ReapplyRun` / `EndRun`, guarda y devuelve `getMovementSpeedOrders`). Incluye `AI/AITaskSystem.h`, nunca `Tasker.h`. La escolta del ayudante, sin estado (sirve para varios): `GuardLeader` (`addOrder(NULL, BODYGUARD o FOLLOW_PLAYER_ORDER, lider, ...)`) y `AttackFor` (`addOrder FOCUSED_MELEE_ATTACK` + `attackTarget`, el patron de TotalSparring sin el lado del duelo). |
| `feeding.cpp` | Darle comida de tu mochila. A un animal le anota la comida en `feeding_ledger` (`RecordPlayerMeal`): solo la comida del jugador lo doma (`DecideAnimal`: `fedByPlayer > 0` y barriga sobre la barra; si no, `Reason_TameNotFed` o `Reason_TameHungry`, y `wantsFood` mientras falte cualquiera de las dos). Un animal en tu jaula va directo a `DecideAnimal` al principio de `DecideCage` (sin espera facil, intimidacion ni salvador), y `ChooseEntryRoute` nunca le da `rescue` ni `easy-healthy`. |

### Lo que ve el jugador

| Archivo | Qué hace |
|---|---|
| `recruit_overlay.cpp` | El bloque junto al menú contextual (dos hooks): **Hablar**, y para un preso despierto en tu jaula **Hablar, Golpear, Noquear** (`ComposeContextBlock`; `TR_RecruitButton`, `TR_BeatButton`, `TR_KnockoutButton`). Pide a `menu_anchor` el rectangulo del menu (una o dos columnas) y delega el tamaño, el estilo y el lugar en `button_block` (una sola llamada a `PlaceButtonBlock`, con la union como menu y el ancho de estilo de la primera columna, como antes). Cada botón escucha presionar y soltar con izquierdo o derecho y su propio gesto deja pasar un solo evento (`menu_button_release`). Golpear / Noquear llaman a `cage_commands`. Tambien dice si el menu contextual esta abierto (`IsContextMenuOpen`, con cualquier objetivo): la tarjeta de lectura se oculta debajo. |
| `button_style.cpp/.h` | El look compartido con TotalSparring y TotalLoot: 44 px de alto, 160 de ancho (o el ancho del menú si es menor), skin `Kenshi_Button1` (respaldo `Button`), capa `Popup`, fuente 18, texto ámbar con sombra y relleno ámbar al pasar el mouse. Port de `TotalLoot/src/button_style`. |
| `menu_anchor.cpp/.h` | Donde esta el menu nativo para el bloque (prisoner-module-fixes F4). Un personaje dentro de un edificio (un preso en la jaula) abre dos columnas lado a lado: la del personaje (`ContextMenu::menuGUI`, 0x48) y la del edificio (`menuGUI2`, 0x50; "Jaula de Prisionero": ABRIR CON LLAVE, DESMANTELAR, PONER EN LIBERTAD). `ReadMenuAnchor` lee el `optionsList` (0xF8) de las dos detras de guardas SEH y pide `UnionMenuRect`: **puro e inline en el header** (los tests lo incluyen sin Kenshi ni MyGUI): la primera columna se toma como siempre (presente y con ancho, sin preguntar si es visible), la segunda solo presente, visible (`getInheritedVisible`) y con ancho; con una sola columna el rectangulo es exactamente el de antes. `menu_slot` no cambia. Log con `probeOnClick`, una vez por menu abierto y otra solo si cambia: `[menu-anchor] gui1=(l,t,w,h) gui2=(l,t,w,h|none) union=(l,t,w,h)` (`ResetMenuAnchorLog` desde el hook de apertura). Tests: `menu_anchor_tests.cpp`. |
| `button_block.cpp/.h` | Crea el botón con ese estilo (`CreateContextButton`) y lo ubica (`PlaceButtonBlock`): arma el pedido para `menu_slot` con los obstáculos de `menu_obstacles` y recuerda el lado elegido (`g_previousSide` en el overlay) para no saltar entre cuadros. Port de `TotalLoot/src/button_block`. |
| `menu_slot.cpp/.h` | **Puro.** Elige un hueco libre al lado del menú: izquierda arriba, derecha arriba, izquierda corrida debajo de lo que haya, derecha corrida, y si nada entra, izquierda ajustada a la pantalla. Si el lado anterior sigue libre, gana (anti-parpadeo). Port verbatim de `TotalLoot/src/menu_slot`; tests en `tests/rules_tests/menu_slot_tests.cpp`. |
| `menu_obstacles.cpp/.h` | Junta los rectángulos a evitar: widgets raíz visibles de MyGUI más los botones conocidos de otros mods por nombre (`TL_*` de TotalLoot, `TS_*` de TotalSparring, `TP_EatButton` de TotalPredator, `LSE_*` de Loot-Scoot-Execute). Ignora los propios (`TR_`) y lo que `IsPlausibleObstacle` descarta (el menú, capas de pantalla completa). |
| `menu_button_release.cpp/.h` | **Puro.** Port de `TotalLoot/src/click_gesture`: un gesto del menu contextual actua una sola vez. El boton **Hablar** escucha presionar **y** soltar (como TotalSparring y TotalLoot), asi funciona el gesto nativo de Kenshi (mantener clic derecho, deslizar sobre la opcion, soltar) ademas del clic izquierdo; el primer evento armado actua y su par es eco. Se arma al abrirse el menu. Tests: `menu_button_release_tests.cpp`. |
| `offer_view.cpp/.h` | **Puro.** Que muestra la ventana de reclutamiento (`BuildOfferView`, `BuildMercyOfferView`, `BuildSubmissionOfferView`: Sometido, "se arrodilla y te pide unirse a tu grupo", Gratis, ACEPTAR / PERDONAR / RECHAZAR): titulo, nombre, etiqueta de ruta, cuerpo, precio / lo que tenes / probabilidad, la linea roja y que botones existen. Precio en cats mayor que la bolsa: bolsa en rojo, "No te alcanzan los cats (te faltan N)" y sin ACEPTAR. Todo rechazo: motivo en rojo, sin ACEPTAR, y DARLE COMIDA si hay comida. Rechazo de un preso para el que `CageCommandsAllowed` (el mismo predicado del menu): GOLPEAR y "NOQUEAR NN%" (`ApplyCageCommands`; nunca en un si ni con el slider); sin ellos, un desafiante lee `popup.reason.defiantNoBeat` (no pide herirlo). Quien no puede irse (`inCage` = `SafeInPrison`) nunca ve IGNORAR (piedad) ni PERDONAR (sometido). Regateo (`OfferHaggleInput`, solo precios en cats, nunca rescate/piedad/rendición): slider del mínimo al pedido, la probabilidad en vivo (`AcceptOdds`, tono `OddsToneOf`: verde desde 70, ámbar 30-69, rojo debajo), la bolsa contra la **oferta** (falta → línea roja del slider, sin botón), OFRECER bajo el pedido y ACEPTAR en el pedido; ofendido hoy: "Se ofendio: hoy no regatea" sin slider. Aliado de Balanced con tramo (`Verdict::allyTiered`): el cuerpo nombra el tramo (`popup.ally.*`: "Aliado (nivel 34): ... con 14% de descuento por amistad", "es tu aliado (nivel 12) y se une gratis", "te acompañará 6 días como miembro temporal") y `priceTag` lleva "-14%" junto al precio (`offer_popup` lo dibuja dos espacios después). Tests: `offer_view_tests.cpp`. |
| `offer_popup.cpp` | La ventana de reclutamiento: **toda** salida de Hablar la abre (ya no hay notificacion de precio ni de rechazo). Marco `Kenshi_WindowCX` en la capa `Info` (titulo, X = Cancelar, arrastre) y adentro el arte plano del mockup (`ui-mockup/readout.html` seccion 5; `LayoutUnguarded` = `layoutOffer` de `js/popup.js`): placa laca, retrato enmarcado (`PortraitManager::setImageWidgetNPC`), nombre en bronce, caja de datos, motivo en rojo, botones planos. Mide el texto (`MeasureWrapText`) en vez de adivinar. Dibuja un `OfferView`; no decide nada. Con regateo (`ShowPricedOffer`) guarda el `OfferInput`: mover el slider reconstruye la vista y solo cambia textos, colores y la fila de botones (la sección tiene alto fijo, la ventana no se re-mide). OFRECER va a `CompleteHaggle`. Todo boton y la X pasan por un solo punto, `Answer`: lee que hay pendiente (suplica de `mercy`, sometido de `submission_director`, oferta de ayuda de `helper`, o una oferta comun), cierra y despacha; la X es "Rechazar" salvo para el sometido y el ayudante, donde es "sin decision" (`DismissSubmission` / `DismissHelperOffer`). GOLPEAR / NOQUEAR dan las ordenes de `cage_commands` al seleccionado (`OrderSelectedToBeat` / `OrderSelectedToKnockout`) y avisan `CageCommandProblem`. Hasta tres botones en una fila; con mas, CERRAR baja solo a una segunda fila a la derecha (`kButtonsPerRow`). |
| `flat_slider.cpp/.h` | El slider plano de TotalSparring (`shared/ui/flat_slider`, el de la apuesta del duelo) portado: riel golddim/sumi, pulgar laca/ámbar de 22 px, superficie `Kenshi_TextBoxEmptySkin` que toma clic y arrastre en toda la fila. Se crea en dos pasos (`MakeFills` con los rellenos, `MakeSurface` después). La matemática pura (`ValueFromX`, `ThumbLeftFor`, `ClampSliderValue`) va inline en el header sin MyGUI. Tests: `flat_slider_tests.cpp`. |
| `ui_style.cpp/.h` | El look compartido por la tarjeta y la ventana: paleta (`ui-mockup/css/base.css`), fuentes, `EnsureResources` (`gui/images/tr_white.png`, si falta `WhiteSkin`), `MakeFill` / `MakeLabel` / `MakeWrapText` / `Place` / `PlaceFrame`, y el boton plano de TotalSparring (`FlatButton`: placa + borde opcional + boton sin skin encima). Regla: lo texturizado se crea antes que lo que va encima. |
| `hail.cpp` | Los cautivos avisan, programado sobre el reloj de juego. Cada llamado tambien levanta el cartel sobre la cabeza (`RaiseHeadNotice`, `headNotice.wantsToTalk`). |
| `head_notice_rules.cpp/.h` | **Puro.** Quien tiene cartel "quiere hablar contigo" (`HeadNoticeBoard`): hasta `kHeadNoticeCap` (8), uno por `CharKey`; levantar otra vez reemplaza texto y vencimiento (el mas nuevo), lleno se va el mas viejo; `Expire` (vence en `expiresAt <= now`), `Dismiss`. `DecideNoticeFrame`: seleccionar a ese NPC lo descarta; sin lugar, con menu contextual o con una ventana nativa abierta (`nativeWindowOpen`, la respuesta de `OverlaysYieldToNativeWindow`) se oculta ese cuadro; si toca la tarjeta normal, gana la tarjeta (`RectsOverlap`). `ClampHeadNoticeSeconds` (10..3600), `ClampHeadNoticeInfoSeconds` (5..3600) y `HeadNoticeSecondsFor` (que duracion toca segun el tipo, `HeadNoticeKind_Prompt` o `HeadNoticeKind_Info`). Tests: `head_notice_rules_tests.cpp`. |
| `head_notice.cpp/.h` | El cartel sobre la cabeza de quien te llama. API para cualquier evento nuevo: `RaiseHeadNotice(Character*, texto, kind = HeadNoticeKind_Prompt)` y `DismissHeadNotice(Character*)`. Hoy: `hail.cpp` y `submission_director.cpp` (al caminar; se baja al abrir la ventana o en todo final del evento, `Apply`). Reloj propio: segundos reales congelados en pausa (`AdvanceEventClock`). Los carteles que piden algo al jugador duran `headNoticeSeconds` (90); los informativos de una linea (`HeadNoticeKind_Info`: union, avisos y despedida del temporal, frases del ayudante) duran `headNoticeInfoSeconds` (25). Cada cuadro desde `frame_tick` despues de `ReadoutTick`: lee la seleccion (`SelectedNpc`, el cambio a ese NPC lo descarta), la tarjeta normal (`ReadoutCardOnScreen`), proyecta con `ReadoutAnchorFor` y ubica con `PlaceCard` (de costado junto a la oferta). Cada 500 ms baja los de NPCs muertos, unidos o con otra clave en esa direccion. Widgets: 8 tarjetas en la capa `ToolTip`, sin foco, look de la tarjeta (placa laca, marco, borde ambar, nombre bronce, linea ambar; `.hn` del mockup); solo rearma el layout si cambian nombre o linea. `load_boundary` lo vacia (`ForgetHeadNotices`), `ShutdownFrameTick` destruye los widgets. `RaiseSquadHeadNotice` es el cartel de un miembro del grupo (el temporal): no se cae por ser tuyo y seleccionarlo no lo quita. |
| `readout.cpp/.h` | La tarjeta de estres y chance sobre el NPC, de punta a punta. Dos entradas. **Seleccion** (`readoutOnSelect`, encendido por defecto): sigue al NPC que el jugador selecciona con clic izquierdo (nunca uno propio); cada cuadro solo compara el puntero de la seleccion con el anterior, calcula (`EvaluatePeek`, sin efectos) cuando cambia y refresca cada 2 s (`kSelectionRefreshMs`) para que precio y estres sigan al dia. **Hablar** (`RequestReadout`, llamado desde `recruit_overlay.cpp`, no desde `AttemptRecruit`, asi el rescate no la abre): se calcula al clic y queda sobre la cabeza mientras la ventana de oferta esta abierta; solo si la taparia se corre de costado (`PlaceCard`); al cerrarse la oferta (o a los 8 s si no se abrio) queda solo si ese NPC sigue seleccionado. Cada cuadro solo se mueve; se oculta fuera de pantalla, con un menu contextual abierto y con el dialogo de Kenshi o un mensaje modal abierto (`NativeWindowCoversOverlays`, tambien fijada por Hablar; vuelve al cerrarse), y se va si el NPC muere. Con `probeOnClick` loguea `[readout] ... [hablar|select]` lo mismo que se ve, y una vez por resultado y NPC `[readout-anchor] <nombre> [select|hablar] <resultado> head=(x,y) view=WxH world=(..) origin=(..) card=(..) offer=(..)`: donde quedo la tarjeta y por que se oculto. |
| `readout_model.cpp/.h` | **Puro.** Que muestra la tarjeta (`BuildReadout`): ruta, estres y sus 3 partes, chance, derrotas, barriga, linea principal y espera. Solo reusa numeros que la decision ya calculo. Field-yield tiene su propia ruta (`ReadoutRoute_FieldYield`, "Sometido"). La tarjeta pregunta sin clave de tirada (`rollKeyed=false`): un "todavia no" de una tirada que no pudo hacer (field-yield, aliado) se muestra como la probabilidad (`kLineRollOpen`, "{chance}%: puede unirse si le hablas"); "dejalo sin comer" solo para un preso en tu jaula, si no `kLineFieldYieldBeatAgain` ("vencelo otra vez"). El precio de un aliado con tramo es `kLineAllyPaidJoin` / `kLineAllyPaidAsk` ("Aliado: 1.840 cats con descuento"). Un miembro temporal seleccionado (`ReadoutInput::tempMember`) muestra solo `kLineTempMemberDaysLeft` ("Temporal: le quedan N dias", los dias en `need`); `readout.cpp` lo elige con `SelectedTempMember` cuando la seleccion no es un NPC ajeno. |
| `readout_catalog.cpp/.h`, `readout_format.cpp/.h` | **Puros.** La linea corta de cada motivo (un renglon por `ReasonKey` + la espera, la probabilidad abierta y "vencelo otra vez"; el test exige que quepan en la tarjeta), la etiqueta de cada ruta (`ReadoutRouteMessageKey` / `ReadoutRouteDefaultText`: una tabla para la tarjeta y la ventana) y como se escriben cats (`9.570`), horas (`3h`, `45min`) y `{chance}`. |
| `readout_text.cpp/.h` | Las palabras de la tarjeta: `messages.json` bloque `readout`, con texto interno si falta. |
| `readout_target.cpp/.h` | Lee la seleccion del jugador (`SelectedNpc`: `ou->player->selectedObject`, si no es un personaje propio) y el ancla de la tarjeta (`ReadoutAnchorFor`): `getPosition()` + 20 unidades (sobre la cabeza), las matrices de la camara del jugador y el origen relativo de la escena (`SceneManager::getRelativeOrigin`), y devuelve todo en un `AnchorReading` para el log. El modo hover se quito: era lento y tapaba la pantalla. Solo lee. |
| `screen_projection.cpp/.h` | **Puro.** `ProjectToScreen`: punto del juego a pixeles, **restando el origen relativo** antes de la vista (como `UtilityT` del motor, RVA 0x9B12C0; `CameraClass::getCameraPos` 0x1008B0 lo suma de vuelta). Sin ese paso la tarjeta caia en el horizonte, "volando arriba". `PlaceCard`: sobre la cabeza, centrada, dentro de la pantalla de costado; si tapa la ventana de oferta, al costado mas cercano a la misma altura, o al otro; sin lugar, oculta. Nunca la mueve arriba/abajo. Tests: `screen_projection_tests.cpp`. |
| `card_dismiss_rules.cpp/.h` | **Puro.** Cuando un clic en otro lado guarda la tarjeta (Kenshi no limpia la seleccion al caminar con clic derecho, asi que antes no se iba nunca). `NextPressEdge`: el clic que empezo este cuadro (la primera lectura solo anota los botones). `PointOnBody`: el cursor dentro de la caja del cuerpo, de los pies a la cabeza proyectados, media anchura 0,3 del alto (minimo 10 px). `NextCardDismissed`: un clic izquierdo o derecho fuera de un widget MyGUI y fuera del NPC la guarda; clic izquierdo en el NPC la vuelve a mostrar, el derecho (menu) no cambia nada; un cambio de seleccion o `Hablar` la muestran; con `Hablar` y la oferta abierta los clics de afuera no cuentan. Tests: `card_dismiss_rules_tests.cpp`. |
| `overlay_visibility_rules.cpp/.h` | **Puro.** `OverlaysYieldToNativeWindow`: la tarjeta y los carteles se ocultan (se guardan, no se descartan) mientras hay una conversacion (`inDialogue` o la `DialogueWindow` visible, cualquiera de las dos) o un mensaje modal; una lectura fallida nunca los oculta. Una sola regla para los dos, asi no pueden discrepar. Ventanas de comercio/inventario no: `isAnyInventoryWindowOpen` tambien es verdad con el inventario propio abierto. Tests: `overlay_visibility_rules_tests.cpp`. |
| `native_windows.cpp/.h` | La lectura con guarda de las ventanas nativas, todo no virtual en `gui` (`ForgottenGUI`): `inDialogue()` y `hasModalMessage()` (lo mismo que usa `TotalLoot/src/pickpocket_reads.cpp`) y `gui->dialogue->getVisible()` (`DialogueWindow`; `isVisible` es virtual, no se llama). `NativeWindowCoversOverlays` = lectura + regla; `readout.cpp` (`Present`) y `head_notice.cpp` (`DrawNotices`) la piden una vez por cuadro. Con `probeOnClick` loguea cada cambio: `[native-window] open: dialogue=1 window=1 modal=0` / `[native-window] closed`. |
| `mouse_input.cpp/.h` | Lecturas del mouse con guarda, sin estado: botones (`GetAsyncKeyState`, como `OverlayDismiss` de TotalSparring), si hay un widget MyGUI bajo el cursor (`getMouseFocusWidget`) y la posicion del cursor. `readout.cpp` lee los botones una vez por cuadro solo mientras hay tarjeta (o una guardada que puede volver), y la caja del NPC (`ReadoutBodySpanFor` de `readout_target`) solo al momento del clic. |
| `readout_ui.cpp/.h` | Los widgets MyGUI de la tarjeta (capa `ToolTip`, sin foco de mouse; paleta y rellenos de `ui_style`). Medidas = las variables CSS de `ui-mockup/css/readout.css`. Solo rearma el layout cuando cambian los textos o la vista (`LayoutKey`); si no, solo la ubica. Donde va lo decide `PlaceCard` (`screen_projection`, igual que `placeCard` del mockup); `PlaceUnguarded` la pone ahi o la oculta si no hay lado libre, y devuelve un `ReadoutPlacement` para el log. |
| `dialogue_authoring.cpp` | Escribe las líneas y las burbujas. |
| `mercy.cpp` | Ruta piedad: al perder un miembro, tres respuestas. |
| `mod_hub_integration.cpp` | Ajustes en el panel de Mod Hub (`src/emc/` es su API). |
| `spawn_panel.cpp` / `spawn_core.cpp` | Panel de spawn. |

### Soporte

| Archivo | Qué hace |
|---|---|
| `cursor_hint.cpp` | El globo de chat vanilla sobre NPCs hablables. Pregunta como espectador (`EvaluatePeek`: sin claves de tirada, sin reloj de encierro, sin log). |
| `talk_hint_cache.cpp/.h` | **Puro.** La memoria del globo: respuesta por NPC con TTL de 2 s, hasta 64 entradas (desaloja la mas vieja). Se vacia en la carga (`ForgetCursorTarget`). Tests en `tests/rules_tests`. |
| `frame_tick.cpp` | Callback por frame, hookeado a `ForgottenGUI::update`: rescate, avisos, tarjeta, cartel sobre la cabeza (`HeadNoticeTick`, despues de la tarjeta), ordenes de jaula, el evento del sometido (`SubmissionTick`), los miembros temporales (`TempMemberTick`), el reto a duelo (`DuelChallengeTick`: engancha TotalSparring y actua sobre sus peleas) y Mod Hub. |
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
3. Los headers en `deps/kenshilib-0.5.0/Include/kenshi/` — la única fuente de
   verdad sobre qué existe (TotalRecruit usa 0.5.0; las rutas de §5 apuntan a la
   copia 0.4.0 compartida, que sirve para buscar pero puede diferir).

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
cd /f/Proyectos/Kenshi-mods/TotalRecruit/deps/kenshilib-0.5.0/Libraries
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

Where a rule lives decides how it is tested:

- What a route decides (`route_*.cpp`) takes `CoreFacts` and a per-route extra from
  `entry_facts.h` plus the `Config`, and returns a `Verdict` whose reason is a
  `ReasonKey`. Test it in `route_decisions_tests.cpp` with plain structs. A roll is
  seeded from the character and the in-game day (`RollFact.dayIndex`), so a test
  searches for a key that rolls the way the case needs on a fixed day (`KeyRolling`)
  instead of depending on the hash.
- What the engine says (`character_facts.cpp`, `engine_reads.cpp`) is not tested; it is
  `QA.md`. Keep `Gather*` free of decisions: if it needs an `if` that is a game rule,
  the rule belongs in a `route_*.cpp` and the gather only reads the extra input.
- Adding a reason: a `ReasonKey` in `verdict.h`, a row in `reason_catalog.cpp` (key, built-in
  text, tone), the line in `messages.json`. The catalog test fails if a row is missing.
- Adding a tunable: the field in `config.h` with its default, one line in `ConfigFields`
  (`config.cpp`), a clamp in `LoadConfig` if a bad value can break the arithmetic, a control
  in `mod_hub_integration.cpp`, a test that the default is the old value and that a
  different value reaches the result.
