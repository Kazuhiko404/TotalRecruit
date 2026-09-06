#include "mod_hub_integration.h"
#include "config.h"
#include "log.h"
#include <windows.h>
#include <tlhelp32.h>
#include "emc/mod_hub_api.h"
#include <sstream>

namespace totalrecruit
{

typedef EMC_Result(__cdecl* GetApiFn)(uint32_t, uint32_t, const EMC_HubApiV1**, uint32_t*);

static GetApiFn ResolveHubGetApi(const void** whereModule)
{
    static const char* kNames[] = { "Emkejs-Mod-Core.dll", "Emkejs-Mod-Core" };
    for (size_t i = 0; i < sizeof(kNames) / sizeof(kNames[0]); ++i)
    {
        HMODULE h = GetModuleHandleA(kNames[i]);
        if (!h) continue;
        FARPROC p = GetProcAddress(h, EMC_MOD_HUB_GET_API_EXPORT_NAME);
        if (!p) p = GetProcAddress(h, EMC_MOD_HUB_GET_API_COMPAT_EXPORT_NAME);
        if (p) { if (whereModule) *whereModule = (const void*)h; return (GetApiFn)p; }
    }
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32 me; me.dwSize = sizeof(me);
    GetApiFn fn = 0;
    if (Module32First(snap, &me))
    {
        do {
            FARPROC p = GetProcAddress(me.hModule, EMC_MOD_HUB_GET_API_EXPORT_NAME);
            if (!p) p = GetProcAddress(me.hModule, EMC_MOD_HUB_GET_API_COMPAT_EXPORT_NAME);
            if (p) { if (whereModule) *whereModule = (const void*)me.hModule; fn = (GetApiFn)p; break; }
        } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
    return fn;
}

static EMC_Result __cdecl cbGetBool(void* user_data, int32_t* out_value) { *out_value = *(bool*)user_data; return EMC_OK; }
static EMC_Result __cdecl cbSetBool(void* user_data, int32_t value, char* err_buf, uint32_t err_buf_size) { *(bool*)user_data = (value != 0); SaveConfig(); return EMC_OK; }
static EMC_Result __cdecl cbGetInt(void* user_data, int32_t* out_value) { *out_value = *(int32_t*)user_data; return EMC_OK; }
static EMC_Result __cdecl cbSetInt(void* user_data, int32_t value, char* err_buf, uint32_t err_buf_size) { *(int32_t*)user_data = value; SaveConfig(); return EMC_OK; }
static EMC_Result __cdecl cbGetFloat(void* user_data, float* out_value) { *out_value = *(float*)user_data; return EMC_OK; }
static EMC_Result __cdecl cbSetFloat(void* user_data, float value, char* err_buf, uint32_t err_buf_size) { *(float*)user_data = value; SaveConfig(); return EMC_OK; }

// Every register_* result was being dropped, so a hub that refused a setting
// looked identical to one that accepted it and the panel just came up short.
// Now a refusal names itself in the log.
static void reportReg(const char* kind, const char* id, EMC_Result r)
{
    if (r == EMC_OK) return;
    char buf[160] = { 0 };
    _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                "Mod Hub rejected %s setting '%s' (result %d).", kind, id, (int)r);
    LogError(std::string(buf));
}

static void regBool(const EMC_HubApiV1* api, EMC_ModHandle mod, const char* id, const char* label, const char* desc, bool* field)
{
    if (api->register_bool_setting == NULL)
    {
        LogError(std::string("Mod Hub has no register_bool_setting; '") + id + "' skipped.");
        return;
    }
    EMC_BoolSettingDefV1 d = { id, label, desc, field, &cbGetBool, &cbSetBool };
    reportReg("bool", id, api->register_bool_setting(mod, &d));
}

static void regInt(const EMC_HubApiV1* api, EMC_ModHandle mod, const char* id, const char* label, const char* desc, int32_t lo, int32_t hi, int32_t step, int32_t* field)
{
    if (api->register_int_setting == NULL)
    {
        LogError(std::string("Mod Hub has no register_int_setting; '") + id + "' skipped.");
        return;
    }
    EMC_IntSettingDefV1 d = { id, label, desc, field, lo, hi, step, &cbGetInt, &cbSetInt };
    reportReg("int", id, api->register_int_setting(mod, &d));
}

static void regFloat(const EMC_HubApiV1* api, EMC_ModHandle mod, const char* id, const char* label, const char* desc, float lo, float hi, float step, int dec, float* field)
{
    if (api->register_float_setting == NULL)
    {
        LogError(std::string("Mod Hub has no register_float_setting; '") + id + "' skipped.");
        return;
    }
    EMC_FloatSettingDefV1 d = { id, label, desc, field, lo, hi, step, (uint32_t)dec, &cbGetFloat, &cbSetFloat };
    reportReg("float", id, api->register_float_setting(mod, &d));
}

static bool g_hubHardFail = false;
static int g_hubState = 0; // 0 = not attached, 1 = attached, -1 = hard fail

static bool DoRegisterModHub()
{
    const void* where = 0;
    GetApiFn getApi = ResolveHubGetApi(&where);
    if (!getApi) return false;

    const EMC_HubApiV1* api = 0; uint32_t apiSize = 0;
    EMC_Result gr = getApi(EMC_HUB_API_VERSION_1, EMC_HUB_API_V1_MIN_SIZE, &api, &apiSize);
    if (gr != EMC_OK || !api || apiSize < EMC_HUB_API_V1_MIN_SIZE)
    {
        char buf[160] = { 0 };
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                    "Mod Hub API negotiation failed: result=%d size=%u (need >= %u).",
                    (int)gr, (unsigned)apiSize, (unsigned)EMC_HUB_API_V1_MIN_SIZE);
        LogError(std::string(buf));
        g_hubHardFail = true;
        return true;
    }

