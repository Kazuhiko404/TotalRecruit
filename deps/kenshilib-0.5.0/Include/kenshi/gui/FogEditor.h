#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <ogre/OgreVector3.h>

class DatapanelGUI;
class DataPanelLine;
class FogVolume;

class FogEditor : public wraps::BaseLayout
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    // no_addr void FogEditor(const class FogEditor & _a1);// public missing arg names
    FogEditor();// public RVA = 0x10FA50
    FogEditor* _CONSTRUCTOR();// public RVA = 0x10FA50
    void show();// public RVA = 0x10A300
    void close();// public RVA = 0x10E350
    bool isActive() const;// public RVA = 0x109380
    bool update();// public RVA = 0x10C590
    void save(const std::string& filename);// public RVA = 0x10C720
    MyGUI::Window* window; // 0xA0 Member
    MyGUI::ListBox* fogList; // 0xA8 Member
    MyGUI::ComboBox* typeList; // 0xB0 Member
    DatapanelGUI* panel; // 0xB8 Member
    void closeWindow(MyGUI::Window* _a1, const std::string& _a2);// protected RVA = 0x10E390 missing arg names
    void addFog(MyGUI::Widget* _a1);// protected RVA = 0x10EB50 missing arg names
    void removeFog(MyGUI::Widget* _a1);// protected RVA = 0x10E3D0 missing arg names
    void selectFog(MyGUI::ListBox* list, unsigned __int64 index);// protected RVA = 0x10D890
    void updateFogName(DataPanelLine* line);// protected RVA = 0x10A150
    void updateFogValues(DataPanelLine* _a1);// protected RVA = 0x10C450 missing arg names
    void selectNextEdge(DataPanelLine* _a1);// protected RVA = 0x10A720 missing arg names
    FogVolume* selectedFog; // 0xC0 Member
    int selectedSide; // 0xC8 Member
    Ogre::Vector3 colour; // 0xCC Member
    float distance; // 0xD8 Member
    float radius; // 0xDC Member
    float edge; // 0xE0 Member
    Ogre::ManualObject* editCage; // 0xE8 Member
    Ogre::SceneNode* editNode; // 0xF0 Member
    Ogre::Vector3 editPos; // 0xF8 Member
    Ogre::Quaternion editRot; // 0x104 Member
    virtual ~FogEditor();// public RVA = 0x1203F0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x1203F0 vtable offset = 0x0
    // no_addr class FogEditor & operator=(const class FogEditor & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};