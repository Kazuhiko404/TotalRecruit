#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <kenshi/util/lektor.h>
#include <kenshi/util/hand.h>


class Building;
class GameDataEditorWindow;
class BuildingInterior;

class InteriorModeButtonWindow : public wraps::BaseLayout, public Ogre::GeneralAllocatedObject
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0xA1, length = 0x1
    // no_addr void InteriorModeButtonWindow(const class InteriorModeButtonWindow & _a1);// public missing arg names
    InteriorModeButtonWindow();// public RVA = 0x778FE0
    InteriorModeButtonWindow* _CONSTRUCTOR();// public RVA = 0x778FE0
    virtual ~InteriorModeButtonWindow();// public RVA = 0x778420 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x778420 vtable offset = 0x0
    void toggleInteriorMode();// public RVA = 0x76B570
    void setVisible(bool v);// public RVA = 0x7808A0
    bool wantExteriorsInvisible() const;// public RVA = 0x4EB730
    bool exteriorsInvisible; // 0xA1 Member
    bool interiorMode; // 0xA2 Member
    void setSelectedBuilding(Building* b);// public RVA = 0x7757B0
    Building* getSelectedBuilding();// public RVA = 0x4F78F0
    bool updateUsageNodes();// public RVA = 0x771DC0
    void refresh();// public RVA = 0x774370
    GameDataEditorWindow* dataEditWindow; // 0xA8 Member
    hand currentBuilding; // 0xB0 Member
    std::string currentInterior; // 0xD0 Member
    std::string currentExterior; // 0xF8 Member
    lektor<hand> updateNodesMessages; // 0x120 Member
    MyGUI::Button* interiorModeButton; // 0x138 Member
    MyGUI::Window* win; // 0x140 Member
    MyGUI::Button* centerButton; // 0x148 Member
    MyGUI::Button* exteriorButton; // 0x150 Member
    MyGUI::TextBox* titleLabel; // 0x158 Member
    MyGUI::Button* saveBut; // 0x160 Member
    MyGUI::Button* deleteBut; // 0x168 Member
    MyGUI::ListBox* listbox; // 0x170 Member
    MyGUI::EditBox* namebox; // 0x178 Member
    MyGUI::Button* saveBut2; // 0x180 Member
    MyGUI::Button* deleteBut2; // 0x188 Member
    MyGUI::ListBox* listbox2; // 0x190 Member
    MyGUI::EditBox* namebox2; // 0x198 Member
    void activateInteriorMode(bool on);// private RVA = 0x7767B0
    void closeWindow(MyGUI::Window* _sender, const std::string& _name);// private RVA = 0x778A20
    void toggleVisButtonPressed(MyGUI::Widget* _sender);// private RVA = 0x76B610
    void interiorModePressed(MyGUI::Widget* _sender);// private RVA = 0x7785B0
    void interiorModeButtonUpdate(MyGUI::Widget* _sender, int _a2, int _a3, MyGUI::MouseButton _a4);// private RVA = 0x76B640
    void interiorModeButtonUpdate2(MyGUI::Widget* _sender, MyGUI::Widget* _sender2);// private RVA = 0x76B670
    void notifyEditTextChange(MyGUI::EditBox* _sender);// private RVA = 0x775690
    void centerButtonPressed(MyGUI::Widget* _sender);// private RVA = 0x76EFB0
    void saveButtonPressed(MyGUI::Widget* _sender);// private RVA = 0x774A40
    void deleteButtonPressed(MyGUI::Widget* _sender);// private RVA = 0x7759D0
    void clearNodes(MyGUI::Widget* _sender);// private RVA = 0x76F2E0
    void clearAll(MyGUI::Widget* _sender);// private RVA = 0x76F330
    void listItemSelected(MyGUI::ListBox* _sender, unsigned __int64 index);// private RVA = 0x774F00
    void confirmDeleteInteriorLayout(int c);// private RVA = 0x775340
    void confirmDeleteExteriorLayout(int c);// private RVA = 0x7754B0
    void saveButtonPressed2(MyGUI::Widget* _sender);// private RVA = 0x774D10
    void deleteButtonPressed2(MyGUI::Widget* _sender);// private RVA = 0x775C30
    void listItemSelected2(MyGUI::ListBox* _sender, unsigned __int64 index);// private RVA = 0x775120
    void notifyEditTextChange2(MyGUI::EditBox* _sender);// private RVA = 0x775720
    void setInteriorLayout(const std::string& name);// private RVA = 0x76F010
    void setExteriorLayout(const std::string& name);// private RVA = 0x76F1D0
    void recheckOutsideFurniture(Building* building);// private RVA = 0x770EA0
    static bool wasTheInteriorLoadedFromASave(BuildingInterior* interior);// private RVA = 0x76C5A0
    // no_addr class InteriorModeButtonWindow & operator=(const class InteriorModeButtonWindow & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};