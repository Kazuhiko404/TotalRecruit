#include "saved_strings.h"
#include "temp_member.h"

namespace totalrecruit
{
namespace
{
const SavedStringField kFields[] = {
    { "total_recruit.temp_home_faction", &SavedTempHomeFaction, &RestoreSavedTempHomeFaction },
};
} // namespace

int SavedStringFieldCount()
{
    return (int)(sizeof(kFields) / sizeof(kFields[0]));
}

const SavedStringField& SavedStringFieldAt(int index)
{
    return kFields[index];
}
} // namespace totalrecruit
