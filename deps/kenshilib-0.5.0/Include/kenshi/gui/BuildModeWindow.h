#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <ogre/OgrePrerequisites.h>

class GameData;
class PlayerBuildMode;
class Research;
class DatapanelGUI;

class BuildModeWindow : public wraps::BaseLayout
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    class BuildingGroup
    {
    public:
        std::string name; // 0x0 Member
        Ogre::vector<GameData*>::type buildings; // 0x28 Member
        bool operator<(const BuildModeWindow::BuildingGroup& group) const;// public RVA = 0x4F1C70
        BuildingGroup(const BuildModeWindow::BuildingGroup& __that);// public RVA = 0x4F43D0
        BuildingGroup* _CONSTRUCTOR(const BuildModeWindow::BuildingGroup& __that);// public RVA = 0x4F43D0
        BuildingGroup();// public RVA = 0x4F3520
        BuildingGroup* _CONSTRUCTOR();// public RVA = 0x4F3520
        ~BuildingGroup();// public RVA = 0x4F3560
        void _DESTRUCTOR();// public RVA = 0x4F3560
        BuildModeWindow::BuildingGroup& operator=(const BuildModeWindow::BuildingGroup& __that);// public RVA = 0x4F45D0
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class BuildingCategory
    {
    public:
        std::string name; // 0x0 Member
        Ogre::vector<BuildModeWindow::BuildingGroup>::type buildingGroups; // 0x28 Member
        bool operator<(const BuildModeWindow::BuildingCategory& bc) const;// public RVA = 0x4F1C90
        BuildingCategory(const BuildModeWindow::BuildingCategory& __that);// public RVA = 0x4F91D0
        BuildingCategory* _CONSTRUCTOR(const BuildModeWindow::BuildingCategory& __that);// public RVA = 0x4F91D0
        BuildingCategory();// public RVA = 0x4F98B0
        BuildingCategory* _CONSTRUCTOR();// public RVA = 0x4F98B0
        ~BuildingCategory();// public RVA = 0x4F98F0
        void _DESTRUCTOR();// public RVA = 0x4F98F0
        BuildModeWindow::BuildingCategory& operator=(const BuildModeWindow::BuildingCategory& __that);// public RVA = 0x4F9D40
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    // Typedef        : BuildingInfo
    // Typedef        : BuildingInfoVector
    // Typedef        : BuildingGroupVector
    // Typedef        : BuildingCategoryVector
    // Typedef        : BuildingCategoryPtrVector
    // no_addr void BuildModeWindow(const class BuildModeWindow & _a1);// public missing arg names
    BuildModeWindow(PlayerBuildMode* playerBuildMode, bool levelEditorMode);// public RVA = 0x4E4A90
    BuildModeWindow* _CONSTRUCTOR(PlayerBuildMode* playerBuildMode, bool levelEditorMode);// public RVA = 0x4E4A90
    virtual ~BuildModeWindow();// public RVA = 0x4D2F00 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x4D2F00 vtable offset = 0x0
    void setMessage(const std::string& message);// public RVA = 0x4D7610
    MyGUI::Widget* getBuildingListWidget() const;// public RVA = 0x970330
    void setVisible(bool v);// public RVA = 0x4EB680
    void setupData();// private RVA = 0x4E3D00
    void listCategories();// private RVA = 0x4D2290
    void listBuildingGroups();// private RVA = 0x4D5670
    void updateBuildingUI();// private RVA = 0x4E18C0
    void categorySelected(MyGUI::ListBox* _sender, unsigned __int64 _index);// private RVA = 0x4D5870
    void buildingSelected(MyGUI::ListBox* _sender, unsigned __int64 _index);// private RVA = 0x4E3A90
    void build();// private RVA = 0x4E1B30
    void confirm(MyGUI::Widget* _sender);// private RVA = 0x4DAE20
    void undo(MyGUI::Widget* _sender);// private RVA = 0x4E1B70
    void close(MyGUI::Widget* _sender);// private RVA = 0x4D3D20
    void buildingTypePrev(MyGUI::Widget* _sender);// private RVA = 0x4E1D30
    void buildingTypeNext(MyGUI::Widget* _sender);// private RVA = 0x4E1D80
    void showBuildingStats();// private RVA = 0x4DECD0
    void update();// private RVA = 0x4E1DD0
    static bool compareBuildMaterials(const GameData* b1, const GameData* b2);// private RVA = 0x4D58E0
    static Ogre::vector<BuildModeWindow::BuildingCategory>::type BuildingsCategories; // RVA = 0x1DD09F8 Static Member
    void changeFloorButtonUp(MyGUI::Widget* _sender);// private RVA = 0x4D88F0
    void changeFloorButtonDown(MyGUI::Widget* _sender);// private RVA = 0x4D8B50
    void changeCurrentIndex(int index);// private RVA = 0x4E1BC0
    PlayerBuildMode* playerBuildMode; // 0xA0 Member
    bool levelEditorMode; // 0xA8 Member
    Ogre::vector<BuildModeWindow::BuildingCategory*>::type playerCategories; // 0xB0 Member
    Research* playerResearch; // 0xD0 Member
    BuildModeWindow::BuildingCategory* currentBuildingCategory; // 0xD8 Member
    BuildModeWindow::BuildingGroup currentBuildingGroup; // 0xE0 Member
    GameData* currentBuildingInfo; // 0x128 Member
    short currentBuildingIndex; // 0x130 Member
    short switchBuildingIndex; // 0x132 Member
    DatapanelGUI* statsDataPanel; // 0x138 Member
    MyGUI::Button* confirmButton; // 0x140 Member
    MyGUI::Button* undoButton; // 0x148 Member
    MyGUI::Button* closeButton; // 0x150 Member
    MyGUI::ListBox* categoriesList; // 0x158 Member
    MyGUI::ListBox* buildingsList; // 0x160 Member
    MyGUI::TextBox* buildingTxt; // 0x168 Member
    MyGUI::Button* buildingTypePrevButton; // 0x170 Member
    MyGUI::Button* buildingTypeNextButton; // 0x178 Member
    MyGUI::ImageBox* buildingImageBox; // 0x180 Member
    MyGUI::Widget* statsPanel; // 0x188 Member
    MyGUI::TextBox* descriptionTxt; // 0x190 Member
    MyGUI::EditBox* messageTextBox; // 0x198 Member
    MyGUI::Button* floorDownButton; // 0x1A0 Member
    MyGUI::Button* floorUpButton; // 0x1A8 Member
    MyGUI::TextBox* floorText; // 0x1B0 Member
    // no_addr class BuildModeWindow & operator=(const class BuildModeWindow & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};