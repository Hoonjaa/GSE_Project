#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include "ItemCatalog.h"

namespace EconomyRules
{
    constexpr std::int64_t SailboatPrice = 50;
    constexpr std::int64_t InitialGold = 50; // Temporary monster preview funds, approved by the user.
    constexpr float SailboatLength = 8;
    constexpr size_t CompanionCapacity = 2, CargoKinds = 5, ItemCount = ItemCatalog::Count;
    const char* ItemName(InventoryItem item);
    std::int64_t ItemPrice(InventoryItem item);
    std::int64_t ItemStack(InventoryItem item);
}

struct CargoSlot
{
    bool occupied = false;
    InventoryItem item = InventoryItem::NormalCoral;
    std::int64_t count = 0;
};

struct EconomyResult
{
    bool success;
    std::string message;
};

class PlayerEconomy
{
public:
    PlayerEconomy();

    bool HasShip() const
    {
        return m_hasShip;
    }

    std::int64_t Gold() const
    {
        return m_gold;
    }

    std::int64_t ItemCount(InventoryItem item) const;
    bool CollectItem(InventoryItem item);
    bool CollectItems(InventoryItem item, std::int64_t count);
    bool ConsumeItems(InventoryItem item, std::int64_t count);
    EconomyResult BuyItem(InventoryItem item, std::int64_t price);
    EconomyResult SellItem(InventoryItem item, std::int64_t price);
    EconomyResult Equip(InventoryItem item);
    EconomyResult Unequip(EquipSlot slot);
    int Equipped(EquipSlot slot) const;
    int EquipmentAttack() const;
    int EquipmentDefence() const;
    int EquipmentHealth() const;
    EconomyResult CollectPirateLoot(int gold, int tentacles);
    EconomyResult BuySailboat();
    EconomyResult StoreItem(InventoryItem item);
    EconomyResult UnloadItem(InventoryItem item);
    int LoseCargoUnits(int amount, std::mt19937& random);
    std::int64_t CargoUnits() const;

    const std::array<CargoSlot, EconomyRules::CargoKinds>& Cargo() const
    {
        return m_cargo;
    }

    size_t UsedCargoKinds() const;

private:
    std::array<std::int64_t, EconomyRules::ItemCount> m_bag = {};
    std::int64_t m_gold = EconomyRules::InitialGold;
    bool m_hasShip = false;
    std::array<CargoSlot, EconomyRules::CargoKinds> m_cargo = {};
    std::array<int, 4> m_equipment = {{static_cast<int>(InventoryItem::TrainingSword), -1, -1, -1}};
};
