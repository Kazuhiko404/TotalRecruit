#pragma once

// What the player sees and hears when Golpear ends (beat-feedback T2, T3): the
// BeatOutcomeSink the context menu's Golpear and the popup's GOLPEAR both hand to
// OrderSelectedToBeat, so the two buttons report the same way.
//
//   Landed   the prisoner complains aloud (SpeakAloud, messages.json
//            "cage.beatOuch", a list picked stably per prisoner; SpeakAloud says
//            nothing when they cannot speak, unconscious included; an animal never
//            speaks, BeatComplaintSpoken), and the player gets the roller line with
//            the notification sound (TellPlayer, "cage.beatLanded": "{actor}
//            golpea a {name}"). Walking there needs no line of its own: the walk is
//            the feedback.
//   Aborted  the player is told why there was no blow (TellPlayer as a problem):
//            the walk's own lines ("cage.beatAbort.*", beat_walk_rules.h
//            BeatAbortMessageKey) or the cage's (CageCommandProblem: not allowed,
//            resting with the hours, failed). A replacement and a load say nothing
//            (BeatAbortTellsPlayer).
//
// High level on purpose, beside the callers: it speaks to the player through
// recruit_core and to the prisoner through dialogue_authoring, which beat_walk and
// cage_commands may not include.

namespace totalrecruit
{
struct BeatOutcome;

void TellBeatOutcome(const BeatOutcome& outcome);
} // namespace totalrecruit
