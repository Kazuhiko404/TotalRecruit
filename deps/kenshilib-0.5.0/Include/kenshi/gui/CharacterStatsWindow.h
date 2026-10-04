#pragma once

#include "GUIWindow.h"
#include <mygui/common/baselayout/BaseLayout.h>
#include <kenshi/Enums.h>

class hand;
class DatapanelGUI;
class Character;

class CharacterStatsWindow : public GUIWindow, public wraps::BaseLayout
{
public:
    // GUIWindow offset = 0x0, length = 0x30
    // wraps::BaseLayout offset = 0x30, length = 0xA0
    class Stat
    {
    public:
        Stat(const CharacterStatsWindow::Stat& __that);// public RVA = 0x8C0970
        Stat* _CONSTRUCTOR(const CharacterStatsWindow::Stat& __that);// public RVA = 0x8C0970
        Stat();// public RVA = 0x8A8C10
        Stat* _CONSTRUCTOR();// public RVA = 0x8A8C10
        Stat(StatsEnumerated type, const std::string& id, const std::string& name, bool active, const std::string& description);// private RVA = 0x8A8E10
        Stat* _CONSTRUCTOR(StatsEnumerated type, const std::string& id, const std::string& name, bool active, const std::string& description);// private RVA = 0x8A8E10
        StatsEnumerated type; // 0x0 Member
        std::string id; // 0x8 Member
        std::string name; // 0x30 Member
        std::string description; // 0x58 Member
        bool active; // 0x80 Member
        ~Stat();// public RVA = 0x8C0170
        void _DESTRUCTOR();// public RVA = 0x8C0170
        CharacterStatsWindow::Stat& operator=(const CharacterStatsWindow::Stat& __that);// public RVA = 0x8C0DB0
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class StatGroup
    {
    public:
        enum Group
        {
            NONE,
            WEAPONS,
            COMBAT,
            THIEVERY,
            ATHLETICS,
            SCIENCES,
            TRADES,
            RANGED
        };

        StatGroup(const CharacterStatsWindow::StatGroup& __that);// public RVA = 0x8C0AF0
        StatGroup* _CONSTRUCTOR(const CharacterStatsWindow::StatGroup& __that);// public RVA = 0x8C0AF0
        StatGroup(CharacterStatsWindow::StatGroup::Group group, const std::string& name);// private RVA = 0x8A8ED0
        StatGroup* _CONSTRUCTOR(CharacterStatsWindow::StatGroup::Group group, const std::string& name);// private RVA = 0x8A8ED0
        StatGroup();// private RVA = 0x8A8C60
        StatGroup* _CONSTRUCTOR();// private RVA = 0x8A8C60
        CharacterStatsWindow::StatGroup::Group group; // 0x0 Member
        std::string name; // 0x8 Member
        ~StatGroup();// public RVA = 0x8C0130
        void _DESTRUCTOR();// public RVA = 0x8C0130
        // no_addr class CharacterStatsWindow::StatGroup & operator=(const class CharacterStatsWindow::StatGroup & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    // Typedef        : StatsMap
    // Typedef        : StatsIdMap
    // Typedef        : StatsGroupList
    // no_addr void CharacterStatsWindow(const class CharacterStatsWindow & _a1);// public missing arg names
    CharacterStatsWindow(const hand& charHandle);// protected RVA = 0x8B8F50
    CharacterStatsWindow* _CONSTRUCTOR(const hand& charHandle);// protected RVA = 0x8B8F50
    virtual ~CharacterStatsWindow();// protected RVA = 0x8A8F20 vtable offset = 0x0
    void _DESTRUCTOR();// protected RVA = 0x8A8F20 vtable offset = 0x0
    virtual void clear() override;// public RVA = 0x8C0800 vtable offset = 0x0
    void _NV_clear();// public RVA = 0x8C0800 vtable offset = 0x0
    virtual void create();// public RVA = 0x8C0810 vtable offset = 0x68
    void _NV_create();// public RVA = 0x8C0810 vtable offset = 0x68
    virtual void update() override;// public RVA = 0x8A9D20 vtable offset = 0x0
    void _NV_update();// public RVA = 0x8A9D20 vtable offset = 0x0
    virtual void show(bool on) override;// public RVA = 0x8A8B00 vtable offset = 0x0
    void _NV_show(bool on);// public RVA = 0x8A8B00 vtable offset = 0x0
    virtual bool isVisible() const override;// public RVA = 0x8A8B70 vtable offset = 0x0
    bool _NV_isVisible() const;// public RVA = 0x8A8B70 vtable offset = 0x0
    virtual void autoChangeSelectedObject(const hand& obj) override;// public RVA = 0x8A9B40 vtable offset = 0x0
    void _NV_autoChangeSelectedObject(const hand& obj);// public RVA = 0x8A9B40 vtable offset = 0x0
    void setAutoUpdateMode(bool on);// public RVA = 0x8A8B90
    void setupStats();// private RVA = 0x8B8740
    void updateStats();// private RVA = 0x8A9260
    void clearDesctiption();// private RVA = 0x8A8C80
    int sumStats() const;// private RVA = 0x8A8BA0
    void closeWindow(MyGUI::Window* _sender, const std::string& _name);// private RVA = 0x8A8CF0
    void statMouseOver(MyGUI::Widget* _sender, int _a2, int _a3);// private RVA = 0x8B7AC0
    DatapanelGUI* getStatsPanel(CharacterStatsWindow::StatGroup::Group group);// private RVA = 0x8A8B20
    Character* character; // 0xD0 Member
    int statsSum; // 0xD8 Member
    StatsEnumerated statProgress; // 0xDC Member
    std::string currentStatIdDescription; // 0xE0 Member
    DatapanelGUI* attributesDatapanel; // 0x108 Member
    DatapanelGUI* skills1Datapanel; // 0x110 Member
    DatapanelGUI* skills2Datapanel; // 0x118 Member
    DatapanelGUI* skills3Datapanel; // 0x120 Member
    DatapanelGUI* skills4Datapanel; // 0x128 Member
    DatapanelGUI* statsDatapanel; // 0x130 Member
    DatapanelGUI* description1Datapanel; // 0x138 Member
    DatapanelGUI* description2Datapanel; // 0x140 Member
    bool autoUpdateMode; // 0x148 Member
    static void addStat(StatsEnumerated type, const std::string& id, const std::string& name, bool active, const std::string& description);// private RVA = 0x8A9FD0
    static CharacterStatsWindow::Stat* getStat(StatsEnumerated type);// private RVA = 0x8AA140
    static CharacterStatsWindow::Stat* getStatById(const std::string& id);// private RVA = 0x8AA170
    static void BuildStats();// private RVA = 0x8AA1A0
    static bool StatsBuilt; // RVA = 0x2131D30 Static Member
    static std::map<StatsEnumerated, CharacterStatsWindow::Stat, std::less<StatsEnumerated>, Ogre::STLAllocator<std::pair<StatsEnumerated const, CharacterStatsWindow::Stat>, Ogre::GeneralAllocPolicy > > Stats; // RVA = 0x2131D50 Static Member
    static std::map<std::string, enum StatsEnumerated, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, enum StatsEnumerated>, Ogre::GeneralAllocPolicy > > StatsById; // RVA = 0x2131D78 Static Member
    static Ogre::vector<CharacterStatsWindow::Stat*>::type Attributes; // RVA = 0x201CE28 Static Member
    static std::vector<std::pair<CharacterStatsWindow::StatGroup, Ogre::vector<CharacterStatsWindow::Stat*>::type >, Ogre::STLAllocator<std::pair<CharacterStatsWindow::StatGroup, Ogre::vector<CharacterStatsWindow::Stat*>::type >, Ogre::GeneralAllocPolicy > > GeneralSkills; // RVA = 0x201CE48 Static Member
    static std::vector<std::pair<CharacterStatsWindow::StatGroup, Ogre::vector<CharacterStatsWindow::Stat*>::type >, Ogre::STLAllocator<std::pair<CharacterStatsWindow::StatGroup, Ogre::vector<CharacterStatsWindow::Stat*>::type >, Ogre::GeneralAllocPolicy > > DerivedStats; // RVA = 0x201CE68 Static Member
    // no_addr class CharacterStatsWindow & operator=(const class CharacterStatsWindow & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// protected vtable offset = 0x0 missing arg names
};