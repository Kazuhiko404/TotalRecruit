#pragma once

#include <mygui/common/itembox/BaseItemBox.h>
#include <mygui/common/itembox/BaseCellView.h>
#include <kenshi/util/hand.h>
#include <ogre/OgreMemoryAllocatorConfig.h>

class ManagementScreen;
class Character;

// TODO move?
class PortraitData : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    enum State
    {
        NORMAL,
        SELECTED,
        HURT,
        DOWN = 0x4,
        COMBAT = 0x8,
        SLAVE = 0x10,
        PRISON = 0x20,
        EATEN = 0x40,
        HUNGER = 0x80
    };

    Character* getCharacter() const;// public RVA = 0x412610
    const hand& getHandle() const;// public RVA = 0x411990
    bool update();// public RVA = 0x414640
    void forceUpdate();// public RVA = 0x411A00
    std::string name; // 0x0 Member
    char flashing; // 0x28 Member
    PortraitData::State state; // 0x2C Member
    bool reloadPortrait; // 0x30 Member
    bool isSelected() const;// public RVA = 0x4119A0
    // no_addr bool isHurt();// public
    bool isDown() const;// public RVA = 0x4119B0
    // no_addr bool isInCombat();// public
    // no_addr bool isSlave();// public
    // no_addr bool isInPrison();// public
    // no_addr bool isHungry();// public
    std::string getBackgroundImageName() const;// public RVA = 0x412260
    std::string getBackOverlayImageName() const;// public RVA = 0x4122C0
    std::string getFrontOverlayImageName() const;// public RVA = 0x412350
    // no_addr void PortraitData(const class PortraitData & _a1);// public missing arg names
    PortraitData(const hand& characterHandle);// private RVA = 0x411F00
    PortraitData* _CONSTRUCTOR(const hand& characterHandle);// private RVA = 0x411F00
    ~PortraitData();// private RVA = 0x411F70
    void _DESTRUCTOR();// private RVA = 0x411F70
    hand characterHandle; // 0x38 Member
    bool updateRequested; // 0x58 Member
    // no_addr class PortraitData & operator=(const class PortraitData & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// private missing arg names
};

class Faction;

