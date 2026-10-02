#pragma once

#include <cstdio>
#include <string>

namespace totalrecruit
{
// Identity for a character that survives death and reload.
//
// A raw Character* is an address, not an identity. The engine frees a dead
// character and hands the same address to the next one, so a map keyed by
// pointer silently applies the dead one's state to a stranger.
//
// The engine handle (index + serial) is better -- the serial tells a reused
// slot from its previous occupant -- but it only holds inside one process: a
// game relaunch hands everybody new handles. So the preferred key is a UUID the
// mod stores in the character's own save data (character_identity.cpp); the
// handle is the fallback for when that id cannot be had (hooks missing, two
// live characters claiming the same id).
//
// A key is one or the other, never both: an id key has index and serial zeroed,
// so two keys for the same character always compare equal. Every per-character
// map in the mod keys on this.
//
// Engine-free on purpose: the pure rule modules and their tests use it.
struct CharKey
{
    std::string id;       // persisted id; empty means "handle key"
    unsigned int index;
    unsigned int serial;

    CharKey() : index(0), serial(0) {}
    CharKey(unsigned int i, unsigned int s) : index(i), serial(s) {}

    // An empty id is not an identity: it yields a handle key (0, 0).
    static CharKey FromId(const std::string& persistedId)
    {
        CharKey key;
        key.id = persistedId;
        return key;
    }

    bool HasId() const { return !id.empty(); }

    bool operator==(const CharKey& other) const
    {
        return id == other.id && index == other.index && serial == other.serial;
    }

    bool operator<(const CharKey& other) const
    {
        if (id != other.id) return id < other.id;
        if (index != other.index) return index < other.index;
        return serial < other.serial;
    }

    // For logs only: "id:<uuid>" or "handle:<index>-<serial>".
    std::string ToString() const
    {
        if (HasId()) return "id:" + id;
        char text[48];
        _snprintf_s(text, sizeof(text), _TRUNCATE, "handle:%u-%u", index, serial);
        return std::string(text);
    }
};
} // namespace totalrecruit
