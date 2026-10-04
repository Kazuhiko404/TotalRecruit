#pragma once

#include "GUIWindow.h"
#include <mygui/common/baselayout/BaseLayout.h>

class CharacterTradingHandler;
class CharacterTradingBox;

class CharacterTradingWindow : public GUIWindow, public wraps::BaseLayout
{
public:
    // GUIWindow offset = 0x0, length = 0x30
    // wraps::BaseLayout offset = 0x30, length = 0xA0
    // no_addr void CharacterTradingWindow(const class CharacterTradingWindow & _a1);// public missing arg names
    CharacterTradingWindow(CharacterTradingHandler* tradingHandler);// public RVA = 0x7296B0
    CharacterTradingWindow* _CONSTRUCTOR(CharacterTradingHandler* tradingHandler);// public RVA = 0x7296B0
    virtual ~CharacterTradingWindow();// public RVA = 0x726EB0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x726EB0 vtable offset = 0x0
    virtual void show(bool on) override;// public RVA = 0x7211D0 vtable offset = 0x0
    void _NV_show(bool on);// public RVA = 0x7211D0 vtable offset = 0x0
    virtual bool isVisible() const override;// public RVA = 0x7211F0 vtable offset = 0x0
    bool _NV_isVisible() const;// public RVA = 0x7211F0 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x721210 vtable offset = 0x0
    void _NV_update();// public RVA = 0x721210 vtable offset = 0x0
    virtual void close();// public RVA = 0x725660 vtable offset = 0x68
    void _NV_close();// public RVA = 0x725660 vtable offset = 0x68
    static CharacterTradingWindow* Instance; // RVA = 0x212FB50 Static Member
    void updateSelected();// private RVA = 0x726FF0
    void confirmButton(MyGUI::Widget* _a1);// private RVA = 0x725600 missing arg names
    void cancelButton(MyGUI::Widget* _a1);// private RVA = 0x721180 missing arg names
    void notifyMouseWheel(MyGUI::Widget* _sender, int _rel);// private RVA = 0x721190
    CharacterTradingHandler* tradingHandler; // 0xD0 Member
    Ogre::FastArray<CharacterTradingBox*> tradingBoxes; // 0xD8 Member
    MyGUI::ScrollView* charactersBox; // 0xF0 Member
    MyGUI::Button* confirmBtn; // 0xF8 Member
    MyGUI::TextBox* currentTotalText; // 0x100 Member
    MyGUI::TextBox* selectedCountText; // 0x108 Member
    // no_addr class CharacterTradingWindow & operator=(const class CharacterTradingWindow & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};