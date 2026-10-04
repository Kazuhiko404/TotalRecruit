#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <mygui/MyGUI_RotatingSkin.h>
#include <ogre/OgreVector3.h>
#include <kenshi/Enums.h>

class DatapanelGUI;
class RealWorldEditableImage;

class ProspectingWindow : public wraps::BaseLayout
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    class ResourceLinePanel : public wraps::BaseLayout
    {
    public:
        // wraps::BaseLayout offset = 0x0, length = 0xA0
        // no_addr void ResourceLinePanel(const class ProspectingWindow::ResourceLinePanel & _a1);// public missing arg names
        ResourceLinePanel(ProspectingWindow* window, MyGUI::Widget* parent, MiningResource resource, const std::string& name, const std::string& value);// public RVA = 0x49CDD0
        ResourceLinePanel* _CONSTRUCTOR(ProspectingWindow* window, MyGUI::Widget* parent, MiningResource resource, const std::string& name, const std::string& value);// public RVA = 0x49CDD0
        virtual ~ResourceLinePanel();// public RVA = 0x48CFE0 vtable offset = 0x0
        void _DESTRUCTOR();// public RVA = 0x48CFE0 vtable offset = 0x0
        MyGUI::Widget* getWidget() const;// public RVA = 0x48B7C0
        MyGUI::Button* button; // 0xA0 Member
        // no_addr class ProspectingWindow::ResourceLinePanel & operator=(const class ProspectingWindow::ResourceLinePanel & _a1);// public missing arg names
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
    };
    // no_addr void ProspectingWindow(const class ProspectingWindow & _a1);// public missing arg names
    ProspectingWindow();// public RVA = 0x49BB60
    ProspectingWindow* _CONSTRUCTOR();// public RVA = 0x49BB60
    virtual ~ProspectingWindow();// public RVA = 0x48D590 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x48D590 vtable offset = 0x0
    static ProspectingWindow* getSingleton();// public RVA = 0x337F50
    void showT(const Ogre::Vector3& pos, float skill, const std::string& name);// public RVA = 0x48E260
    void _show();// public RVA = 0x49EA00
    void hide();// public RVA = 0x48D6A0
    bool getVisible() const;// public RVA = 0x48B7D0
    void closeButton(MyGUI::Window* _sender, const std::string& _name);// public RVA = 0x48D6D0
    void updateCameraMarker();// public RVA = 0x48C1F0
    void resourceSelected(MyGUI::Widget* _sender);// private RVA = 0x49CD50
    MyGUI::types::TPoint<int> worldToMapCoords(const Ogre::Vector3& position) const;// private RVA = 0x48C110
    void updateMap(MiningResource resource);// private RVA = 0x49C5E0
    void refresh();// private RVA = 0x49D0D0
    void clear();// private RVA = 0x48D000
    MyGUI::Window* window; // 0xA0 Member
    MyGUI::ImageBox* imgMap; // 0xA8 Member
    MyGUI::Widget* listResourcesPanel; // 0xB0 Member
    MyGUI::Button* listResourcesSelected; // 0xB8 Member
    MyGUI::ImageBox* cameraMarker; // 0xC0 Member
    MyGUI::RotatingSkin* cameraMarkerSkin; // 0xC8 Member
    DatapanelGUI* datapanelOverview1; // 0xD0 Member
    DatapanelGUI* datapanelOverview2; // 0xD8 Member
    Ogre::vector<ProspectingWindow::ResourceLinePanel*>::type lines; // 0xE0 Member
    RealWorldEditableImage* resourceImage; // 0x100 Member
    Ogre::Vector3 lastPos; // 0x108 Member
    float lastSkill; // 0x114 Member
    std::string lastName; // 0x118 Member
    // no_addr class ProspectingWindow & operator=(const class ProspectingWindow & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};