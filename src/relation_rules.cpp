#include "relation_rules.h"

namespace totalrecruit
{
const char* RelationBand(float relation)
{
    if (relation >= 75.0f)  return "aliado";
    if (relation >= 25.0f)  return "amistoso";
    if (relation >= 5.0f)   return "cordial";
    if (relation >= -5.0f)  return "neutral";
    if (relation >= -15.0f) return "receloso";
    if (relation >= -30.0f) return "hostil";
    return "enemigo";
}
} // namespace totalrecruit
