#pragma once

#include <mygui/MyGUI.h>
#include <kenshi/util/lektor.h>

class SplashScreen
{
public:
    // no_addr void SplashScreen(const class SplashScreen & _a1);// public missing arg names
    SplashScreen();// public RVA = 0x914990
    SplashScreen* _CONSTRUCTOR();// public RVA = 0x914990
    ~SplashScreen();// public RVA = 0x911F00
    void _DESTRUCTOR();// public RVA = 0x911F00
    void update();// public RVA = 0x911F70
    char getState() const;// public RVA = 0x911E70
    bool getEnded() const;// public RVA = 0x911E60
    void skip();// public RVA = 0x911D10
    char currentState; // 0x0 Member
    double startTime; // 0x8 Member
    bool skipLogo; // 0x10 Member
    int delay; // 0x14 Member
    MyGUI::Widget* mainPanel; // 0x18 Member
    lektor<MyGUI::ImageBox*> logos; // 0x20 Member
    void addLogo(const std::string& file, float scale);// private RVA = 0x9130A0
    // no_addr class SplashScreen & operator=(const class SplashScreen & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};