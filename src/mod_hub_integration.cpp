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

static void regBool(const EMC_HubApiV1* api, EMC_ModHandle mod, const char* id, const char* label, const char* desc, bool* field)
{
    EMC_BoolSettingDefV1 d = { id, label, desc, field, &cbGetBool, &cbSetBool };
    api->register_bool_setting(mod, &d);
}

static void regInt(const EMC_HubApiV1* api, EMC_ModHandle mod, const char* id, const char* label, const char* desc, int32_t lo, int32_t hi, int32_t step, int32_t* field)
{
    EMC_IntSettingDefV1 d = { id, label, desc, field, lo, hi, step, &cbGetInt, &cbSetInt };
    api->register_int_setting(mod, &d);
}

static void regFloat(const EMC_HubApiV1* api, EMC_ModHandle mod, const char* id, const char* label, const char* desc, float lo, float hi, float step, int dec, float* field)
{
    EMC_FloatSettingDefV1 d = { id, label, desc, field, lo, hi, step, (uint32_t)dec, &cbGetFloat, &cbSetFloat };
    api->register_float_setting(mod, &d);
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
        g_hubHardFail = true;
        return true;
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

    regBool(api, mod, "showRecruitButton", "Mostrar botón UI", "Muestra un botón de reclutar en el panel", &c.showRecruitButton);
    regBool(api, mod, "probeOnClick", "Debug Log (Rasgos)", "Muestra rasgos en el log al hacer clic", &c.probeOnClick);
    
    // Rutas
    regBool(api, mod, "rescueEnabled", "Ruta: Rescate", "Permite reclutar curando a gente", &c.rescueEnabled);
    regBool(api, mod, "captivityEnabled", "Ruta: Jaulas", "Permite reclutar de jaulas", &c.captivityEnabled);
    regBool(api, mod, "surrenderEnabled", "Ruta: Rendición", "Los enemigos se rinden al estar heridos", &c.surrenderEnabled);
    regBool(api, mod, "animalsEnabled", "Ruta: Animales", "Aplica al reclutamiento animal", &c.animalsEnabled);
    
    // Prices
    regInt(api, mod, "preset", "Dificultad (Preset)", "0: Cheat, 1: Balanced, 2: Vanilla+", 0, 2, 1, &c.preset);
    regInt(api, mod, "catsPerLevel", "Cats por nivel", "Precio base multiplicador de nivel", 0, 500, 10, &c.prices.catsPerLevel);
    regInt(api, mod, "catsMinimum", "Cats (Mínimo)", "Precio mínimo para cualquier recluta", 0, 10000, 100, &c.prices.catsMinimum);
    regFloat(api, mod, "hungerThreshold", "Hambre extra", "Suma requerimiento extra de hambre (0 por defecto)", 0.0f, 10.0f, 0.5f, 1, &c.prices.hungerThreshold);
    regFloat(api, mod, "woundedThreshold", "Salud (Máximo para reclutar)", "Si la salud es <= a esto, escuchan (0..1)", 0.0f, 1.0f, 0.1f, 2, &c.prices.woundedThreshold);
    regFloat(api, mod, "hoursCagedForProud", "Horas jaula (Orgullosos)", "Horas a esperar para NPCs orgullosos", 0.0f, 72.0f, 1.0f, 0, &c.prices.hoursCagedForProud);
    regFloat(api, mod, "uniqueExtraHours", "Horas jaula (Únicos)", "Horas extra requeridas para únicos", 0.0f, 168.0f, 6.0f, 0, &c.prices.uniqueExtraHours);
    
    // Multipliers
    regFloat(api, mod, "fearfulMultiplier", "Precio miedosos", "Multplicador para personalidades miedosas", 0.1f, 2.0f, 0.1f, 1, &c.prices.fearfulMultiplier);
    regFloat(api, mod, "braveMultiplier", "Precio valientes", "Multplicador para personalidades valientes", 0.1f, 4.0f, 0.1f, 1, &c.prices.braveMultiplier);
    regFloat(api, mod, "factionWeight", "Peso de Facción", "Impacto de rep en el precio (0..2)", 0.0f, 2.0f, 0.1f, 1, &c.prices.factionWeight);

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
