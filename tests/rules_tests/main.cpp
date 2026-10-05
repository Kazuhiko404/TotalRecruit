// Unit tests for TotalRecruit's pure rule modules. No engine: the plugin's
// rules take plain values and the config structs, so this is a small console
// exe built with the same toolset as the plugin (rules_tests.vcxproj).
//
// A suite is a Run function; it prints one PASS/FAIL line per suite and the
// process exits non-zero if any CHECK failed.

#include "test_support.h"

#include <cstdio>

namespace rules_tests
{
namespace
{
int g_failures = 0;
int g_checks = 0;
} // namespace

void Check(bool ok, const char* testName, const char* what)
{
    ++g_checks;
    if (!ok)
    {
        ++g_failures;
        std::printf("  FAIL %s: %s\n", testName, what);
    }
}

int Failures() { return g_failures; }
int Checks()   { return g_checks; }
} // namespace rules_tests

namespace
{
struct Suite
{
    const char* label;
    void (*run)();
};

const Suite kSuites[] = {
    { "chance_rules", RunChanceRulesTests },
    { "beating_stress_rules", RunBeatingStressRulesTests },
    { "price_rules",  RunPriceRulesTests },
    { "route_rules",  RunRouteRulesTests },
    { "defeat_rules", RunDefeatRulesTests },
    { "defeat_episode_rules", RunDefeatEpisodeRulesTests },
    { "defeat_ledger", RunDefeatLedgerTests },
    { "mercy_rules",  RunMercyRulesTests },
    { "plea_tracker", RunPleaTrackerTests },
    { "hold_clock", RunHoldClockTests },
    { "money_rules", RunMoneyRulesTests },
    { "json_merge", RunJsonMergeTests },
    { "atomic_file", RunAtomicFileTests },
    { "char_key", RunCharKeyTests },
    { "identity_registry", RunIdentityRegistryTests },
    { "route_decisions", RunRouteDecisionsTests },
    { "body_rules", RunBodyRulesTests },
    { "reason_catalog", RunReasonCatalogTests },
    { "animal_rules", RunAnimalRulesTests },
    { "readout_format", RunReadoutFormatTests },
    { "readout_catalog", RunReadoutCatalogTests },
    { "readout_model", RunReadoutModelTests },
    { "talk_hint_cache", RunTalkHintCacheTests },
    { "menu_slot", RunMenuSlotTests },
    { "menu_button_release", RunMenuButtonReleaseTests },
    { "offer_view", RunOfferViewTests },
    { "screen_projection", RunScreenProjectionTests },
    { "haggle_rules", RunHaggleRulesTests },
    { "card_dismiss_rules", RunCardDismissRulesTests },
    { "flat_slider", RunFlatSliderTests },
    { "cage_command_rules", RunCageCommandRulesTests },
    { "feeding_ledger", RunFeedingLedgerTests },
    { "submission_rules", RunSubmissionRulesTests },
    { "submission_tracker", RunSubmissionTrackerTests },
    { "head_notice_rules", RunHeadNoticeRulesTests },
    { "personality_groups", RunPersonalityGroupsTests },
    { "ally_tier_rules", RunAllyTierRulesTests },
    { "temp_member_rules", RunTempMemberRulesTests },
    { "helper_rules", RunHelperRulesTests },
    { "leave_homing_rules", RunLeaveHomingRulesTests },
    { "overlay_visibility_rules", RunOverlayVisibilityRulesTests },
    { "duel_challenge_rules", RunDuelChallengeRulesTests },
    { "level_rules", RunLevelRulesTests },
    { "duel_record_cache_rules", RunDuelRecordCacheRulesTests },
    { "duel_join_rules", RunDuelJoinRulesTests },
    { "knockout_watch_rules", RunKnockoutWatchRulesTests },
    { "menu_anchor", RunMenuAnchorTests },
    { "preset_rules", RunPresetRulesTests },
    { "cheat_waits", RunCheatWaitsTests },
    { "cared_for_rules", RunCaredForRulesTests },
    { "insistence_rules", RunInsistenceRulesTests },
};
} // namespace

int main()
{
    const int count = sizeof(kSuites) / sizeof(kSuites[0]);
    int failedSuites = 0;
    for (int i = 0; i < count; ++i)
    {
        const int before = rules_tests::Failures();
        kSuites[i].run();
        const bool ok = (rules_tests::Failures() == before);
        if (!ok)
        {
            ++failedSuites;
        }
        std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", kSuites[i].label);
    }

    std::printf("\n%d/%d suites passed, %d/%d checks failed\n",
                count - failedSuites, count,
                rules_tests::Failures(), rules_tests::Checks());
    return failedSuites == 0 ? 0 : 1;
}
