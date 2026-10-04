#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <ogre/OgreMemoryAllocatorConfig.h>

class Faction;
class ManagementScreen;
class DatapanelGUI;

class FactionsScreen
{
public:
    class FactionRelationsLine : public wraps::BaseLayout
    {
    public:
        // wraps::BaseLayout offset = 0x0, length = 0xA0
        // no_addr void FactionRelationsLine(const class FactionsScreen::FactionRelationsLine & _a1);// public missing arg names
        FactionRelationsLine(Faction* faction);// public RVA = 0x4917E0
        FactionRelationsLine* _CONSTRUCTOR(Faction* faction);// public RVA = 0x4917E0
        void attachToWidget(MyGUI::Widget* widget);// public RVA = 0x48D990
        void update();// public RVA = 0x498EB0
        Faction* faction; // 0xA0 Member
        float value; // 0xA8 Member
        MyGUI::TextBox* valueText; // 0xB0 Member
        MyGUI::Widget* leftBar; // 0xB8 Member
        MyGUI::Widget* rightBar; // 0xC0 Member
        static MyGUI::types::TCoord<float> BarLeftCoords; // RVA = 0x212C430 Static Member
        static MyGUI::types::TCoord<float> BarRightCoords; // RVA = 0x212C440 Static Member
        struct LessSort
        {
            bool operator()(const FactionsScreen::FactionRelationsLine* l1, const FactionsScreen::FactionRelationsLine* l2);// public RVA = 0x48B940
        };
        virtual ~FactionRelationsLine();// public RVA = 0x4C4BA0 vtable offset = 0x0
        void _DESTRUCTOR();// public RVA = 0x4C4BA0 vtable offset = 0x0
        // no_addr class FactionsScreen::FactionRelationsLine & operator=(const class FactionsScreen::FactionRelationsLine & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
    };
    // no_addr void FactionsScreen(const class FactionsScreen & _a1);// public missing arg names
    FactionsScreen(ManagementScreen* screen);// public RVA = 0x493860
    FactionsScreen* _CONSTRUCTOR(ManagementScreen* screen);// public RVA = 0x493860
    ~FactionsScreen();// public RVA = 0x48DA20
    void _DESTRUCTOR();// public RVA = 0x48DA20
    bool getVisible() const;// public RVA = 0x48B970
    void clear();// public RVA = 0x48CE80
    void update();// public RVA = 0x4991E0
    void refresh();// public RVA = 0x499920
    void updateInfo();// protected RVA = 0x493FA0
    void notifyEditTextChange(MyGUI::EditBox* _sender);// protected RVA = 0x491C50
    void setFocus(MyGUI::Widget* _a1, MyGUI::Widget* _a2);// protected RVA = 0x4A6D80 missing arg names
    void loseFocus(MyGUI::Widget* _a1, MyGUI::Widget* _a2);// protected RVA = 0x492050 missing arg names
    void factionLineSelected(MyGUI::Widget* _sender);// protected RVA = 0x4942D0
    void notifyMouseWheel(MyGUI::Widget* _sender, int _rel);// protected RVA = 0x48B980
    MyGUI::Widget* mainWidget; // 0x0 Member
    Faction* selectedFaction; // 0x8 Member
    std::map<Faction*, FactionsScreen::FactionRelationsLine*, std::less<Faction*>, Ogre::STLAllocator<std::pair<Faction* const, FactionsScreen::FactionRelationsLine*>, Ogre::GeneralAllocPolicy > > lines; // 0x10 Member
    MyGUI::ScrollView* scrollList; // 0x38 Member
    int scrollListItemTop; // 0x40 Member
    int scrollListItemWidth; // 0x44 Member
    MyGUI::EditBox* nameText; // 0x48 Member
    DatapanelGUI* infoPanel; // 0x50 Member
    float updateTimer; // 0x58 Member
    // no_addr class FactionsScreen & operator=(const class FactionsScreen & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};