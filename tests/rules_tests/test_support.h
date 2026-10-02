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
void RunMercyRulesTests();
void RunPleaTrackerTests();
void RunHoldClockTests();
void RunMoneyRulesTests();
void RunJsonMergeTests();
void RunAtomicFileTests();
void RunCharKeyTests();
void RunIdentityRegistryTests();
