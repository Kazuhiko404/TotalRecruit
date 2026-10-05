# Insistencia solo por rechazos reales, animales sin insistencia; en Cheat un animal alimentado se une

Rama: `fix/animal-insistence-cheat-feed` (sobre `7bf8750`).
Ruta: escritor delegado (un solo escritor acotado; 2+ archivos no triviales por tarea).

## Objetivo
Que la insistencia (R2.4) castigue solo la insistencia real: un rechazo que es una decision del NPC contra el jugador. Que un animal nunca se harte. Que en Cheat un animal que comio de la mano del jugador se una al momento.

## Problema
- Log real (Balanced): `Iridescent Beak Thing -> refuses (cuidados) esta malherido: ninguna comida arregla eso` (x2) y despues `refuses (rechazo) esta harto de tu insistencia`. `Bone Jackal Alpha` termina en "Harto de ti: dejalo en paz un rato" tras "primero las heridas, despues la comida". Entre medio, "inconsciente: espera a que despierte".
- `AttemptRecruit` registraba un intento ANTES de evaluar, en cada Hablar: contaban los estados (herido, inconsciente, hambriento, en jaula ajena), las instrucciones y hasta los si.
- En Cheat un animal solo se une en la jaula del jugador tras 1 minuto, aunque ya haya comido de su mano.

## Por que
La insistencia existe para que preguntar lo mismo a quien ya dijo que no tenga un costo (R2.4). Un "curalo primero" no es un no: es la instruccion que el jugador esta siguiendo. Un animal no negocia: se doma con comida (R5). Cheat es un gesto, no un costo (R1.2): comer de tu mano ya es el gesto.

## Alcance
- A1: regla pura `insistence_rules` (`ReasonCountsAsInsistence`, `CountsAsInsistence(verdict, animal)`), tabla de cada `ReasonKey`; `AttemptRecruit` registra despues de evaluar y solo si cuenta; `DecideAccessRefusal` ignora la insistencia de un animal; `GatherAccess` no la lee para animales.
- A2: `CheatFacts` gana `animal`, `belongsToSomeone`, `fedByPlayer` (el mismo `PlayerMeals` de `GatherAnimal`); `DecideCheat` une al animal alimentado sin jaula ni espera (`Reason_TameReady`); un animal sin alimentar fuera de la jaula sigue pidiendo jaula pero ofrece el boton de comida.

## Restricciones
- Toolset v100: sin C++11. Lecturas del motor detras de `TR_TRY/TR_CATCH` en marcos solo POD (C2712).
- No se toca el `config.json` desplegado; plantillas solo claves NUEVAS. Sin deploy, sin push, sin merge.
- Modulos aislados (SOLID), sin parche sobre parche. Las DLL versionadas no se commitean.

## Checklist
- [x] A1 insistencia solo por rechazos reales; animales sin insistencia: `insistence_rules` (`ReasonCountsAsInsistence`, `CountsAsInsistence`, tabla en el header: 12 razones cuentan, el resto no, una razon nueva no cuenta); `AttemptRecruit` registra despues de `Evaluate` y solo si cuenta; `DecideAccessRefusal` ignora el bloqueo de un animal; `GatherAccess(character, animal)` no lo lee para animales. Ruta: escritor delegado. Evidencia: RED = C1083 `insistence_rules.h`, luego 1 check (animal con bloqueo guardado); GREEN = 51/51, 0/72539; DLL compila. Commit `7767860`.
- [x] A2 Cheat: un animal alimentado se une al momento: `CheatFacts::animal/belongsToSomeone/fedByPlayer` (`GatherCheatFacts`, mismo `PlayerMeals` que `GatherAnimal`); `DecideCheat` une al animal sin dueno alimentado con `Reason_TameReady` (con o sin jaula, sin espera); sin comer y fuera de la jaula: `Reason_CheatNeedsCage` + `Currency_Care` + `wantsFood` (boton Dar comida). Sin claves nuevas en messages.json (reusa `animal.tame.ready` / `readout.line.animal.tame.ready`); sin maqueta (no cambia ningun elemento visible: la linea y el boton ya existen). Ruta: escritor delegado. Evidencia: RED = C2039 `CheatFacts::animal`, luego 9 checks; GREEN = 51/51, 0/72566; DLL compila. Commit en el registro de abajo.

## Criterios de aceptacion
- Herido, inconsciente, muriendose, hambriento, esperando, en jaula ajena, sin alimentar, enfriamiento de rechazo, la propia insistencia y los si no suman intentos; un rechazo real (tirada fallida, honor, lealtad, rencor, tier) si.
- Un animal nunca responde "harto de tu insistencia", aunque tuviera un registro viejo en memoria.
- En Cheat un animal alimentado (y sin dueno) se une sin jaula; uno sin alimentar fuera de la jaula pide la jaula y ofrece comida; la jaula + 1 minuto sigue igual; humanos y `cheatNeedsCage=false` sin cambios.
- rules_tests pasa entero; TotalRecruit.dll compila.

## Checks
- `"$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 && tests/rules_tests/x64/Release/rules_tests.exe`
- `"$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64`
- `scripts/deploy.sh --dry-run`: config.json / messages.json solo agregan claves.

## Progreso
- Documento creado antes de tocar el codigo.
- A1 `7767860`, A2 hechos en `fix/animal-insistence-cheat-feed`. Sin deploy, sin push.

Decisiones:
- A1: `CatsAsk` no cuenta (un precio sobre la mesa es "trae los cats", no un no). `Defiant` y `UniqueZealot` cuentan (rechazo real), aunque la jaula nunca consulta el castigo (12.7 N1). Un si nunca cuenta.
- A1: el intento se registra despues de contestar: el tercer no real todavia dice su motivo y la cuarta charla oye "harto" (antes la tercera ya oia "harto" porque se contaba antes de evaluar).
- A1: la insistencia vive solo en memoria (una recarga la olvida): no hay saves trabados; un registro viejo en la sesion ya no bloquea a un animal.
- A2: un animal con dueno sigue entrando solo por la jaula (como la ruta animal de Balanced fuera de la jaula). `animalsEnabled` no se lee en Cheat (como antes); `exclude.animals` corta antes de evaluar.
- A2: el camino de la jaula queda igual (sin `isAnimal`, "quiere hablar contigo: se une"); solo los caminos nuevos marcan `isAnimal`.

## Siguiente paso
Deploy (lo hace el padre) y QA en juego: QA 5 I1, I6-I9; 8 H11-H14; 12.7 N1.
