#pragma once

#include <ogre/OgrePrerequisites.h>
#include <mygui/MyGUI.h>

class GameData;
class DatapanelGUI;

class GameDataEditorWindow : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x8, length = 0x1
    // VTable         : (none)
    // no_addr void GameDataEditorWindow(const class GameDataEditorWindow & _a1);// public missing arg names
    // no_addr void GameDataEditorWindow();// public
    virtual ~GameDataEditorWindow();// public RVA = 0x66A150 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x66A150 vtable offset = 0x0
    virtual void show(bool on);// public RVA = 0x66BED0 vtable offset = 0x8
    void _NV_show(bool on);// public RVA = 0x66BED0 vtable offset = 0x8
    void show(bool on, float left, float top, GameData* _data);// public RVA = 0x66ACD0
    DatapanelGUI* win; // 0x8 Member
    struct DataItem
    {
        MyGUI::EditBox* edit; // 0x0 Member
        MyGUI::TextBox* label; // 0x8 Member
        std::string name; // 0x10 Member
        // no_addr void DataItem(const struct GameDataEditorWindow::DataItem & _a1);// public missing arg names
        // no_addr void DataItem();// public
        // no_addr void ~DataItem();// public
        // no_addr struct GameDataEditorWindow::DataItem & operator=(const struct GameDataEditorWindow::DataItem & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    GameData* data; // 0x10 Member
    void initDataValues();// private RVA = 0x66A1E0
    // no_addr class GameDataEditorWindow & operator=(const class GameDataEditorWindow & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};