#pragma once

#include <kenshi/util/iVector2.h>
#include <ogre/Ogre.h>
#include <mygui/MyGUI.h>
#include <mygui/common/itembox/BaseCellView.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/OgreUnordered.h>

class PortraitData;
class Character;
class ActivePlatoon;

class PortraitImage : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    // no_addr void PortraitImage(const class PortraitImage & _a1);// public missing arg names
    PortraitImage(const std::string& textureName, unsigned short index, const MyGUI::types::TCoord<int>& coords, const Ogre::TRect<float>& rect);// private RVA = 0x4121A0
    PortraitImage* _CONSTRUCTOR(const std::string& textureName, unsigned short index, const MyGUI::types::TCoord<int>& coords, const Ogre::TRect<float>& rect);// private RVA = 0x4121A0
    void updateImageWidget(MyGUI::ImageBox* image, bool force);// private RVA = 0x412060
    unsigned short index; // 0x0 Member
    bool created; // 0x2 Member
    std::string textureName; // 0x8 Member
    Ogre::TRect<float> textureRect; // 0x30 Member
    MyGUI::types::TCoord<int> coords; // 0x40 Member
    ~PortraitImage();// public RVA = 0x41E230
    void _DESTRUCTOR();// public RVA = 0x41E230
    // no_addr class PortraitImage & operator=(const class PortraitImage & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};

class PortraitManager : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    // Typedef        : CharaterPotraitMap
    // Typedef        : PortraitUsageList
    static PortraitManager* getInstance();// public RVA = 0x37D850
    void init();// public RVA = 0x412B80
    void destroy();// public RVA = 0x414E30
    void setUpdateEnabled(bool enabled);// public RVA = 0x411B60
    void reloadPortraits();// public RVA = 0x4120F0
    void reloadTexture();// public RVA = 0x412810
    void saveTexture();// public RVA = 0x414500
    PortraitData* getPortrait(const hand& characterHandle);// public RVA = 0x413230
    void updatePortrait(const hand& oldHandle, const hand& newHandle);// public RVA = 0x412EA0
    bool updatePortraitImage(const hand& characterHandle);// public RVA = 0x4143C0
    void clearPortraits(bool loadTexture);// public RVA = 0x4140D0
    void setImageWidget(const hand& characterHandle, MyGUI::ImageBox* imgBox, bool force);// public RVA = 0x412F50
    bool getCharacterFlashing(Character* character);// public RVA = 0x411C20
    bool getCharacterInCombat(Character* character);// public RVA = 0x411C00
    bool getCharacterDown(Character* character);// public RVA = 0x411D10
    bool getSquadFlashing(ActivePlatoon* platoon);// public RVA = 0x411D70
    // no_addr void PortraitManager(const class PortraitManager & _a1);// public missing arg names
    PortraitManager();// private RVA = 0x413C00
    PortraitManager* _CONSTRUCTOR();// private RVA = 0x413C00
    ~PortraitManager();// private RVA = 0x413CD0
    void _DESTRUCTOR();// private RVA = 0x413CD0
    bool createPortraitImage(PortraitImage*& portraitImage, Character* character);// private RVA = 0x412B90
    void getNextIndices(unsigned char& indexX, unsigned char& indexY);// private RVA = 0x411B80
    void setImageWidgetNPC(Character* character, MyGUI::ImageBox* imageBox);// private RVA = 0x411FB0
    iVector2 textureSize; // 0x0 Member
    std::string textureName; // 0x8 Member
    iVector2 texturePortraitSize; // 0x30 Member
    unsigned short maxIndex; // 0x38 Member
    Ogre::SharedPtr<Ogre::Texture> texture; // 0x40 Member
    std::vector<std::pair<bool, PortraitImage*>, Ogre::STLAllocator<std::pair<bool, PortraitImage*>, Ogre::GeneralAllocPolicy > > images; // 0x50 Member
    std::map<hand, std::pair<PortraitData*, PortraitImage*>, std::less<hand>, Ogre::STLAllocator<std::pair<hand const, std::pair<PortraitData*, PortraitImage*> >, Ogre::GeneralAllocPolicy > > characterPortraits; // 0x70 Member
    bool* imagesIndices; // 0x98 Member
    bool disableUpdate; // 0xA0 Member
    // no_addr class PortraitManager & operator=(const class PortraitManager & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// private missing arg names
};

class PortraitMainCellView : public wraps::BaseCellView<PortraitData*>, public Ogre::GeneralAllocatedObject
{
public:
    // wraps::BaseCellView<PortraitData *> offset = 0x0, length = 0xA0
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0xA1, length = 0x1
    // no_addr void PortraitMainCellView(const class PortraitMainCellView & _a1);// public missing arg names
    PortraitMainCellView(MyGUI::Widget* parent);// public RVA = 0x413860
    PortraitMainCellView* _CONSTRUCTOR(MyGUI::Widget* parent);// public RVA = 0x413860
    virtual ~PortraitMainCellView();// public RVA = 0x425CD0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x425CD0 vtable offset = 0x0
    void update(const MyGUI::IBDrawItemInfo& _info, PortraitData* _data);// public RVA = 0x4149D0
    static void getCellDimension(MyGUI::Widget* _sender, MyGUI::types::TCoord<int>& _coord, bool _drop);// public RVA = 0x412700
    void resize(int size);// private RVA = 0x411A10
    void setName(const std::string& newName);// private RVA = 0x4123F0
    hand characterHandle; // 0xA8 Member
    std::string name; // 0xC8 Member
    MyGUI::Button* border; // 0xF0 Member
    MyGUI::ImageBox* imageBackground; // 0xF8 Member
    MyGUI::ImageBox* imagePortrait; // 0x100 Member
    MyGUI::ImageBox* imageOverlayBack; // 0x108 Member
    MyGUI::ImageBox* imageOverlayFront; // 0x110 Member
    MyGUI::TextBox* textName; // 0x118 Member
    static ogre_unordered_map<MyGUI::Widget*, unsigned short>::type ItemBoxSizes; // RVA = 0x1D730E0 Static Member
    static ogre_unordered_map<ActivePlatoon*, unsigned short>::type ItemBoxSizesPlatoon; // RVA = 0x1D73120 Static Member
    static std::string TextNameFontNormal; // RVA = 0x1D730B0 Static Member
    static std::string TextNameFontSmall; // RVA = 0x1D73088 Static Member
    // no_addr class PortraitMainCellView & operator=(const class PortraitMainCellView & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};