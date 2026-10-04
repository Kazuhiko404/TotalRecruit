#pragma once

#include <ogre/OgreVector3.h>
#include <ogre/OgreVector2.h>
#include <mygui/MyGUI_Widget.h>

class InstanceID;
class ZoneMap;
class DataPanelLine;
class DatapanelGUI;
class DataPanelLine_TextEditable;
class DataPanelLine_Button;
class Gizmo;

class TransformWindow : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    static TransformWindow* getSingleton();// public RVA = 0x96BED0
    TransformWindow();// public RVA = 0x96BC70
    TransformWindow* _CONSTRUCTOR();// public RVA = 0x96BC70
    ~TransformWindow();// public RVA = 0x96BF20
    void _DESTRUCTOR();// public RVA = 0x96BF20
    void show(Ogre::SceneNode* node, Ogre::Vector3* p, Ogre::Quaternion* o, Ogre::Vector3* s, Ogre::SceneNode* pr);// public RVA = 0x96DE10
    void setCaption(const std::string& s);// public RVA = 0x96BDD0
    void updateState(const InstanceID* id);// public RVA = 0x96C020
    void close();// public RVA = 0x96BCF0
    void refresh();// public RVA = 0x96C8B0
    bool updateGizmo();// public RVA = 0x96CC20
    bool isActive() const;// public RVA = 0x96BE70
    bool isVisible() const;// public RVA = 0x96BEA0
    bool hasChanged() const;// public RVA = 0x7808C0
    void clearChangedFlag();// public RVA = 0x7808D0
    int getMode() const;// public RVA = 0x7808E0
    bool setMode(int m);// public RVA = 0x96F0D0
    ZoneMap* getLastZone() const;// public RVA = 0x7808F0
    ZoneMap* getZone() const;// public RVA = 0x780900
    // no_addr class Ogre::SceneNode * getNode();// public
    void changeMode(DataPanelLine* line);// private RVA = 0x96CCE0
    void changeCoord(DataPanelLine* line);// private RVA = 0x96D830
    void changeValue(DataPanelLine* line);// private RVA = 0x96C680
    void confirmValue(MyGUI::Widget* _a1, MyGUI::Widget* _a2);// private RVA = 0x96BE00 missing arg names
    void reset(DataPanelLine* _a1);// private RVA = 0x96CAF0 missing arg names
    void revert(DataPanelLine* _a1);// private RVA = 0x96DD10 missing arg names
    void hide(MyGUI::Window* _a1, const std::string& _a2);// private RVA = 0x96BD60 missing arg names
    static TransformWindow* instance; // RVA = 0x2132480 Static Member
    DatapanelGUI* window; // 0x0 Member
    DataPanelLine_TextEditable* windowXValue; // 0x8 Member
    DataPanelLine_TextEditable* windowYValue; // 0x10 Member
    DataPanelLine_TextEditable* windowZValue; // 0x18 Member
    DataPanelLine_Button* modeButton; // 0x20 Member
    DataPanelLine_Button* axisButton; // 0x28 Member
    DataPanelLine_Button* revertButton; // 0x30 Member
    Ogre::SceneNode* node; // 0x38 Member
    Ogre::SceneNode* parentNode; // 0x40 Member
    Gizmo* gizmo; // 0x48 Member
    int mode; // 0x50 Member
    int coordinateSystem; // 0x54 Member
    bool hasScale; // 0x58 Member
    ZoneMap* lastZone; // 0x60 Member
    ZoneMap* currentZone; // 0x68 Member
    const InstanceID* currentInstance; // 0x70 Member
    bool changed; // 0x78 Member
    bool editChanged; // 0x79 Member
    Ogre::Vector2 lastMouse; // 0x7C Member
    // no_addr class TransformWindow & operator=(const class TransformWindow & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};