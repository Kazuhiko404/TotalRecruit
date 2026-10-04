// Tests for src/overlay_visibility_rules.cpp: when the readout card and the head
// notices step aside for one of Kenshi's own windows (the dialogue window).

#include "test_support.h"
#include "overlay_visibility_rules.h"

using namespace totalrecruit;

namespace
{
NativeWindowReading Read()
{
    NativeWindowReading reading;
    reading.read = true;
    return reading;
}

void NothingOpenKeepsTheOverlays()
{
    const char* name = "NothingOpenKeepsTheOverlays";
    CHECK(!OverlaysYieldToNativeWindow(Read()));
}

void AnOpenDialogueHidesTheOverlays()
{
    const char* name = "AnOpenDialogueHidesTheOverlays";
    NativeWindowReading reading = Read();
    reading.inDialogue = true;
    CHECK(OverlaysYieldToNativeWindow(reading));
}

void AVisibleDialogueWindowAloneIsEnough()
{
    const char* name = "AVisibleDialogueWindowAloneIsEnough";
    // Either engine signal counts: the window can be up a frame before (or after)
    // the in-dialogue flag.
    NativeWindowReading reading = Read();
    reading.dialogueWindowVisible = true;
    CHECK(OverlaysYieldToNativeWindow(reading));
}

void AModalMessageHidesTheOverlays()
{
    const char* name = "AModalMessageHidesTheOverlays";
    NativeWindowReading reading = Read();
    reading.modalMessage = true;
    CHECK(OverlaysYieldToNativeWindow(reading));
}

void AFailedReadKeepsTheOverlays()
{
    const char* name = "AFailedReadKeepsTheOverlays";
    // A read that faulted says nothing: the overlays behave as before the fix
    // rather than vanishing for good.
    NativeWindowReading reading;
    reading.read = false;
    reading.inDialogue = true;
    reading.dialogueWindowVisible = true;
    reading.modalMessage = true;
    CHECK(!OverlaysYieldToNativeWindow(reading));
}

void TheDefaultReadingHidesNothing()
{
    const char* name = "TheDefaultReadingHidesNothing";
    const NativeWindowReading reading;
    CHECK(!reading.read);
    CHECK(!reading.inDialogue);
    CHECK(!reading.dialogueWindowVisible);
    CHECK(!reading.modalMessage);
    CHECK(!OverlaysYieldToNativeWindow(reading));
}
} // namespace

void RunOverlayVisibilityRulesTests()
{
    NothingOpenKeepsTheOverlays();
    AnOpenDialogueHidesTheOverlays();
    AVisibleDialogueWindowAloneIsEnough();
    AModalMessageHidesTheOverlays();
    AFailedReadKeepsTheOverlays();
    TheDefaultReadingHidesNothing();
}
