#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>

enum class CoralLoot
{
    Normal,
    Boss
};
enum class InventoryItem : unsigned
{
    NormalCoral,
    BossCoral,
    Fish,
    Tentacle,
    TradeBase
};

namespace EconomyRules
{
    constexpr std::int64_t SailboatPrice = 50;
    constexpr std::int64_t InitialGold = 50; // Temporary monster preview funds, approved by the user.
    constexpr float SailboatLength = 8;
    constexpr size_t CompanionCapacity = 1, CargoKinds = 5, ItemCount = 36;
    InventoryItem CoralItem(CoralLoot kind);
    const char* ItemName(InventoryItem item);
    std::int64_t ItemPrice(InventoryItem item);
    std::int64_t ItemStack(InventoryItem item);
    const char* LootName(CoralLoot kind);
    std::int64_t SellPrice(CoralLoot kind);
    std::int64_t StackLimit(CoralLoot kind);
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

    std::int64_t LootCount(CoralLoot kind) const;
    std::int64_t ItemCount(InventoryItem item) const;
    bool Collect(CoralLoot kind);
    bool CollectItem(InventoryItem item);
    EconomyResult CollectPirateLoot(int gold, int tentacles);
    EconomyResult SellAll();
    EconomyResult BuySailboat();
    EconomyResult Store(CoralLoot kind);
    EconomyResult Unload(CoralLoot kind);
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
};
