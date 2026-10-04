#pragma once

#include "util/lektor.h"

#include <map>
#include <ogre/OgreMemoryAllocatorConfig.h>
#include <ogre/OgreVector2.h>
#include <ogre/OgreVector3.h>
#include <ois/OISKeyboard.h>

class InputHandler : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    enum Masks
    {
        SHIFT_MASK = 0x100,
        CTRL_MASK = 0x200,
        ALT_MASK = 0x400,
        ALL_MASK = 0x700,
        NONE_MASK = 0x0
    };

    enum GameMode
    {
        GLOBAL,
        EDITOR
    };

    struct Command
    {
        Command(const Command& __that);// public RVA = 0x366340
        Command* _CONSTRUCTOR(const Command& __that);// public RVA = 0x366340
        Command();// public RVA = 0x35EB90
        Command* _CONSTRUCTOR();// public RVA = 0x35EB90
        GameMode gameMode; // 0x0 Member
        bool isKey; // 0x4 Member
        int code; // 0x8 Member
        bool* boolean; // 0x10 Member
        std::string name; // 0x18 Member
        int bound; // 0x40 Member
        Masks masks; // 0x44 Member
        ~Command();// public RVA = 0x35EBC0
        void _DESTRUCTOR();// public RVA = 0x35EBC0
        Command& operator=(const Command& __that);// public RVA = 0x366860
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    // Typedef        : EventIterator
    // Typedef        : CommandMap
    std::map<std::string, Command, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, Command>, Ogre::GeneralAllocPolicy > > commands; // 0x0 Member
    std::map<int, Command*, std::less<int>, Ogre::STLAllocator<std::pair<int const, Command*>, Ogre::GeneralAllocPolicy > > map; // 0x28 Member
    std::map<int, Command*, std::less<int>, Ogre::STLAllocator<std::pair<int const, Command*>, Ogre::GeneralAllocPolicy > > editMap; // 0x50 Member
    std::set<Command*, std::less<Command*>, Ogre::STLAllocator<Command*, Ogre::GeneralAllocPolicy > > events; // 0x78 Member
    OIS::Keyboard* keyboard; // 0xA0 Member
    std::map<std::string, int, std::less<std::string >, Ogre::STLAllocator<std::pair<std::string const, int>, Ogre::GeneralAllocPolicy > > keyNameMap; // 0xA8 Member
    std::string getKeyUTF8(int key);// private RVA = 0x360C50
    int parseKey(const std::string& s);// private RVA = 0x3616E0
    Command* getCommand(const std::string& s);// private RVA = 0x361AA0
    void keyDownEvent(OIS::KeyCode key);// private RVA = 0x360680
    void keyUpEvent(OIS::KeyCode key);// private RVA = 0x3608F0
    void clearMessages();// private RVA = 0x3605D0
    // no_addr void InputHandler(const class InputHandler & _a1);// public missing arg names
    InputHandler();// public RVA = 0x364E10
    InputHandler* _CONSTRUCTOR();// public RVA = 0x364E10
    void initialise();// public RVA = 0x363210
    void addCommand(const std::string& name, int value, int key, int alt, Masks masks, GameMode mode);// public RVA = 0x362F90
    void addCommand(const std::string& name, bool& value, int key, int alt, Masks masks, GameMode mode);// public RVA = 0x363130
    void addKey(const std::string& name, bool& value, int key, int alt, GameMode mode);// public RVA = 0x3631A0
    void bind(const std::string& name, int key);// public RVA = 0x361DB0
    void unbind(int code, GameMode mode);// public RVA = 0x360BC0
    void unbind(const std::string& cmd);// public RVA = 0x361E40
    void unbindAll();// public RVA = 0x360520
    bool isBound(const std::string& command);// public RVA = 0x361BE0
    bool isBound(int key);// public RVA = 0x360B00
    bool isKeyState(const std::string& command);// public RVA = 0x361C10
    GameMode getMode(const std::string& command);// public RVA = 0x361C40
    void sendEvent(const std::string& name);// public RVA = 0x361B30
    void loadConfig();// public RVA = 0x361F80
    void saveConfig();// public RVA = 0x361000
    std::string keyString(int key, bool translated);// public RVA = 0x360E30
    lektor<int> getBoundKeys(const std::string& command);// public RVA = 0x361C60
    const std::string& getBoundCommand(int key, GameMode mode);// public RVA = 0x361590
    bool controlEnabled; // 0xD0 Member
    GameMode gameMode; // 0xD4 Member
    bool ctrl; // 0xD8 Member
    bool shift; // 0xD9 Member
    bool alt; // 0xDA Member
    bool up; // 0xDB Member
    bool down; // 0xDC Member
    bool left; // 0xDD Member
    bool right; // 0xDE Member
    bool space; // 0xDF Member
    bool pgup; // 0xE0 Member
    bool pgdn; // 0xE1 Member
    bool comma; // 0xE2 Member
    bool period; // 0xE3 Member
    bool f1; // 0xE4 Member
    bool f2; // 0xE5 Member
    bool escape; // 0xE6 Member
    bool del; // 0xE7 Member
    bool escape_msg; // 0xE8 Member
    bool zoomin; // 0xE9 Member
    bool zoomout; // 0xEA Member
    bool buildTiltInc; // 0xEB Member
    bool buildTiltDec; // 0xEC Member
    bool rotate; // 0xED Member
    bool rotRight; // 0xEE Member
    bool rotLeft; // 0xEF Member
    bool tiltDown; // 0xF0 Member
    bool tiltUp; // 0xF1 Member
    bool highlight; // 0xF2 Member
    bool mLeft; // 0xF3 Member
    bool mRight; // 0xF4 Member
    bool lastMLeft; // 0xF5 Member
    bool lastMRight; // 0xF6 Member
    bool mLDown; // 0xF7 Member
    bool mRDown; // 0xF8 Member
    bool mLUp; // 0xF9 Member
    bool mRUp; // 0xFA Member
    bool swapMouseButtons; // 0xFB Member
    Ogre::Vector2 mPos; // 0xFC Member
    Ogre::Vector2 mPosAbs; // 0x104 Member
    Ogre::Vector3 mSpeed; // 0x10C Member
    int mWheel; // 0x118 Member
    ~InputHandler();// public RVA = 0x827E10
    void _DESTRUCTOR();// public RVA = 0x827E10
    // no_addr class InputHandler & operator=(const class InputHandler & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};