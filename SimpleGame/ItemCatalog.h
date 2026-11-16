#pragma once
#include <string>
#include <array>
#include <cstdint>

enum class InventoryItem : unsigned
{
    NormalCoral,
    BossCoral,
    Fish,
    Tentacle,
    TradeBase = 4,
    MonsterBase = 36,
    SailorSword = 76,
    SteelSword,
    ClothHat,
    LeatherHat,
    SailorShirt,
    LeatherArmor,
    SailorPants,
    LeatherPants,
    UpgradeStone,
    TrainingSword,
    Count
};

enum class EquipSlot : int
{
    None = -1,
    Weapon,
    Head,
    Body,
    Legs
};
enum class ItemIcon : unsigned
{
    Material,
    Trade,
    Fish,
    Tentacle,
    Sword,
    Head,
    Body,
    Legs,
    Stone,
    Count
};

struct ItemDefinition
{
    std::string name;
    std::int64_t price = 0, stack = 20;
    EquipSlot equipment = EquipSlot::None;
    ItemIcon icon = ItemIcon::Material;
    int attack = 0, defence = 0, health = 0;
};

namespace ItemCatalog
{
    constexpr unsigned Count = static_cast<unsigned>(InventoryItem::Count);
    constexpr unsigned TradeBegin = static_cast<unsigned>(InventoryItem::TradeBase);
    constexpr unsigned MaterialBegin = static_cast<unsigned>(InventoryItem::MonsterBase);
    constexpr unsigned TradeCount = MaterialBegin - TradeBegin;
    constexpr unsigned MaterialCount = static_cast<unsigned>(InventoryItem::SailorSword) - MaterialBegin;
    const ItemDefinition& Get(InventoryItem item);
    InventoryItem MonsterMaterial(unsigned species);
    InventoryItem Trade(unsigned good);
    const char* SlotName(EquipSlot slot);
}

struct DropEntry
{
    InventoryItem item;
    unsigned chance = 50;
    int count = 1;
};

std::array<DropEntry, 3> MonsterDrops(unsigned species, bool epic);
