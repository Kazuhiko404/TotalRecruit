#pragma once

#include <mygui/MyGUI.h>
#include <ogre/OgrePrerequisites.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>
#include <kenshi/util/StringPair.h>

class ToolTip : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x8, length = 0x1
    // VTable         : (none)
    enum Type
    {
        TEXT,
        MULTILINE,
        GAMEDATA,
        ITEM,
        HAND
    };

    class ToolTipLine
    {
    public:
        ToolTipLine(MyGUI::Widget* parent, float marginH, const std::string& captionLeft, const std::string& captionRight);// public RVA = 0x91F120
        ToolTipLine* _CONSTRUCTOR(MyGUI::Widget* parent, float marginH, const std::string& captionLeft, const std::string& captionRight);// public RVA = 0x91F120
        ~ToolTipLine();// public RVA = 0x91E750
        void _DESTRUCTOR();// public RVA = 0x91E750
        MyGUI::Widget* content; // 0x0 Member
        MyGUI::EditBox* leftBox; // 0x8 Member
        MyGUI::EditBox* rightBox; // 0x10 Member
        int width; // 0x18 Member
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    // no_addr void ToolTip(const class ToolTip & _a1);// public missing arg names
    ToolTip(MyGUI::Widget* _panel, const std::string& skin);// public RVA = 0x91F560
    ToolTip* _CONSTRUCTOR(MyGUI::Widget* _panel, const std::string& skin);// public RVA = 0x91F560
    virtual ~ToolTip();// public RVA = 0x91F780 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x91F780 vtable offset = 0x0
    void setup(MyGUI::Widget* widget, const hand& data);// public RVA = 0x91FAB0
    void setup(MyGUI::Widget* widget, GameData* data);// public RVA = 0x91F970
    void setup(MyGUI::Widget* widget, const lektor<StringPair>& lines);// public RVA = 0x920AB0
    void setup(MyGUI::Widget* widget, const std::string& text);// public RVA = 0x91F830
    virtual void clear(MyGUI::Widget* widget);// public RVA = 0x91EB20 vtable offset = 0x8
    void _NV_clear(MyGUI::Widget* widget);// public RVA = 0x91EB20 vtable offset = 0x8
    virtual void update();// public RVA = 0x91E790 vtable offset = 0x10
    void _NV_update();// public RVA = 0x91E790 vtable offset = 0x10
    virtual void hide();// public RVA = 0x91E7F0 vtable offset = 0x18
    void _NV_hide();// public RVA = 0x91E7F0 vtable offset = 0x18
    bool getVisible() const;// public RVA = 0x91E810
    virtual void _setup(MyGUI::Widget* widget);// protected RVA = 0x920400 vtable offset = 0x20
    void _NV__setup(MyGUI::Widget* widget);// protected RVA = 0x920400 vtable offset = 0x20
    virtual void clearData(MyGUI::Widget* widget);// protected RVA = 0x920730 vtable offset = 0x28
    void _NV_clearData(MyGUI::Widget* widget);// protected RVA = 0x920730 vtable offset = 0x28
    virtual void show(MyGUI::Widget* _sender, const MyGUI::types::TPoint<int>& _point);// protected RVA = 0x91FC20 vtable offset = 0x30
    void _NV_show(MyGUI::Widget* _sender, const MyGUI::types::TPoint<int>& _point);// protected RVA = 0x91FC20 vtable offset = 0x30
    virtual void setContent(MyGUI::Widget* widget);// protected RVA = 0x922AE0 vtable offset = 0x38
    void _NV_setContent(MyGUI::Widget* widget);// protected RVA = 0x922AE0 vtable offset = 0x38
    void showText(MyGUI::Widget* widget);// protected RVA = 0x921010
    void showMultiLine(MyGUI::Widget* widget);// protected RVA = 0x921120
    void showGameData(const hand& h);// protected RVA = 0x9216B0
    void showGameData(MyGUI::Widget* widget);// protected RVA = 0x922220
    virtual void setVisible(bool visible);// protected RVA = 0x91E7A0 vtable offset = 0x40
    void _NV_setVisible(bool visible);// protected RVA = 0x91E7A0 vtable offset = 0x40
    virtual void setPosition(const MyGUI::types::TPoint<int>& _point);// protected RVA = 0x91ECC0 vtable offset = 0x48
    void _NV_setPosition(const MyGUI::types::TPoint<int>& _point);// protected RVA = 0x91ECC0 vtable offset = 0x48
    void addLine(const std::string& textLeft, const std::string& textRight);// protected RVA = 0x920ED0
    void clearLines();// protected RVA = 0x91ED50
    void notifyToolTip(MyGUI::Widget* _sender, const MyGUI::ToolTipInfo& _info);// protected RVA = 0x91E7C0
    MyGUI::Widget* panel; // 0x8 Member
    Ogre::vector<ToolTip::ToolTipLine*>::type lines; // 0x10 Member
    int panelWidth; // 0x30 Member
    float lineMarginH; // 0x34 Member
    int panelMarginV; // 0x38 Member
    int lineSpacing; // 0x3C Member
    MyGUI::Widget* caller; // 0x40 Member
    // no_addr class ToolTip & operator=(const class ToolTip & _a1);// public missing arg names
    // no_addr void __dflt_ctor_closure();// public
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class ToolTipFixed : public ToolTip
{
public:
    // ToolTip offset = 0x0, length = 0x48
    // no_addr void ToolTipFixed(const class ToolTipFixed & _a1);// public missing arg names
    ToolTipFixed(MyGUI::Widget* panel, MyGUI::Widget* parentPanel);// public RVA = 0x91FC90
    ToolTipFixed* _CONSTRUCTOR(MyGUI::Widget* panel, MyGUI::Widget* parentPanel);// public RVA = 0x91FC90
    virtual ~ToolTipFixed();// public RVA = 0x91FD40 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x91FD40 vtable offset = 0x0
    virtual void clear(MyGUI::Widget* widget) override;// public RVA = 0x91EBC0 vtable offset = 0x0
    void _NV_clear(MyGUI::Widget* widget);// public RVA = 0x91EBC0 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x91E930 vtable offset = 0x0
    void _NV_update();// public RVA = 0x91E930 vtable offset = 0x0
    void setBottomPosition(const MyGUI::types::TPoint<int>& position);// public RVA = 0x91E870
    virtual void setVisible(bool visible) override;// protected RVA = 0x91E830 vtable offset = 0x0
    void _NV_setVisible(bool visible);// protected RVA = 0x91E830 vtable offset = 0x0
    virtual void setPosition(const MyGUI::types::TPoint<int>& _point) override;// protected RVA = 0x91EE20 vtable offset = 0x0
    void _NV_setPosition(const MyGUI::types::TPoint<int>& _point);// protected RVA = 0x91EE20 vtable offset = 0x0
    virtual void _setup(MyGUI::Widget* widget) override;// protected RVA = 0x9204D0 vtable offset = 0x0
    void _NV__setup(MyGUI::Widget* widget);// protected RVA = 0x9204D0 vtable offset = 0x0
    void mouseMoved(MyGUI::Widget* _sender, int _left, int _top);// protected RVA = 0x91E9B0
    MyGUI::Widget* parentPanel; // 0x48 Member
    int minHeight; // 0x50 Member
    // no_addr class ToolTipFixed & operator=(const class ToolTipFixed & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class ToolTipStatic : public ToolTip
{
public:
    // ToolTip offset = 0x0, length = 0x48
    // no_addr void ToolTipStatic(const class ToolTipStatic & _a1);// public missing arg names
    ToolTipStatic(MyGUI::Widget* panel);// public RVA = 0x920010
    ToolTipStatic* _CONSTRUCTOR(MyGUI::Widget* panel);// public RVA = 0x920010
    virtual ~ToolTipStatic();// public RVA = 0x9200A0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x9200A0 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x91F0A0 vtable offset = 0x0
    void _NV_update();// public RVA = 0x91F0A0 vtable offset = 0x0
    virtual void clear(MyGUI::Widget* widget) override;// public RVA = 0x91EC40 vtable offset = 0x0
    void _NV_clear(MyGUI::Widget* widget);// public RVA = 0x91EC40 vtable offset = 0x0
    virtual void _setup(MyGUI::Widget* widget) override;// protected RVA = 0x920660 vtable offset = 0x0
    void _NV__setup(MyGUI::Widget* widget);// protected RVA = 0x920660 vtable offset = 0x0
    virtual void setVisible(bool visible) override;// protected RVA = 0x91E8B0 vtable offset = 0x0
    void _NV_setVisible(bool visible);// protected RVA = 0x91E8B0 vtable offset = 0x0
    virtual void setPosition(const MyGUI::types::TPoint<int>& _point) override;// protected RVA = 0x91E8C0 vtable offset = 0x0
    void _NV_setPosition(const MyGUI::types::TPoint<int>& _point);// protected RVA = 0x91E8C0 vtable offset = 0x0
    void mouseMoved(MyGUI::Widget* _sender, int _left, int _top);// protected RVA = 0x91EAF0
    // no_addr class ToolTipStatic & operator=(const class ToolTipStatic & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class ToolTipDynamic : public ToolTip
{
public:
    // ToolTip offset = 0x0, length = 0x48
    // no_addr void ToolTipDynamic(const class ToolTipDynamic & _a1);// public missing arg names
    ToolTipDynamic();// public RVA = 0x91FF20
    ToolTipDynamic* _CONSTRUCTOR();// public RVA = 0x91FF20
    virtual ~ToolTipDynamic();// public RVA = 0x91FFF0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x91FFF0 vtable offset = 0x0
    virtual void setVisible(bool visible) override;// protected RVA = 0x91EA90 vtable offset = 0x0
    void _NV_setVisible(bool visible);// protected RVA = 0x91EA90 vtable offset = 0x0
    virtual void setPosition(const MyGUI::types::TPoint<int>& _point) override;// protected RVA = 0x91EFE0 vtable offset = 0x0
    void _NV_setPosition(const MyGUI::types::TPoint<int>& _point);// protected RVA = 0x91EFE0 vtable offset = 0x0
    // no_addr class ToolTipDynamic & operator=(const class ToolTipDynamic & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class ToolTipInventory : public ToolTip
{
public:
    // ToolTip offset = 0x0, length = 0x48
    // no_addr void ToolTipInventory(const class ToolTipInventory & _a1);// public missing arg names
    ToolTipInventory();// public RVA = 0x91FD60
    ToolTipInventory* _CONSTRUCTOR();// public RVA = 0x91FD60
    virtual ~ToolTipInventory();// public RVA = 0x91FE10 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x91FE10 vtable offset = 0x0
    virtual void _setup(MyGUI::Widget* widget) override;// public RVA = 0x9205A0 vtable offset = 0x0
    void _NV__setup(MyGUI::Widget* widget);// public RVA = 0x9205A0 vtable offset = 0x0
    virtual void setup(MyGUI::Widget* widget);// public RVA = 0x91FE30 vtable offset = 0x50
    void _NV_setup(MyGUI::Widget* widget);// public RVA = 0x91FE30 vtable offset = 0x50
    virtual void show(MyGUI::Widget* _sender, const MyGUI::types::TPoint<int>& _point) override;// public RVA = 0x9213C0 vtable offset = 0x0
    void _NV_show(MyGUI::Widget* _sender, const MyGUI::types::TPoint<int>& _point);// public RVA = 0x9213C0 vtable offset = 0x0
    virtual void update() override;// public RVA = 0x91E9E0 vtable offset = 0x0
    void _NV_update();// public RVA = 0x91E9E0 vtable offset = 0x0
    virtual void setContent(MyGUI::Widget* widget) override;// protected RVA = 0x921490 vtable offset = 0x0
    void _NV_setContent(MyGUI::Widget* widget);// protected RVA = 0x921490 vtable offset = 0x0
    virtual void clearData(MyGUI::Widget* widget) override;// protected RVA = 0x920D70 vtable offset = 0x0
    void _NV_clearData(MyGUI::Widget* widget);// protected RVA = 0x920D70 vtable offset = 0x0
    void mouseMoved(MyGUI::Widget* _sender, int _left, int _top);// protected RVA = 0x91EA60
    virtual void setPosition(const MyGUI::types::TPoint<int>& _point) override;// protected RVA = 0x91EF30 vtable offset = 0x0
    void _NV_setPosition(const MyGUI::types::TPoint<int>& _point);// protected RVA = 0x91EF30 vtable offset = 0x0
    ToolTipInventory* compareTooltip; // 0x48 Member
    // no_addr class ToolTipInventory & operator=(const class ToolTipInventory & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};