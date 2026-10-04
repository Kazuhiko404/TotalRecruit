#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <kenshi/util/lektor.h>

class LoadingWindow : public wraps::BaseLayout
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    static LoadingWindow* getInstance();// public RVA = 0x6EE0E0
    void reload();// public RVA = 0x911BB0
    void setVisible(bool visible);// public RVA = 0x9154F0
    bool getVisible() const;// public RVA = 0x911BE0
    void show();// public RVA = 0x911C00
    void hide();// public RVA = 0x911C10
    // no_addr void LoadingWindow(const class LoadingWindow & _a1);// public missing arg names
    LoadingWindow();// protected RVA = 0x918AD0
    LoadingWindow* _CONSTRUCTOR();// protected RVA = 0x918AD0
    virtual ~LoadingWindow();// protected RVA = 0x912810 vtable offset = 0x0
    void _DESTRUCTOR();// protected RVA = 0x912810 vtable offset = 0x0
    static LoadingWindow* instance; // RVA = 0x2132248 Static Member
    void frameEntered(float _frame);// private RVA = 0x911C20
    void setRandomBackground();// private RVA = 0x914C30
    void setRandomTip();// private RVA = 0x911E90
    unsigned char currentIndex; // 0xA0 Member
    float currentTime; // 0xA4 Member
    MyGUI::UString loadingStr; // 0xA8 Member
    MyGUI::TextBox* loadingText; // 0xE8 Member
    MyGUI::EditBox* messageText; // 0xF0 Member
    MyGUI::ImageBox* backgroundImage; // 0xF8 Member
    lektor<MyGUI::UString> tips; // 0x100 Member
    // no_addr class LoadingWindow & operator=(const class LoadingWindow & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// protected vtable offset = 0x0 missing arg names
};