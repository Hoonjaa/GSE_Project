#include "stdafx.h"
#include "MapCatalog.h"
#include "TradeRules.h"
#include <stdexcept>

namespace
{
    const std::array<IslandCatalogEntry, 8> Entries = {
        {{IslandShape::Round,
          1,
          "원형 섬",
          37,
          {{TradeGood::Grain, TradeGood::TropicalFruit, TradeGood::Timber, TradeGood::Herbs}}},
         {IslandShape::Crescent,
          3,
          "초승달 섬",
          9,
          {{TradeGood::Pearls, TradeGood::Shells, TradeGood::Salt, TradeGood::DriedFish}}},
         {IslandShape::UShape,
          7,
          "U자형 섬",
          9,
          {{TradeGood::Rope, TradeGood::Sailcloth, TradeGood::Woodwork, TradeGood::ProcessedSeafood}}},
         {IslandShape::Dumbbell,
          9,
          "아령형 섬",
          9,
          {{TradeGood::Spices, TradeGood::Dyes, TradeGood::Textiles, TradeGood::Handicrafts}}},
         {IslandShape::Irregular,
          11,
          "불규칙 섬",
          9,
          {{TradeGood::IronOre, TradeGood::CopperOre, TradeGood::RoughGems, TradeGood::Stone}}},
         {IslandShape::Teardrop,
          13,
          "물방울 섬",
          9,
          {{TradeGood::Flowers, TradeGood::Perfume, TradeGood::Honey, TradeGood::MedicinalPlants}}},
         {IslandShape::Split,
          17,
          "분열형 섬",
          9,
          {{TradeGood::Seaweed, TradeGood::Crab, TradeGood::Shrimp, TradeGood::CoralOrnaments}}},
         {IslandShape::Reef,
          18,
          "암초군 섬",
          9,
          {{TradeGood::SeaUrchin, TradeGood::Sponge, TradeGood::RareShells, TradeGood::SeabirdFeathers}}}}};
    const char* Goods[] = {
        "곡물",        "열대과일", "목재",      "약초",        "진주",   "조개", "소금",      "건어물",
        "선박용 밧줄", "돛천",     "목공품",    "가공 해산물", "향신료", "염료", "직물",      "수공예품",
        "철광석",      "구리광석", "보석 원석", "석재",        "꽃",     "향유", "꿀",        "약용식물",
        "해조류",      "게",       "새우",      "산호 장식품", "성게",   "해면", "희귀 조개", "바닷새 깃털"};
}

const std::array<IslandCatalogEntry, 8>& MapCatalog::Islands()
{
    return Entries;
}

const IslandCatalogEntry& MapCatalog::Island(IslandShape shape)
{
    const auto index = static_cast<size_t>(shape);
    if (index >= Entries.size())
    {
        throw std::out_of_range("Invalid island shape.");
    }
    return Entries[index];
}

IslandShape MapCatalog::SelectShape(unsigned roll)
{
    if (roll >= 100)
    {
        throw std::out_of_range("Island roll must be in [0, 99].");
    }
    for (const auto& entry : Entries)
    {
        if (roll < entry.weight)
        {
            return entry.shape;
        }
        roll -= entry.weight;
    }
    throw std::logic_error("Island weights do not total 100.");
}

const char* MapCatalog::GoodName(TradeGood good)
{
    const auto index = static_cast<size_t>(good);
    if (index >= sizeof(Goods) / sizeof(Goods[0]))
    {
        throw std::out_of_range("Invalid trade good.");
    }
    return Goods[index];
}

bool MapCatalog::IsSpecialty(IslandShape shape, TradeGood good)
{
    for (auto item : Island(shape).specialties)
    {
        if (item == good)
        {
            return true;
        }
    }
    return false;
}

std::array<MarketOffer, 32> MapCatalog::Offers(IslandShape shape, int affinity)
{
    const PrototypeTradePricing pricing;
    std::array<MarketOffer, 32> result;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i].good = static_cast<TradeGood>(i);
        result[i].localSpecialty = IsSpecialty(shape, result[i].good);
        result[i].buy = pricing.Buy(shape, result[i].good, affinity);
        result[i].sell = pricing.Sell(shape, result[i].good, affinity);
    }
    return result;
}
