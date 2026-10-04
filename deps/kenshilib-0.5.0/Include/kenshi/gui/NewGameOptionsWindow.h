#pragma once

#include <mygui/common/baselayout/BaseLayout.h>

class GameplayOptions;
class DatapanelGUI;
class ToolTip;

class NewGameOptionsWindow : public wraps::BaseLayout
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    // no_addr void NewGameOptionsWindow(const class NewGameOptionsWindow & _a1);// public missing arg names
    NewGameOptionsWindow();// public RVA = 0x913A20
    NewGameOptionsWindow* _CONSTRUCTOR();// public RVA = 0x913A20
    virtual ~NewGameOptionsWindow();// public RVA = 0x9126F0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x9126F0 vtable offset = 0x0
    void setVisible(bool visible);// public RVA = 0x911740
    bool getVisible() const;// public RVA = 0x911760
    void setPosition(int x, int y);// public RVA = 0x911780
    void update();// public RVA = 0x9117A0
    void setOptions(const GameplayOptions& options);// public RVA = 0x9117D0
    DatapanelGUI* optionsPanel; // 0xA0 Member
    ToolTip* tooltip; // 0xA8 Member
    // no_addr class NewGameOptionsWindow & operator=(const class NewGameOptionsWindow & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};