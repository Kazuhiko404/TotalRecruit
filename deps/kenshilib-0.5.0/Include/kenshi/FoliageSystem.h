#pragma once

#include <kenshi/util/OgreUnordered.h>
#include <kenshi/util/lektor.h>
#include <kenshi/Enums.h>
#include <ogre/OgreVector4.h>
#include <ogre/OgreMatrix4.h>

class GameData;
class ZoneMap;
class Building;
class RealWorldEditableImage;
class SimplePhysXEntity;
class SoundEmitter;

namespace Forests
{
    class TreeLoader3D;
    class PagedGeometry;
    class GrassLoader;
}

class FoliageSystem : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
    // Typedef        : GameDataPagedGeometryMap
    class EntData : public Ogre::GeneralAllocatedObject
    {
    public:
        // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
        // no_addr void EntData(const class FoliageSystem::EntData & _a1);// public missing arg names
        EntData(GameData* dat, bool rld, GameData* layer);// public RVA = 0x6CD720
        EntData* _CONSTRUCTOR(GameData* dat, bool rld, GameData* layer);// public RVA = 0x6CD720
        ~EntData();// public RVA = 0x6CBC20
        void _DESTRUCTOR();// public RVA = 0x6CBC20
        Ogre::Entity* createEntity1();// public RVA = 0x6D0E90
        void createEntity1Callback(Ogre::SharedPtr<Ogre::Resource> _a1, void* entityData);// public RVA = 0x6CEC00
        Ogre::Entity* createEntity2();// public RVA = 0x6D1290
        void createEntity2Callback(Ogre::SharedPtr<Ogre::Resource> _a1, void* entityData);// public RVA = 0x6D0940
        Ogre::Entity* selectBiomeEntity(const Ogre::Vector3& position);// public RVA = 0x6CE700
        bool isLoaded() const;// public RVA = 0x6CB4C0
        GameData* data; // 0x0 Member
        float maxSlope; // 0x8 Member
        float minSlope; // 0xC Member
        float chance; // 0x10 Member
        float minAltitude; // 0x14 Member
        float maxAltitude; // 0x18 Member
        bool slopeAlign; // 0x1C Member
        bool floating; // 0x1D Member
        bool uniformScale; // 0x1E Member
        bool accurateTrace; // 0x1F Member
        bool upright; // 0x20 Member
        Ogre::Vector4 scales; // 0x24 Member
        float minV; // 0x34 Member
        float maxV; // 0x38 Member
        bool clustered; // 0x3C Member
        bool limitToGrassAreas; // 0x3D Member
        bool nonBatched; // 0x3E Member
        float carveRadius; // 0x40 Member
        float grassSpot; // 0x44 Member
        float roadRadius; // 0x48 Member
        float clusterMin; // 0x4C Member
        float clusterMax; // 0x50 Member
        float clusterRadiusMin; // 0x54 Member
        float clusterRadiusMax; // 0x58 Member
        float viewDistance; // 0x5C Member
        int seedOffset; // 0x60 Member
        bool biomeSeparation; // 0x64 Member
        float childClusterRadius; // 0x68 Member
        std::string xmlFilename; // 0x70 Member
        Ogre::Entity* entity1; // 0x98 Member
        Ogre::Entity* entity2; // 0xA0 Member
        ogre_unordered_map<GameData*, Ogre::Entity*>::type extraEntities; // 0xA8 Member
        bool entity1Loaded; // 0xE8 Member
        bool entity2Loaded; // 0xE9 Member
        lektor<FoliageSystem::EntData*> subEnts; // 0xF0 Member
        Ogre::Vector3 position; // 0x108 Member
        itemType type; // 0x114 Member
        unsigned int referenceCount; // 0x118 Member
        // no_addr class FoliageSystem::EntData & operator=(const class FoliageSystem::EntData & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    // no_addr void FoliageSystem(const class FoliageSystem & _a1);// public missing arg names
    FoliageSystem(ZoneMap* _map, bool firstTimeLoad, bool buildOverlay, bool reloadData);// public RVA = 0x6D33F0
    FoliageSystem* _CONSTRUCTOR(ZoneMap* _map, bool firstTimeLoad, bool buildOverlay, bool reloadData);// public RVA = 0x6D33F0
    ~FoliageSystem();// public RVA = 0x6CBFC0
    void _DESTRUCTOR();// public RVA = 0x6CBFC0
    void update();// public RVA = 0x6CBA30
    unsigned int getCollisionHash() const;// public RVA = 0x3B2A30
    int getNavmeshCarvers(lektor<Ogre::Vector4>& carvers) const;// public RVA = 0x6CCDB0
    static void clearStaticData();// public RVA = 0x6CC4B0
    class TreeData : public Ogre::GeneralAllocatedObject
    {
    public:
        // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x0, length = 0x1
        Ogre::Vector3 position; // 0x0 Member
        Ogre::Degree rotation; // 0xC Member
        float scale; // 0x10 Member
        Forests::TreeLoader3D* target; // 0x18 Member
        Building* building; // 0x20 Member
        Ogre::Matrix4 transform; // 0x28 Member
        TreeData();// public RVA = 0x6D9890
        TreeData* _CONSTRUCTOR();// public RVA = 0x6D9890
        ~TreeData();// public RVA = 0x6D7ED0
        void _DESTRUCTOR();// public RVA = 0x6D7ED0
        FoliageSystem::TreeData& operator=(const FoliageSystem::TreeData& __that);// public RVA = 0x6D9E40
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    void setupGrass(GameData* layerData, Forests::PagedGeometry* paged, Forests::GrassLoader* grassLoader, int biomeCode);// private RVA = 0x6CF900
    void setupFoliage(GameData* dat, Forests::PagedGeometry* paged, Forests::TreeLoader3D* treeLoader, int biomeCode);// private RVA = 0x6D2D00
    void loadEnts(GameData* parent, lektor<FoliageSystem::EntData*>& entities, bool sub);// private RVA = 0x6CEF90
    bool spawn(FoliageSystem::EntData* ent, Ogre::Vector3& position, Forests::TreeLoader3D* _treeLoader);// private RVA = 0x6D19D0
    bool finalMakeEnt(FoliageSystem::EntData* ent, const Ogre::Vector3& position, const Ogre::Degree& rotationYaw, const float& scale, Forests::TreeLoader3D* treeLoader, Building* building, const Ogre::Matrix4& transform);// private RVA = 0x6D1610
    void setupWind(lektor<FoliageSystem::EntData*>& list, Forests::PagedGeometry* paged);// private RVA = 0x6CCAD0
    // no_addr void generateCoverageMap();// private
    int randomInt(int min, int max);// private RVA = 0x6D7D80
    // no_addr float random();// private
    float random(float min, float max);// private RVA = 0x6D7DB0
    int addGrassLayerToCoverageMap(RealWorldEditableImage* coverageMap, GameData* grassdata, int channel, int biomeCode);// private RVA = 0x6CCF30
    bool reloaded; // 0x0 Member
    bool generateOverlay; // 0x1 Member
    bool firstTimeLoad; // 0x2 Member
    unsigned int collisionHash; // 0x4 Member
    lektor<Forests::PagedGeometry*> pageLayers; // 0x8 Member
    lektor<Forests::TreeLoader3D*> treeListsWithCarvers; // 0x20 Member
    ogre_unordered_set<FoliageSystem::EntData*>::type loadedEnts; // 0x38 Member
    bool loadComplete; // 0x78 Member
    ZoneMap* zoneMap; // 0x80 Member
    RealWorldEditableImage* coverageMap; // 0x88 Member
    ogre_unordered_map<GameData*, int>::type usedEntsCount; // 0x90 Member
    lektor<SimplePhysXEntity*> collisionList; // 0xD0 Member
    lektor<SoundEmitter*> soundEmitters; // 0xE8 Member
    lektor<SoundEmitter*> soundUpdate; // 0x100 Member
    static boost::unordered::unordered_map<Ogre::Entity*, Ogre::FastArray<FoliageSystem::TreeData>, boost::hash<Ogre::Entity*>, std::equal_to<Ogre::Entity*>, Ogre::STLAllocator<std::pair<Ogre::Entity* const, Ogre::FastArray<FoliageSystem::TreeData> >, Ogre::GeneralAllocPolicy > > treesToAdd; // RVA = 0x1F19B90 Static Member
    static ogre_unordered_map<GameData*, FoliageSystem::EntData*>::type allEnts; // RVA = 0x1F19BD0 Static Member
    // no_addr class FoliageSystem & operator=(const class FoliageSystem & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};