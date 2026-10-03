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

    // R6.3 is blocked on this: the hub hands back 160 bytes where our header
    // declares 72, so roughly a dozen function pointers sit past the end of
    // what we know how to name -- register_setting_section among them, going by
    // the strings in the DLL.
    //
    // Calling one at a guessed offset is the trap that takes the process down
    // (see notes S11), so nothing is called. The slots are dumped instead, and
    // whether an address looks like code is the evidence that settles which
    // ones are real function pointers.
    if (apiSize > EMC_HUB_API_V1_MIN_SIZE)
    {
        const void* const* slots = (const void* const*)api;
        const unsigned slotCount = (unsigned)(apiSize / sizeof(void*));
        std::ostringstream dump;
        dump << "Mod Hub slots (" << slotCount << " of " << apiSize << " bytes):";
        for (unsigned i = 0; i < slotCount && i < 32; ++i)
        {
            dump << " [" << i << "]=" << (slots[i] ? "fn" : "null");
        }
        LogInfo(dump.str());
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

    // Lectura de estres y chance sobre el NPC
    regBool(api, mod, "readout_enabled", "Lectura: activa", "Muestra estres, chance y que falta sobre el NPC con el que podrias hablar", &c.readoutEnabled);
    regBool(api, mod, "readout_compact", "Lectura: compacta", "Version mas chica: sin el desglose del estres ni las derrotas", &c.readoutCompact);
    regBool(api, mod, "readout_on_hover", "Lectura: al pasar el mouse", "Apagado, solo aparece sobre el NPC seleccionado o el del menu abierto", &c.readoutOnHover);

    // Exclusiones
    regBool(api, mod, "exclude_dead", "Excluir muertos", "Nunca reclutar cadaveres", &c.exclude.dead);
    regBool(api, mod, "exclude_unconscious", "Excluir inconscientes", "Nunca reclutar a quien esta KO", &c.exclude.unconscious);
    regBool(api, mod, "exclude_animals", "Excluir animales", "Deja los animales fuera por completo", &c.exclude.animals);
    regBool(api, mod, "exclude_same_faction", "Excluir tu faccion", "Ignora a los que ya son tuyos", &c.exclude.sameFaction);

    regInt(api, mod, "max_spawned_characters", "Spawn: maximo", "Tope del panel de spawn", 1, 500, 10, &c.maxSpawnedCharacters);

    // Prices
    regInt(api, mod, "preset", "Dificultad (Preset)", "0: Cheat, 1: Balanced, 2: Vanilla+", 0, 2, 1, &c.preset);
    regInt(api, mod, "cats_per_level", "Cats por nivel", "Peso del tramo bajo: mantiene baratos los niveles chicos", 0, 300, 5, &c.prices.catsPerLevel);
    regInt(api, mod, "cats_ceiling", "Cats: techo", "A cuanto tiende el precio de los mas fuertes", 0, 200000, 5000, &c.prices.catsCeiling);
    regFloat(api, mod, "price_steepness", "Curva: pendiente", "Que tan brusco encarece alrededor del punto medio", 0.02f, 0.4f, 0.01f, 2, &c.prices.priceCurveSteepness);
    regFloat(api, mod, "price_midpoint", "Curva: punto medio", "Nivel donde el precio sube mas rapido", 5.0f, 90.0f, 5.0f, 0, &c.prices.priceCurveMidpoint);
    regInt(api, mod, "cats_minimum", "Cats (Mínimo)", "Precio mínimo para cualquier recluta", 0, 10000, 100, &c.prices.catsMinimum);
    regFloat(api, mod, "hunger_threshold", "Umbral de hambre", "Considera hambriento por debajo de este % de estomago lleno (0 = usar solo el motor)", 0.0f, 1.0f, 0.05f, 2, &c.prices.hungerThreshold);
    regFloat(api, mod, "wounded_threshold", "Salud (Máximo para reclutar)", "Si la salud es <= a esto, escuchan (0..1)", 0.0f, 1.0f, 0.1f, 2, &c.prices.woundedThreshold);
    regFloat(api, mod, "easy_hours_caged", "Horas jaula (modo facil)", "Horas en jaula que bastan por si solas, iguales para todos. 0 lo desactiva", 0.0f, 72.0f, 1.0f, 0, &c.prices.easyHoursCaged);
    
    // Multipliers
    regFloat(api, mod, "fearful_multiplier", "Precio miedosos", "Multplicador para personalidades miedosas", 0.1f, 2.0f, 0.1f, 1, &c.prices.fearfulMultiplier);
    regFloat(api, mod, "brave_multiplier", "Precio valientes", "Multplicador para personalidades valientes", 0.1f, 4.0f, 0.1f, 1, &c.prices.braveMultiplier);
    regFloat(api, mod, "talent_weight", "Peso del talento", "Cuanto sube el precio un NPC con un oficio o habilidad rara", 0.0f, 4.0f, 0.25f, 2, &c.prices.talentWeight);
    regFloat(api, mod, "talent_max_multiplier", "Talento: tope", "Lo maximo que el talento puede multiplicar el precio", 1.0f, 5.0f, 0.25f, 2, &c.prices.talentMaxMultiplier);
    regFloat(api, mod, "premium_max_multiplier", "Prima máxima (talento x único x carácter)", "Tope conjunto de talento, unico y personalidad sobre el precio base (la faccion va aparte)", 1.0f, 5.0f, 0.1f, 2, &c.prices.premiumMaxMultiplier);
    regInt(api, mod, "cats_hard_cap", "Cats: tope final", "Precio maximo tras todos los multiplicadores (0 = sin tope)", 0, 500000, 10000, &c.prices.catsHardCap);
    regFloat(api, mod, "unique_multiplier", "Precio de unicos", "Cuanto multiplican su precio los personajes con nombre", 1.0f, 5.0f, 0.25f, 2, &c.prices.uniqueMultiplier);
    regFloat(api, mod, "faction_weight", "Peso de Facción", "Impacto de rep en el precio (0..2)", 0.0f, 2.0f, 0.1f, 1, &c.prices.factionWeight);

    // Animales: domesticacion por barra de hambre. Cada valor es una fraccion
    // del estomago lleno, asi que lo que se escribe aqui es lo que la barra
    // tiene que alcanzar.
    regFloat(api, mod, "animal_cub_maturity", "Animal: umbral de cría", "Por debajo de esta fraccion de su vida cuenta como cachorro", 0.0f, 1.0f, 0.05f, 2, &c.animals.cubMaturity);
    regInt(api, mod, "animal_weak_level", "Animal: nivel débil", "A este nivel o menos cuenta como debil, sea cual sea su edad", 0, 60, 1, &c.animals.weakLevel);
    regFloat(api, mod, "animal_cub_fullness", "Animal: comida cría", "Cuanto estomago hay que llenarle a una cria o a un debil", 0.0f, 1.0f, 0.05f, 2, &c.animals.cubFullness);
    regFloat(api, mod, "animal_base_fullness", "Animal: comida base", "Piso de comida para cualquier animal adulto", 0.0f, 1.0f, 0.05f, 2, &c.animals.baseFullness);
    regFloat(api, mod, "animal_maturity_weight", "Animal: peso de la edad", "Cuanto suma estar completamente crecido", 0.0f, 1.0f, 0.05f, 2, &c.animals.maturityWeight);
    regInt(api, mod, "animal_strong_level", "Animal: nivel fuerte", "Por encima de este nivel pide mas comida", 0, 100, 5, &c.animals.strongLevel);
    regFloat(api, mod, "animal_strong_penalty", "Animal: recargo fuerte", "Cuanta comida extra pide un animal de nivel alto", 0.0f, 1.0f, 0.05f, 2, &c.animals.strongPenalty);
    regFloat(api, mod, "animal_edge_discount", "Animal: descuento +10", "Descuento si le sacas 10 niveles o mas", 0.0f, 1.0f, 0.05f, 2, &c.animals.edgeDiscount);
    regFloat(api, mod, "friendly_relation", "Relacion amistosa", "Reputacion a partir de la cual te consideran amigo", -100.0f, 100.0f, 5.0f, 0, &c.friendlyRelation);
    regBool(api, mod, "cheat_needs_cage", "Cheat: exige jaula", "En Cheat hay que enjaularlo igual (sin ninguna otra condicion)", &c.cheatNeedsCage);
    regFloat(api, mod, "cheat_hours_caged", "Cheat: espera en jaula", "Horas de juego antes de que te llame (0.02 = un minuto)", 0.0f, 24.0f, 0.01f, 2, &c.cheatHoursCaged);
    regInt(api, mod, "rescue_base_bonus", "Rescate: peso de la deuda", "Cuanto pesa haberle salvado la vida, antes de contar la cura", 0, 90, 5, &c.rescueBaseBonus);
    regFloat(api, mod, "rescue_heal_weight", "Rescate: peso de la cura", "Cuanta chance suma haberlo curado del todo", 0.0f, 90.0f, 5.0f, 0, &c.rescueHealWeight);

    regInt(api, mod, "insist_attempts", "Insistencia: intentos", "Cuantas veces podes preguntar antes de que te ignore", 1, 20, 1, &c.insistence.attemptsBeforeIgnored);
    regFloat(api, mod, "insist_base_hours", "Insistencia: horas base", "Horas de castigo por cada intento de mas (se acumula)", 0.0f, 48.0f, 1.0f, 1, &c.insistence.baseHours);
    regFloat(api, mod, "insist_max_hours", "Insistencia: techo", "Castigo maximo por insistir", 1.0f, 240.0f, 6.0f, 0, &c.insistence.maxHours);
    regFloat(api, mod, "insist_forget_hours", "Insistencia: olvido", "Horas de silencio tras las que vuelve a empezar de cero", 0.0f, 48.0f, 1.0f, 1, &c.insistence.forgetAfterHours);

    // Motor de chance: el nivel pesa, nunca decide solo.
    regInt(api, mod, "chance_level_base", "Chance: base a nivel parejo", "De cuanto parte la negociacion entre iguales", 0, 100, 5, &c.chance.levelBase);
    regFloat(api, mod, "chance_level_gap_penalty", "Chance: castigo por nivel", "Chance que pierde por cada nivel que el NPC te saca", 0.0f, 20.0f, 0.5f, 1, &c.chance.levelGapPenalty);
    regInt(api, mod, "chance_level_gap_floor", "Chance: piso por nivel", "Chance minima cuando te saca niveles (0 = imposible)", 0, 50, 1, &c.chance.levelGapFloor);
    regFloat(api, mod, "chance_level_edge_bonus", "Chance: bono por ventaja", "Chance que ganas por cada nivel que le sacas", 0.0f, 10.0f, 0.5f, 1, &c.chance.levelEdgeBonus);
    regInt(api, mod, "chance_level_edge_cap", "Chance: techo del bono", "Cuanto puede ayudarte como maximo sacarle niveles", 0, 100, 5, &c.chance.levelEdgeCap);
    regInt(api, mod, "chance_floor", "Chance: piso global", "Nunca menos que esto (0 = permitir imposibles)", 0, 50, 1, &c.chance.floor);
    regInt(api, mod, "chance_unique_penalty", "Chance: castigo a unicos", "Cuanto mas cuesta reclutar a un personaje con nombre", 0, 90, 5, &c.chance.uniquePenalty);
    regInt(api, mod, "chance_grudge_penalty", "Chance: rencor", "Cuanta chance pierde si mataste a alguien suyo", 0, 95, 5, &c.chance.grudgePenalty);
    regBool(api, mod, "chance_grudge_absolute", "Rencor: honorables no perdonan", "Honorables, valientes y bondadosos rechazan sin importar nada mas", &c.chance.grudgeIsAbsolute);
    regInt(api, mod, "chance_intimidation", "Chance: peso de intimidar", "Cuanto pesa tenerlo encerrado en tu jaula", 0, 60, 5, &c.chance.intimidationBonus);
    regInt(api, mod, "chance_ceiling", "Chance: techo global", "Nunca mas que esto (100 = permitir garantias)", 50, 100, 1, &c.chance.ceiling);

    // Sumision: cuanto aguanta antes de ceder.
    regBool(api, mod, "sub_count_defeats", "Sumision: contar derrotas", "Cuenta cuantas veces lo dejaste inconsciente", &c.submission.countDefeats);
    regFloat(api, mod, "sub_debounce_hours", "Sumision: horas entre derrotas", "Horas de juego antes de que otra caida cuente de nuevo", 0.0f, 12.0f, 0.25f, 2, &c.submission.defeatDebounceHours);
    regInt(api, mod, "sub_persist_floor", "Sumision: piso tras recarga", "Cuantas derrotas recuerda la partida (pierde precision, no progreso). Nunca baja de las necesarias para quebrar", 0, 10, 1, &c.submission.persistedFloorCount);
    regInt(api, mod, "sub_chance_per_defeat", "Sumision: chance por derrota", "Cuanta chance suma cada derrota", 0, 50, 1, &c.submission.chancePerDefeat);
    regInt(api, mod, "sub_max_defeat_chance", "Sumision: techo por derrotas", "Cuanto pueden sumar las derrotas como maximo", 0, 95, 5, &c.submission.maxDefeatChance);
    regBool(api, mod, "sub_beg_near_death", "Sumision: rogar al morir", "Ruega por su vida cuando le queda poca sangre", &c.submission.begWhenNearDeath);
    regFloat(api, mod, "sub_near_death_blood", "Sumision: umbral de sangre", "Fraccion de sangre restante que cuenta como estar por morir", 0.05f, 0.9f, 0.05f, 2, &c.submission.nearDeathBlood);
    regFloat(api, mod, "sub_fearful_resolve", "Sumision: aguante miedoso", "Multiplica lo que aguanta un cobarde (menos = cede antes)", 0.1f, 3.0f, 0.1f, 1, &c.submission.fearfulResolve);
    regFloat(api, mod, "sub_brave_resolve", "Sumision: aguante valiente", "Multiplica lo que aguanta un valiente", 0.1f, 4.0f, 0.1f, 1, &c.submission.braveResolve);
    regFloat(api, mod, "sub_honourable_resolve", "Sumision: aguante honorable", "Multiplica lo que aguanta un honorable", 0.1f, 4.0f, 0.1f, 1, &c.submission.honourableResolve);

    regInt(api, mod, "sub_defeats_to_break", "Sumision: derrotas para quebrar", "Cuantas veces hay que vencerlo (sin jaula) antes de que ceda", 1, 10, 1, &c.submission.defeatsToBreak);
    regInt(api, mod, "sub_field_yield_bonus", "Sumision: peso de rendirse", "Cuanto pesa haberse rendido en el campo", 0, 60, 5, &c.submission.fieldYieldBonus);
    regFloat(api, mod, "sub_stress_hunger", "Estres: peso del hambre", "Cuanto pesa el hambre en quebrarlo", 0.0f, 1.0f, 0.05f, 2, &c.submission.stressFromHunger);
    regFloat(api, mod, "sub_stress_wounds", "Estres: peso de las heridas", "Cuanto pesan las heridas sin curar", 0.0f, 1.0f, 0.05f, 2, &c.submission.stressFromWounds);
    regFloat(api, mod, "sub_stress_tired", "Estres: peso del agotamiento", "Cuanto pesa no dejarlo descansar", 0.0f, 1.0f, 0.05f, 2, &c.submission.stressFromExhaustion);
    regInt(api, mod, "sub_max_stress", "Estres: chance maxima", "Cuanta chance puede sumar la barra entera", 0, 90, 5, &c.submission.maxStressChance);
    regBool(api, mod, "sub_allow_beating", "Sumision: permitir golpear", "Deja castigar al prisionero en tu jaula", &c.submission.allowBeating);
    regFloat(api, mod, "sub_beating_cooldown", "Sumision: horas entre golpizas", "Horas de juego antes de poder volver a golpearlo", 0.5f, 48.0f, 0.5f, 1, &c.submission.beatingCooldownHours);
    regFloat(api, mod, "sub_beating_ko_seconds", "Sumision: segundos inconsciente", "Cuanto queda fuera de combate tras la golpiza", 5.0f, 600.0f, 5.0f, 0, &c.submission.beatingKnockoutSeconds);

    regFloat(api, mod, "animal_nutrition_per_meal", "Animal: comida por racion", "Cuanto llena el estomago una racion (escala 0..3 del motor)", 0.05f, 3.0f, 0.05f, 2, &c.animals.nutritionPerMeal);
    regBool(api, mod, "animal_personality_matters", "Animal: cuenta la personalidad", "Un animal desconfiado pide mas comida que uno timido", &c.animals.personalityMatters);
    regBool(api, mod, "animal_owned_need_cage", "Animal: domesticos solo por jaula", "Un animal con dueno no se convence con comida: hay que enjaularlo", &c.animals.ownedNeedCage);
    regFloat(api, mod, "animal_big_edge_discount", "Animal: descuento +20", "Descuento si le sacas 20 niveles o mas", 0.0f, 1.0f, 0.05f, 2, &c.animals.bigEdgeDiscount);
    regInt(api, mod, "animal_edge_level", "Animal: niveles para el descuento", "Cuantos niveles le tenes que sacar para el descuento chico", 1, 100, 1, &c.animals.edgeLevel);
    regInt(api, mod, "animal_big_edge_level", "Animal: niveles para el descuento grande", "Cuantos niveles le tenes que sacar para el descuento grande", 1, 100, 1, &c.animals.bigEdgeLevel);
    regFloat(api, mod, "animal_temperament_base", "Animal: base del temperamento", "Con temperamento normal la barra se multiplica por base + peso (1.0 = no cambia)", 0.0f, 2.0f, 0.05f, 2, &c.animals.temperamentBase);
    regFloat(api, mod, "animal_temperament_weight", "Animal: peso del temperamento", "Cuanto mueve la barra el aguante de su temperamento (0 = no cuenta)", 0.0f, 2.0f, 0.05f, 2, &c.animals.temperamentWeight);

    // Chance engine: the terms that used to be literals in the rules.
    regFloat(api, mod, "chance_relation_weight", "Chance: peso de la reputacion", "Que fraccion de la reputacion (-100..100) se suma a la chance", 0.0f, 1.0f, 0.05f, 2, &c.chance.relationWeight);
    regInt(api, mod, "chance_allied_bonus", "Chance: bono de aliado", "Lo que suma ser amigo (reputacion sobre el umbral amistoso)", 0, 60, 5, &c.chance.alliedBonus);
    regInt(api, mod, "chance_pers_fearful", "Personalidad: miedoso", "Puntos de chance que suma o resta ser miedoso", -50, 50, 1, &c.chance.personality[Temperament_Fearful]);
    regInt(api, mod, "chance_pers_traitorous", "Personalidad: traidor", "Puntos de chance que suma o resta ser traidor", -50, 50, 1, &c.chance.personality[Temperament_Traitorous]);
    regInt(api, mod, "chance_pers_dumb", "Personalidad: simple", "Puntos de chance que suma o resta ser simple", -50, 50, 1, &c.chance.personality[Temperament_Dumb]);
    regInt(api, mod, "chance_pers_smart", "Personalidad: astuto", "Puntos de chance que suma o resta ser astuto", -50, 50, 1, &c.chance.personality[Temperament_Smart]);
    regInt(api, mod, "chance_pers_brave", "Personalidad: valiente", "Puntos de chance que suma o resta ser valiente", -50, 50, 1, &c.chance.personality[Temperament_Brave]);
    regInt(api, mod, "chance_pers_honorable", "Personalidad: honorable", "Puntos de chance que suma o resta ser honorable", -50, 50, 1, &c.chance.personality[Temperament_Honorable]);
    regInt(api, mod, "chance_pers_maniacal", "Personalidad: maniaco", "Puntos de chance que suma o resta ser maniaco", -50, 50, 1, &c.chance.personality[Temperament_Maniacal]);
    regInt(api, mod, "chance_pers_coldcruel", "Personalidad: frio y cruel", "Puntos de chance que suma o resta ser frio y cruel", -50, 50, 1, &c.chance.personality[Temperament_ColdCruel]);
    regInt(api, mod, "chance_pers_warmkind", "Personalidad: calido y bondadoso", "Puntos de chance que suma o resta ser calido y bondadoso", -50, 50, 1, &c.chance.personality[Temperament_WarmKind]);

    // Reputation gates and the talent families of the price.
    regFloat(api, mod, "relation_refuse_good_natured", "Reputacion: honorables se niegan", "Por debajo de esta reputacion un honorable o bondadoso no trata contigo", -100.0f, 100.0f, 5.0f, 0, &c.relationRefuseGoodNatured);
    regFloat(api, mod, "relation_refuse_all", "Reputacion: nadie trata", "Por debajo de esta reputacion ni siquiera el dinero los convence", -100.0f, 100.0f, 5.0f, 0, &c.relationRefuseAll);
    regFloat(api, mod, "talent_craft_weight", "Talento: peso de oficio", "Cuanto pesa el talento en oficios (ingenieria, herreria, medicina...)", 0.0f, 5.0f, 0.1f, 1, &c.prices.craftFamilyWeight);
    regFloat(api, mod, "talent_shadow_weight", "Talento: peso de sombra", "Cuanto pesa el talento en sigilo, robo y asesinato", 0.0f, 5.0f, 0.1f, 1, &c.prices.shadowFamilyWeight);

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
