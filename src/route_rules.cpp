#include "route_rules.h"
#include "chance_rules.h"
#include "body_rules.h"

namespace totalrecruit
{
RouteFacts MakeRouteFacts(const CoreFacts& core, const Config& config)
{
    RouteFacts f;
    f.animal             = core.animal;
    f.surrenderEnabled   = config.surrenderEnabled;
    f.countDefeats       = config.submission.countDefeats;
    f.defeats            = core.defeats;
    f.defeatsToBreak     = config.submission.defeatsToBreak;
    f.unconscious        = core.unconscious;
    f.dying              = core.dying;
    f.grudge             = core.grudge;
    f.grudgeUnforgivable = GrudgeIsUnforgivable(core.temperament, config.chance);
    f.inMyCage           = core.inMyCage;
    f.rescueEnabled      = config.rescueEnabled;
    f.remembersSaviour   = config.rescueEnabled && core.rememberedAsSaviour;
    f.easyHealthy        = IsEasyHealthy(config.prices.easyHoursCaged, core.wounded,
                                         core.starving, core.health);
    f.surrendered        = core.surrendered;
    return f;
}

Route ChooseEntryRoute(const RouteFacts& f)
{
    // Loyalty first. To an honourable, brave or kind NPC refusing the person
    // who killed a comrade is the thing that defines them, so no beating
    // outside the cage may buy it off -- this used to sit below field-yield,
    // which handed such an NPC to the player after two defeats.
    if (f.grudge && f.grudgeUnforgivable && !f.inMyCage)
    {
        return Route_GrudgeRefusal;
    }

    // Field-yield is the route for a beating in the open, so it applies outside
    // the cage only. Inside, the defeats still count -- they feed the chance --
    // but a caged NPC keeps every cage route (easy wait, healthy, care, cats);
    // taking this one first returned before any of them could run.
    if (!f.inMyCage && !f.animal && f.surrenderEnabled && f.countDefeats &&
        f.defeats >= f.defeatsToBreak)
    {
        return Route_FieldYield;
    }

    if (f.unconscious)
    {
        return Route_Unconscious;
    }

    if (f.rescueEnabled && f.remembersSaviour)
    {
        return Route_Rescue;
    }

    if (f.rescueEnabled && f.dying)
    {
        return Route_Dying;
    }

    if (f.rescueEnabled && f.easyHealthy && f.inMyCage)
    {
        return Route_EasyHealthy;
    }

    if (f.surrenderEnabled && !f.animal && f.surrendered)
    {
        return Route_Surrendered;
    }

    return f.inMyCage ? Route_Cage : Route_FreeWorld;
}

const char* RouteName(Route route)
{
    switch (route)
    {
    case Route_FieldYield:     return "field-yield";
    case Route_Unconscious:    return "unconscious";
    case Route_GrudgeRefusal:  return "grudge-refusal";
    case Route_Rescue:         return "rescue";
    case Route_Dying:          return "dying";
    case Route_EasyHealthy:    return "easy-healthy";
    case Route_Surrendered:    return "surrendered";
    case Route_FreeWorld:      return "free-world";
    case Route_Cage:           return "cage";
    default:                   return "?";
    }
}
} // namespace totalrecruit
