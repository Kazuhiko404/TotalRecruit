#pragma once

#include <ogre/OgreRay.h>
#include <mygui/MyGUI_Widget.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>

class RootObject;
class Building;
class GameData;
class DataPanelLine;
class DatapanelGUI;
class LevelEditorPanel;
class InteriorModeButtonWindow;
class MapFeature;
class Road;
class FoliageEraser;
class FogEditor;
class RealWorldEditableImage;

class LevelEditor : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    // no_addr void LevelEditor(const class LevelEditor & _a1);// public missing arg names
    LevelEditor();// public RVA = 0x808B70
    LevelEditor* _CONSTRUCTOR();// public RVA = 0x808B70
    ~LevelEditor();// public RVA = 0x808CB0
    void _DESTRUCTOR();// public RVA = 0x808CB0
    // no_addr void show();// public
    void close();// public RVA = 0x77C220
    void update();// public RVA = 0x777770
    bool updateGizmo();// public RVA = 0x76B330
    RootObject* hitFurnitureOrItemGroup(const Ogre::Ray& ray, Ogre::Vector3& point, int floor, float max, bool includeNodes);// public RVA = 0x773AA0
    static std::pair<bool, float> hitOgreMesh(const Ogre::Ray& _ray, float max, Ogre::MovableObject* e);// public RVA = 0x773340
    static std::pair<bool, float> hitBuildingMesh(const Ogre::Ray& ray, float max, Building* b);// public RVA = 0x773950
    void levelEditMode(bool on);// public RVA = 0x77AC80
    bool levelEditModeOn; // 0x0 Member
    bool isInteriorEditMode();// public RVA = 0x4EB750
    int getActiveModIndex() const;// public RVA = 0x76C710
    std::string getActiveMod() const;// public RVA = 0x76C630
    bool isBaseActive() const;// public RVA = 0x76B110
    void setEditSquad(GameData* squad);// public RVA = 0x76EF60
    // no_addr class MyGUI::Window * createGameDataSelectionWindow();// public
    void saveMod(MyGUI::Widget* _sender);// public RVA = 0x776AD0
    void regenerate(MyGUI::Widget* _sender);// public RVA = 0x76B160
    void repath(DataPanelLine* _a1);// public RVA = 0x76B180 missing arg names
    void toggleSeedMode(DataPanelLine* line);// public RVA = 0x76F3D0
    void clearSeedPoints(DataPanelLine* _a1);// public RVA = 0x76F410 missing arg names
    void setSeedMode(int mode);// public RVA = 0x76DB10
    void toggleRoadEditor(DatapanelGUI* _a1, int tab);// public RVA = 0x76F460
    void toggleFoliageRemoval(DataPanelLine* line);// public RVA = 0x76F650
    void closeNavmeshWindow(MyGUI::Window* _a1, const std::string& _a2);// public RVA = 0x76F6C0 missing arg names
    void fixBuildings(MyGUI::Widget* _a1);// public RVA = 0x771A50 missing arg names
    bool clearChanges(RootObject* object) const;// public RVA = 0x7772C0
    int deleteDuplicateBuildings() const;// public RVA = 0x770F40
    int fixMountedBuildings() const;// public RVA = 0x76D690
    void deleteObject(const hand& h);// public RVA = 0x776690
    int deleteAll(GameData* data, bool confirmed);// public RVA = 0x76C320
    void adjustAltitudeSpeed(float s);// public RVA = 0x76DA10
    int fixInvalidInteriorFurnitureStates() const;// public RVA = 0x7715A0
    bool fixBuildingFloorState(Building* b, Building* parent) const;// public RVA = 0x770B90
    LevelEditorPanel* panel; // 0x8 Member
    InteriorModeButtonWindow* interiorModeWindow; // 0x10 Member
    void setSelectedObject(RootObject* obj);// public RVA = 0x775E90
    int selectedLocked; // 0x18 Member
    hand selectedObject; // 0x20 Member
    MapFeature* selectedFeature; // 0x40 Member
    int seedMode; // 0x48 Member
    int roadMode; // 0x4C Member
    lektor<Road*> selectedRoads; // 0x50 Member
    int roadIndex; // 0x68 Member
    Ogre::SceneNode* roadMarker; // 0x70 Member
    Ogre::ManualObject* roadMakerMO; // 0x78 Member
    FoliageEraser* foliageEraser; // 0x80 Member
    FogEditor* fogEditor; // 0x88 Member
    class GamedataSelectionList
    {
    public:
        // VTable         : (none)
        // no_addr void GamedataSelectionList(const class LevelEditor::GamedataSelectionList & _a1);// public missing arg names
        GamedataSelectionList(LevelEditor* ed, itemType type, float top, float left, float w, float h);// public RVA = 0x7785E0
        GamedataSelectionList* _CONSTRUCTOR(LevelEditor* ed, itemType type, float top, float left, float w, float h);// public RVA = 0x7785E0
        ~GamedataSelectionList();// public RVA = 0x7811F0
        void _DESTRUCTOR();// public RVA = 0x7811F0
        void deselectItem();// public RVA = 0x76F710
        bool selectItem(GameData* item);// public RVA = 0x76DC10
        GameData* getSelectedItem() const;// public RVA = 0x76E050
        MyGUI::Window* win; // 0x8 Member
        MyGUI::ListBox* list; // 0x10 Member
        lektor<GameData*> items; // 0x18 Member
        LevelEditor* levelEditor; // 0x30 Member
        unsigned __int64 lastItem; // 0x38 Member
        void listItemSelected(MyGUI::ListBox* _sender, unsigned __int64 _index);// protected RVA = 0x777100
        void changeFilter(MyGUI::EditBox* e);// protected RVA = 0x76D260
        virtual void itemSelected(GameData* _a1) = 0;// protected vtable offset = 0x0 missing arg names
        virtual std::string formatItem(GameData* o);// protected RVA = 0x76C780 vtable offset = 0x8
        std::string _NV_formatItem(GameData* o);// protected RVA = 0x76C780 vtable offset = 0x8
        // no_addr class LevelEditor::GamedataSelectionList & operator=(const class LevelEditor::GamedataSelectionList & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class FactionListWindow : public LevelEditor::GamedataSelectionList
    {
    public:
        // LevelEditor::GamedataSelectionList offset = 0x0, length = 0x40
        // no_addr void FactionListWindow(const class LevelEditor::FactionListWindow & _a1);// public missing arg names
        FactionListWindow(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x784720
        FactionListWindow* _CONSTRUCTOR(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x784720
        virtual void itemSelected(GameData* item) override;// protected RVA = 0x774350 vtable offset = 0x0
        void _NV_itemSelected(GameData* item);// protected RVA = 0x774350 vtable offset = 0x0
        ~FactionListWindow();// public RVA = 0x781360
        void _DESTRUCTOR();// public RVA = 0x781360
        // no_addr class LevelEditor::FactionListWindow & operator=(const class LevelEditor::FactionListWindow & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class SquadListWindow : public LevelEditor::GamedataSelectionList
    {
    public:
        // LevelEditor::GamedataSelectionList offset = 0x0, length = 0x40
        // no_addr void SquadListWindow(const class LevelEditor::SquadListWindow & _a1);// public missing arg names
        SquadListWindow(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x784780
        SquadListWindow* _CONSTRUCTOR(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x784780
        void refresh(GameData* faction);// public RVA = 0x774130
        virtual void itemSelected(GameData* item) override;// protected RVA = 0x76F7F0 vtable offset = 0x0
        void _NV_itemSelected(GameData* item);// protected RVA = 0x76F7F0 vtable offset = 0x0
        ~SquadListWindow();// public RVA = 0x7813E0
        void _DESTRUCTOR();// public RVA = 0x7813E0
        // no_addr class LevelEditor::SquadListWindow & operator=(const class LevelEditor::SquadListWindow & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class NpcListWindow : public LevelEditor::GamedataSelectionList
    {
    public:
        // LevelEditor::GamedataSelectionList offset = 0x0, length = 0x40
        // no_addr void NpcListWindow(const class LevelEditor::NpcListWindow & _a1);// public missing arg names
        NpcListWindow(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x7847E0
        NpcListWindow* _CONSTRUCTOR(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x7847E0
        virtual ~NpcListWindow();// public RVA = 0x76B690 vtable offset = 0x10
        void _DESTRUCTOR();// public RVA = 0x76B690 vtable offset = 0x10
        virtual void itemSelected(GameData* item) override;// protected RVA = 0x76B2A0 vtable offset = 0x0
        void _NV_itemSelected(GameData* item);// protected RVA = 0x76B2A0 vtable offset = 0x0
        // no_addr class LevelEditor::NpcListWindow & operator=(const class LevelEditor::NpcListWindow & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x10 missing arg names
    };
    class TownListWindow : public LevelEditor::NpcListWindow
    {
    public:
        // LevelEditor::NpcListWindow offset = 0x0, length = 0x40
        // no_addr void TownListWindow(const class LevelEditor::TownListWindow & _a1);// public missing arg names
        TownListWindow(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x7848E0
        TownListWindow* _CONSTRUCTOR(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x7848E0
        virtual std::string formatItem(GameData* o) override;// public RVA = 0x770810 vtable offset = 0x0
        std::string _NV_formatItem(GameData* o);// public RVA = 0x770810 vtable offset = 0x0
        virtual ~TownListWindow();// public RVA = 0x784960 vtable offset = 0x0
        void _DESTRUCTOR();// public RVA = 0x784960 vtable offset = 0x0
        // no_addr class LevelEditor::TownListWindow & operator=(const class LevelEditor::TownListWindow & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x10 missing arg names
    };
    class ItemListWindow : public LevelEditor::GamedataSelectionList
    {
    public:
        // LevelEditor::GamedataSelectionList offset = 0x0, length = 0x40
        // no_addr void ItemListWindow(const class LevelEditor::ItemListWindow & _a1);// public missing arg names
        ItemListWindow(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x784A80
        ItemListWindow* _CONSTRUCTOR(LevelEditor* ed, itemType t, float top, float left, float w, float h);// public RVA = 0x784A80
        virtual ~ItemListWindow();// public RVA = 0x76B700 vtable offset = 0x10
        void _DESTRUCTOR();// public RVA = 0x76B700 vtable offset = 0x10
        virtual void itemSelected(GameData* item) override;// protected RVA = 0x76B310 vtable offset = 0x0
        void _NV_itemSelected(GameData* item);// protected RVA = 0x76B310 vtable offset = 0x0
        // no_addr class LevelEditor::ItemListWindow & operator=(const class LevelEditor::ItemListWindow & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x10 missing arg names
    };
    LevelEditor::TownListWindow* townListWindow; // 0x90 Member
    LevelEditor::FactionListWindow* factionListWindow; // 0x98 Member
    LevelEditor::SquadListWindow* squadListWindow; // 0xA0 Member
    LevelEditor::NpcListWindow* npcListWindow; // 0xA8 Member
    LevelEditor::NpcListWindow* featureListWindow; // 0xB0 Member
    LevelEditor::ItemListWindow* itemListWindow; // 0xB8 Member
    DatapanelGUI* navmeshWindow; // 0xC0 Member
    // no_addr void destroy(class GameData * _a1);// private missing arg names
    MyGUI::Window* editMapTitleWindow; // 0xC8 Member
    MyGUI::TextBox* factionTxt; // 0xD0 Member
    MyGUI::TextBox* altitudeTxt; // 0xD8 Member
    MyGUI::ComboBox* activeModTxt; // 0xE0 Member
    void factionButton(MyGUI::Widget* _sender);// private RVA = 0x778A70
    void npcButton(MyGUI::Widget* _sender);// private RVA = 0x778CD0
    void townButton(MyGUI::Widget* _sender);// private RVA = 0x778BB0
    void squadButton(MyGUI::Widget* _sender);// private RVA = 0x76B1F0
    void featureButton(MyGUI::Widget* _sender);// private RVA = 0x778DE0
    void fogButton(MyGUI::Widget* _sender);// private RVA = 0x76B240
    void itemsButton(MyGUI::Widget* _sender);// private RVA = 0x778ED0
    void buildingsButton(MyGUI::Widget* _sender);// private RVA = 0x776FC0
    void navmeshButton(MyGUI::Widget* _a1);// private RVA = 0x772270 missing arg names
    void exitLevelEditMode(MyGUI::Widget* _sender);// private RVA = 0x76B550
    void activeModChanged(MyGUI::ComboBox* _sender, unsigned __int64 _a2);// private RVA = 0x775620
    void updateRoadEditor();// private RVA = 0x76F850
    void setRoadWidth(DataPanelLine* l);// private RVA = 0x76C490
    void setRoadHidden(DataPanelLine* l);// private RVA = 0x76BA90
    void drawRoads(DataPanelLine* _a1);// private RVA = 0x771EF0 missing arg names
    void eraseRoads(DataPanelLine* _a1);// private RVA = 0x771FC0 missing arg names
    void drawAllRoads(DataPanelLine* _a1);// private RVA = 0x772090 missing arg names
    void projectRoads(DataPanelLine* _a1);// private RVA = 0x76C7D0 missing arg names
    void validateRoads(DataPanelLine* _a1);// private RVA = 0x76E8B0 missing arg names
    bool paintRoads(RealWorldEditableImage* image, const lektor<Road*>& list, bool draw);// private RVA = 0x76BB10
    bool paintFoliageEraser(RealWorldEditableImage* image, const FoliageEraser* e, bool draw);// private RVA = 0x76B3C0
    void selectRoad(Road* r, bool clear);// private RVA = 0x76E510
    void createRoadMarker();// private RVA = 0x76C9D0
    void destroyRoadMaker();// private RVA = 0x76B360
    void updateFoliageRemoval();// private RVA = 0x76B770
    void updateSeedEditor();// private RVA = 0x76E0A0
    // no_addr class LevelEditor & operator=(const class LevelEditor & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};