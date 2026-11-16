#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

enum class IslandShape
{
    Round,
    Crescent,
    UShape,
    Dumbbell,
    Irregular,
    Teardrop,
    Split,
    Reef
};

enum class TownServiceKind
{
    None,
    Shop,
    Shipyard
};

enum class TradeGood
{
    Grain,
    TropicalFruit,
    Timber,
    Herbs,
    Pearls,
    Shells,
    Salt,
    DriedFish,
    Rope,
    Sailcloth,
    Woodwork,
    ProcessedSeafood,
    Spices,
    Dyes,
    Textiles,
    Handicrafts,
    IronOre,
    CopperOre,
    RoughGems,
    Stone,
    Flowers,
    Perfume,
    Honey,
    MedicinalPlants,
    Seaweed,
    Crab,
    Shrimp,
    CoralOrnaments,
    SeaUrchin,
    Sponge,
    RareShells,
    SeabirdFeathers
};

struct IslandCatalogEntry
{
    IslandShape shape;
    int documentId;
    const char* name;
    unsigned weight;
    std::array<TradeGood, 4> specialties;
};

namespace MapCatalog
{
    constexpr float PlayerRadius = .2f;
    constexpr float MinimumIslandSize = 40;
    constexpr float MaximumIslandSize = 80;
    constexpr float MinimumIslandGap = 300;
    constexpr float MaximumVillageRatio = .2f;

    const std::array<IslandCatalogEntry, 8>& Islands();
    const IslandCatalogEntry& Island(IslandShape shape);
    IslandShape SelectShape(unsigned roll);
    const char* GoodName(TradeGood good);
    bool IsSpecialty(IslandShape shape, TradeGood good);
}
