#pragma once

class GameData;

class GameplayOptions
{
public:
    GameplayOptions();// public RVA = 0x37D7C0
    GameplayOptions* _CONSTRUCTOR();// public RVA = 0x37D7C0
    void reset();// public RVA = 0x3E70B0
    float deathFrequency; // 0x0 Member
    bool easyProspecting; // 0x4 Member
    float globalDamageMultiplier; // 0x8 Member
    float buildingSpeed; // 0xC Member
    float numNestsMult; // 0x10 Member
    float researchSpeed; // 0x14 Member
    float productionSpeed; // 0x18 Member
    float hungerTime; // 0x1C Member
    bool banditsLootPlayer; // 0x20 Member
    bool animalsEat; // 0x21 Member
    bool difficultHealing; // 0x22 Member
    float getStarvationTimeInHours();// public RVA = 0x3E7100
    void save(GameData* data);// public RVA = 0x3EE3F0
    void load(GameData* data);// public RVA = 0x3EEA00
};