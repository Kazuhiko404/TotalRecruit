#pragma once

#include <string>
#include <ogre/OgreVector3.h>
#include "util/OgreUnordered.h"
#include "Enums.h"
#include "GameData.h"

class GameDataCopyStandalone;
class Faction;
class Character;
class RaceGroupData;
class AnimationData;

class AppearanceManager : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    static AppearanceManager* getInstance();// public RVA = 0x53E630
    class DataRange
    {
    public:
        // VTable         : (none)
        std::string name; // 0x8 Member
        short max; // 0x30 Member
        short min; // 0x32 Member
        short mid; // 0x34 Member
        short random_group; // 0x36 Member
        short random_variation; // 0x38 Member
        virtual void clamp(GameData* data) const;// public RVA = 0x71400 vtable offset = 0x0
        void _NV_clamp(GameData* data) const;// public RVA = 0x71400 vtable offset = 0x0
        DataRange(const DataRange& __that);// public RVA = 0x601AB0
        DataRange* _CONSTRUCTOR(const DataRange& __that);// public RVA = 0x601AB0
        DataRange();// public RVA = 0x961A0
        DataRange* _CONSTRUCTOR();// public RVA = 0x961A0
        ~DataRange();// public RVA = 0x961D0
        void _DESTRUCTOR();// public RVA = 0x961D0
        DataRange& operator=(const DataRange& __that);// public RVA = 0x9FC90
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class DataRangePose : public DataRange
    {
    public:
        // DataRange offset = 0x0, length = 0x40
        std::string pose_name; // 0x40 Member
        std::string pose_opposite_name; // 0x68 Member
        virtual void clamp(GameData* data) const override;// public RVA = 0x714E0 vtable offset = 0x0
        void _NV_clamp(GameData* data) const;// public RVA = 0x714E0 vtable offset = 0x0
        DataRangePose(const DataRangePose& __that);// public RVA = 0x601B50
        DataRangePose* _CONSTRUCTOR(const DataRangePose& __that);// public RVA = 0x601B50
        DataRangePose();// public RVA = 0x96210
        DataRangePose* _CONSTRUCTOR();// public RVA = 0x96210
        ~DataRangePose();// public RVA = 0x96260
        void _DESTRUCTOR();// public RVA = 0x96260
        DataRangePose& operator=(const DataRangePose& __that);// public RVA = 0x9FD10
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class DataRangeVector : public DataRange
    {
    public:
        // DataRange offset = 0x0, length = 0x40
        void addValue(const Ogre::Vector3& value);// public RVA = 0x6E690
        Ogre::Vector3 getValue(int value) const;// public RVA = 0x6E300
        int getValueIndex(const Ogre::Vector3& value) const;// public RVA = 0x6E420
        int size() const;// public RVA = 0x6E570
        virtual void clamp(GameData* data) const override;// public RVA = 0x71790 vtable offset = 0x0
        void _NV_clamp(GameData* data) const;// public RVA = 0x71790 vtable offset = 0x0
        Ogre::FastArray<Ogre::Vector3> values; // 0x40 Member
        DataRangeVector(const DataRangeVector& __that);// public RVA = 0x603160
        DataRangeVector* _CONSTRUCTOR(const DataRangeVector& __that);// public RVA = 0x603160
        DataRangeVector();// public RVA = 0x96310
        DataRangeVector* _CONSTRUCTOR();// public RVA = 0x96310
        ~DataRangeVector();// public RVA = 0x96350
        void _DESTRUCTOR();// public RVA = 0x96350
        DataRangeVector& operator=(const DataRangeVector& __that);// public RVA = 0x9FDC0
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class DataCategory
    {
    public:
        enum Enum
        {
            None,
            Face,
            Body,
            Hair,
            Personality
        };

        DataCategory(const DataCategory& __that);// public RVA = 0xA8820
        DataCategory* _CONSTRUCTOR(const DataCategory& __that);// public RVA = 0xA8820
        DataCategory(const std::string& categoryStr);// public RVA = 0x6F4D0
        DataCategory* _CONSTRUCTOR(const std::string& categoryStr);// public RVA = 0x6F4D0
        DataCategory(DataCategory::Enum category);// public RVA = 0x6E6E0
        DataCategory* _CONSTRUCTOR(DataCategory::Enum category);// public RVA = 0x6E6E0
        void setValue(const std::string& categoryStr);// public RVA = 0x6F160
        DataCategory::Enum getValue() const;// public RVA = 0x6E070
        Ogre::FastArray<DataRange> ranges; // 0x0 Member
        Ogre::FastArray<DataRangePose> posesRanges; // 0x18 Member
        Ogre::FastArray<DataRangeVector> vectorRanges; // 0x30 Member
        DataCategory::Enum value; // 0x48 Member
        ~DataCategory();// public RVA = 0x9C730
        void _DESTRUCTOR();// public RVA = 0x9C730
        DataCategory& operator=(const DataCategory& __that);// public RVA = 0xA9450
        // no_addr void __dflt_ctor_closure();// public
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    class Gender
    {
    public:
        enum Enum
        {
            MALE,
            FEMALE
        };

        Gender(const std::string& genderStr);// public RVA = 0x6F360
        Gender* _CONSTRUCTOR(const std::string& genderStr);// public RVA = 0x6F360
        Gender(GameData* appearanceData);// public RVA = 0x6F520
        Gender* _CONSTRUCTOR(GameData* appearanceData);// public RVA = 0x6F520
        Gender(Gender::Enum gender);// public RVA = 0x6E0A0
        Gender* _CONSTRUCTOR(Gender::Enum gender);// public RVA = 0x6E0A0
        bool isMale() const;// public RVA = 0x6E0B0
        bool isFemale() const;// public RVA = 0x6E0C0
        std::string toString() const;// public RVA = 0x6E720
        Gender::Enum getValue() const;// public RVA = 0x6E0D0
        Gender::Enum value; // 0x0 Member
        // no_addr void __dflt_ctor_closure();// public
    };
    class AppearanceData
    {
    public:
        AppearanceData(const AppearanceData& __that);// public RVA = 0xB52A0
        AppearanceData* _CONSTRUCTOR(const AppearanceData& __that);// public RVA = 0xB52A0
        AppearanceData();// public RVA = 0xB4B00
        AppearanceData* _CONSTRUCTOR();// public RVA = 0xB4B00
        GameData* race; // 0x0 Member
        Gender gender; // 0x8 Member
        Ogre::FastArray<GameData*> headsData; // 0x10 Member
        boost::unordered::unordered_map<int, Ogre::FastArray<GameData*>, boost::hash<int>, std::equal_to<int>, Ogre::STLAllocator<std::pair<int const, Ogre::FastArray<GameData*> >, Ogre::GeneralAllocPolicy > > attachments; // 0x28 Member
        ogre_unordered_map<DataCategory::Enum, DataCategory>::type categories; // 0x68 Member
        boost::unordered::unordered_map<std::string, DataRange const*, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, DataRange const*>, Ogre::GeneralAllocPolicy > > rangesByName; // 0xA8 Member
        Ogre::FastArray<float> headsDataProb; // 0xE8 Member
        boost::unordered::unordered_map<int, Ogre::FastArray<float>, boost::hash<int>, std::equal_to<int>, Ogre::STLAllocator<std::pair<int const, Ogre::FastArray<float> >, Ogre::GeneralAllocPolicy > > attachmentsProb; // 0x100 Member
        boost::unordered::unordered_map<unsigned __int64, Ogre::FastArray<float>, boost::hash<unsigned __int64>, std::equal_to<unsigned __int64>, Ogre::STLAllocator<std::pair<unsigned __int64 const, Ogre::FastArray<float> >, Ogre::GeneralAllocPolicy > > factionAttachments; // 0x140 Member
        boost::unordered::unordered_map<std::string, std::string, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, std::string >, Ogre::GeneralAllocPolicy > > texturesSkin; // 0x180 Member
        std::string meshFile; // 0x1C0 Member
        ~AppearanceData();// public RVA = 0xB4E90
        void _DESTRUCTOR();// public RVA = 0xB4E90
        AppearanceData& operator=(const AppearanceData& __that);// public RVA = 0xB5BD0
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    GameDataCopyStandalone* createAppearanceData(GameData* race);// public RVA = 0x72C10
    void cleanValidateAppearanceData(GameData* appearanceData);// public RVA = 0x765A0
    void resetAll(GameData* appearanceData, bool clearAttachments);// public RVA = 0x7A450
    void randomiseAll(GameData* appearanceData);// public RVA = 0x7F000
    void randomiseAllManual(GameData* appearanceData, float faceRandomnessMult, float bodyRandomnessMult);// public RVA = 0x7F050
    void randomHair(GameData* appearanceData, Faction* faction);// public RVA = 0x7AD80
    void randomFace(GameData* appearanceData);// public RVA = 0x7B2D0
    void randomFaceManual(GameData* appearanceData, float randomnessMult);// public RVA = 0x7B4C0
    void randomPoses(GameData* appearanceData, float deviation);// public RVA = 0x7B780
    void randomBody(GameData* appearanceData, bool isPlayer);// public RVA = 0x7B900
    void randomBodyManual(GameData* appearanceData, float randomnessMult);// public RVA = 0x7BC70
    void randomGender(GameData* appearanceData, float femaleChance);// public RVA = 0x764C0
    void randomAnimations(GameData* appearanceData, Character* character);// public RVA = 0x6F8F0
    GameData* getRaceData(int index);// public RVA = 0x6E100
    int getRaceIndex(GameData* data);// public RVA = 0x6E120
    int getRacesCount() const;// public RVA = 0x6E150
    bool isSingleGender(GameData* raceData);// public RVA = 0x6F430
    void setGender(GameData* appearanceData, Gender::Enum genderValue);// public RVA = 0x763F0
    void getEditorData(boost::unordered::unordered_map<RaceGroupData*, Ogre::FastArray<GameData*>, boost::hash<RaceGroupData*>, std::equal_to<RaceGroupData*>, Ogre::STLAllocator<std::pair<RaceGroupData* const, Ogre::FastArray<GameData*> >, Ogre::GeneralAllocPolicy > >& _racesGroups, boost::unordered::unordered_map<GameData*, ogre_unordered_map<Gender::Enum, AppearanceData>::type, boost::hash<GameData*>, std::equal_to<GameData*>, Ogre::STLAllocator<std::pair<GameData* const, ogre_unordered_map<Gender::Enum, AppearanceData>::type >, Ogre::GeneralAllocPolicy > >& _raceAppearanceData, bool playableOnly, const Ogre::vector<GameDataReference>::type* filter);// public RVA = 0x79B70
    void setHead(GameData* appearanceData, GameData* headData);// public RVA = 0x70E70
    const std::string& getHeadName(GameData* appearanceData, AppearanceData* appearance);// public RVA = 0x70F20
    void setRandomHead(GameData* raceData, GameData* appearanceData);// public RVA = 0x7C840
    bool getHeadIsValid(GameData* raceData, GameData* appearanceData, GameData* headData);// public RVA = 0x7C9A0
    void setAttachment(GameData* appearanceData, AppearanceData* appearance, const AttachSlot slot, const int index);// public RVA = 0x71300
    const std::string& getAttachmentName(GameData* appearanceData, AppearanceData* appearance, const AttachSlot slot);// public RVA = 0x71080
    void updateModifiers(GameData* appearanceData, GameData* raceData, const Gender& gender);// public RVA = 0x7CAA0
    const Ogre::FastArray<AnimationData*>& getCharacterIdleAnimations() const;// public RVA = 0x5E85D0
    const std::string& getNormalMap_Plain(GameData* race, const Gender& gender) const;// public RVA = 0x6EBC0
    const std::string& getNormalMap_Muscle(GameData* race, const Gender& gender) const;// public RVA = 0x6EDA0
    const std::string& getNormalMap_Starving(GameData* race, const Gender& gender) const;// public RVA = 0x6EF80
    static std::string APPEARANCE_IN_EDITOR_KEY; // RVA = 0x1BD7388 Static Member
    static std::string APPEARANCE_FROM_FILE_KEY; // RVA = 0x1BD73B0 Static Member
    // no_addr void AppearanceManager(const class AppearanceManager & _a1);// public missing arg names
    AppearanceManager();// protected RVA = 0x7F0B0
    AppearanceManager* _CONSTRUCTOR();// protected RVA = 0x7F0B0
    ~AppearanceManager();// protected RVA = 0x76530
    void _DESTRUCTOR();// protected RVA = 0x76530
    void setupData();// protected RVA = 0x7D440
    void setupFaces(GameData* raceData, AppearanceData& data);// protected RVA = 0x74640
    void setupFactionHair(AppearanceData& data, Faction* faction, AttachSlot slot);// protected RVA = 0x75140
    void loadEditorXMLData(const std::string& fileName, GameData* race);// protected RVA = 0x77050
    bool isValidAttachment(const std::string& attachmentId, const AppearanceData& data, const AttachSlot slot);// protected RVA = 0x6E780
    const std::string& getRandomAttachment(const AppearanceData& data, const AttachSlot slot, Faction* faction);// protected RVA = 0x6E8F0
    const std::string& getNoneAttachment(const AppearanceData& data, const AttachSlot slot);// protected RVA = 0x6F5D0
    GameData* getRandomValue(const Ogre::FastArray<GameData*>& values, const Ogre::FastArray<float>& probabilities);// protected RVA = 0x6E160
    bool getCategoryRange(const AppearanceData& data, const std::string& name, int& min, int& max, int& mid) const;// protected RVA = 0x6EAF0
    void randomiseRanges(const Ogre::FastArray<DataRangeVector>& ranges, GameData* appearanceData);// protected RVA = 0x70420
    void randomiseRanges(const Ogre::FastArray<DataRangePose>& ranges, GameData* appearanceData, float deviation);// protected RVA = 0x6FFD0
    void randomiseRanges(const Ogre::FastArray<DataRange>& ranges, GameData* appearanceData);// protected RVA = 0x6FCB0
    int randomFeatureSingle(const DataRangePose& range, float randomValue) const;// protected RVA = 0x6E040
    int randomFeatureSingle(const DataRange& range, float randomValue) const;// protected RVA = 0x6E010
    Ogre::Vector3 randomFeatureVector(const DataRangeVector& range, float randomValue) const;// protected RVA = 0x6E580
    int randomFeatureSingleManual(const DataRangePose& range, float randomnessMult) const;// protected RVA = 0x6E270
    int randomFeatureSingleManual(const DataRange& range, float randomnessMult) const;// protected RVA = 0x6E1E0
    Ogre::Vector3 randomFeatureVectorManual(const DataRangeVector& range, float randomnessMult) const;// protected RVA = 0x6E5E0
    GameData* getValidateRace(GameData* appearanceData);// protected RVA = 0x74EC0
    void setMaterialTextures(GameData* raceData, Gender::Enum gender, boost::unordered::unordered_map<std::string, std::string, boost::hash<std::string >, std::equal_to<std::string >, Ogre::STLAllocator<std::pair<std::string const, std::string >, Ogre::GeneralAllocPolicy > >& textures) const;// protected RVA = 0x70750
    const std::string& getBodyMesh(GameData* raceData, Gender::Enum gender) const;// protected RVA = 0x6EAB0
    static Ogre::Entity* switchBodyMesh(Ogre::Entity* entity, Ogre::SharedPtr<Ogre::Mesh> newMesh);// protected RVA = 0x71920
    static Ogre::Entity* createMorphedMesh(const std::string& name, Ogre::Entity* base, GameData* appearanceData, bool shaved);// protected RVA = 0x71A80
    Ogre::Entity* getMorphedBodyMesh(GameData* appearanceData, Ogre::Entity* entity, bool alwaysUnique, float ugliness, bool shaved);// protected RVA = 0x7C030
    Ogre::Entity* getBodyMeshForEditor(Ogre::Entity* base);// protected RVA = 0x71D80
    Ogre::FastArray<GameData*> races; // 0x0 Member
    boost::unordered::unordered_map<GameData*, ogre_unordered_map<Gender::Enum, AppearanceData>::type, boost::hash<GameData*>, std::equal_to<GameData*>, Ogre::STLAllocator<std::pair<GameData* const, ogre_unordered_map<Gender::Enum, AppearanceData>::type >, Ogre::GeneralAllocPolicy > > racesAppearanceData; // 0x18 Member
    Ogre::FastArray<AnimationData*> characterIdleAnimations; // 0x58 Member
    GameDataCopyStandalone* baseAppearanceData; // 0x70 Member
    // no_addr class AppearanceManager & operator=(const class AppearanceManager & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// protected missing arg names
};