#pragma once

#include <ogre/OgreVector3.h>

class hkaiNavMesh;
class hkaiDirectedGraphExplicitCost;
class hkaiDirectedGraphExplicitCost;
class hkaiDirectedGraphInstance;
class hkaiNavMeshQueryMediator;
class hkaiNavMeshInstance;

struct NavInstance
{
    // VTable         : (none)
    hkaiNavMesh* mesh; // 0x8 Member
    hkaiDirectedGraphExplicitCost* graph; // 0x10 Member
    hkaiDirectedGraphInstance* graphInstance; // 0x18 Member
    hkaiNavMeshQueryMediator* mediator; // 0x20 Member
    hkaiNavMeshInstance* instance; // 0x28 Member
    Ogre::Vector3 offset; // 0x30 Member
    unsigned int uid; // 0x3C Member
    unsigned int hash; // 0x40 Member
    bool changed; // 0x44 Member
    bool temp; // 0x45 Member
    virtual bool isNavMeshSector() const;// public RVA = 0x3B2AE0 vtable offset = 0x0
    bool _NV_isNavMeshSector() const;// public RVA = 0x3B2AE0 vtable offset = 0x0
    // no_addr void NavInstance(const struct NavInstance & _a1);// public missing arg names
    NavInstance();// public RVA = 0x3B2AB0
    NavInstance* _CONSTRUCTOR();// public RVA = 0x3B2AB0
    // no_addr struct NavInstance & operator=(const struct NavInstance & _a1);// public missing arg names
};