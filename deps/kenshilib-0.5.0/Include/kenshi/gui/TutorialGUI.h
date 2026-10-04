#pragma once

#include "GUIWindow.h"
#include <mygui/common/baselayout/BaseLayout.h>
#include <kenshi/util/OgreUnordered.h>

class TutorialSubItem : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x8, length = 0x1
    // VTable         : (none)
    // no_addr void TutorialSubItem(const class TutorialSubItem & _a1);// public missing arg names
    TutorialSubItem(const std::string& description, bool skippeable);// public RVA = 0x970FF0
    TutorialSubItem* _CONSTRUCTOR(const std::string& description, bool skippeable);// public RVA = 0x970FF0
    TutorialSubItem(bool skippeable);// public RVA = 0x970A90
    TutorialSubItem* _CONSTRUCTOR(bool skippeable);// public RVA = 0x970A90
    virtual void start();// public RVA = 0x970580 vtable offset = 0x0
    void _NV_start();// public RVA = 0x970580 vtable offset = 0x0
    virtual bool run();// public RVA = 0x970590 vtable offset = 0x8
    bool _NV_run();// public RVA = 0x970590 vtable offset = 0x8
    virtual void end();// public RVA = 0x9705A0 vtable offset = 0x10
    void _NV_end();// public RVA = 0x9705A0 vtable offset = 0x10
    std::string description; // 0x8 Member
    bool skippeable; // 0x30 Member
    ~TutorialSubItem();// public RVA = 0x97A400
    void _DESTRUCTOR();// public RVA = 0x97A400
    // no_addr class TutorialSubItem & operator=(const class TutorialSubItem & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};

