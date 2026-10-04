#pragma once

#include <kenshi/NavMesh.h>

class hkaiNavMesh;
class hkaiNavMeshGenerationSettings;

class NavMeshSeeds
{
public:
    // Typedef        : SeedMap
    std::map<ZoneMap*, lektor<Ogre::Vector3>, std::less<ZoneMap*>, Ogre::STLAllocator<std::pair<ZoneMap* const, lektor<Ogre::Vector3> >, Ogre::GeneralAllocPolicy > > seedPoints; // 0x0 Member
    void load();// public RVA = 0x3C4B30
    void save();// public RVA = 0x3C3240
    void addSeedPoint(const Ogre::Vector3& p, bool terrain);// public RVA = 0x3C4D00
    int removeSeedPoints(const Ogre::Vector3& pos, float radius);// public RVA = 0x3C33B0
    int clearSeedPoints(ZoneMap* zone);// public RVA = 0x3C5010
    int removeIsland(const Ogre::Vector3& pos, float radius);// public RVA = 0x3C3890
    void projectToTerrain(ZoneMap* zone);// public RVA = 0x3C51A0
    const lektor<Ogre::Vector3>* getSeedPoints(ZoneMap* zone) const;// public RVA = 0x3C0BE0
    // no_addr void NavMeshSeeds(const class NavMeshSeeds & _a1);// public missing arg names
    NavMeshSeeds();// public RVA = 0x3DFA50
    NavMeshSeeds* _CONSTRUCTOR();// public RVA = 0x3DFA50
    ~NavMeshSeeds();// public RVA = 0x3DFA70
    void _DESTRUCTOR();// public RVA = 0x3DFA70
    // no_addr class NavMeshSeeds & operator=(const class NavMeshSeeds & _a1);// public missing arg names
    // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
};

class NxBounds3;
class hkGeometry;
class NxVec3;
class hkAabb;
class hkaiPlaneVolume;
class NxShape;
class hkQTransform;
class hkaiVolume;
class NxTriangleMeshDesc;
class NxMat34;
class NxConvexMeshDesc;

class NavMeshGenerator : public ThreadClass
{
public:
    // ThreadClass offset = 0x0, length = 0x88
    struct Task
    {
        ZoneMap* zone; // 0x0 Member
        lektor<hand> buildings; // 0x8 Member
        unsigned int hash; // 0x20 Member
        Ogre::Vector3 offset; // 0x24 Member
        Ogre::Aabb bounds; // 0x30 Member
        hkaiNavMesh* mesh; // 0x48 Member
        NavInstance* output; // 0x50 Member
        int flags; // 0x58 Member
        NavMeshGenerator::Task* next; // 0x60 Member
        // no_addr void Task(const struct NavMeshGenerator::Task & _a1);// public missing arg names
        Task();// public RVA = 0x3D6A90
        Task* _CONSTRUCTOR();// public RVA = 0x3D6A90
        ~Task();// public RVA = 0x3D4870
        void _DESTRUCTOR();// public RVA = 0x3D4870
        // no_addr struct NavMeshGenerator::Task & operator=(const struct NavMeshGenerator::Task & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    struct TaskQueue
    {
        NavMeshGenerator::Task* front; // 0x0 Member
        NavMeshGenerator::Task** back; // 0x8 Member
        // no_addr void TaskQueue(const struct NavMeshGenerator::TaskQueue & _a1);// public missing arg names
        TaskQueue();// public RVA = 0x3C5980
        TaskQueue* _CONSTRUCTOR();// public RVA = 0x3C5980
        void push(NavMeshGenerator::Task* t);// public RVA = 0x3C6240
        NavMeshGenerator::Task* pop();// public RVA = 0x3C62A0
        boost::shared_mutex mutex; // 0x10 Member
        ~TaskQueue();// public RVA = 0x3D25D0
        void _DESTRUCTOR();// public RVA = 0x3D25D0
        // no_addr struct NavMeshGenerator::TaskQueue & operator=(const struct NavMeshGenerator::TaskQueue & _a1);// public missing arg names
        // no_addr void * __vecDelDtor(unsigned int _a1);// public missing arg names
    };
    enum TaskType
    {
        GENERATE_ZONE,
        GENERATE_SPLICE,
        GENERATE_CARVE,
        GENERATE_INTERIOR,
        GENERATE_STITCH
    };

