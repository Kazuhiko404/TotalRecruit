// Tests for src/char_key.h: the key every per-character map uses. It is either
// a persisted id (stable across a relaunch) or a handle (the fallback).

#include "test_support.h"
#include "char_key.h"

#include <map>
#include <set>

using namespace totalrecruit;

namespace
{
void IdKeysCompareByIdOnly()
{
    const char* name = "char key: two keys with the same id are the same character";
    const CharKey a = CharKey::FromId("aaaa-1111");
    const CharKey b = CharKey::FromId("aaaa-1111");
    const CharKey c = CharKey::FromId("bbbb-2222");
    CHECK(a.HasId());
    CHECK(a == b);
    CHECK(!(a < b) && !(b < a));
    CHECK(!(a == c));
    CHECK((a < c) != (c < a));
}

void HandleKeysStillWork()
{
    const char* name = "char key: a handle-only key keeps comparing by index and serial";
    const CharKey a(10, 1);
    CHECK(!a.HasId());
    CHECK(a == CharKey(10, 1));
    CHECK(!(a == CharKey(10, 2)));      // a reused slot has another serial
    CHECK(!(a == CharKey(11, 1)));
}

void AnIdAndAHandleNeverMatch()
{
    const char* name = "char key: an id key never equals a handle key";
    const CharKey id = CharKey::FromId("aaaa-1111");
    const CharKey handle(0, 0);
    CHECK(!(id == handle));
    CHECK((id < handle) != (handle < id));
}

void UsableAsAMapAndSetKey()
{
    const char* name = "char key: works as a std::map / std::set key";
    std::map<CharKey, int> m;
    m[CharKey::FromId("aaaa")] = 1;
    m[CharKey::FromId("bbbb")] = 2;
    m[CharKey(3, 4)] = 3;
    m[CharKey::FromId("aaaa")] += 10;
    CHECK(m.size() == 3);
    CHECK(m[CharKey::FromId("aaaa")] == 11);
    CHECK(m[CharKey(3, 4)] == 3);
    std::set<CharKey> s;
    s.insert(CharKey::FromId("aaaa"));
    s.insert(CharKey::FromId("aaaa"));
    CHECK(s.size() == 1);
}

void EmptyIdIsAHandleKey()
{
    const char* name = "char key: an empty id is not an identity";
    CHECK(!CharKey::FromId("").HasId());
}

void ToStringNamesTheKind()
{
    const char* name = "char key: ToString shows which kind of key it is";
    CHECK(CharKey::FromId("aaaa").ToString() == "id:aaaa");
    CHECK(CharKey(7, 9).ToString() == "handle:7-9");
}
} // namespace

void RunCharKeyTests()
{
    IdKeysCompareByIdOnly();
    HandleKeysStillWork();
    AnIdAndAHandleNeverMatch();
    UsableAsAMapAndSetKey();
    EmptyIdIsAHandleKey();
    ToStringNamesTheKind();
}