    // Which slots the hub actually handed back decides which settings can be
    // registered at all: a smaller struct silently ends before the later
    // register_* pointers.
    {
        char buf[160] = { 0 };
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                    "Mod Hub attached: api v%u, size %u bytes.",
                    (unsigned)api->api_version, (unsigned)apiSize);
        LogInfo(std::string(buf));
    }

    EMC_ModDescriptorV1 desc = {0};
    desc.namespace_id = "totalrecruit";
    desc.namespace_display_name = "TotalRecruit";
    desc.mod_id = "totalrecruit";
    desc.mod_display_name = "TotalRecruit";
    desc.mod_user_data = NULL;

    EMC_ModHandle mod = 0;
    EMC_Result rr = api->register_mod(&desc, &mod);
    if (rr == EMC_ERR_CONFLICT) return true;
    if (rr != EMC_OK || !mod)
    {
        g_hubHardFail = true;
        return true;
    }

    Config& c = GetConfig();

    regBool(api, mod, "show_recruit_button", "Mostrar botón UI", "Muestra un botón de reclutar en el panel", &c.showRecruitButton);
    regBool(api, mod, "enabled", "Mod activo", "Apaga el mod entero sin desinstalarlo", &c.enabled);
    regBool(api, mod, "auto_recruit", "Reclutar al aceptar", "Ejecuta el reclutamiento al aceptar la oferta", &c.autoRecruit);
    regBool(api, mod, "show_talk_cursor", "Cursor de hablar", "Cambia el cursor sobre reclutables (sin efecto conocido)", &c.showTalkCursor);
    regBool(api, mod, "resolve_cage_occupant", "Resolver ocupante de jaula", "Al clickear una jaula, apunta a quien esta dentro", &c.resolveCageOccupant);
    regBool(api, mod, "probe_on_click", "Debug Log (Rasgos)", "Muestra rasgos en el log al hacer clic", &c.probeOnClick);
    
    // Rutas
    regBool(api, mod, "rescue_enabled", "Ruta: Rescate", "Permite reclutar curando a gente", &c.rescueEnabled);
    regBool(api, mod, "rescue_player_only", "Rescate: solo tus medicos", "Solo cuenta si lo curo alguien de tu escuadron", &c.rescuePlayerOnly);
    regBool(api, mod, "captivity_enabled", "Ruta: Jaulas", "Permite reclutar de jaulas", &c.captivityEnabled);
    regBool(api, mod, "surrender_enabled", "Ruta: Rendición", "Los enemigos se rinden al estar heridos", &c.surrenderEnabled);
    regBool(api, mod, "animals_enabled", "Ruta: Animales", "Aplica al reclutamiento animal", &c.animalsEnabled);
    
    // Aviso de cautivos
    regBool(api, mod, "hail_when_ready", "Aviso: activo", "Los cautivos llaman cuando estan listos", &c.hailWhenReady);
    regFloat(api, mod, "hail_first_hours", "Aviso: primeras horas", "Horas de juego antes del primer llamado", 8.0f, 72.0f, 1.0f, 0, &c.hailFirstHours);
    regFloat(api, mod, "hail_repeat_hours", "Aviso: cada cuantas horas", "Horas entre repeticiones", 1.0f, 24.0f, 1.0f, 0, &c.hailRepeatHours);
    regInt(api, mod, "hail_repeats", "Aviso: repeticiones", "Cuantas veces llama en total", 1, 10, 1, &c.hailRepeats);

    // Exclusiones
    regBool(api, mod, "exclude_dead", "Excluir muertos", "Nunca reclutar cadaveres", &c.exclude.dead);
    regBool(api, mod, "exclude_unconscious", "Excluir inconscientes", "Nunca reclutar a quien esta KO", &c.exclude.unconscious);
    regBool(api, mod, "exclude_animals", "Excluir animales", "Deja los animales fuera por completo", &c.exclude.animals);
    regBool(api, mod, "exclude_same_faction", "Excluir tu faccion", "Ignora a los que ya son tuyos", &c.exclude.sameFaction);

    regInt(api, mod, "max_spawned_characters", "Spawn: maximo", "Tope del panel de spawn", 1, 500, 10, &c.maxSpawnedCharacters);

    // Prices
    regInt(api, mod, "preset", "Dificultad (Preset)", "0: Cheat, 1: Balanced, 2: Vanilla+", 0, 2, 1, &c.preset);
    regInt(api, mod, "cats_per_level", "Cats por nivel", "Precio base multiplicador de nivel", 0, 500, 10, &c.prices.catsPerLevel);
    regInt(api, mod, "cats_minimum", "Cats (Mínimo)", "Precio mínimo para cualquier recluta", 0, 10000, 100, &c.prices.catsMinimum);
    regFloat(api, mod, "hunger_threshold", "Hambre extra", "Suma requerimiento extra de hambre (0 por defecto)", 0.0f, 10.0f, 0.5f, 1, &c.prices.hungerThreshold);
    regFloat(api, mod, "wounded_threshold", "Salud (Máximo para reclutar)", "Si la salud es <= a esto, escuchan (0..1)", 0.0f, 1.0f, 0.1f, 2, &c.prices.woundedThreshold);
    regFloat(api, mod, "easy_hours_caged", "Horas jaula (modo facil)", "Horas en jaula que bastan por si solas. 0 lo desactiva", 0.0f, 72.0f, 1.0f, 0, &c.prices.easyHoursCaged);
    regFloat(api, mod, "hours_caged_for_proud", "Horas jaula (Orgullosos)", "Horas a esperar para NPCs orgullosos", 0.0f, 72.0f, 1.0f, 0, &c.prices.hoursCagedForProud);
    regFloat(api, mod, "unique_extra_hours", "Horas jaula (Únicos)", "Horas extra requeridas para únicos", 0.0f, 168.0f, 6.0f, 0, &c.prices.uniqueExtraHours);
    
    // Multipliers
    regFloat(api, mod, "fearful_multiplier", "Precio miedosos", "Multplicador para personalidades miedosas", 0.1f, 2.0f, 0.1f, 1, &c.prices.fearfulMultiplier);
    regFloat(api, mod, "brave_multiplier", "Precio valientes", "Multplicador para personalidades valientes", 0.1f, 4.0f, 0.1f, 1, &c.prices.braveMultiplier);
    regFloat(api, mod, "talent_weight", "Peso del talento", "Cuanto sube el precio un NPC con un oficio o habilidad rara", 0.0f, 4.0f, 0.25f, 2, &c.prices.talentWeight);
    regFloat(api, mod, "faction_weight", "Peso de Facción", "Impacto de rep en el precio (0..2)", 0.0f, 2.0f, 0.1f, 1, &c.prices.factionWeight);

    return true;
}

void InitModHub()
{
    g_hubState = 0;
    g_hubHardFail = false;
    if (DoRegisterModHub()) {
        g_hubState = g_hubHardFail ? -1 : 1;
    }
}

void ModHubTick()
{
    if (g_hubState == 0) {
        if (DoRegisterModHub()) g_hubState = g_hubHardFail ? -1 : 1;
    }
}

} // namespace
