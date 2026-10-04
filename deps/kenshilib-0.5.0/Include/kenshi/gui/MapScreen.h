#pragma once

#include <kenshi/util/hand.h>
#include <mygui/MyGUI.h>
#include <ogre/Ogre.h>
#include <kenshi/util/OgreUnordered.h>
#include <kenshi/util/lektor.h>

class TownBase;
class ManagementScreen;
class ToolTip;

class MapScreen
{
public:
    static int ImageMarkerLabelSeparation; // RVA = 0x1DAA040 Static Member
    class MapMarkerTown
    {
    public:
        // no_addr void MapMarkerTown(const class MapScreen::MapMarkerTown & _a1);// public missing arg names
        MapMarkerTown(TownBase* town, MapScreen* mapScreen);// public RVA = 0x48DAB0
        MapMarkerTown* _CONSTRUCTOR(TownBase* town, MapScreen* mapScreen);// public RVA = 0x48DAB0
        ~MapMarkerTown();// public RVA = 0x48B5E0
        void _DESTRUCTOR();// public RVA = 0x48B5E0
        hand handle; // 0x0 Member
        MapZoomLevel zoomLayer; // 0x20 Member
        MyGUI::Widget* root; // 0x28 Member
        MyGUI::ImageBox* image; // 0x30 Member
        MyGUI::TextBox* label; // 0x38 Member
        MyGUI::types::TSize<int> fullSize; // 0x40 Member
        void update(const MyGUI::types::TPoint<int>& mapPosition, MapZoomLevel zoom);// public RVA = 0x48F590
        void setVisible(bool value);// public RVA = 0x48B620
        bool getVisible() const;// public RVA = 0x48B640
        // no_addr class MapScreen::MapMarkerTown & operator=(const class MapScreen::MapMarkerTown & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class MapMarkerCharacter
    {
    public:
        // no_addr void MapMarkerCharacter(const class MapScreen::MapMarkerCharacter & _a1);// public missing arg names
        MapMarkerCharacter(RootObjectBase* character, MapScreen* mapScreen);// public RVA = 0x492690
        MapMarkerCharacter* _CONSTRUCTOR(RootObjectBase* character, MapScreen* mapScreen);// public RVA = 0x492690
        ~MapMarkerCharacter();// public RVA = 0x48B660
        void _DESTRUCTOR();// public RVA = 0x48B660
        hand handle; // 0x0 Member
        MyGUI::ImageBox* image; // 0x20 Member
        void update(const MyGUI::types::TPoint<int>& mapPosition);// public RVA = 0x48F9D0
        void setVisible(bool value);// public RVA = 0x48B6A0
        bool getVisible() const;// public RVA = 0x48B6C0
        // no_addr class MapScreen::MapMarkerCharacter & operator=(const class MapScreen::MapMarkerCharacter & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    // no_addr void MapScreen(const class MapScreen & _a1);// public missing arg names
    MapScreen(ManagementScreen* screen);// public RVA = 0x48FA90
    MapScreen* _CONSTRUCTOR(ManagementScreen* screen);// public RVA = 0x48FA90
    ~MapScreen();// public RVA = 0x48E6C0
    void _DESTRUCTOR();// public RVA = 0x48E6C0
    void clear();// public RVA = 0x48D370
    bool getVisible() const;// public RVA = 0x48B6E0
    void centerCamera();// public RVA = 0x48C480
    void clearTempMarkers();// public RVA = 0x48B780
    void refresh();// public RVA = 0x49A7E0
    void update();// public RVA = 0x49A510
    void addSquad(Platoon* squad);// public RVA = 0x493310
    void removeSquad(Platoon* squad);// public RVA = 0x48E960
    void hide();// public RVA = 0x48E990
    void setMovementMarker(const Ogre::Vector3& p, float time);// public RVA = 0x48B7A0
    void showRoads(bool on);// public RVA = 0x493270
    void updatePlayerTownMapMarker(const hand& town);// public RVA = 0x4C3F50
    void updateZoom();// protected RVA = 0x490AF0
    void updateCameraMarker();// protected RVA = 0x48C590
    void refreshTowns();// protected RVA = 0x490F80
    void refreshSquads();// protected RVA = 0x4986C0
    void updateCharacterMarker(RootObjectBase* object);// protected RVA = 0x493350
    bool getMouseWorldPosition(Ogre::Vector3& position);// protected RVA = 0x48C7F0
    MyGUI::types::TPoint<int> worldToMapCoords(const Ogre::Vector3& position);// protected RVA = 0x48BC60
    Ogre::Vector3 mapCoordsToWorld(const MyGUI::types::TPoint<int>& _point);// protected RVA = 0x48C6B0
    void mapMouseWheel(MyGUI::Widget* _sender, int _rel);// protected RVA = 0x48BCF0
    void mapMousePressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);// protected RVA = 0x48B700
    void mapMouseReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);// protected RVA = 0x48C990
    void setupRoads();// protected RVA = 0x492A50
    void updateRoads();// protected RVA = 0x48E750
    void centerButton(MyGUI::Widget* _sender);// protected RVA = 0x48C7A0
    void zoomInButton(MyGUI::Widget* _sender);// protected RVA = 0x48BE60
    void zoomOutButton(MyGUI::Widget* _sender);// protected RVA = 0x48BFA0
    static const MyGUI::Colour& getMarkerColor(RootObjectBase* who);// protected RVA = 0x48EBA0
    float updateTimer; // 0x0 Member
    ToolTip* toolTip; // 0x8 Member
    MyGUI::Widget* mainWidget; // 0x10 Member
    MyGUI::ScrollView* mapScrollView; // 0x18 Member
    MyGUI::ImageBox* mapImage; // 0x20 Member
    MyGUI::ImageBox* cameraMarker; // 0x28 Member
    MyGUI::RotatingSkin* cameraMarkerSkin; // 0x30 Member
    bool zoomUpdated; // 0x38 Member
    float zoomValueBase; // 0x3C Member
    float zoomValueMax; // 0x40 Member
    float zoomValueCurrent; // 0x44 Member
    Ogre::Vector4 zoomCenterOffset; // 0x48 Member
    MapZoomLevel zoomLevelCurrent; // 0x58 Member
    MyGUI::Widget* mapOverlaysContainer[0x4]; // 0x60 Member
    float zoomLevels[0x3]; // 0x80 Member
    ogre_unordered_map<hand, MapScreen::MapMarkerTown*>::type mapMarkersTowns; // 0x90 Member
    Ogre::FastArray<hand> mapMarkersTownsNew; // 0xD0 Member
    ogre_unordered_map<hand, MapScreen::MapMarkerCharacter*>::type mapMarkersCharacters; // 0xE8 Member
    MyGUI::ImageBox* mapMarkerMovement; // 0x128 Member
    Ogre::Vector3 mapMarkerMovementPosition; // 0x130 Member
    float mapMarkerMovementFadeOutDelay; // 0x13C Member
    ogre_unordered_set<hand>::type squadsList; // 0x140 Member
    bool mapDragging; // 0x180 Member
    Ogre::Vector2 mapMouseLastPosition; // 0x184 Member
    Ogre::Vector2 mapOffsetView; // 0x18C Member
    Ogre::Vector4 worldBounds; // 0x194 Member
    Ogre::Vector2 worldSize; // 0x1A4 Member
    struct MapRoad
    {
        MyGUI::Widget* widget; // 0x0 Member
        std::vector<MyGUI::types::TPoint<float>, std::allocator<MyGUI::types::TPoint<float> > > line; // 0x8 Member
        // no_addr void MapRoad(const struct MapScreen::MapRoad & _a1);// public missing arg names
        MapRoad();// public RVA = 0x4C3FE0
        MapRoad* _CONSTRUCTOR();// public RVA = 0x4C3FE0
        // no_addr void ~MapRoad();// public
        // no_addr struct MapScreen::MapRoad & operator=(const struct MapScreen::MapRoad & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    lektor<MapScreen::MapRoad*> roads; // 0x1B0 Member
    static MyGUI::Colour MarkerColourAlly; // RVA = 0x212C468 Static Member
    static MyGUI::Colour MarkerColourNetural; // RVA = 0x212C478 Static Member
    static MyGUI::Colour MarkerColourEnemy; // RVA = 0x212C488 Static Member
    static MyGUI::Colour MarkerColourPlayer; // RVA = 0x212C498 Static Member
    static MyGUI::Colour MarkerColourPlayerSelected; // RVA = 0x212C4A8 Static Member
    // no_addr class MapScreen & operator=(const class MapScreen & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};