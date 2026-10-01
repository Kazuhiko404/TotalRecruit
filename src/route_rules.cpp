#include "route_rules.h"

namespace totalrecruit
{
Route ChooseEntryRoute(const RouteFacts& f)
{
    if (!f.animal && f.surrenderEnabled && f.countDefeats &&
        f.defeats >= f.defeatsToBreak)
    {
        return Route_FieldYield;
    }

    if (f.unconscious)
    {
        return Route_Unconscious;
    }

    if (f.grudge && f.grudgeUnforgivable && !f.inMyCage)
    {
        return Route_GrudgeRefusal;
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
