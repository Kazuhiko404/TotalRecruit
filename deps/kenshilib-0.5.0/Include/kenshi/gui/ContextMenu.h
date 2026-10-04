#pragma once

#include <kenshi/util/lektor.h>
#include <kenshi/util/hand.h>
#include <mygui/common/baselayout/BaseLayout.h>

class RootObject;

class ContextMenuGUI : public wraps::BaseLayout, public Ogre::GeneralAllocatedObject
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0xA1, length = 0x1
    // no_addr void ContextMenuGUI(const class ContextMenuGUI & _a1);// public missing arg names
    ContextMenuGUI();// public RVA = 0x796540
    ContextMenuGUI* _CONSTRUCTOR();// public RVA = 0x796540
    virtual ~ContextMenuGUI();// public RVA = 0x793D70 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x793D70 vtable offset = 0x0
    MyGUI::Widget* getMainWidget() const;// public RVA = 0x7BF260
    bool getVisible() const;// public RVA = 0x7BF270
    void setVisible(bool visible);// public RVA = 0x7938B0
    void show(const lektor<int>& ordersList, const std::string& _name, bool offset);// public RVA = 0x7A6D80
    void optionSelected(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);// public RVA = 0x7A6CB0
    hand contextMenuTarget; // 0xA8 Member
    std::string name; // 0xC8 Member
    MyGUI::TextBox* nameText; // 0xF0 Member
    MyGUI::Widget* optionsList; // 0xF8 Member
    MyGUI::types::TCoord<int> optionCoords; // 0x100 Member
    MyGUI::types::TCoord<int> buttonCoords; // 0x110 Member
    MyGUI::types::TCoord<int> valueCoords; // 0x120 Member
    // no_addr class ContextMenuGUI & operator=(const class ContextMenuGUI & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class ContextMenu : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    // no_addr void ContextMenu(const class ContextMenu & _a1);// public missing arg names
    ContextMenu();// public RVA = 0x793750
    ContextMenu* _CONSTRUCTOR();// public RVA = 0x793750
    ~ContextMenu();// public RVA = 0x793800
    void _DESTRUCTOR();// public RVA = 0x793800
    void showContextMenu(bool on, RootObject* what);// public RVA = 0x7A5960
    bool isVisible() const;// public RVA = 0x790B60
    void update();// public RVA = 0x8055A0
    bool rightMouseActivated; // 0x0 Member
    lektor<int> orders; // 0x8 Member
    std::string contextMenuName; // 0x20 Member
    ContextMenuGUI* menuGUI; // 0x48 Member
    ContextMenuGUI* menuGUI2; // 0x50 Member
    void _destroyMenuGUICheck();// private RVA = 0x7AB660
    bool delayedDestroyFlag; // 0x58 Member
    // no_addr class ContextMenu & operator=(const class ContextMenu & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};