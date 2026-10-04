#pragma once

// The whole harness: a failure counter and a CHECK macro. Each *_tests.cpp
// exposes one Run function that main.cpp calls; a test is a void function that
// defines `const char* name` and CHECKs.

namespace rules_tests
{
void Check(bool ok, const char* testName, const char* what);
int Failures();
int Checks();
} // namespace rules_tests

#define CHECK(cond) ::rules_tests::Check((cond), name, #cond)

void RunChanceRulesTests();
void RunPriceRulesTests();
void RunRouteRulesTests();
void RunDefeatRulesTests();
void RunDefeatEpisodeRulesTests();
void RunDefeatLedgerTests();
void RunMercyRulesTests();
void RunPleaTrackerTests();
void RunHoldClockTests();
void RunMoneyRulesTests();
void RunJsonMergeTests();
void RunAtomicFileTests();
void RunCharKeyTests();
void RunIdentityRegistryTests();
void RunRouteDecisionsTests();
void RunBodyRulesTests();
void RunReasonCatalogTests();
void RunAnimalRulesTests();
void RunReadoutFormatTests();
void RunReadoutCatalogTests();
void RunReadoutModelTests();
void RunTalkHintCacheTests();
void RunMenuSlotTests();
void RunMenuButtonReleaseTests();
void RunOfferViewTests();
void RunScreenProjectionTests();
void RunHaggleRulesTests();
void RunCardDismissRulesTests();
void RunFlatSliderTests();
void RunCageCommandRulesTests();
void RunFeedingLedgerTests();
void RunSubmissionRulesTests();
void RunSubmissionTrackerTests();
void RunHeadNoticeRulesTests();
void RunPersonalityGroupsTests();
void RunAllyTierRulesTests();
void RunTempMemberRulesTests();
void RunHelperRulesTests();
void RunLeaveHomingRulesTests();
void RunOverlayVisibilityRulesTests();
