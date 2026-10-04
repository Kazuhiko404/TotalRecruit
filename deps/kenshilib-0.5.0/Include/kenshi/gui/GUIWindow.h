#pragma once

#include <kenshi/util/hand.h>
#include <mygui/MyGUI_Widget.h> 
#include <ogre/OgreMemoryAllocatorConfig.h>

class GUIWindow : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x8, length = 0x1
    // VTable         : (none)
    // no_addr void GUIWindow(const class GUIWindow & _a1);// public missing arg names
    GUIWindow();// protected RVA = 0x6E2510
    GUIWindow* _CONSTRUCTOR();// protected RVA = 0x6E2510
    virtual ~GUIWindow();// protected RVA = 0x6E1940 vtable offset = 0x0
    void _DESTRUCTOR();// protected RVA = 0x6E1940 vtable offset = 0x0
    virtual void clear();// public RVA = 0x402A20 vtable offset = 0x8
    void _NV_clear();// public RVA = 0x402A20 vtable offset = 0x8
    virtual void show(bool _a1) = 0;// public vtable offset = 0x10 missing arg names
    virtual void setPositionReal(float x, float y);// public RVA = 0x6E19A0 vtable offset = 0x18
    void _NV_setPositionReal(float x, float y);// public RVA = 0x6E19A0 vtable offset = 0x18
    virtual void setPosition(int x, int y);// public RVA = 0x6E19C0 vtable offset = 0x20
    void _NV_setPosition(int x, int y);// public RVA = 0x6E19C0 vtable offset = 0x20
    virtual int getWidth() const;// public RVA = 0x6E19E0 vtable offset = 0x28
    int _NV_getWidth() const;// public RVA = 0x6E19E0 vtable offset = 0x28
    virtual int getHeight() const;// public RVA = 0x6E19F0 vtable offset = 0x30
    int _NV_getHeight() const;// public RVA = 0x6E19F0 vtable offset = 0x30
    virtual float getWidthReal() const;// public RVA = 0x6E1A00 vtable offset = 0x38
    float _NV_getWidthReal() const;// public RVA = 0x6E1A00 vtable offset = 0x38
    virtual float getHeightReal() const;// public RVA = 0x6E1A20 vtable offset = 0x40
    float _NV_getHeightReal() const;// public RVA = 0x6E1A20 vtable offset = 0x40
    virtual bool isVisible() const;// public RVA = 0x6E1A40 vtable offset = 0x48
    bool _NV_isVisible() const;// public RVA = 0x6E1A40 vtable offset = 0x48
    virtual void update();// public RVA = 0x6E1A90 vtable offset = 0x50
    void _NV_update();// public RVA = 0x6E1A90 vtable offset = 0x50
    virtual void autoChangeSelectedObject(const hand& obj);// public RVA = 0x6E1AA0 vtable offset = 0x58
    void _NV_autoChangeSelectedObject(const hand& obj);// public RVA = 0x6E1AA0 vtable offset = 0x58
    MyGUI::Widget* getWidget();// public RVA = 0x6E1AD0
    virtual void resize(int w, int h);// public RVA = 0x6E1AE0 vtable offset = 0x60
    void _NV_resize(int w, int h);// public RVA = 0x6E1AE0 vtable offset = 0x60
    MyGUI::Widget* win; // 0x8 Member
    hand selectedObject; // 0x10 Member
    // no_addr class GUIWindow & operator=(const class GUIWindow & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// protected vtable offset = 0x0 missing arg names
};