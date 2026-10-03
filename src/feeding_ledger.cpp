#include "feeding_ledger.h"

#include <map>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace totalrecruit
{
const int kMaxSavedMeals = 1000;

namespace
{
typedef std::map<CharKey, int> Meals;
Meals g_meals;

struct MealsMutex
{
    CRITICAL_SECTION value;
    MealsMutex() { InitializeCriticalSection(&value); }
    ~MealsMutex() { DeleteCriticalSection(&value); }
} g_mutex;

struct Guard
{
    Guard() { EnterCriticalSection(&g_mutex.value); }
    ~Guard() { LeaveCriticalSection(&g_mutex.value); }
private:
    Guard(const Guard&);
    Guard& operator=(const Guard&);
};

int Sanitise(int raw)
{
    if (raw < 0) return 0;
    return raw > kMaxSavedMeals ? kMaxSavedMeals : raw;
}
} // namespace

int RecordPlayerMeal(const CharKey& key)
{
    Guard lock;
    int& meals = g_meals[key];
    if (meals < kMaxSavedMeals)
    {
        ++meals;
    }
    return meals;
}

int PlayerMeals(const CharKey& key)
{
    Guard lock;
    Meals::const_iterator it = g_meals.find(key);
    return it == g_meals.end() ? 0 : it->second;
}

int SavedPlayerMeals(const std::string& persistedId)
{
    if (persistedId.empty())
    {
        return 0;
    }
    return PlayerMeals(CharKey::FromId(persistedId));
}

void RestoreSavedPlayerMeals(const std::string& persistedId, int meals)
{
    if (persistedId.empty())
    {
        return;
    }
    const CharKey key = CharKey::FromId(persistedId);
    const int safe = Sanitise(meals);
    Guard lock;
    if (safe <= 0)
    {
        g_meals.erase(key);
        return;
    }
    g_meals[key] = safe;
}

void ForgetAllPlayerMeals()
{
    Guard lock;
    g_meals.clear();
}
} // namespace totalrecruit
