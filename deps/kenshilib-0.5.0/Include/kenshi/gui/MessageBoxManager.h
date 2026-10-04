#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <ogre/OgreMemoryAllocatorConfig.h>
#include <kenshi/util/lektor.h>

class MessageBoxManager : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    // Typedef        : EventHandle_IntVoid
    // Typedef        : MessageButtonList
    class Box : private wraps::BaseLayout
    {
    public:
        // wraps::BaseLayout offset = 0x0, length = 0xA0
        // no_addr void Box(const class MessageBoxManager::Box & _a1);// public missing arg names
        Box();// public RVA = 0x6E6210
        Box* _CONSTRUCTOR();// public RVA = 0x6E6210
        virtual ~Box();// public RVA = 0x6E2F50 vtable offset = 0x0
        void _DESTRUCTOR();// public RVA = 0x6E2F50 vtable offset = 0x0
        void buttonClick(MyGUI::Widget* _sender);// public RVA = 0x6E6340
        Ogre::vector<MyGUI::Button*>::type buttons; // 0xA0 Member
        bool modal; // 0xC0 Member
        MyGUI::delegates::IDelegate1<int>* callback; // 0xC8 Member
        // no_addr class MessageBoxManager::Box & operator=(const class MessageBoxManager::Box & _a1);// public missing arg names
        // no_addr void __local_vftable_ctor_closure();// public
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
    };
    static MyGUI::Window* createMessageBox(const std::string& title, const std::string& message, const std::vector<std::pair<std::string, int>, Ogre::STLAllocator<std::pair<std::string, int>, Ogre::GeneralAllocPolicy > >& buttons, bool modal, MyGUI::delegates::IDelegate1<int>* callback);// public RVA = 0x6E63A0
    static bool hideMessageBox(bool enter);// public RVA = 0x6E6FA0
    static bool hasModalMessage();// public RVA = 0x6E24C0
    static void removeMessageBox(MessageBoxManager::Box* box, int button);// private RVA = 0x6E29C0
    static lektor<MessageBoxManager::Box*> messages; // RVA = 0x1F26A28 Static Member
    static int frameNumber; // RVA = 0x212F9A8 Static Member
    // no_addr void MessageBoxManager();// public
    // no_addr void ~MessageBoxManager();// public
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};