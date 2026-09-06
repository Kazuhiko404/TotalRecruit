#include "common.h"
#include "settings_panel.h"
#include "config.h"
#include "log.h"

#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/DatapanelGUI.h>
#include <kenshi/gui/DataPanelLine.h>

namespace totalrecruit
{
namespace
{
// Category ids double as page ids. Picked high to stay clear of the game's own
// reserved values (General 0x1, Gameplay 0x22, Graphics 0x17, Audio 0x18,
// Controls 0x19).
const int kCatGeneral = 610;
const int kCatRoutes = 611;
const int kCatPrices = 612;

DatapanelGUI* g_panel = NULL;
bool g_visible = false;

// The dropdown writes an int, and Preset is an enum, so the panel binds this
// and the value is copied back when the page is rebuilt.
int g_presetValue = 0;

// Sliders and checkboxes write straight through these pointers, so the config
// is the model. Ints that the panel edits as floats live here too.
float g_catsPerLevel = 0.0f;
float g_catsMinimum = 0.0f;

void PushConfigToPanelVars()
{
    const Config& config = GetConfig();
    g_presetValue = config.preset;
    g_catsPerLevel = (float)config.prices.catsPerLevel;
    g_catsMinimum = (float)config.prices.catsMinimum;
}

void PullPanelVarsToConfig()
{
    Config& config = GetConfig();
    config.preset = g_presetValue;
    config.prices.catsPerLevel = (int)g_catsPerLevel;
    config.prices.catsMinimum = (int)g_catsMinimum;
}

// Every panel call materialises std::string temporaries, so the real work sits
// in plain functions and only the call is guarded; C2712 counts every object in
// the whole frame, not just those inside the __try block.

void BuildGeneralPage()
{
    g_panel->clearPage(kCatGeneral);

    g_panel->setLine("How strict recruiting is", "", kCatGeneral, false, true);
    DataPanelLine_DropBox* preset =
        g_panel->setLineDropBox("Preset", kCatGeneral, &g_presetValue, false, 0.7f);
    if (preset != NULL)
    {
        preset->addAValue("Cheat - anyone, instantly", Preset_Cheat);
        preset->addAValue("Balanced - each NPC has a price", Preset_Balanced);
        preset->addAValue("Vanilla+ - price shifts with faction", Preset_VanillaPlus);
        preset->setSelectedValue(g_presetValue);
    }

    g_panel->addSpace(kCatGeneral, 0.5f);
    g_panel->setLineCheckbox("Show the recruit button", &GetConfig().showRecruitButton, kCatGeneral);
    g_panel->setLineCheckbox("Log NPC traits on click (diagnostic)",
                             &GetConfig().probeOnClick, kCatGeneral);
}

void BuildRoutesPage()
{
    g_panel->clearPage(kCatRoutes);

    g_panel->setLine("Rescue", "", kCatRoutes, false, true);
    g_panel->setLineCheckbox("Recruit those you save from dying",
                             &GetConfig().rescueEnabled, kCatRoutes);
    g_panel->setLineCheckbox("Only your own medics count",
                             &GetConfig().rescuePlayerOnly, kCatRoutes);

    g_panel->addSpace(kCatRoutes, 0.5f);
    g_panel->setLine("Captivity", "", kCatRoutes, false, true);
    g_panel->setLineCheckbox("Recruit prisoners in your cages",
                             &GetConfig().captivityEnabled, kCatRoutes);

    g_panel->addSpace(kCatRoutes, 0.5f);
    g_panel->setLine("Surrender", "", kCatRoutes, false, true);
    g_panel->setLineCheckbox("Beaten enemies can yield to you",
                             &GetConfig().surrenderEnabled, kCatRoutes);

    g_panel->addSpace(kCatRoutes, 0.5f);
    g_panel->setLine("Animals", "", kCatRoutes, false, true);
    g_panel->setLineCheckbox("Tame animals with food and care",
                             &GetConfig().animalsEnabled, kCatRoutes);
}

void BuildPricesPage()
{
    g_panel->clearPage(kCatPrices);
    Prices& prices = GetConfig().prices;

    g_panel->setLine("What they charge", "", kCatPrices, false, true);

    DataPanelLine_SliderEditable* line =
        g_panel->setLineSliderEditable("Cats per level", kCatPrices, true, 0.0f, 500.0f,
                                       &g_catsPerLevel);
    if (line != NULL) line->setPrecision(0);

    line = g_panel->setLineSliderEditable("Minimum cats", kCatPrices, true, 0.0f, 10000.0f,
                                          &g_catsMinimum);
    if (line != NULL) line->setPrecision(0);

    g_panel->addSpace(kCatPrices, 0.5f);
    g_panel->setLine("When they will listen", "", kCatPrices, false, true);

    // 0 leaves it to the engine's own isReallyHungry; the raw field is not a
    // 0..1 fraction, so the useful range runs well above 1.
    line = g_panel->setLineSliderEditable("Extra hunger trigger (0 = engine)", kCatPrices,
                                          true, 0.0f, 10.0f, &prices.hungerThreshold);
    if (line != NULL) line->setPrecision(1);

    line = g_panel->setLineSliderEditable("Wounds needed", kCatPrices, true, 0.0f, 1.0f,
                                          &prices.woundedThreshold);
    if (line != NULL) line->setPrecision(2);

    line = g_panel->setLineSliderEditable("Hours caged (proud)", kCatPrices, true, 0.0f, 72.0f,
                                          &prices.hoursCagedForProud);
    if (line != NULL) line->setPrecision(0);

    line = g_panel->setLineSliderEditable("Extra hours (unique)", kCatPrices, true, 0.0f, 168.0f,
                                          &prices.uniqueExtraHours);
    if (line != NULL) line->setPrecision(0);

    g_panel->addSpace(kCatPrices, 0.5f);
    g_panel->setLine("Personality", "", kCatPrices, false, true);

    line = g_panel->setLineSliderEditable("Fearful pay less", kCatPrices, true, 0.1f, 2.0f,
                                          &prices.fearfulMultiplier);
    if (line != NULL) line->setPrecision(2);

    line = g_panel->setLineSliderEditable("Brave pay more", kCatPrices, true, 0.1f, 4.0f,
                                          &prices.braveMultiplier);
    if (line != NULL) line->setPrecision(2);

    line = g_panel->setLineSliderEditable("Faction weight (Vanilla+)", kCatPrices, true, 0.0f, 2.0f,
                                          &prices.factionWeight);
    if (line != NULL) line->setPrecision(2);
}

void BuildPanel()
{
    if (gui == NULL)
    {
        return;
    }

    g_panel = gui->createDatapanel(0.06f, 0.62f, 0.30f, 0.56f, true, "Window", true);
    if (g_panel == NULL)
    {
        return;
    }

    g_panel->setCaption("TotalRecruit");
    g_panel->setPanelName("TotalRecruit");
    g_panel->setLineSpacing(28.0f);

    BuildGeneralPage();
    BuildRoutesPage();
    BuildPricesPage();

    g_panel->showTabs(true);
    g_panel->changeCategory(kCatGeneral);
    g_panel->addTab(kCatGeneral, "General", "");
    g_panel->addTab(kCatRoutes, "Routes", "");
    g_panel->addTab(kCatPrices, "Prices", "");
}

void SafeBuildPanel()
{
    TR_TRY { BuildPanel(); }
    TR_CATCH { g_panel = NULL; }
}

void ShowPanel(bool on)
{
    g_panel->show(on);
}

void SafeShowPanel(bool on)
{
    TR_TRY { ShowPanel(on); }
    TR_CATCH { }
}

void DestroyPanel()
{
    if (gui != NULL && g_panel != NULL)
    {
        gui->destroy(g_panel);
    }
}

void SafeDestroyPanel()
{
    TR_TRY { DestroyPanel(); }
    TR_CATCH { }
}
} // namespace

void ToggleSettingsPanel()
{
    if (g_panel == NULL)
    {
        PushConfigToPanelVars();
        SafeBuildPanel();
        if (g_panel == NULL)
        {
            LogError("Could not create the settings panel.");
            return;
        }
        g_visible = false;
    }

    g_visible = !g_visible;
    SafeShowPanel(g_visible);

    if (!g_visible)
    {
        // Closing is the natural commit point for the values the widgets wrote
        // into their bound variables.
        PullPanelVarsToConfig();
    }
}

void ShutdownSettingsPanel()
{
    if (g_panel == NULL)
    {
        return;
    }

    PullPanelVarsToConfig();
    SafeDestroyPanel();
    g_panel = NULL;
    g_visible = false;
}
} // namespace totalrecruit
