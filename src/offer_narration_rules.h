#pragma once

// How the submission event (and the proactive offer riding on it) is told to the
// player, without the engine.
//
// proactive-offer: animals walk up and offer themselves too. A person speaks
// (submissionEvent.approach over its head, the "approaching" head notice) and its roller
// lines name it; an animal never speaks (duel_narration_rules.h, mercy.cpp), so it gets a
// narrator's roller lines of its own and no words or notice over its head:
//
//   line             person                          animal
//   ApproachNotice   submissionEvent.approachNotice  submissionEvent.animal.approachNotice
//   CalmNotice       submissionEvent.calmNotice      submissionEvent.animal.calmNotice   ({seconds})
//   IgnoredNotice    submissionEvent.ignoredNotice   submissionEvent.animal.ignoredNotice
//   JoinedNotice     submissionEvent.joinedNotice    submissionEvent.animal.joinedNotice
//   speaks / notice  yes                             never
//
// Each line carries its messages.json key and the built-in text used when the file has
// none ({name} and {seconds} filled by the caller). submission_director.cpp only asks;
// tests/rules_tests/offer_narration_rules_tests.cpp.

namespace totalrecruit
{
enum OfferNarrationId
{
    OfferNarration_ApproachNotice,
    OfferNarration_CalmNotice,
    OfferNarration_IgnoredNotice,
    OfferNarration_JoinedNotice,
    // Not a line: how many there are.
    OfferNarration_Count
};

struct OfferNarration
{
    const char* key;
    const char* fallback;
};

// The line for `id`, in a person's or an animal's form. An id out of range reads as the
// approach line.
OfferNarration OfferNarrationFor(OfferNarrationId id, bool animal);

// Whether the NPC says its own lines aloud and carries the head notice while it walks
// up: a person, yes; an animal, never.
bool OfferSpeaks(bool animal);
} // namespace totalrecruit
