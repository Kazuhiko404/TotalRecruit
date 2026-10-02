#pragma once

// PURE character-identity registry: which live object carries which persisted
// id, for how long, and whether two live objects claim the same id. No engine
// include -- objects are opaque `const void*` keys -- so tests/rules_tests
// compiles it.
//
// Ported from TotalSparring's FighterIdentityPolicy::Registry (itself a port of
// Proving Grounds'): Begin / Current / Load / Bind / Find / SavedId / End /
// Clear keep that exact behaviour. The "created this session" bookkeeping and
// the reverse lookup were dropped: TotalRecruit has no legacy history to merge
// and never resolves an id back to a character.
//
// Lifetimes exist because the engine reuses addresses: an id is bound to
// (object, lifetime), so a stranger constructed at a freed address starts with
// no id instead of inheriting the previous occupant's.

#include "char_key.h"

#include <map>
#include <set>
#include <string>

#ifndef NULL
#define NULL 0
#endif

namespace totalrecruit
{
namespace identity
{
typedef unsigned long long Lifetime;

class Registry
{
public:
    Registry() : sequence_(0) {}

    // A new lifetime for `object` (replacing any previous one). 0 for a null
    // object or an exhausted counter.
    Lifetime Begin(const void* object)
    {
        if (!object || sequence_ == ~Lifetime(0)) return 0;
        Entry entry;
        entry.lifetime = ++sequence_;
        entries_[object] = entry;
        return entry.lifetime;
    }

    // 0 when the object is not tracked.
    Lifetime Current(const void* object) const
    {
        Entries::const_iterator it = entries_.find(object);
        return it == entries_.end() ? 0 : it->second.lifetime;
    }

    // The engine loaded `object` from a save. `hasSaveState` is false for the
    // native loadFromSerialise(NULL), which is a no-op and not a new lifetime.
    // A real save that lacks our field (written before this mod, or by a
    // character we never touched) still creates a lifetime, just with no id.
    void Load(const void* object, bool hasSaveState, const std::string& id, bool& conflict)
    {
        conflict = false;
        if (!hasSaveState) return;
        const Lifetime lifetime = Begin(object);
        if (!id.empty()) Bind(object, lifetime, id, conflict);
    }

    // Attaches `id` to the live lifetime. False when the lifetime is stale, the
    // id empty, a different id is already bound, or the id is conflicted.
    // `conflict` is set when THIS bind made the id a duplicate.
    bool Bind(const void* object, Lifetime lifetime, const std::string& id, bool& conflict)
    {
        conflict = false;
        Entries::iterator it = entries_.find(object);
        if (it == entries_.end() || it->second.lifetime != lifetime || id.empty()) return false;
        if (!it->second.id.empty() && it->second.id != id) return false;
        it->second.id = id;
        for (Entries::const_iterator other = entries_.begin(); other != entries_.end(); ++other)
        {
            if (other->first != object && other->second.id == id)
            {
                conflict = conflicts_.insert(id).second || conflict;
            }
        }
        return conflicts_.find(id) == conflicts_.end();
    }

    // The id a map may key on: bound, live, and not conflicted.
    bool Find(const void* object, Lifetime lifetime, std::string& id) const
    {
        if (!SavedId(object, lifetime, id)) return false;
        if (conflicts_.find(id) == conflicts_.end()) return true;
        id.clear();
        return false;
    }

    // Serialisation only: a conflicted token still belongs in the save, but must
    // never key anything.
    bool SavedId(const void* object, Lifetime lifetime, std::string& id) const
    {
        id.clear();
        Entries::const_iterator it = entries_.find(object);
        if (it == entries_.end() || it->second.lifetime != lifetime || it->second.id.empty()) return false;
        id = it->second.id;
        return true;
    }

    // The character was destroyed. A stale lifetime is ignored.
    void End(const void* object, Lifetime lifetime)
    {
        Entries::iterator it = entries_.find(object);
        if (it != entries_.end() && it->second.lifetime == lifetime) entries_.erase(it);
    }

    // The whole world was abandoned (load begin).
    void Clear()
    {
        entries_.clear();
        conflicts_.clear();
    }

    int Count() const { return (int)entries_.size(); }

private:
    struct Entry
    {
        Lifetime lifetime;
        std::string id;
        Entry() : lifetime(0) {}
    };
    typedef std::map<const void*, Entry> Entries;

    Entries entries_;
    // Sticky within a world: a duplicate disappearing is not a decision about
    // which character owns the id.
    std::set<std::string> conflicts_;
    Lifetime sequence_;
};

// The key a character gets. The persisted id when the hooks are installed and
// an unconflicted id exists; otherwise the handle key, so state keeps being
// kept (not fail-closed). `usedFallback` reports which one it was.
inline CharKey ChooseKey(bool identityReady, bool foundId, const std::string& id,
                         const CharKey& handleKey, bool* usedFallback)
{
    const bool useId = identityReady && foundId && !id.empty();
    if (usedFallback != NULL) *usedFallback = !useId;
    return useId ? CharKey::FromId(id) : handleKey;
}
} // namespace identity
} // namespace totalrecruit
