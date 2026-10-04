#pragma once

// THIS FILE IS SEPARATE TO MYGUI HEADERS BECAUSE IT'S RECREATED FROM SYMBOLS
// I haven't found a version of MyGUI containing this header

#include <mygui/MyGUI_Widget.h>
#include "MyGUI_Slider.h"

namespace MyGUI
{
    class MultiSlider : public Widget
    {
    public:
        // MyGUI::Widget offset = 0x0, length = 0x478
        // Typedef        : EventHandle_MultiSliderPtrSliderPtrIntInt
        // no_addr static const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & getClassTypeName();// public
        virtual const std::string& getTypeName() = 0;// public vtable offset = 0x0
        // Typedef        : Base
        // Typedef        : RTTIBase
        virtual bool isType(const type_info& _a1) = 0;// public vtable offset = 0x0 missing arg names
        // no_addr void MultiSlider(const class MyGUI::MultiSlider & _a1);// public missing arg names
        // no_addr void MultiSlider();// public
        // no_addr void setCaption(const class MyGUI::UString & _a1);// public missing arg names
        // no_addr const class MyGUI::UString & getCaption();// public
        // no_addr void setSliderCaption(const unsigned __int64 _a1, const class MyGUI::UString & _a2);// public missing arg names
        // no_addr const class MyGUI::UString & getSliderCaption(const unsigned __int64 _a1);// public missing arg names
        // no_addr class MyGUI::Slider * getSlider(const unsigned __int64 _a1);// public missing arg names
        delegates::CMultiDelegate4<MultiSlider*, Slider*, int, int> eventValueChange; // 0x478 Member
        virtual void initialiseOverride() override = 0;// protected vtable offset = 0x0
        virtual void shutdownOverride() override = 0;// protected vtable offset = 0x0
        // no_addr void valueChanged(class MyGUI::Slider * _a1, int _a2);// protected missing arg names
        MyGUI::TextBox* mName; // 0x490 Member
        bool mDisableEvents; // 0x498 Member
        std::vector<Slider*> mSliders; // 0x4A0 Member
        virtual ~MultiSlider() = 0;// public vtable offset = 0x0
        // no_addr class MyGUI::MultiSlider & operator=(const class MyGUI::MultiSlider & _a1);// public missing arg names
        // no_addr void __local_vftable_ctor_closure();// public
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x10 missing arg names
    };
}