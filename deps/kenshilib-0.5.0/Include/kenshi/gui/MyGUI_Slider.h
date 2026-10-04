#pragma once

// THIS FILE IS SEPARATE TO MYGUI HEADERS BECAUSE IT'S RECREATED FROM SYMBOLS
// I haven't found a version of MyGUI containing this header

#include <mygui/MyGUI_Widget.h>

namespace MyGUI
{
    class Slider : public Widget
    {
    public:
        // MyGUI::Widget offset = 0x0, length = 0x478
        // Typedef        : EventHandle_SliderPtrInt
        // no_addr static const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & getClassTypeName();// public
        virtual const std::string& getTypeName() = 0;// public vtable offset = 0x0
        // Typedef        : Base
        // Typedef        : RTTIBase
        virtual bool isType(const type_info& _a1) = 0;// public vtable offset = 0x0 missing arg names
        // no_addr void Slider(const class MyGUI::Slider & _a1);// public missing arg names
        // no_addr void Slider();// public
        // no_addr void setCaption(const class MyGUI::UString & _a1);// public missing arg names
        // no_addr const class MyGUI::UString & getCaption();// public
        // no_addr void setScrollRange(int _a1, int _a2);// public missing arg names
        // no_addr void setScrollRange(int _a1);// public missing arg names
        // no_addr void setScrollPosition(int _a1);// public missing arg names
        // no_addr int getScrollValue();// public
        delegates::CMultiDelegate2<Slider*, int> eventValueChange; // 0x478 Member
        virtual void initialiseOverride() = 0;// protected vtable offset = 0x0
        virtual void shutdownOverride() = 0;// protected vtable offset = 0x0
        // no_addr void valueChanged(class MyGUI::ScrollBar * _a1, unsigned __int64 _a2);// protected missing arg names
        bool mDisableEvents; // 0x490 Member
        ScrollBar* mScrollbar; // 0x498 Member
        TextBox* mSliderName; // 0x4A0 Member
        TextBox* mSliderValue; // 0x4A8 Member
        int mLoValue; // 0x4B0 Member
        int mHiValue; // 0x4B4 Member
        virtual ~Slider() = 0;// public vtable offset = 0x0
        // no_addr class MyGUI::Slider & operator=(const class MyGUI::Slider & _a1);// public missing arg names
        // no_addr void __local_vftable_ctor_closure();// public
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x10 missing arg names
    };

}