#pragma once

#include <ogre/OgreMemoryAllocatorConfig.h>
#include <ogre/OgreVector2.h>
#include <ogre/OgreVector3.h>
#include <ogre/OgreQuaternion.h>
#include <ogre/OgreVector4.h>
#include <ogre/OgreMatrix4.h>
#include <string>
#include "lektor.h"
#include <boost/filesystem.hpp>
#include "iVector2.h"

class physHit;
struct PhysHitItem;
class Building;
class hand;
class ZoneMap;
struct NxShape;
struct NxMat33;
struct NxBox;

// TODO move?
class rendHit
{
public:
    rendHit();// public RVA = 0x9AF6D0
    rendHit* _CONSTRUCTOR();// public RVA = 0x9AF6D0
    int data; // 0x0 Member
    Ogre::Vector3 hit; // 0x4 Member
    // no_addr class rendHit & operator=(const class rendHit & _a1);// public missing arg names
};

// TODO move?
class TimerClass
{
public:
    TimerClass();// public RVA = 0x9AF6B0
    TimerClass* _CONSTRUCTOR();// public RVA = 0x9AF6B0
    // no_addr void startTimer();// public
    // no_addr int endTimer();// public
    __int64 freq; // 0x0 Member
    __int64 ctr1; // 0x8 Member
    __int64 ctr2; // 0x10 Member
};

