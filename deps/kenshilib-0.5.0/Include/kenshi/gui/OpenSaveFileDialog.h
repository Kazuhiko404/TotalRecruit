#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <ogre/OgrePrerequisites.h>
#include <boost/function.hpp>

class OpenSaveFileDialog : public wraps::BaseLayout
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    // Typedef        : CloseDialogCallback
    // no_addr void OpenSaveFileDialog(const class OpenSaveFileDialog & _a1);// public missing arg names
    OpenSaveFileDialog(MyGUI::Widget* parent);// public RVA = 0x1A0660
    OpenSaveFileDialog* _CONSTRUCTOR(MyGUI::Widget* parent);// public RVA = 0x1A0660
    virtual ~OpenSaveFileDialog();// public RVA = 0x19F740 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x19F740 vtable offset = 0x0
    void setVisible(bool visible);// public RVA = 0x19FD40
    bool getVisible() const;// public RVA = 0x19F580
    void setDialogInfo(const std::string& _caption, const std::string& _button, bool _folderMode);// public RVA = 0x19FD80
    void setCurrentFolder(const std::string& _folder);// public RVA = 0x1A03A0
    const std::string& getCurrentFolder() const;// public RVA = 0x19F5A0
    void setFileName(const std::string& _value);// public RVA = 0x19F6A0
    const std::string& getFileName() const;// public RVA = 0x19F5B0
    void setRecentFolders(const Ogre::vector<std::string>::type& _listFolders);// public RVA = 0x19F5D0
    void setFileMask(const std::string& _value);// public RVA = 0x1A00D0
    const std::string& getFileMask() const;// public RVA = 0x19F5C0
    boost::function<void(OpenSaveFileDialog*, bool, std::string)> eventCloseDialog; // 0xA0 Member
    void notifyDirectoryComboAccept(MyGUI::ComboBox* _sender, unsigned __int64 _index);// private RVA = 0x1A0520
    void notifyDirectoryComboChangePosition(MyGUI::ComboBox* _sender, unsigned __int64 _index);// private RVA = 0x1A05D0
    void notifyListChangePosition(MyGUI::ListBox* _sender, unsigned __int64 _index);// private RVA = 0x1A0100
    void notifyListSelectAccept(MyGUI::ListBox* _sender, unsigned __int64 _index);// private RVA = 0x1A0220
    void closeWindow(MyGUI::Window* _sender, const std::string& _name);// private RVA = 0x19FF60
    void update();// private RVA = 0x19F8C0
    void accept(MyGUI::Widget* _a1);// private RVA = 0x19FE60 missing arg names
    void cancel(MyGUI::Widget* _a1);// private RVA = 0x19FEE0 missing arg names
    void upFolder(MyGUI::Widget* _a1);// private RVA = 0x19FFE0 missing arg names
    MyGUI::ListBox* filesList; // 0xC8 Member
    MyGUI::EditBox* fileNameTxt; // 0xD0 Member
    MyGUI::ComboBox* currentFolderList; // 0xD8 Member
    MyGUI::Button* openSaveButton; // 0xE0 Member
    std::string currentFolder; // 0xE8 Member
    std::string fileName; // 0x110 Member
    std::string fileMask; // 0x138 Member
    std::string currentSelected; // 0x160 Member
    bool folderMode; // 0x188 Member
    // no_addr class OpenSaveFileDialog & operator=(const class OpenSaveFileDialog & _a1);// public missing arg names
    // no_addr void __dflt_ctor_closure();// public
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};