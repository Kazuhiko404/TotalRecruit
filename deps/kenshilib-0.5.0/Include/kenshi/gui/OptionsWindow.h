#pragma once

#include "GUIWindow.h"
#include <mygui/common/baselayout/BaseLayout.h>
#include <kenshi/util/lektor.h>
#include <ois/OISKeyboard.h>

class DataPanelLine;
class DataPanelLine_KeyConfig;
class DatapanelGUI;
class ToolTip;

class OptionsWindow : public GUIWindow, public wraps::BaseLayout
{
public:
    // GUIWindow offset = 0x0, length = 0x30
    // wraps::BaseLayout offset = 0x30, length = 0xA0
    static OptionsWindow* singleton; // RVA = 0x0 Static Member
    static OptionsWindow* getSingleton();// public RVA = 0x4067B0
    virtual void show(bool value) override;// public RVA = 0x3FAF50 vtable offset = 0x0
    void _NV_show(bool value);// public RVA = 0x3FAF50 vtable offset = 0x0
    void show();// public RVA = 0x3FAE70
    void hide();// public RVA = 0x3EE300
    void toggle();// public RVA = 0x3FAF10
    bool isVisible();// public RVA = 0x3E6D20
    bool waitingForKey();// public RVA = 0x3E6D00
    void setKey(const OIS::KeyCode& code);// public RVA = 0x3EFAE0
    virtual void update() override;// public RVA = 0x3E6D40 vtable offset = 0x0
    void _NV_update();// public RVA = 0x3E6D40 vtable offset = 0x0
    // no_addr void OptionsWindow(const class OptionsWindow & _a1);// public missing arg names
    OptionsWindow();// private RVA = 0x3EBFD0
    OptionsWindow* _CONSTRUCTOR();// private RVA = 0x3EBFD0
    virtual ~OptionsWindow();// private RVA = 0x3E7670 vtable offset = 0x0
    void _DESTRUCTOR();// private RVA = 0x3E7670 vtable offset = 0x0
    void saveLocationChanged(DataPanelLine* line);// private RVA = 0x3EAEF0
    void changeFontSize(DataPanelLine* line);// private RVA = 0x3E7FB0
    void changeVolume(DataPanelLine* line);// private RVA = 0x3E6D70
    void toggleNames(DataPanelLine* line);// private RVA = 0x3E6E10
    void resetTutorials(DataPanelLine* _a1);// private RVA = 0x3E6ED0 missing arg names
    void toggleTutorials(DataPanelLine* _a1);// private RVA = 0x3E6E50 missing arg names
    void changeDistances(DataPanelLine* _a1);// private RVA = 0x3E6F70 missing arg names
    void toggleCompositor(DataPanelLine* l);// private RVA = 0x3E7120
    void resetAllKeys(DataPanelLine* _a1);// private RVA = 0x3E7030 missing arg names
    void saveOptions();// private RVA = 0x3EC570
    void create();// private RVA = 0x3EFD40
    void updateResolutions();// private RVA = 0x3E78A0
    void closeButton(MyGUI::Widget* _sender);// private RVA = 0x3EAEA0
    bool invertX; // 0xD0 Member
    bool invertY; // 0xD1 Member
    lektor<std::string > resolutions; // 0xD8 Member
    int resolutionIndex; // 0xF0 Member
    DataPanelLine_KeyConfig* keyConfig; // 0xF8 Member
    DatapanelGUI* keysDatapanel; // 0x100 Member
    MyGUI::TabControl* tabs; // 0x108 Member
    bool created; // 0x110 Member
    ToolTip* tooltip; // 0x118 Member
    bool previewFontSize; // 0x120 Member
    // no_addr class OptionsWindow & operator=(const class OptionsWindow & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// private vtable offset = 0x0 missing arg names
};