class TutorialItem : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x8, length = 0x1
    // VTable         : (none)
    enum State
    {
        INACTIVE,
        ACTIVE,
        ENDED
    };

    // no_addr void TutorialItem(const class TutorialItem & _a1);// public missing arg names
    TutorialItem();// public RVA = 0x970B70
    TutorialItem* _CONSTRUCTOR();// public RVA = 0x970B70
    virtual ~TutorialItem();// public RVA = 0x970EA0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x970EA0 vtable offset = 0x0
    unsigned int getId() const;// public RVA = 0x970500
    const std::string& getTitle() const;// public RVA = 0x970510
    bool isLastSubItem() const;// public RVA = 0x970840
    bool isSkippable() const;// public RVA = 0x970860
    bool isActive() const;// public RVA = 0x970520
    bool hasEnded() const;// public RVA = 0x970530
    TutorialSubItem* getCurrentSubItem() const;// public RVA = 0x970880
    TutorialSubItem* getSubItemAt(int index) const;// public RVA = 0x9708A0
    int getCurrentSubItemIndex() const;// public RVA = 0x970540
    int getNumSubItems() const;// public RVA = 0x9708C0
    bool hasNextSubItem() const;// public RVA = 0x9708D0
    bool hasPrevSubItem() const;// public RVA = 0x970550
    void nextSubItem();// public RVA = 0x9708F0
    void prevSubItem();// public RVA = 0x970940
    bool getStarted() const;// public RVA = 0x970560
    bool getEnded() const;// public RVA = 0x970990
    virtual void reset();// public RVA = 0x970570 vtable offset = 0x8
    void _NV_reset();// public RVA = 0x970570 vtable offset = 0x8
    virtual bool conditionsMet() = 0;// public vtable offset = 0x10
    virtual void start();// public RVA = 0x9709B0 vtable offset = 0x18
    void _NV_start();// public RVA = 0x9709B0 vtable offset = 0x18
    virtual bool run();// public RVA = 0x9709D0 vtable offset = 0x20
    bool _NV_run();// public RVA = 0x9709D0 vtable offset = 0x20
    virtual void end();// public RVA = 0x970A60 vtable offset = 0x28
    void _NV_end();// public RVA = 0x970A60 vtable offset = 0x28
    unsigned int id; // 0x8 Member
    bool started; // 0xC Member
    bool locked; // 0xD Member
    std::string title; // 0x10 Member
    TutorialItem::State state; // 0x38 Member
    unsigned char subItemIndex; // 0x3C Member
    Ogre::FastArray<TutorialSubItem*> subItems; // 0x40 Member
    Ogre::FastArray<std::string > requiredTutorialsStr; // 0x58 Member
    Ogre::FastArray<TutorialItem*> requiredTutorials; // 0x70 Member
    // no_addr class TutorialItem & operator=(const class TutorialItem & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class TutorialGUI : public GUIWindow, public wraps::BaseLayout
{
public:
    // GUIWindow offset = 0x0, length = 0x30
    // wraps::BaseLayout offset = 0x30, length = 0xA0
    class TutorialGUILine : public wraps::BaseLayout
    {
    public:
        // wraps::BaseLayout offset = 0x0, length = 0xA0
        // no_addr void TutorialGUILine(const class TutorialGUI::TutorialGUILine & _a1);// public missing arg names
        TutorialGUILine(MyGUI::Widget* parent, TutorialItem* item, bool flash);// public RVA = 0x973E30
        TutorialGUILine* _CONSTRUCTOR(MyGUI::Widget* parent, TutorialItem* item, bool flash);// public RVA = 0x973E30
        bool updateHighlight(float time);// public RVA = 0x970340
        MyGUI::Button* nameButton; // 0xA0 Member
        MyGUI::Button* closeButton; // 0xA8 Member
        MyGUI::Widget* flashWidget; // 0xB0 Member
        virtual ~TutorialGUILine();// public RVA = 0x97CAB0 vtable offset = 0x0
        void _DESTRUCTOR();// public RVA = 0x97CAB0 vtable offset = 0x0
        // no_addr class TutorialGUI::TutorialGUILine & operator=(const class TutorialGUI::TutorialGUILine & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
    };
    // Typedef        : TutorialItemLineMap
    // Typedef        : TutorialLinesList
    enum HighlightItem
    {
        NONE,
        PORTRAITS,
        PANEL_STATS,
        PANEL_STATS_BOTTOM,
        PANEL_STATS_BOTTOM_POWER_BUTTON,
        PANEL_STATS_BOTTOM_WORKERS,
        PANEL_MEDICAL,
        PANEL_MEDICAL_STATUS,
        PANEL_MEDICAL_BAR_BODY,
        PANEL_MEDICAL_BAR_LIMBS,
        PANEL_MEDICAL_BAR_BLOOD,
        PANEL_MEDICAL_BAR_POWER,
        PANEL_MEDICAL_BAR_MATERIALS,
        PANEL_MEDICAL_BAR_MATERIALS_BUILDING,
        PANEL_MEDICAL_BAR_CONDITION,
        PANEL_MEDICAL_BAR_CONDITION_BUILDING,
        PANEL_GAME_SPEED,
        PANEL_TIME_MONEY,
        BUTTON_BUILD,
        PANEL_BOUNTY,
        PANEL_MENU_BAR,
        PANEL_MENU_BAR_SQUAD,
        PANEL_MENU_BAR_STATS,
        PANEL_MENU_BAR_TECH,
        PANEL_ORDERS,
        PANEL_ORDERS_COMMANDS,
        PANEL_ORDERS_JOBS,
        PANEL_ORDERS_JOB_REMOVE,
        PANEL_ORDERS_JOB_OBEDIENCE,
        PANEL_ORDERS_MEDIC_BUTTON,
        PANEL_ORDERS_RESCUE_BUTTON,
        MAPSCREEN_CRAFT_TAB,
        MAPSCREEN_CRAFT_TAB_ITEMS,
        BUILDING_INVENTORY_INPUT,
        BUILD_MODE_PANEL_BUILDINGS
    };

    // no_addr void TutorialGUI(const class TutorialGUI & _a1);// public missing arg names
    TutorialGUI();// public RVA = 0x9744F0
    TutorialGUI* _CONSTRUCTOR();// public RVA = 0x9744F0
    virtual ~TutorialGUI();// public RVA = 0x9731F0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x9731F0 vtable offset = 0x0
    virtual void clear() override;// public RVA = 0x9711C0 vtable offset = 0x0
    void _NV_clear();// public RVA = 0x9711C0 vtable offset = 0x0
    void startHighlight(TutorialGUI::HighlightItem item);// public RVA = 0x972AB0
    void endHighlight();// public RVA = 0x9703C0
    void _setHightlightCoords();// public RVA = 0x9712A0
    virtual void update() override;// public RVA = 0x975000 vtable offset = 0x0
    void _NV_update();// public RVA = 0x975000 vtable offset = 0x0
    void setEnabled(bool value);// public RVA = 0x9706C0
    virtual void show(bool value) override;// public RVA = 0x970420 vtable offset = 0x0
    void _NV_show(bool value);// public RVA = 0x970420 vtable offset = 0x0
    void addTutorialsToList(const Ogre::vector<TutorialItem*>::type& tutorials, bool flash);// private RVA = 0x975270
    void removeTutorialFromList(TutorialItem* tutorial);// private RVA = 0x972720
    void arrangeList();// private RVA = 0x970720
    void refreshUI();// private RVA = 0x973880
    void windowButtonEvent(MyGUI::Window* _sender, const std::string& _name);// private RVA = 0x9707E0
    void windowPrevEvent(MyGUI::Widget* _sender);// private RVA = 0x974190
    void windowNextEvent(MyGUI::Widget* _sender);// private RVA = 0x974220
    void dismissButtonEvent(MyGUI::Widget* _sender);// private RVA = 0x972940
    void tooltipOpen(MyGUI::Widget* _sender);// private RVA = 0x9751A0
    void tooltipClose(MyGUI::Widget* _sender);// private RVA = 0x9742D0
    void showTutorialWindow(TutorialItem* item);// private RVA = 0x9743A0
    void closeTutorialWindow();// private RVA = 0x970470
    void updateCurrentItem();// private RVA = 0x974440
    TutorialGUI::HighlightItem highlightItem; // 0xD0 Member
    MyGUI::Widget* highlightItemWidget; // 0xD8 Member
    MyGUI::Widget* highlightPanel; // 0xE0 Member
    char highlightDirection; // 0xE8 Member
    float highlightAlpha; // 0xEC Member
    MyGUI::Window* window; // 0xF0 Member
    MyGUI::Button* windowPrevButton; // 0xF8 Member
    MyGUI::Button* windowNextButton; // 0x100 Member
    MyGUI::EditBox* windowText; // 0x108 Member
    MyGUI::Button* dismissButton; // 0x110 Member
    MyGUI::TextBox* pagingText; // 0x118 Member
    MyGUI::Widget* tooltipsPanel; // 0x120 Member
    TutorialItem* currentTutorialItem; // 0x128 Member
    Ogre::vector<TutorialGUI::TutorialGUILine*>::type activeTutorialsList; // 0x130 Member
    ogre_unordered_map<TutorialItem*, TutorialGUI::TutorialGUILine*>::type activeTutorials; // 0x150 Member
    bool flashNewItem; // 0x190 Member
    bool enabled; // 0x191 Member
    // no_addr class TutorialGUI & operator=(const class TutorialGUI & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class TutorialpediaGUI : public GUIWindow, public wraps::BaseLayout
{
public:
    // GUIWindow offset = 0x0, length = 0x30
    // wraps::BaseLayout offset = 0x30, length = 0xA0
    virtual void show(bool value) override;// public RVA = 0x9705D0 vtable offset = 0x0
    void _NV_show(bool value);// public RVA = 0x9705D0 vtable offset = 0x0
    virtual void clear() override;// public RVA = 0x9705F0 vtable offset = 0x0
    void _NV_clear();// public RVA = 0x9705F0 vtable offset = 0x0
    virtual bool isVisible() const override;// public RVA = 0x9705B0 vtable offset = 0x0
    bool _NV_isVisible() const;// public RVA = 0x9705B0 vtable offset = 0x0
    // no_addr void TutorialpediaGUI(const class TutorialpediaGUI & _a1);// public missing arg names
    TutorialpediaGUI();// private RVA = 0x975E90
    TutorialpediaGUI* _CONSTRUCTOR();// private RVA = 0x975E90
    virtual ~TutorialpediaGUI();// private RVA = 0x972A60 vtable offset = 0x0
    void _DESTRUCTOR();// private RVA = 0x972A60 vtable offset = 0x0
    void setup();// private RVA = 0x971080
    void updateCurrentItem();// private RVA = 0x973BD0
    void tutorialSelectedEvent(MyGUI::ListBox* _a1, unsigned __int64 _index);// private RVA = 0x974470
    void tutorialPrevEvent(MyGUI::Widget* _a1);// private RVA = 0x973D90 missing arg names
    void tutorialNextEvent(MyGUI::Widget* _a1);// private RVA = 0x973DE0 missing arg names
    void tutorialActivateButtonEvent(MyGUI::Widget* _a1);// private RVA = 0x975DF0 missing arg names
    void tutorialWindowButton(MyGUI::Window* _a1, const std::string& _a2);// private RVA = 0x970670 missing arg names
    TutorialItem* currentItem; // 0xD0 Member
    int currentItemIndex; // 0xD8 Member
    MyGUI::ListBox* tutorialsList; // 0xE0 Member
    MyGUI::EditBox* descriptionText; // 0xE8 Member
    MyGUI::Button* activateButton; // 0xF0 Member
    MyGUI::Button* prevButton; // 0xF8 Member
    MyGUI::Button* nextButton; // 0x100 Member
    MyGUI::TextBox* pagingText; // 0x108 Member
    // no_addr class TutorialpediaGUI & operator=(const class TutorialpediaGUI & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// private vtable offset = 0x0 missing arg names
};