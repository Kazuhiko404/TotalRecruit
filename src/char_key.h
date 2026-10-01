#pragma once

namespace totalrecruit
{
// Identity for a character that survives death and reload.
//
// A raw Character* is an address, not an identity. The engine frees a dead
// character and hands the same address to the next one, so a map keyed by
// pointer silently applies the dead one's state to a stranger -- and a reload
// moves everyone, so the key does not survive the save either.
//
// The handle carries index AND serial, and the serial is what tells a reused
// slot apart from its previous occupant. Every per-character map in the mod
// keys on this; three of them used to declare their own copy of it.
//
// Engine-free on purpose: the pure rule modules and their tests use it.
struct CharKey
{
    unsigned int index;
    unsigned int serial;

    CharKey() : index(0), serial(0) {}
    CharKey(unsigned int i, unsigned int s) : index(i), serial(s) {}

    bool operator<(const CharKey& other) const
    {
        if (index != other.index) return index < other.index;
        return serial < other.serial;
    }
};
} // namespace totalrecruit
