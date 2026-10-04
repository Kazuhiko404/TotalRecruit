# Prompt de arranque: ajuste fino de Balanced

Pegar en una sesion nueva:

```text
Trabajamos en el ajuste fino de Balanced de TotalRecruit, en F:\Proyectos\Kenshi-mods\TotalRecruit, rama feat/balanced-tuning (no crear ni borrar ramas, no push, no deploy sin pedirlo).
Lee primero: odd/tasks/balanced-tuning.md, odd/balance/balance-sheet.md y odd/system-status.md.
Engram: proyecto totalrecruit; corre mem_context y mem_search "odd/balanced-tuning/tasks", y reconcilia el espejo con el archivo.
Flujo ODD: explorar, tarea por tarea, test-first con tests/rules_tests, un commit (Conventional Commits) por tarea, documento y espejo al dia.
Antes y despues de cada cambio de numeros regenera la hoja (MSBUILD="/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"):
"$MSBUILD" tests/balance_sheet/balance_sheet.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo && tests/balance_sheet/x64/Release/balance_sheet.exe "$(git rev-parse --short HEAD)" > odd/balance/balance-sheet.md
Respeta los principios del documento: lore-friendly, aleatoriedad min/max estable (DailySeed, nunca re-tirable), todo numero configurable (config.json + Mod Hub), modulos aislados SOLID sin parche sobre parche, maqueta en ui-mockup/ antes de tocar UI, nunca pisar el config.json desplegado.
Orden: empieza por T1 y confirmame la tabla objetivo (es la unica pregunta de producto); despues T2-T4, luego T5, luego T6.
Una pregunta por vez y solo para decisiones de producto reales; el resto decidilo con la hoja.
```
