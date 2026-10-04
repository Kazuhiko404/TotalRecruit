#pragma once

#include <mygui/common/baselayout/BaseLayout.h>
#include <kenshi/util/lektor.h>
#include <kenshi/GameData.h>
#include <kenshi/AppearanceManager.h>

enum CharacterEditMode
{
    EDIT_NEWGAME,
    EDIT_MIDGAME,
    EDIT_DEBUG
};

class Character;
class OpenSaveFileDialog;
class CharacterEditCamera;
class DatapanelGUI;

namespace MyGUI
{
    class Slider;
    class ListScrollBar;
    class MultiSlider;
}

class CharacterEditWindow : public wraps::BaseLayout, public Ogre::GeneralAllocatedObject
{
public:
    // wraps::BaseLayout offset = 0x0, length = 0xA0
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0xA1, length = 0x1
    // Typedef        : GenderAppearanceDataMap
    // Typedef        : RaceAppearanceDataMap
    // Typedef        : RaceGenderAppearanceMap
    // Typedef        : CharacterRaceGenderAppearanceMap
    // no_addr void CharacterEditWindow(const class CharacterEditWindow & _a1);// public missing arg names
    CharacterEditWindow(const lektor<Character*>& _characters, CharacterEditMode mode, const Ogre::vector<GameDataReference>::type* races);// public RVA = 0x5F4650
    CharacterEditWindow* _CONSTRUCTOR(const lektor<Character*>& _characters, CharacterEditMode mode, const Ogre::vector<GameDataReference>::type* races);// public RVA = 0x5F4650
    virtual ~CharacterEditWindow();// public RVA = 0x5F5590 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x5F5590 vtable offset = 0x0
    void update();// public RVA = 0x5F0F20
    void loadData();// private RVA = 0x5E9000
    void initCharacters();// private RVA = 0x5EFC40
    void nameChanged(MyGUI::EditBox* _sender);// private RVA = 0x5E8C90
    void prevRace(MyGUI::Widget* _sender);// private RVA = 0x5EF3C0
    void nextRace(MyGUI::Widget* _sender);// private RVA = 0x5EF440
    void prevSubRace(MyGUI::Widget* _sender);// private RVA = 0x5EF4C0
    void nextSubRace(MyGUI::Widget* _sender);// private RVA = 0x5EF550
    void updateRace();// private RVA = 0x5EEC10
    void changeGender(MyGUI::Widget* _sender);// private RVA = 0x5EEE20
    void changeAppearanceData(GameData* prevRace, const AppearanceManager::Gender& prevGender);// private RVA = 0x5EA640
    void updateCharacterPoses();// private RVA = 0x5EAB20
    void clearCharacterPoses();// private RVA = 0x5EAB00
    void prevCharacter(MyGUI::Widget* _sender);// private RVA = 0x5F0E80
    void nextCharacter(MyGUI::Widget* _sender);// private RVA = 0x5F0ED0
    void updateCharacter();// private RVA = 0x5EF5E0
    void showCharacter(Character* c, bool on);// private RVA = 0x5E8650
    void changeCategory(MyGUI::Widget* _sender);// private RVA = 0x5EEF30
    void updateCurrentCategory();// private RVA = 0x5EB5D0
    void resetAppearance(MyGUI::Widget* _sender);// private RVA = 0x5EF0F0
    void randomiseAll(MyGUI::Widget* _sender);// private RVA = 0x5EF1B0
    void randomisePart(MyGUI::Widget* _sender);// private RVA = 0x5EF2D0
    void importCharacter(MyGUI::Widget* _sender);// private RVA = 0x5E86D0
    void exportCharacter(MyGUI::Widget* _sender);// private RVA = 0x5E9C90
    void loadImportedCharacter(OpenSaveFileDialog* _dialog, bool accept, std::string result);// private RVA = 0x5F3B00
    void toggleClothes(MyGUI::Widget* _sender);// private RVA = 0x5E96F0
    void setClothesVisible(Character* _character, bool visible);// private RVA = 0x5E8720
    void updateAppearanceSlider(MyGUI::Slider* _sender, int _value);// private RVA = 0x5EA010
    void updateAppearanceSliderPose(MyGUI::Slider* _sender, int _value);// private RVA = 0x5EAE40
    void updateAppearanceSliderVector(MyGUI::Slider* _sender, int _value);// private RVA = 0x5EA0F0
    void updateAppearanceList(MyGUI::ListScrollBar* _sender, const std::string& _value, int _index);// private RVA = 0x5E9360
    void updateAppearanceMultiSlider(MyGUI::MultiSlider* _sender, MyGUI::Slider* _target, int _targetIndex, int _value);// private RVA = 0x5EA1C0
    void updateAnimationIdle(MyGUI::Slider* _sender, int _value);// private RVA = 0x5EA2B0
    void confirmButton(MyGUI::Widget* _sender);// private RVA = 0x5E97C0
    void confirmMessageCallback(int _btn);// private RVA = 0x5E9C50
    void setupUI();// private RVA = 0x5F12C0
    void setUIEnabled(bool enabled);// private RVA = 0x5EEB20
    void refreshUI();// private RVA = 0x5ED940
    void updateLiveObject();// private RVA = 0x5E8770
    void refreshCharacterPoses();// private RVA = 0x5EB2E0
    void exportMeshes();// private RVA = 0x5EA380
    const std::string& getCurrentHeadName() const;// private RVA = 0x5E8780
    const std::string& getCurrentAttachmentName(AttachSlot slot) const;// private RVA = 0x5E88A0
    void setCurrentHeadName(unsigned __int64 index) const;// private RVA = 0x5E8940
    void setCurrentAttachmentName(AttachSlot slot, unsigned __int64 index) const;// private RVA = 0x5E8800
    Ogre::Entity* getCharacterEntity(unsigned __int64 index);// private RVA = 0x5E89E0
    static std::string NONE_VALUE; // RVA = 0x1E739C8 Static Member
    static std::string EXPORT_FOLDER; // RVA = 0x1E739F0 Static Member
    static std::string EXPORT_EXTENSION; // RVA = 0x1E73A18 Static Member
    CharacterEditCamera* camera; // 0xA8 Member
    MyGUI::EditBox* txtName; // 0xB0 Member
    MyGUI::TextBox* txtGender; // 0xB8 Member
    MyGUI::TextBox* txtRace; // 0xC0 Member
    MyGUI::TextBox* txtSubRace; // 0xC8 Member
    MyGUI::TextBox* txtCharIdx; // 0xD0 Member
    MyGUI::Button* importButton; // 0xD8 Member
    MyGUI::Button* exportButton; // 0xE0 Member
    ogre_unordered_map<AppearanceManager::DataCategory::Enum, MyGUI::Button*>::type buttonsCategories; // 0xE8 Member
    MyGUI::Widget* panelTabs; // 0x128 Member
    MyGUI::ScrollView* panelTabsView; // 0x130 Member
    MyGUI::Button* btnPrevGender; // 0x138 Member
    MyGUI::Button* btnNextGender; // 0x140 Member
    MyGUI::Button* btnPrevRace; // 0x148 Member
    MyGUI::Button* btnNextRace; // 0x150 Member
    MyGUI::Button* btnPrevSubRace; // 0x158 Member
    MyGUI::Button* btnNextSubRace; // 0x160 Member
    MyGUI::types::TSize<int> sliderSize; // 0x168 Member
    MyGUI::types::TSize<int> multiSliderSize; // 0x170 Member
    MyGUI::types::TSize<int> listScrollBarSize; // 0x178 Member
    MyGUI::Button* btnReset; // 0x180 Member
    MyGUI::Button* btnRandomiseAll; // 0x188 Member
    MyGUI::Button* btnRandomisePart; // 0x190 Member
    MyGUI::Widget* descriptionPanel; // 0x198 Member
    MyGUI::EditBox* descriptionPanelText; // 0x1A0 Member
    MyGUI::Widget* statsPanel; // 0x1A8 Member
    DatapanelGUI* statsDataPanel; // 0x1B0 Member
    OpenSaveFileDialog* dialog; // 0x1B8 Member
    bool isMidGame; // 0x1C0 Member
    unsigned int multipleCharactersChangedCheck; // 0x1C4 Member
    bool characterEntityUpdated; // 0x1C8 Member
    bool clothVisible; // 0x1C9 Member
    GameDataCopyStandalone* appearanceData; // 0x1D0 Member
    GameData* raceData; // 0x1D8 Member
    AppearanceManager::DataCategory currentCategory; // 0x1E0 Member
    short currentRaceGroupIndex; // 0x230 Member
    short currentSubRaceIndex; // 0x232 Member
    AppearanceManager::Gender currentGender; // 0x234 Member
    ogre_unordered_map<AppearanceManager::Gender::Enum, AppearanceManager::AppearanceData>::type* currentRaceData; // 0x238 Member
    AppearanceManager::AppearanceData* currentEditData; // 0x240 Member
    Ogre::FastArray<AnimationData*> idleAnimations; // 0x248 Member
    Ogre::FastArray<Character*> characters; // 0x260 Member
    Character* character; // 0x278 Member
    unsigned short currentCharacterIdx; // 0x280 Member
    boost::unordered::unordered_map<Character*, boost::unordered::unordered_map<std::pair<GameData*, enum AppearanceManager::Gender::Enum>, GameDataCopyStandalone*, boost::hash<std::pair<GameData*, enum AppearanceManager::Gender::Enum> >, std::equal_to<std::pair<GameData*, enum AppearanceManager::Gender::Enum> >, Ogre::STLAllocator<std::pair<std::pair<GameData*, enum AppearanceManager::Gender::Enum> const, GameDataCopyStandalone*>, Ogre::GeneralAllocPolicy > >, boost::hash<Character*>, std::equal_to<Character*>, Ogre::STLAllocator<std::pair<Character* const, boost::unordered::unordered_map<std::pair<GameData*, enum AppearanceManager::Gender::Enum>, GameDataCopyStandalone*, boost::hash<std::pair<GameData*, enum AppearanceManager::Gender::Enum> >, std::equal_to<std::pair<GameData*, enum AppearanceManager::Gender::Enum> >, Ogre::STLAllocator<std::pair<std::pair<GameData*, enum AppearanceManager::Gender::Enum> const, GameDataCopyStandalone*>, Ogre::GeneralAllocPolicy > > >, Ogre::GeneralAllocPolicy > > charactersAppearanceByRace; // 0x288 Member
    AnimationData* currentAnimation; // 0x2C8 Member
    const Ogre::vector<GameDataReference>::type* raceLimits; // 0x2D0 Member
    boost::unordered::unordered_map<RaceGroupData*, Ogre::FastArray<GameData*>, boost::hash<RaceGroupData*>, std::equal_to<RaceGroupData*>, Ogre::STLAllocator<std::pair<RaceGroupData* const, Ogre::FastArray<GameData*> >, Ogre::GeneralAllocPolicy > > races; // 0x2D8 Member
    Ogre::FastArray<RaceGroupData*> racesGroups; // 0x318 Member
    boost::unordered::unordered_map<GameData*, ogre_unordered_map<AppearanceManager::Gender::Enum, AppearanceManager::AppearanceData>::type, boost::hash<GameData*>, std::equal_to<GameData*>, Ogre::STLAllocator<std::pair<GameData* const, ogre_unordered_map<AppearanceManager::Gender::Enum, AppearanceManager::AppearanceData>::type >, Ogre::GeneralAllocPolicy > > raceAppearanceData; // 0x330 Member
    boost::unordered::unordered_map<std::string, Ogre::Pose*, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, Ogre::Pose*>, Ogre::GeneralAllocPolicy > > poses; // 0x370 Member
    boost::unordered::unordered_map<Character*, boost::unordered::unordered_map<std::string, float, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, float>, Ogre::GeneralAllocPolicy > >, boost::hash<Character*>, std::equal_to<Character*>, Ogre::STLAllocator<std::pair<Character* const, boost::unordered::unordered_map<std::string, float, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, float>, Ogre::GeneralAllocPolicy > > >, Ogre::GeneralAllocPolicy > > posesCharactersValues; // 0x3B0 Member
    bool showNamesOption; // 0x3F0 Member
    Ogre::FastArray<Ogre::Vector3> initialPositions; // 0x3F8 Member
    Ogre::SceneNode* editorRootNode; // 0x410 Member
    Ogre::SceneNode* characterPositionNode; // 0x418 Member
    Ogre::SceneNode* characterRotationNode; // 0x420 Member
    Ogre::CompositorWorkspace* workspace; // 0x428 Member
    Ogre::Entity* platform; // 0x430 Member
    bool requestUpdateLiveObject; // 0x438 Member
    // no_addr class CharacterEditWindow & operator=(const class CharacterEditWindow & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};