    NavMeshGenerator::TaskQueue queue; // 0x88 Member
    NavMeshGenerator::TaskQueue done; // 0xB8 Member
    NavMeshGenerator::Task* current; // 0xE8 Member
    NavMesh* navmesh; // 0xF0 Member
    bool needSave; // 0xF8 Member
    hkaiNavMeshGenerationSettings* settings; // 0x100 Member
    bool threadRunning; // 0x108 Member
    bool doingStuff; // 0x109 Member
    boost::shared_mutex taskMutex; // 0x110 Member
    NavMeshGenerator::Task* lockedTask; // 0x130 Member
    NavMeshSeeds seedPoints; // 0x138 Member
    bool lockZone(const iVector2& zone, bool wait);// private RVA = 0x3C6FD0
    void unlockZone(const iVector2& zone);// private RVA = 0x3BF180
    bool isGenerating() const;// private RVA = 0x3B2A20
    void setup();// private RVA = 0x3C44F0
    static void setup(NavInstance* inst, unsigned int uid, hkaiNavMesh* mesh, bool createGraph);// private RVA = 0x3BF1A0
    virtual unsigned long threadProc() override;// private RVA = 0x3CDF10 vtable offset = 0x0
    unsigned long _NV_threadProc();// private RVA = 0x3CDF10 vtable offset = 0x0
    bool updateBT();// private RVA = 0x3CDC50
    void generateTaskBT(NavMeshGenerator::Task* task);// private RVA = 0x3CBA80
    int validateBuildingStates(NavMeshGenerator::Task* task);// private RVA = 0x3C7040
    int carve(NavMeshGenerator::Task* task);// private RVA = 0x3C9790
    int shiftEdge(NavMeshGenerator::Task* t, hkaiNavMesh* mesh, const NavInstance* base);// private RVA = 0x3C59B0
    int shiftHole(NavMeshGenerator::Task* t, hkaiNavMesh* mesh);// private RVA = 0x3C2D40
    int splice(NavMeshGenerator::Task* t);// private RVA = 0x3C9EB0
    int stitch(NavInstance* a, NavInstance* b);// private RVA = 0x3C5310
    int stitchUnloadedZone(NavInstance* a, int bx, int by, bool interiors);// private RVA = 0x3CAD60
    int stitchInterior(NavMeshGenerator::Task* t);// private RVA = 0x3CB6D0
    int stitchExterior(NavMeshGenerator::Task* t);// private RVA = 0x3CB320
    int stitchWithInteriors(NavInstance* a, const lektor<NavInstance*>& list);// private RVA = 0x3C5830
    void revertSettings();// private RVA = 0x3C1F20
    int getSeedPointsFromAdjacentZone(const ZoneMap* zone, const iVector2& dir);// private RVA = 0x3C9300
    NavInstance* getCompletedTask(unsigned int uid, NavMeshGenerator::Task*& task);// private RVA = 0x3C71F0
    void unlockTask(NavMeshGenerator::Task* task);// private RVA = 0x3C7440
    NavMeshGenerator::Task* newTask(ZoneMap* z, const Ogre::Aabb& b, const Ogre::Vector3& o);// private RVA = 0x3BF770
    // no_addr void pushTask(struct NavMeshGenerator::Task * _a1);// private missing arg names
    // no_addr struct NavMeshGenerator::Task * popTask();// private
    // no_addr void NavMeshGenerator(const class NavMeshGenerator & _a1);// public missing arg names
    NavMeshGenerator(NavMesh* nav);// public RVA = 0x3C8500
    NavMeshGenerator* _CONSTRUCTOR(NavMesh* nav);// public RVA = 0x3C8500
    virtual ~NavMeshGenerator();// public RVA = 0x3C8680 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x3C8680 vtable offset = 0x0
    // no_addr void loadSeedPoints();// public
    // no_addr void saveSeedPoints();// public
    // no_addr void addSeedPoint(const class Ogre::Vector3 & _a1);// public missing arg names
    // no_addr void removeSeedPoints(const class Ogre::Vector3 & _a1, float _a2);// public missing arg names
    // no_addr void clearSeedPoints(class ZoneMap * _a1);// public missing arg names
    // no_addr void fixSeedPoints(class ZoneMap * _a1);// public missing arg names
    void addJob(ZoneMap* zone, const lektor<Building*>& buildings, bool splice);// public RVA = 0x3C6640
    void addJob(ZoneMap* zone, const hand& building, unsigned int hash);// public RVA = 0x3C6AB0
    void addJob(ZoneMap* zone, const Ogre::Aabb& box);// public RVA = 0x3C6500
    void addJob(ZoneMap* zone, unsigned int hash);// public RVA = 0x3C6300
    void addStitchJob(NavInstance* s);// public RVA = 0x3C6CA0
    bool hasJob(ZoneMap* zone) const;// public RVA = 0x3BEF80
    void update();// public RVA = 0x3C8790
    static int getCollisionShapes(const NxBounds3& bounds, unsigned int groups);// private RVA = 0x3C3CA0
    static bool createHeightmapMesh(ZoneMap* zone, hkGeometry* geom, const NxVec3& offset);// private RVA = 0x3C11A0
    static bool addWaterPlane(const hkAabb& box, float offset, hkGeometry* geom);// private RVA = 0x3BFD30
    static int generateInteriorMesh(Building* b, hkGeometry* geom, hkaiPlaneVolume* carver, const NxVec3& offset, unsigned int overrideGroups);// private RVA = 0x3C7490
    static int generateBuildingMesh(const NxBounds3& bounds, hkGeometry* geom, const NxVec3& offset);// private RVA = 0x3C8140
    // no_addr static int generateMeshFromShapes(const class NxShape * const * _a1, int _a2, struct hkGeometry * _a3);// private missing arg names
    static void getInteriorCavers(const NxBounds3& bounds, hkaiNavMeshGenerationSettings& settings, const NxVec3& offset);// private RVA = 0x3C3E70
    static int getVolumePoints(const NxShape* shape, const hkQTransform& t, hkArray<hkVector4f, hkContainerHeapAllocator>& points);// private RVA = 0x3BFFF0
    static void getDoorPainters(const NxBounds3& bounds, hkaiNavMeshGenerationSettings& settings, const NxVec3& offset);// private RVA = 0x3C4000
    static int addFoliageCarvers(ZoneMap* zone, hkaiNavMeshGenerationSettings& s);// private RVA = 0x3BF4F0
    static hkaiVolume* createVolume(const NxShape* shape, bool inverted, const NxVec3& offset);// private RVA = 0x3C15C0
    static int addGeometry(const NxTriangleMeshDesc& mesh, const NxMat34& pose, hkGeometry& geom, const NxVec3& offset, int material);// private RVA = 0x3C0960
    static int addGeometry(const NxConvexMeshDesc& mesh, const NxMat34& pose, hkGeometry& geom, const NxVec3& offset, int material);// private RVA = 0x3C06E0
    static int addGeometry(const NxShape* shape, hkGeometry& geom, const NxVec3& offset, int material);// private RVA = 0x3C17D0
    static NxShape** shapeBuffer; // RVA = 0x212BDD8 Static Member
    static int shapeBufferSize; // RVA = 0x1D2A0D0 Static Member
    // no_addr class NavMeshGenerator & operator=(const class NavMeshGenerator & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};