class SquadManagementScreen
{
public:
    // no_addr void SquadManagementScreen(const class SquadManagementScreen & _a1);// public missing arg names
    SquadManagementScreen(ManagementScreen* screen);// public RVA = 0x49B040
    SquadManagementScreen* _CONSTRUCTOR(ManagementScreen* screen);// public RVA = 0x49B040
    ~SquadManagementScreen();// public RVA = 0x48D700
    void _DESTRUCTOR();// public RVA = 0x48D700
    bool getVisible() const;// public RVA = 0x48B7F0
    void update();// public RVA = 0x498900
    void reset();// public RVA = 0x48CD00
    void refreshSquads();// public RVA = 0x48ED40
    class PortraitSquadCellView : public wraps::BaseCellView<PortraitData*>
    {
    public:
        // wraps::BaseCellView<PortraitData *> offset = 0x0, length = 0xA0
        // no_addr void PortraitSquadCellView(const class SquadManagementScreen::PortraitSquadCellView & _a1);// public missing arg names
        PortraitSquadCellView(MyGUI::Widget* parent);// public RVA = 0x493600
        PortraitSquadCellView* _CONSTRUCTOR(MyGUI::Widget* parent);// public RVA = 0x493600
        virtual ~PortraitSquadCellView();// public RVA = 0x4C5910 vtable offset = 0x0
        void _DESTRUCTOR();// public RVA = 0x4C5910 vtable offset = 0x0
        void update(const MyGUI::IBDrawItemInfo& _info, PortraitData* _data);// public RVA = 0x48CA60
        static void getCellDimension(MyGUI::Widget* _sender, MyGUI::types::TCoord<int>& _coord, bool _drop);// public RVA = 0x48C460
        void resize();// private RVA = 0x48B910
        void setName(const std::string& newName);// private RVA = 0x48D830
        static int Size; // RVA = 0x1DAA04C Static Member
        hand characterHandle; // 0xA0 Member
        std::string name; // 0xC0 Member
        MyGUI::Button* border; // 0xE8 Member
        MyGUI::ImageBox* imgBoxPortrait; // 0xF0 Member
        MyGUI::TextBox* txtName; // 0xF8 Member
        // no_addr class SquadManagementScreen::PortraitSquadCellView & operator=(const class SquadManagementScreen::PortraitSquadCellView & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
    };
    class SquadData;
    class SquadCellView;
    class PortraitSquadItemBox : public wraps::BaseItemBox<SquadManagementScreen::PortraitSquadCellView>
    {
    public:
        // wraps::BaseItemBox<SquadManagementScreen::PortraitSquadCellView> offset = 0x0, length = 0xF8
        // no_addr void PortraitSquadItemBox(const class SquadManagementScreen::PortraitSquadItemBox & _a1);// public missing arg names
        PortraitSquadItemBox(MyGUI::Widget* parent);// public RVA = 0x49AD90
        PortraitSquadItemBox* _CONSTRUCTOR(MyGUI::Widget* parent);// public RVA = 0x49AD90
        virtual ~PortraitSquadItemBox();// public RVA = 0x4C7B70 vtable offset = 0x0
        void _DESTRUCTOR();// public RVA = 0x4C7B70 vtable offset = 0x0
        int getItemCount() const;// public RVA = 0x48C440
        void update();// public RVA = 0x498CF0
        SquadManagementScreen::SquadData* squad; // 0xF8 Member
        SquadManagementScreen::SquadCellView* squadView; // 0x100 Member
        // no_addr class SquadManagementScreen::PortraitSquadItemBox & operator=(const class SquadManagementScreen::PortraitSquadItemBox & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
    };
    class SquadCellView : public wraps::BaseCellView<SquadManagementScreen::SquadData*>
    {
    public:
        // wraps::BaseCellView<SquadManagementScreen::SquadData *> offset = 0x0, length = 0xA0
        // no_addr void SquadCellView(const class SquadManagementScreen::SquadCellView & _a1);// public missing arg names
        SquadCellView(MyGUI::Widget* parent);// public RVA = 0x4A0E40
        SquadCellView* _CONSTRUCTOR(MyGUI::Widget* parent);// public RVA = 0x4A0E40
        virtual ~SquadCellView();// public RVA = 0x4C9DC0 vtable offset = 0x0
        void _DESTRUCTOR();// public RVA = 0x4C9DC0 vtable offset = 0x0
        void update(const MyGUI::IBDrawItemInfo& _info, SquadManagementScreen::SquadData* _data);// public RVA = 0x49ACB0
        void updateSquadSize();// public RVA = 0x498AB0
        static void getCellDimension(MyGUI::Widget* _sender, MyGUI::types::TCoord<int>& _coord, bool _drop);// public RVA = 0x48C400
        static int Width; // RVA = 0x1DAA044 Static Member
        static int Height; // RVA = 0x1DAA048 Static Member
        void onNameChanged(MyGUI::EditBox* _a1);// private RVA = 0x48D780 missing arg names
        void onRemove(MyGUI::Widget* _a1);// private RVA = 0x4A0CE0 missing arg names
        SquadManagementScreen::PortraitSquadItemBox* portraitsBox; // 0xA0 Member
        MyGUI::EditBox* txtName; // 0xA8 Member
        MyGUI::TextBox* txtSquadSize; // 0xB0 Member
        SquadManagementScreen::SquadData* squad; // 0xB8 Member
        // no_addr class SquadManagementScreen::SquadCellView & operator=(const class SquadManagementScreen::SquadCellView & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
    };
    class SquadItemBox : public wraps::BaseItemBox<SquadManagementScreen::SquadCellView>
    {
    public:
        // wraps::BaseItemBox<SquadManagementScreen::SquadCellView> offset = 0x0, length = 0xF8
        // no_addr void SquadItemBox(const class SquadManagementScreen::SquadItemBox & _a1);// public missing arg names
        SquadItemBox(MyGUI::Widget* parent);// public RVA = 0x4A14B0
        SquadItemBox* _CONSTRUCTOR(MyGUI::Widget* parent);// public RVA = 0x4A14B0
        virtual ~SquadItemBox();// public RVA = 0x4CA4F0 vtable offset = 0x0
        void _DESTRUCTOR();// public RVA = 0x4CA4F0 vtable offset = 0x0
        void setCellSize(int width, int height);// public RVA = 0x48B8F0
        int getItemCount() const;// public RVA = 0x48C420
        // no_addr class SquadManagementScreen::SquadItemBox & operator=(const class SquadManagementScreen::SquadItemBox & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
    };
    class SquadData
    {
    public:
        SquadData(ActivePlatoon* platoon);// public RVA = 0x48B8B0
        SquadData* _CONSTRUCTOR(ActivePlatoon* platoon);// public RVA = 0x48B8B0
        ~SquadData();// public RVA = 0x48B8C0
        void _DESTRUCTOR();// public RVA = 0x48B8C0
        void setName(const std::string& name);// public RVA = 0x48D760
        const std::string& getName() const;// public RVA = 0x48B8D0
        ActivePlatoon* platoon; // 0x0 Member
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    void notifyStartDropSquad(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool& _result);// protected RVA = 0x48B800
    void notifyRequestDropSquad(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool& _result);// protected RVA = 0x48B820
    void notifyEndDropSquad(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool _result);// protected RVA = 0x493530
    void notifyStartDropPortrait(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool& _result);// protected RVA = 0x48B880
    void notifyRequestDropPortrait(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool& _result);// protected RVA = 0x48C370
    void notifyEndDropPortrait(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool _result);// protected RVA = 0x49A8D0
    void onAddSquad(MyGUI::Widget* _a1);// protected RVA = 0x4913D0 missing arg names
    void removeSquad(SquadManagementScreen::SquadData* squad);// protected RVA = 0x4915A0
    SquadManagementScreen::SquadData* getSquad(ActivePlatoon* platoon);// protected RVA = 0x48E380
    void dismissCharacter(int result);// protected RVA = 0x48F2B0
    MyGUI::Widget* mainWidget; // 0x0 Member
    MyGUI::Button* btnAddSquad; // 0x8 Member
    MyGUI::TextBox* txtFactionSize; // 0x10 Member
    int factionSize; // 0x18 Member
    SquadManagementScreen::SquadItemBox* panelSquads; // 0x20 Member
    SquadManagementScreen::PortraitSquadItemBox* panelDismiss; // 0x28 Member
    Character* dismissChar; // 0x30 Member
    Faction* faction; // 0x38 Member
    std::map<ActivePlatoon*, SquadManagementScreen::SquadData*, std::less<ActivePlatoon*>, Ogre::STLAllocator<std::pair<ActivePlatoon* const, SquadManagementScreen::SquadData*>, Ogre::GeneralAllocPolicy > > squads; // 0x40 Member
    // no_addr class SquadManagementScreen & operator=(const class SquadManagementScreen & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};