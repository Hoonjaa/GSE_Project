#pragma once
#include "MapCatalog.h"
#include "GeometryTypes.h"
#include <array>

enum class MonsterForm
{
    Boar,
    Wolf,
    Tree,
    Bee,
    Hermit,
    Crab,
    Jellyfish,
    Siren,
    WoodGolem,
    Rope,
    Goblin,
    Rat,
    Chameleon,
    Spider,
    Vine,
    Monkey,
    StoneGolem,
    Bat,
    Worm,
    Troll,
    Mandrake,
    Flower,
    Butterfly,
    Mushroom,
    Lizard,
    Slime,
    Frog,
    Seaweed,
    Urchin,
    ReefCrab,
    Snake,
    Harpy,
    Pearl,
    Ghost,
    Crystal,
    WaterGuardian,
    Predator
};

struct MonsterDefinition
{
    unsigned id;
    const char* name;
    MonsterForm form;
    Color color;
    bool epic;
};

namespace MonsterCatalog
{
    constexpr unsigned Count = 40, SlotsPerIsland = 5;
    const MonsterDefinition& Get(unsigned id);
    unsigned ForIsland(IslandShape shape, unsigned slot);
}

// Preview tuning inherited from the previous prototype, not final species balance.
namespace MonsterPreviewRules
{
    constexpr float AreaPerNormal = 100;
    constexpr int MinimumNormalCount = 8;
    constexpr float NormalHealth = 36, EpicHealth = 360;
    constexpr int NormalDamage = 8, EpicDamage = 30;
    constexpr float NormalSpeed = 1.9f, EpicSpeed = 1.5f;
    constexpr float NormalAggro = 12, EpicAggro = 13;
    constexpr float RespawnTime = 25, CoarseInterval = .25f, DetailedRadius = 24;
}
