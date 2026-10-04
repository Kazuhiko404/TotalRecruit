#pragma once

#include "GUIWindow.h"
#include <mygui/common/baselayout/BaseLayout.h>

class Dialogue;
class Character;

class DialogueWindow : public GUIWindow, public wraps::BaseLayout
{
public:
    // GUIWindow offset = 0x0, length = 0x30
    // wraps::BaseLayout offset = 0x30, length = 0xA0
    bool getVisible() const;// public RVA = 0x720CF0
    void setVisible(bool visible);// public RVA = 0x720D10
    virtual void show(bool on) override;// public RVA = 0x720DB0 vtable offset = 0x0
    void _NV_show(bool on);// public RVA = 0x720DB0 vtable offset = 0x0
    void show(Dialogue* _dialogue);// public RVA = 0x726650
    void hide(Dialogue* _dialogue);// public RVA = 0x720D50
    virtual bool isVisible() const override;// public RVA = 0x720DF0 vtable offset = 0x0
    bool _NV_isVisible() const;// public RVA = 0x720DF0 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x726950 vtable offset = 0x0
    void _NV_update();// public RVA = 0x726950 vtable offset = 0x0
    int getTop() const;// public RVA = 0x720D30
    void activateResponse(int index);// public RVA = 0x721820
    void setNPCText(const std::string& text);// public RVA = 0x721860
    void setResponses(const Ogre::vector<std::string>::type& responses);// public RVA = 0x722810
    void clearResponses();// public RVA = 0x721950
    void changePortrait(Character* character, bool player);// public RVA = 0x721D20
    // no_addr void DialogueWindow(const class DialogueWindow & _a1);// public missing arg names
    DialogueWindow();// private RVA = 0x724570
    DialogueWindow* _CONSTRUCTOR();// private RVA = 0x724570
    virtual ~DialogueWindow();// private RVA = 0x721FE0 vtable offset = 0x0
    void _DESTRUCTOR();// private RVA = 0x721FE0 vtable offset = 0x0
    void updatePanelsPosition();// private RVA = 0x720E10
    Dialogue* dialogue; // 0xD0 Member
    int conversationPanelsMargin; // 0xD8 Member
    int converstaionPCPanelBottom; // 0xDC Member
    int conversationPCSpeechPanelMinHeight; // 0xE0 Member
    int conversationNPCSpeechPanelMinHeight; // 0xE4 Member
    int conversationPCPanelHeightDiffSpeechPanel; // 0xE8 Member
    int conversationNPCPanelHeightDiffSpeechPanel; // 0xEC Member
    Character* conversationPC; // 0xF0 Member
    MyGUI::Widget* conversationPCPanel; // 0xF8 Member
    MyGUI::Widget* conversationNPCPanel; // 0x100 Member
    MyGUI::TextBox* conversationNPCNameText; // 0x108 Member
    MyGUI::TextBox* conversationPCNameText; // 0x110 Member
    MyGUI::TextBox* conversationPCMoneyText; // 0x118 Member
    MyGUI::EditBox* conversationNPCSpeech; // 0x120 Member
    MyGUI::ScrollView* conversationPCSpeechPanel; // 0x128 Member
    MyGUI::ImageBox* conversationNPCPortrait; // 0x130 Member
    MyGUI::ImageBox* conversationPCPortrait; // 0x138 Member
    MyGUI::Colour conversationTextColor; // 0x140 Member
    MyGUI::Colour conversationTextSelectedColor; // 0x150 Member
    Ogre::FastArray<MyGUI::EditBox*> replyTexts; // 0x160 Member
    // no_addr class DialogueWindow & operator=(const class DialogueWindow & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// private vtable offset = 0x0 missing arg names
};