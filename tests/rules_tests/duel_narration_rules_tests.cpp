// Tests for src/duel_narration_rules.cpp: which line tells the player how a duel went,
// a person's own voice or a narrator's for an animal, and whether a notice is raised
// over the NPC's head (never for an animal: it does not speak).

#include "test_support.h"
#include "duel_narration_rules.h"

#include <string>

using namespace totalrecruit;

namespace
{
bool StartsWith(const char* text, const char* prefix)
{
    return std::string(text).compare(0, std::string(prefix).size(), prefix) == 0;
}

bool Contains(const char* text, const char* part)
{
    return std::string(text).find(part) != std::string::npos;
}

void APersonKeepsTheExistingDuelLines()
{
    const char* name = "narration: a person keeps duel.challenged/joined/stays/lost";
    CHECK(std::string(DuelNarrationFor(DuelNarration_Challenged, false).key) == "duel.challenged");
    CHECK(std::string(DuelNarrationFor(DuelNarration_Joined, false).key) == "duel.joined");
    CHECK(std::string(DuelNarrationFor(DuelNarration_Stays, false).key) == "duel.stays");
    CHECK(std::string(DuelNarrationFor(DuelNarration_Lost, false).key) == "duel.lost");
}

void AnAnimalGetsItsOwnNarratorLines()
{
    const char* name = "narration: an animal reads duel.animal.* lines";
    CHECK(std::string(DuelNarrationFor(DuelNarration_Challenged, true).key) == "duel.animal.challenged");
    CHECK(std::string(DuelNarrationFor(DuelNarration_Joined, true).key) == "duel.animal.joined");
    CHECK(std::string(DuelNarrationFor(DuelNarration_Stays, true).key) == "duel.animal.stays");
    CHECK(std::string(DuelNarrationFor(DuelNarration_Lost, true).key) == "duel.animal.lost");
}

void EveryLineHasAFallbackNamingTheNpc()
{
    const char* name = "narration: every line has a built-in text with {name}";
    for (int animal = 0; animal < 2; ++animal)
    {
        for (int id = 0; id < DuelNarration_Count; ++id)
        {
            const DuelNarration line = DuelNarrationFor((DuelNarrationId)id, animal != 0);
            CHECK(line.fallback != 0 && line.fallback[0] != '\0');
            CHECK(Contains(line.fallback, "{name}"));
        }
    }
    // The numbers the player acts on stay in the animal's lines too.
    CHECK(Contains(DuelNarrationFor(DuelNarration_Stays, true).fallback, "{hours}"));
    CHECK(Contains(DuelNarrationFor(DuelNarration_Stays, true).fallback, "{chance}"));
    CHECK(Contains(DuelNarrationFor(DuelNarration_Lost, true).fallback, "{hours}"));
}

void AnAnimalNeverSpeaksOverItsHead()
{
    const char* name = "narration: a head notice for a person, never for an animal";
    CHECK(DuelRaisesHeadNotice(false));
    CHECK(!DuelRaisesHeadNotice(true));
    // An animal line is a narrator's sentence about it, never a quote in its voice.
    for (int id = 0; id < DuelNarration_Count; ++id)
    {
        const DuelNarration line = DuelNarrationFor((DuelNarrationId)id, true);
        CHECK(StartsWith(line.fallback, "{name} "));
        CHECK(!Contains(line.fallback, "{name}:"));
    }
}

void AnUnknownIdReadsAsTheChallenge()
{
    const char* name = "narration: an out-of-range id falls back to the challenge line";
    CHECK(std::string(DuelNarrationFor((DuelNarrationId)99, true).key) == "duel.animal.challenged");
    CHECK(std::string(DuelNarrationFor((DuelNarrationId)-1, false).key) == "duel.challenged");
}
} // namespace

void RunDuelNarrationRulesTests()
{
    APersonKeepsTheExistingDuelLines();
    AnAnimalGetsItsOwnNarratorLines();
    EveryLineHasAFallbackNamingTheNpc();
    AnAnimalNeverSpeaksOverItsHead();
    AnUnknownIdReadsAsTheChallenge();
}
