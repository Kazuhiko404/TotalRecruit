#pragma once

#include <ogre/OgreVector3.h>
#include "Enums.h"
#include "util/hand.h"

class CharMovement;
class AI;
class AnimationClass;
class Character;
class Tasker;
class RootObject;
class CombatClass;
class CharStats;
class Faction;
class PatrolInfo;
class AITaskSytem;

class CharBody : public Ogre::GeneralAllocatedObject
{
public:
    // Ogre::AllocatedObject<Ogre::CategorisedAllocPolicy<0> > offset = 0x8, length = 0x1
    // VTable         : (none)
    // no_addr void CharBody(const class CharBody & _a1);// public missing arg names
    CharBody();// public RVA = 0x620890
    CharBody* _CONSTRUCTOR();// public RVA = 0x620890
    virtual ~CharBody();// public RVA = 0x620DA0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x620DA0 vtable offset = 0x0
    void create(CharMovement* m, AI* a, AnimationClass* an, Character* c, CharStats* st);// public RVA = 0x620CE0
    virtual void update(float _time);// public RVA = 0x5C65A0 vtable offset = 0x8
    void _NV_update(float _time);// public RVA = 0x5C65A0 vtable offset = 0x8
    void periodicUpdate(float time);// public RVA = 0x5C6040
    virtual bool setCurrentAction(Tasker* startActionMsg);// public RVA = 0x5C6740 vtable offset = 0x18
    bool _NV_setCurrentAction(Tasker* startActionMsg);// public RVA = 0x5C6740 vtable offset = 0x18
    virtual bool setCurrentAction(TaskType t, RootObject* target);// public RVA = 0x5C5F00 vtable offset = 0x10
    bool _NV_setCurrentAction(TaskType t, RootObject* target);// public RVA = 0x5C5F00 vtable offset = 0x10
    void notifyBodyTaskComplete();// public RVA = 0x5C62E0
    virtual void notifyTaskComplete();// public RVA = 0x5C6060 vtable offset = 0x20
    void _NV_notifyTaskComplete();// public RVA = 0x5C6060 vtable offset = 0x20
    virtual void notifyTaskImpossible();// public RVA = 0x5C6090 vtable offset = 0x28
    void _NV_notifyTaskImpossible();// public RVA = 0x5C6090 vtable offset = 0x28
    virtual void notifyPathImpossible(const hand& h);// public RVA = 0x5C60C0 vtable offset = 0x30
    void _NV_notifyPathImpossible(const hand& h);// public RVA = 0x5C60C0 vtable offset = 0x30
    CombatClass* getCombatClass() const;// public RVA = 0x43FCB0
    bool isCrouched() const;// public RVA = 0x595DB0
    virtual Ogre::Vector3 getPosition();// public RVA = 0x620910 vtable offset = 0x38
    Ogre::Vector3 _NV_getPosition();// public RVA = 0x620910 vtable offset = 0x38
    std::string getName() const;// public RVA = 0x639930
    virtual bool isCharacter() const;// public RVA = 0x620940 vtable offset = 0x40
    bool _NV_isCharacter() const;// public RVA = 0x620940 vtable offset = 0x40
    virtual Character* getCharacter();// public RVA = 0x620950 vtable offset = 0x48
    Character* _NV_getCharacter();// public RVA = 0x620950 vtable offset = 0x48
    virtual CharBody* getCharBody();// public RVA = 0x620960 vtable offset = 0x50
    CharBody* _NV_getCharBody();// public RVA = 0x620960 vtable offset = 0x50
    void getUpFromRagdoll(const std::string& animationName);// public RVA = 0x5C6270
    virtual Platoon* getPlatoon() const;// public RVA = 0x5C5ED0 vtable offset = 0x58
    Platoon* _NV_getPlatoon() const;// public RVA = 0x5C5ED0 vtable offset = 0x58
    CombatClass* combatClass; // 0x8 Member
    AnimationClass* animation; // 0x10 Member
    Character* character; // 0x18 Member
    CharStats* stats; // 0x20 Member
    hand target; // 0x28 Member
    bool gotItem; // 0x48 Member
    bool crouched; // 0x49 Member
    bool jogMode; // 0x4A Member
    float arbitraryCatchupDist; // 0x4C Member
    Tasker* getCurrentAction();// public RVA = 0x331A20
    Tasker* getCurrentActionOrMessage();// public RVA = 0x50BEA0
    void endAction();// public RVA = 0x50BEB0
    bool isIdle() const;// public RVA = 0x286270
    // no_addr enum TaskType getActionType();// public
    hand getHandle() const;// public RVA = 0x5C6360
    Faction* getFaction() const;// public RVA = 0x5C62C0
    AI* ai; // 0x50 Member
    CharMovement* movement; // 0x58 Member
    void _move(RootObjectBase* who, const Ogre::Vector3& location);// public RVA = 0x5C6120
    void _patrol(Tasker* task, int end, Ogre::Vector3 pos, PatrolInfo* area, AITaskSytem* taskSys, TownBase* town);// public RVA = 0x5C63B0
    hand getCurrentSubject() const;// public RVA = 0x5C6300
    float frameTIME; // 0x60 Member
    virtual void _endAction();// protected RVA = 0x5C5FF0 vtable offset = 0x60
    void _NV__endAction();// protected RVA = 0x5C5FF0 vtable offset = 0x60
    Tasker* currentAction; // 0x68 Member
    bool amIdle; // 0x70 Member
    // no_addr class CharBody & operator=(const class CharBody & _a1);// public missing arg names
    // no_addr void __local_vftable_ctor_closure();// public
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};