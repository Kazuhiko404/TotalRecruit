#pragma once

// THIS FILE IS SEPARATE TO MYGUI HEADERS BECAUSE IT'S RECREATED FROM SYMBOLS
// I haven't found a version of MyGUI containing this header

#include <mygui/MyGUI_ScrollBar.h>

namespace MyGUI
{
    class ListScrollBar : public ScrollBar
    {
    public:
        // MyGUI::ScrollBar offset = 0x0, length = 0x510
        // Typedef        : EventHandle_ListScrollBarPtrStringInt
        // no_addr static const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & getClassTypeName();// public
        virtual const std::string& getTypeName() = 0;// public vtable offset = 0x0
        // Typedef        : Base
        // Typedef        : RTTIBase
        virtual bool isType(const type_info& _a1) = 0;// public vtable offset = 0x0 missing arg names
        // no_addr void ListScrollBar(const class MyGUI::ListScrollBar & _a1);// public missing arg names
        // no_addr void ListScrollBar();// public
        // no_addr void setCaption(const class MyGUI::UString & _a1);// public missing arg names
        // no_addr const class MyGUI::UString & getCaption();// public
        // no_addr const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & getValueSelected();// public
        // no_addr void setValueSelected(const class std::basic_string<char,std::char_traits<char>,std::allocator<char> > & _a1);// public missing arg names
        // no_addr unsigned __int64 getValueSelectedIndex();// public
        // no_addr void setValueSelectedIndex(const unsigned __int64 _a1);// public missing arg names
        // no_addr void setValues(const class std::vector<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::allocator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > > & _a1);// public missing arg names
        virtual void setEnabled(bool _a1) override = 0;// public vtable offset = 0x0 missing arg names
        MyGUI::delegates::CMultiDelegate3<MyGUI::ListScrollBar*, std::string const&, int> eventValueChange; // 0x510 Member
        virtual void initialiseOverride() override = 0;// protected vtable offset = 0x0
        virtual void shutdownOverride() override = 0;// protected vtable offset = 0x0
        // no_addr void valueChanged(class MyGUI::ScrollBar * _a1, unsigned __int64 _a2);// protected missing arg names
        bool disableEvents; // 0x528 Member
        MyGUI::TextBox* mListName; // 0x530 Member
        MyGUI::TextBox* mListValue; // 0x538 Member
        MyGUI::TextBox* mListIndex; // 0x540 Member
        std::vector<std::string, std::allocator<std::string > > mValues; // 0x548 Member
        virtual ~ListScrollBar() = 0;// public vtable offset = 0x0
        // no_addr class MyGUI::ListScrollBar & operator=(const class MyGUI::ListScrollBar & _a1);// public missing arg names
        // no_addr void __local_vftable_ctor_closure();// public
        // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x10 missing arg names
    };
}