#pragma once

#include <ogre/OgreCamera.h>
#include <kenshi/util/lektor.h>
#include <kenshi/util/hand.h>

class RootObject;
class GameData;
class Character;

class CameraClass
{
public:
    // no_addr void CameraClass(const class CameraClass & _a1);// public missing arg names
    CameraClass(Ogre::Camera* cam);// public RVA = 0x6AF410
    CameraClass* _CONSTRUCTOR(Ogre::Camera* cam);// public RVA = 0x6AF410
    void reset();// public RVA = 0x6B0AD0
    void update(bool controlEnabled);// public RVA = 0x6B05E0
    void focusCameraOnObject(RootObject* object, const Ogre::Vector3& offset, bool nearZoom);// public RVA = 0x6AEE30
    void updateOptionSettings();// public RVA = 0x6AE1E0
    void save(GameData* data);// public RVA = 0x6AFF60
    void load(GameData* data);// public RVA = 0x7ACC00
    bool isVisible(const Ogre::Vector3& pos, float radius) const;// public RVA = 0x6AE030
    bool isVisible(const Ogre::Vector3& pos) const;// public RVA = 0x6ADF90
    int intersectScreenEdge(const Ogre::Vector3& a, const Ogre::Vector3& b, float padding, float& out) const;// public RVA = 0x6AEF70
    void teleport(const Ogre::Vector3& pos);// public RVA = 0x6AF190
    void followObject(const hand& object);// public RVA = 0x6ADDA0
    void stopFollowing();// public RVA = 0x6ADDE0
    const hand& getFollowObject() const;// public RVA = 0x7BF1A0
    void manuallySetOrientationAndZoom(Ogre::Quaternion rot, float zoom);// public RVA = 0x6AE160
    Ogre::Vector3 getFacingDirection() const;// public RVA = 0x6AE0E0
    const Ogre::Vector3 getCenter() const;// public RVA = 0x100880
    const Ogre::Vector3 getCameraPos() const;// public RVA = 0x1008B0
    Ogre::SceneNode* getCenterNode() const;// public RVA = 0x166E30
    Ogre::SceneNode* getCameraNode() const;// public RVA = 0x100930
    bool isInitialised() const;// public RVA = 0xA19880
    void restrictPosition(lektor<Character*>& objects);// public RVA = 0x6AE210
    bool isFreeCameraMode() const;// public RVA = 0x6ADD90
    void setFreeCameraMode(bool on);// public RVA = 0x6AF1E0
    void updateFreeCamera();// public RVA = 0x6B0180
    void move(const Ogre::Vector3& dir);// private RVA = 0x6ADE30
    void zoom();// private RVA = 0x6AE380
    void toGround(bool smooth);// private RVA = 0x6AE540
    void setZoomDist(float dist);// private RVA = 0x6ADF40
    void rotate(float _yaw, float _pitch);// private RVA = 0x6AF940
    void rotationUpdate();// private RVA = 0x6AF700
    void updateAudio();// private RVA = 0x6AF960
    bool isRotating; // 0x0 Member
    tagPOINT lastMousePos; // 0x4 Member
    Ogre::Entity* rotationMarker; // 0x10 Member
    float yaw; // 0x18 Member
    float pitch; // 0x1C Member
    bool initialised; // 0x20 Member
    bool terrainLoaded; // 0x21 Member
    hand objectCurrentlyFollowing; // 0x28 Member
    Ogre::Vector3 objectCurrentlyFollowingOffset; // 0x48 Member
    Ogre::SceneNode* center; // 0x58 Member
    float altitude; // 0x60 Member
    Ogre::Camera* camera; // 0x68 Member
    Ogre::SceneNode* node; // 0x70 Member
    int currentMusic; // 0x78 Member
    hand inBuilding; // 0x80 Member
    float timeInGame; // 0xA0 Member
    float targetPositionY; // 0xA4 Member
    float speedY; // 0xA8 Member
    Building* centerBuilding; // 0xB0 Member
    float centerBuildingY; // 0xB8 Member
    unsigned short currentCollisionGroup; // 0xBC Member
    unsigned char currentFloor; // 0xBE Member
    bool freeCameraMode; // 0xBF Member
    // no_addr class CameraClass & operator=(const class CameraClass & _a1);// public missing arg names
};