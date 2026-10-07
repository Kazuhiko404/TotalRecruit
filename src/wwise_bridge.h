#pragma once

// The game's own Wwise sound engine, called directly (priority-one S1).
//
// kenshi_x64.exe exports the AK::SoundEngine API it links statically; this
// module resolves two entry points once with GetProcAddress on the main module,
// as KenshiExtensionPlugin does for PostEvent (KenshiExtensionPlugin/src/
// CrossbowEx.cpp KEP::CrossbowEx::init):
//
//   AK::SoundEngine::PostEvent(const char*, AkGameObjectID, flags, callback,
//                              cookie, externals, externalSources, playingId)
//       returns the AkPlayingID, 0 (AK_INVALID_PLAYING_ID) when the event is not
//       in a loaded bank or the game object is unknown. Character::audioEvent
//       returns true even then, so this is the only way to know.
//   AK::SoundEngine::SetSwitch(const char* group, const char* value, AkGameObjectID)
//       returns an AKRESULT, 1 (AK_Success) when set.
//
// The game object is the character's own (Character::getAudioObject). Every call
// sits in a POD-only guarded frame (C2712). When an export does not resolve (a
// different game build) the functions say so and the caller falls back to
// Character::audioEvent / audioValue.

class Character;

namespace totalrecruit
{
// The character's Wwise game object, false when unreadable or 0 (no audio set up).
bool SafeAudioObject(Character* character, unsigned __int64* out);

// Posts `eventName` on the game object. Returns the playing id; 0 = not played
// (or PostEvent not exported: see WwisePostEventAvailable).
unsigned long WwisePostEvent(const char* eventName, unsigned __int64 gameObject);
bool WwisePostEventAvailable();

// Sets a switch on the game object. False when not set or SetSwitch not exported.
bool WwiseSetSwitch(const char* group, const char* value, unsigned __int64 gameObject);
bool WwiseSetSwitchAvailable();
} // namespace totalrecruit
