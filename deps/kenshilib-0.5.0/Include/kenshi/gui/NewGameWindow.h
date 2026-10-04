#pragma once

#include "GUIWindow.h"
#include <mygui/common/baselayout/BaseLayout.h>

class DatapanelGUI;
class GameData;
class NewGameOptionsWindow;

class NewGameWindow : public GUIWindow, public wraps::BaseLayout
{
public:
    // GUIWindow offset = 0x0, length = 0x30
    // wraps::BaseLayout offset = 0x30, length = 0xA0
    // no_addr void NewGameWindow(const class NewGameWindow & _a1);// public missing arg names
    NewGameWindow();// public RVA = 0x915D80
    NewGameWindow* _CONSTRUCTOR();// public RVA = 0x915D80
    virtual ~NewGameWindow();// public RVA = 0x912640 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x912640 vtable offset = 0x0
    void setVisible(bool visible);// public RVA = 0x9156E0
    bool getVisible();// public RVA = 0x911710
    void setRealPosition(float x, float y);// public RVA = 0x911D20
    virtual void setPosition(int x, int y) override;// public RVA = 0x911D70 vtable offset = 0x0
    void _NV_setPosition(int x, int y);// public RVA = 0x911D70 vtable offset = 0x0
    void showWindow(MyGUI::Widget* _sender);// public RVA = 0x9157A0
    virtual void show(bool _a1) override;// public RVA = 0x911730 vtable offset = 0x0 missing arg names
    void _NV_show(bool _a1);// public RVA = 0x911730 vtable offset = 0x0 missing arg names
    virtual void update() override;// public RVA = 0x911E30 vtable offset = 0x0
    void _NV_update();// public RVA = 0x911E30 vtable offset = 0x0
    void loadData();// private RVA = 0x9123C0
    void updateCurrentData();// private RVA = 0x914E70
    void close(MyGUI::Widget* _sender);// private RVA = 0x915820
    void prevStart(MyGUI::Widget* _sender);// private RVA = 0x915600
    void nextStart(MyGUI::Widget* _sender);// private RVA = 0x915670
    void newGameStart(MyGUI::Widget* _a1);// private RVA = 0x915C50 missing arg names
    void toggleAdvancedOptions(MyGUI::Widget* _sender);// private RVA = 0x911DC0
    int currentStart; // 0xD0 Member
    DatapanelGUI* startInfo; // 0xD8 Member
    MyGUI::TextBox* startNameTextBox; // 0xE0 Member
    Ogre::FastArray<GameData*> startsData; // 0xE8 Member
    NewGameOptionsWindow* newGameOptions; // 0x100 Member
    // no_addr class NewGameWindow & operator=(const class NewGameWindow & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};