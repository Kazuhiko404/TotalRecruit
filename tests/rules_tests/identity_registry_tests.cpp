// Tests for src/identity_registry.h: which live object carries which persisted
// id, for how long, and what key a character gets when the id is unusable.

#include "test_support.h"
#include "identity_registry.h"

using namespace totalrecruit;
using namespace totalrecruit::identity;

namespace
{
int a, b, c;   // three distinct "characters" (only their addresses matter)

void LoadBindsTheSavedId()
{
    const char* name = "identity: a character loaded with an id keeps it";
    Registry r;
    bool conflict = true;
    r.Load(&a, true, "id-a", conflict);
    CHECK(!conflict);
    const Lifetime life = r.Current(&a);
    CHECK(life != 0);
    std::string id;
    CHECK(r.Find(&a, life, id));
    CHECK(id == "id-a");
    CHECK(r.Count() == 1);
}

void LoadWithoutSaveStateIsNotALifetime()
{
    const char* name = "identity: loadFromSerialise(NULL) creates nothing";
    Registry r;
    bool conflict = false;
    r.Load(&a, false, "id-a", conflict);
    CHECK(r.Current(&a) == 0);
    CHECK(r.Count() == 0);
}

void LegacySaveHasALifetimeButNoId()
{
    const char* name = "identity: a save from before the mod binds no id (a new one is minted later)";
    Registry r;
    bool conflict = false;
    r.Load(&a, true, "", conflict);
    const Lifetime life = r.Current(&a);
    CHECK(life != 0);
    std::string id;
    CHECK(!r.Find(&a, life, id));
    CHECK(!r.SavedId(&a, life, id));
    // The id minted this session can then be bound to that lifetime.
    CHECK(r.Bind(&a, life, "fresh", conflict));
    CHECK(r.Find(&a, life, id) && id == "fresh");
}

void BindRefusesStaleOrDifferent()
{
    const char* name = "identity: a bind needs the live lifetime and never changes an id";
    Registry r;
    bool conflict = false;
    const Lifetime life = r.Begin(&a);
    CHECK(life != 0);
    CHECK(!r.Bind(&a, life + 5, "x", conflict));       // wrong lifetime
    CHECK(!r.Bind(&a, life, "", conflict));            // empty id
    CHECK(r.Bind(&a, life, "x", conflict));
    CHECK(r.Bind(&a, life, "x", conflict));            // same id again is fine
    CHECK(!r.Bind(&a, life, "y", conflict));           // never rebinds
    std::string id;
    CHECK(r.Find(&a, life, id) && id == "x");
    CHECK(!r.Bind(&b, 1, "z", conflict));              // unknown object
}

void DestroyedCharacterForgetsItsId()
{
    const char* name = "identity: a freed address does not hand its id to the next occupant";
    Registry r;
    bool conflict = false;
    r.Load(&a, true, "id-a", conflict);
    const Lifetime old = r.Current(&a);
    r.End(&a, old);
    CHECK(r.Current(&a) == 0);
    std::string id;
    CHECK(!r.Find(&a, old, id));
    const Lifetime fresh = r.Begin(&a);              // a stranger at the same address
    CHECK(fresh != old);
    CHECK(!r.Find(&a, fresh, id));
    CHECK(!r.Find(&a, old, id));
}

void EndWithAStaleLifetimeDoesNothing()
{
    const char* name = "identity: End with a stale lifetime keeps the live entry";
    Registry r;
    bool conflict = false;
    r.Load(&a, true, "id-a", conflict);
    r.End(&a, r.Current(&a) + 9);
    CHECK(r.Current(&a) != 0);
}

void DuplicateIdIsAConflictForBoth()
{
    const char* name = "identity: two live characters with one id are both refused";
    Registry r;
    bool conflict = false;
    r.Load(&a, true, "dup", conflict);
    CHECK(!conflict);
    r.Load(&b, true, "dup", conflict);
    CHECK(conflict);
    std::string id;
    CHECK(!r.Find(&a, r.Current(&a), id));
    CHECK(!r.Find(&b, r.Current(&b), id));
    // The token still belongs in the save, though it must not key anything.
    CHECK(r.SavedId(&a, r.Current(&a), id) && id == "dup");
    // Sticky: one of them leaving does not make the other trustworthy.
    r.End(&b, r.Current(&b));
    CHECK(!r.Find(&a, r.Current(&a), id));
    // An unrelated id is unaffected.
    r.Load(&c, true, "other", conflict);
    CHECK(!conflict);
    CHECK(r.Find(&c, r.Current(&c), id) && id == "other");
}

void ClearWipesEntriesAndConflicts()
{
    const char* name = "identity: clearing the world forgets every binding and conflict";
    Registry r;
    bool conflict = false;
    r.Load(&a, true, "dup", conflict);
    r.Load(&b, true, "dup", conflict);
    r.Clear();
    CHECK(r.Count() == 0);
    CHECK(r.Current(&a) == 0);
    r.Load(&a, true, "dup", conflict);
    CHECK(!conflict);
    std::string id;
    CHECK(r.Find(&a, r.Current(&a), id));
}

void NullObjectGetsNothing()
{
    const char* name = "identity: a null character is never tracked";
    Registry r;
    CHECK(r.Begin(0) == 0);
    CHECK(r.Count() == 0);
}

void ChooseKeyUsesTheIdWhenItCan()
{
    const char* name = "identity: the key is the id when ready and unconflicted";
    bool fallback = true;
    const CharKey key = ChooseKey(true, true, "id-a", CharKey(5, 6), &fallback);
    CHECK(key.HasId());
    CHECK(key == CharKey::FromId("id-a"));
    CHECK(!fallback);
}

void ChooseKeyFallsBackToTheHandle()
{
    const char* name = "identity: any reason the id is unusable falls back to the handle";
    const CharKey handle(5, 6);
    bool fallback = false;
    CHECK(ChooseKey(false, true, "id-a", handle, &fallback) == handle);   // hooks failed
    CHECK(fallback);
    fallback = false;
    CHECK(ChooseKey(true, false, "", handle, &fallback) == handle);       // conflicted / no id
    CHECK(fallback);
    fallback = false;
    CHECK(ChooseKey(true, true, "", handle, &fallback) == handle);        // empty id is not an id
    CHECK(fallback);
    CHECK(ChooseKey(false, false, "", handle, NULL) == handle);           // no out-flag is fine
}
} // namespace

void RunIdentityRegistryTests()
{
    LoadBindsTheSavedId();
    LoadWithoutSaveStateIsNotALifetime();
    LegacySaveHasALifetimeButNoId();
    BindRefusesStaleOrDifferent();
    DestroyedCharacterForgetsItsId();
    EndWithAStaleLifetimeDoesNothing();
    DuplicateIdIsAConflictForBoth();
    ClearWipesEntriesAndConflicts();
    NullObjectGetsNothing();
    ChooseKeyUsesTheIdWhenItCan();
    ChooseKeyFallsBackToTheHandle();
}
