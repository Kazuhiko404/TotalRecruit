#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <kenshi/util/lektor.h>
#include <kenshi/SaveInfo.h>

class DatapanelGUI;

class LoadSaveWindow : public wraps::BaseLayout
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    // no_addr void LoadSaveWindow(const class LoadSaveWindow & _a1);// public missing arg names
    LoadSaveWindow(const std::string& layout);// public RVA = 0x47F980
    LoadSaveWindow* _CONSTRUCTOR(const std::string& layout);// public RVA = 0x47F980
    virtual ~LoadSaveWindow();// public RVA = 0x47B020 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x47B020 vtable offset = 0x0
    void close(MyGUI::Widget* _sender);// private RVA = 0x47A980
    void selectGame(MyGUI::MultiListBox* _sender, unsigned __int64 index);// private RVA = 0x47F300
    void keyPressed(MyGUI::Widget* w, MyGUI::KeyCode key, unsigned int _a3);// private RVA = 0x47F580
    void toggleCheck(MyGUI::Widget* _sender);// private RVA = 0x47B2E0
    virtual void select(int index);// private RVA = 0x489990 vtable offset = 0x8
    void _NV_select(int index);// private RVA = 0x489990 vtable offset = 0x8
    MyGUI::Widget* getWidget() const;// private RVA = 0x47A2F0
    void deleteSelectedSave(int mbResult);// private RVA = 0x47F360
    void sortListComparer(MyGUI::MultiListBox* _sender, unsigned __int64 _column, const MyGUI::UString& _firstItem, const MyGUI::UString& _secondItem, bool& _less);// protected RVA = 0x47C630
    void createInfo(MyGUI::Widget* panel);// protected RVA = 0x47AC30
    void updateInfo(unsigned __int64 index);// protected RVA = 0x47E000
    MyGUI::MultiListBox* list; // 0xA0 Member
    lektor<SaveInfo> games; // 0xA8 Member
    DatapanelGUI* infoPanel; // 0xC0 Member
    lektor<MyGUI::Button*> options; // 0xC8 Member
    void addOption(int key, const std::string& check, const std::string& label, const std::string& caption);// protected RVA = 0x47C410
    void enableOption(int key, bool enabled);// protected RVA = 0x47A380
    int getOptions() const;// protected RVA = 0x47A300
    std::map<MyGUI::UString, int, std::less<MyGUI::UString>, Ogre::STLAllocator<std::pair<MyGUI::UString const, int>, Ogre::GeneralAllocPolicy > > savesTimesStr; // 0xE0 Member
    // no_addr class LoadSaveWindow & operator=(const class LoadSaveWindow & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class NewGameOptionsWindow;

// TODO move?
class ImportGameMenu : public LoadSaveWindow
{
public:
    // LoadSaveWindow offset = 0x0, length = 0x108
    // no_addr void ImportGameMenu(const class ImportGameMenu & _a1);// public missing arg names
    ImportGameMenu();// public RVA = 0x4809B0
    ImportGameMenu* _CONSTRUCTOR();// public RVA = 0x4809B0
    virtual ~ImportGameMenu();// public RVA = 0x47B140 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x47B140 vtable offset = 0x0
    void importPress(MyGUI::Widget* _sender);// private RVA = 0x47B350
    void toggleAdvancedOptions(MyGUI::Widget* _sender);// private RVA = 0x47A3F0
    virtual void select(int index) override;// private RVA = 0x47C760 vtable offset = 0x0
    void _NV_select(int index);// private RVA = 0x47C760 vtable offset = 0x0
    NewGameOptionsWindow* newGameOptions; // 0x108 Member
    // no_addr class ImportGameMenu & operator=(const class ImportGameMenu & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};