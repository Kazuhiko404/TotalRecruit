#pragma once

#include "ProductionBuilding.h"

class GeneratorBuilding : public ProductionBuilding
{
public:
    // ProductionBuilding offset = 0x0, length = 0x490
    // no_addr void GeneratorBuilding(const class GeneratorBuilding & _a1);// public missing arg names
    GeneratorBuilding(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x588E50
    GeneratorBuilding* _CONSTRUCTOR(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x588E50
    virtual float getPowerOutput() const override;// public RVA = 0x2972B0 vtable offset = 0x0
    float _NV_getPowerOutput() const;// public RVA = 0x2972B0 vtable offset = 0x0
    virtual void getGUIState(DatapanelGUI* datapanel, int category) override;// public RVA = 0x300690 vtable offset = 0x0
    void _NV_getGUIState(DatapanelGUI* datapanel, int category);// public RVA = 0x300690 vtable offset = 0x0
    virtual float getInputValue(Item* item) override;// public RVA = 0x588ED0 vtable offset = 0x0
    float _NV_getInputValue(Item* item);// public RVA = 0x588ED0 vtable offset = 0x0
    virtual float getFuelConsumptionRate() override;// public RVA = 0x2FD8D0 vtable offset = 0x0
    float _NV_getFuelConsumptionRate();// public RVA = 0x2FD8D0 vtable offset = 0x0
    virtual ~GeneratorBuilding();// public RVA = 0x588EE0 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x588EE0 vtable offset = 0x0
    // no_addr class GeneratorBuilding & operator=(const class GeneratorBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};

class WindGeneratorBuilding : public GeneratorBuilding
{
public:
    // GeneratorBuilding offset = 0x0, length = 0x490
    // no_addr void WindGeneratorBuilding(const class WindGeneratorBuilding & _a1);// public missing arg names
    WindGeneratorBuilding(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x588EF0
    WindGeneratorBuilding* _CONSTRUCTOR(GameData* data, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, Faction* _participant, const hand& town, const hand& _handle, Layout* __isfurnitureOf, Building* _indoors);// public RVA = 0x588EF0
    virtual float getPowerOutput() const override;// public RVA = 0x297360 vtable offset = 0x0
    float _NV_getPowerOutput() const;// public RVA = 0x297360 vtable offset = 0x0
    virtual void getGUIState(DatapanelGUI* datapanel, int category) override;// public RVA = 0x300D30 vtable offset = 0x0
    void _NV_getGUIState(DatapanelGUI* datapanel, int category);// public RVA = 0x300D30 vtable offset = 0x0
    virtual void getGUIPower(DatapanelGUI* datapanel, int category) override;// public RVA = 0x30A470 vtable offset = 0x0
    void _NV_getGUIPower(DatapanelGUI* datapanel, int category);// public RVA = 0x30A470 vtable offset = 0x0
    virtual float getSoundIntensity() override;// public RVA = 0x2973C0 vtable offset = 0x0
    float _NV_getSoundIntensity();// public RVA = 0x2973C0 vtable offset = 0x0
    virtual bool isAnyInputsEmpty() const override;// public RVA = 0x588F70 vtable offset = 0x0
    bool _NV_isAnyInputsEmpty() const;// public RVA = 0x588F70 vtable offset = 0x0
    virtual ~WindGeneratorBuilding();// public RVA = 0x588F80 vtable offset = 0x0
    void _DESTRUCTOR();// public RVA = 0x588F80 vtable offset = 0x0
    // no_addr class WindGeneratorBuilding & operator=(const class WindGeneratorBuilding & _a1);// public missing arg names
    // virtual void * __vecDelDtor(unsigned int _a1) = 0;// public vtable offset = 0x0 missing arg names
};