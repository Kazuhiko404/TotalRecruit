#pragma once

#include "ProductionBuilding.h"

class RainCollectorBuilding : public ProductionBuilding
{
public:
    // ProductionBuilding offset = 0x0, length = 0x490
    // no_addr void RainCollectorBuilding(const class RainCollectorBuilding & _a1);// public missing arg names
    RainCollectorBuilding(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* participant, const hand& town, const hand& handle, Layout* isfurnitureOf, Building* indoors);// protected RVA = 0x588F90
    RainCollectorBuilding* _CONSTRUCTOR(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* participant, const hand& town, const hand& handle, Layout* isfurnitureOf, Building* indoors);// protected RVA = 0x588F90
    virtual float calculateEfficiencyMult() const override;// protected RVA = 0x297FC0 vtable offset = 0x0
    float _NV_calculateEfficiencyMult() const;// protected RVA = 0x297FC0 vtable offset = 0x0
    virtual void getGUIState(DatapanelGUI* datapanel, int category) override;// protected RVA = 0x300F60 vtable offset = 0x0
    void _NV_getGUIState(DatapanelGUI* datapanel, int category);// protected RVA = 0x300F60 vtable offset = 0x0
    virtual void getGUIToolTipForGroundResourceEfficiency(lektor<StringPair>& out) override;// protected RVA = 0x30AFB0 vtable offset = 0x0
    void _NV_getGUIToolTipForGroundResourceEfficiency(lektor<StringPair>& out);// protected RVA = 0x30AFB0 vtable offset = 0x0
    float getRainAmount() const;// protected RVA = 0x297F50
    virtual ~RainCollectorBuilding();// public RVA = 0x589010 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x589010 vtable offset = 0x0
    // no_addr class RainCollectorBuilding & operator=(const class RainCollectorBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};