class UtilityT
{
public:
    UtilityT();// public RVA = 0x9AF730
    UtilityT* _CONSTRUCTOR();// public RVA = 0x9AF730
    Ogre::Matrix4 cachedViewMatrix; // 0x0 Member
    static Ogre::Ray getMouseRay();// public RVA = 0x9AFFE0
    static void mouseTrace(physHit& hit, const Ogre::Vector2& mouse, unsigned int group);// public RVA = 0x9B5E30
    static void rayTrace(physHit& hit, const Ogre::Ray& ray, unsigned int group);// public RVA = 0x9B5BB0
    static bool ceilingTrace(const Ogre::Vector3& pos, unsigned int groups, bool abortValue);// public RVA = 0x9B5C80
    static void traceNoActors(physHit& hit, const Ogre::Vector3& origin, const Ogre::Vector3& dir, unsigned int group);// public RVA = 0x9B50A0
    static void trace(physHit& hit, const Ogre::Vector3& origin, const Ogre::Vector3& dir, unsigned int group);// public RVA = 0x9B52E0
    static void traceFloorHeight(physHit& hit, const Ogre::Vector3& origin, int floorNum);// public RVA = 0x9B5DB0
    static void _trace_TEST(physHit& hit, const Ogre::Vector3& origin, const Ogre::Vector3& dir, unsigned int group);// public RVA = 0x9B5D70
    static Ogre::Vector3 tracePosOnly(const Ogre::Vector3& origin, const Ogre::Vector3& dir, unsigned int groups);// public RVA = 0x9B2220
    static void sphereTrace(physHit& hit, const Ogre::Vector3& origin, const Ogre::Vector3& motion, const float radius, unsigned int group, bool staticOnly);// public RVA = 0x9B5780
    static void boxTrace(physHit& hit, const Ogre::Vector3& origin, const Ogre::Vector3& motion, const Ogre::Vector3& boxExtents, unsigned int group, bool staticOnly);// public RVA = 0x9B5950
    static void mouseTraceAll(lektor<PhysHitItem>& result, unsigned int group);// public RVA = 0x9B2160
    static void traceAll(lektor<PhysHitItem>& result, const Ogre::Vector3& origin, const Ogre::Vector3& dir, unsigned int group);// public RVA = 0x9B1FF0
    static float getTerrainHeight(float x, float z);// public RVA = 0x9B1F60
    static float getTerrainWithWaterHeight(float x, float z);// public RVA = 0x9B1F70
    static float getTerrainWithWaterHeightFromRenderer(const Ogre::Vector3& pos);// public RVA = 0x9B1290
    static bool getPositionInWater(float x, float z);// public RVA = 0x9B1FC0
    static float getFloorHeight(const Ogre::Vector3& origin, int floorNum, bool withTerrain, bool furniture);// public RVA = 0x9B2660
    static float getFloorHeight(const Ogre::Vector3& origin, bool withTerrain, bool furniture);// public RVA = 0x9B2730
    static float getBuildingGroundFloorHeight(const Ogre::Vector3& origin, bool withTerrain);// public RVA = 0x9B25F0
    static Building* isIndoors(const Ogre::Vector3& point);// public RVA = 0x9B13F0
    static bool isIndoorsFast(const Ogre::Vector3& point);// public RVA = 0x9B5550
    static Building* isIndoors_forWaypoint(const Ogre::Vector3& point);// public RVA = 0x9B1300
    static void cascadeSetStatic(Ogre::SceneNode* node, bool bStatic);// public RVA = 0x9B29D0
    float perlinNoise_2D(double x, double y, double zoom, float dropPoint);// public RVA = 0x6CB6A0
    static int getFloorNumber(int collisionGroup);// public RVA = 0x9B00A0
    static int getFloorNumber(const Ogre::Vector3& position, hand* hitBuilding);// public RVA = 0x9B5620
    float getTerrainHeightFastWithNormal(float x, float z, Ogre::Vector3& normal, ZoneMap* map);// public RVA = 0x9B0570
    static float getTerrainHeightFast(float x, float z, ZoneMap* map);// public RVA = 0x9B1B40
    float getTerrainSlopeFast(float x, float z, ZoneMap* map);// public RVA = 0x9B0680
    static float random(float lo, float hi);// public RVA = 0x9B04D0
    static float random();// public RVA = 0x9B0460
    static bool randomBool();// public RVA = 0x9B0490
    static int randomInt(int lo, int hi);// public RVA = 0x9B2390
    static void seed(unsigned int s);// public RVA = 0x9B0450
    static void seed();// public RVA = 0x9B0430
    static int round(float value);// public RVA = 0x9B1B00
    static float nlerp(const float& a, const float& b, const float& t);// public RVA = 0x9B0520
    static int getNextPow2(int value);// public RVA = 0x9B0540
    static void quatSetDirection(Ogre::Quaternion& q, const Ogre::Vector3& targetDir, const Ogre::Vector3& facing);// public RVA = 0x9AFEE0
    static iVector2 getSubMapSector(float X, float Z);// public RVA = 0x9B15E0
    static Ogre::Vector4 getSubMapSectorBounds(const iVector2 sector);// public RVA = 0x9B0120
    static bool getResourceFilePath(const std::string& filename, std::string& filepath);// public RVA = 0x9B5EE0
    static std::string removePathFromString(const std::string& s);// public RVA = 0x9B2E60
    static std::string removeFilenameFromPath(const std::string& s);// public RVA = 0x9B2FC0
    static std::string removeFileExtensionFromString(const std::string& s);// public RVA = 0x9B2F30
    static std::string getFileExtensionFromString(const std::string& s);// public RVA = 0x9B2DD0
    static void removeInvalidFileNameChars(std::string& str);// public RVA = 0x9B2820
    static bool compareStringsCaseInsenstive(const std::string& a, const std::string& b);// public RVA = 0x9B2520
    TimerClass timer; // 0x40 Member
    void profilesClear();// public RVA = 0x9B0B30
    void profilesPrint();// public RVA = 0x9B0B40
    int profileGet(const std::string& name);// public RVA = 0x9B0B50
    static float getTerrainHeightFromRenderer(const Ogre::Vector3& pos, bool useCollisionForLowLod, int minLod);// public RVA = 0x9B07D0
    static float getApproxTerrainHeightFromRenderer(const Ogre::Vector3& pos, Ogre::Vector3& normal);// public RVA = 0x9B08A0
    bool worldToScreenRel(const Ogre::Vector3& pos, float& x, float& y);// public RVA = 0x9B09E0
    bool worldToScreenPX(const Ogre::Vector3& pos, float& x, float& y);// public RVA = 0x9B1C40
    bool worldToScreenWithRadius(const Ogre::Vector3& pos, const float rad, float& x, float& y, float& cx, float& cy);// public RVA = 0x9B0B10
    Ogre::Entity* placeOrientedMarker(const Ogre::Vector3& pos, const Ogre::Quaternion& rot, Ogre::SceneNode* attach);// public RVA = 0x9B0940
    void placeMarker(const Ogre::Vector3& pos, const void* key, int colour);// public RVA = 0x9B0910
    void placeMarker(const Ogre::Vector3& pos, int colour);// public RVA = 0x9B0900
    void debugBox(NxShape* shape);// public RVA = 0x9B0920
    void placeBox(Ogre::Vector3 pos, Ogre::SceneNode* attach);// public RVA = 0x9B0930
    // no_addr void placeArrow(const class Ogre::Vector3 & _a1, const class Ogre::Vector3 & _a2, const void * _a3, float _a4, float _a5, const class Ogre::ColourValue & _a6);// public missing arg names
    void placeArrow(const Ogre::Vector3& pos, const Ogre::Vector3& dir, const void* key, float len, float scale, int colour);// public RVA = 0x9B0960
    // no_addr void placeArrowTo(const class Ogre::Vector3 & _a1, const class Ogre::Vector3 & _a2, const void * _a3, float _a4, const class Ogre::ColourValue & _a5);// public missing arg names
    void placeArrowTo(const Ogre::Vector3& from, const Ogre::Vector3& to, const void* key, float size, int colour);// public RVA = 0x9B0970
    void placeArrowBT(const Ogre::Vector3& pos, const Ogre::Vector3& dir, const void* key, float len, float scale, int colour);// public RVA = 0x9B0990
    void removeArrow(const void* key);// public RVA = 0x9B0980
    void placeSphere(const Ogre::Vector3& pos, const void* key, float radius, int colour);// public RVA = 0x9B0950
    void placeAxis(const Ogre::Vector3& pos, const Ogre::Quaternion& q);// public RVA = 0x9B09B0
    void placeAxis(const Ogre::Vector3& pos, const NxMat33& m);// public RVA = 0x9B09C0
    rendHit ogreTrace(const Ogre::Vector3& origin, const Ogre::Vector3& dir, int group);// public RVA = 0x9B0770
    Ogre::SharedPtr<Ogre::Material> getDebugMaterial(int colour, bool xray);// public RVA = 0x9B6080
    void flushMarkerQueue();// public RVA = 0x9B09A0
    void removeMarkers();// public RVA = 0x9B09D0
    bool folderExists(const std::string& folder);// public RVA = 0x9B2C60
    bool fileExists(const std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >& file);// public RVA = 0x9B0B20
    bool fileExistsASCII(const std::string& file);// public RVA = 0x9B3980
    bool createFile(const std::string& path, const std::string& content);// public RVA = 0x9B35D0
    bool writeFile(const std::string& path, const std::string& content, bool append);// public RVA = 0x9B3730
    bool readFile(const std::string& path, std::string& content);// public RVA = 0x9B4EA0
    bool isFile(const std::string& _path);// public RVA = 0x9B2AC0
    bool isDirectory(const std::string& _path);// public RVA = 0x9B2B90
    static void makeSureGameFolderExists(const std::string& relativefolder);// public RVA = 0x9B3380
    static void makeSureFolderExists(const std::string& folder);// public RVA = 0x9B34F0
    static std::string getFullPath(const std::string& dir);// public RVA = 0x9B4740
    static bool getRelativePath(const boost::filesystem::path& pathA, const boost::filesystem::path& pathB, boost::filesystem::path& relativePath);// public RVA = 0x9B4B90
    bool getFilesInDir(lektor<std::string >& out, const std::string& dir, const std::string& type);// public RVA = 0x9B64A0
    bool getFilesInGameDir(lektor<std::string >& out, const std::string& dir, const std::string& type);// public RVA = 0x9B6E50
    void getDirsInDir(lektor<std::string >& out, const std::string& dir);// public RVA = 0x9B68C0
    std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > getCurrentDir();// public RVA = 0x9B32E0
    void setCurrentDir(const std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >& dir);// public RVA = 0x9B2410
    void deleteFilesInFolder(const std::string& dir, const std::string& filter, bool recursive);// public RVA = 0x9B72F0
    void deleteFolder(const std::string& dir);// public RVA = 0x9B7590
    void deleteFile(const std::string& dir);// public RVA = 0x9B4290
    bool moveFile(const std::string& from, const std::string& to);// public RVA = 0x9B48F0
    bool copyFile(const std::string& from, const std::string& to);// public RVA = 0x9B45A0
    bool copyFilesInFolder(const std::string& from, const std::string& to, const std::string& filter);// public RVA = 0x9B7610
    static bool positionIsInFrontOfMe(const Ogre::Vector3& facingDir, const Ogre::Vector3& targetDir);// public RVA = 0x9B01B0
    static bool pointInPlane(Ogre::Vector3 norm, Ogre::Vector3 planepos, Ogre::Vector3 point);// public RVA = 0x9AF810
    static bool OBBCollision(NxBox& a, NxBox& b);// public RVA = 0x9AF860
    static std::string colorToGUIString(const Ogre::ColourValue& col);// public RVA = 0x9B4B00
    // no_addr static void aggressionBreakpoint(class Character * _a1, class Character * _a2);// public missing arg names
    void roundToNearest(float& input, int roundTo);// public RVA = 0x9B0B60
    Ogre::Entity* marker; // 0x58 Member
};