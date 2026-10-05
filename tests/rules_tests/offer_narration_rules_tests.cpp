// Tests for src/offer_narration_rules.cpp: the roller lines of the submission event
// (the proactive offer), a person's or a narrator's for an animal, and whether the NPC
// speaks and carries a head notice (never an animal).

#include "test_support.h"
#include "offer_narration_rules.h"

#include <string>

using namespace totalrecruit;

namespace
{
bool Contains(const char* text, const char* part)
{
    return std::string(text).find(part) != std::string::npos;
}

void APersonKeepsTheShippedKeys()
{
    const char* name = "offer narration: a person keeps submissionEvent.approachNotice / joinedNotice";
    CHECK(std::string(OfferNarrationFor(OfferNarration_ApproachNotice, false).key) ==
          "submissionEvent.approachNotice");
    CHECK(std::string(OfferNarrationFor(OfferNarration_JoinedNotice, false).key) ==
          "submissionEvent.joinedNotice");
    CHECK(std::string(OfferNarrationFor(OfferNarration_CalmNotice, false).key) ==
          "submissionEvent.calmNotice");
    CHECK(std::string(OfferNarrationFor(OfferNarration_IgnoredNotice, false).key) ==
          "submissionEvent.ignoredNotice");
}

void AnAnimalGetsNarratorLines()
{
    const char* name = "offer narration: an animal reads submissionEvent.animal.* narrator lines";
    for (int id = 0; id < OfferNarration_Count; ++id)
    {
        const OfferNarration line = OfferNarrationFor((OfferNarrationId)id, true);
        CHECK(std::string(line.key).compare(0, 23, "submissionEvent.animal.") == 0);
        // A sentence about it, never a quote in its voice.
        CHECK(std::string(line.fallback).compare(0, 7, "{name} ") == 0);
    }
}

void EveryLineNamesTheNpcAndTheCalmItsSeconds()
{
    const char* name = "offer narration: every line has a fallback with {name}; the calm says {seconds}";
    for (int animal = 0; animal < 2; ++animal)
    {
        for (int id = 0; id < OfferNarration_Count; ++id)
        {
            const OfferNarration line = OfferNarrationFor((OfferNarrationId)id, animal != 0);
            CHECK(line.fallback != 0 && Contains(line.fallback, "{name}"));
        }
        CHECK(Contains(OfferNarrationFor(OfferNarration_CalmNotice, animal != 0).fallback, "{seconds}"));
    }
}

void OnlyAPersonSpeaks()
{
    const char* name = "offer narration: a person speaks and carries the head notice; an animal never";
    CHECK(OfferSpeaks(false));
    CHECK(!OfferSpeaks(true));
    CHECK(std::string(OfferNarrationFor((OfferNarrationId)99, true).key) ==
          std::string(OfferNarrationFor(OfferNarration_ApproachNotice, true).key));
}
} // namespace

void RunOfferNarrationRulesTests()
{
    APersonKeepsTheShippedKeys();
    AnAnimalGetsNarratorLines();
    EveryLineNamesTheNpcAndTheCalmItsSeconds();
    OnlyAPersonSpeaks();
}
