#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

enum class CoralLoot
{
    Normal,
    Boss
};

namespace EconomyRules
{
    constexpr std::int64_t SailboatPrice = 50;
    constexpr float SailboatLength = 8;
    constexpr size_t CompanionCapacity = 1;
    constexpr size_t CargoKinds = 5;
    const char* LootName(CoralLoot kind);
    std::int64_t SellPrice(CoralLoot kind);
    std::int64_t StackLimit(CoralLoot kind);
}

struct CargoSlot
{
    bool occupied = false;
    CoralLoot kind = CoralLoot::Normal;
    std::int64_t count = 0;
};

struct EconomyResult
{
    bool success;
    std::string message;
};

// Player possessions and ship cargo survive home/death resets within this session.
class PlayerEconomy
{
public:
    bool HasShip() const
    {
        return m_hasShip;
    }

    std::int64_t Gold() const
    {
        return m_gold;
    }

    std::int64_t LootCount(CoralLoot kind) const;
    bool Collect(CoralLoot kind);
    EconomyResult SellAll();
    EconomyResult BuySailboat();
    EconomyResult Store(CoralLoot kind);
    EconomyResult Unload(CoralLoot kind);

    const std::array<CargoSlot, EconomyRules::CargoKinds>& Cargo() const
    {
        return m_cargo;
    }

    size_t UsedCargoKinds() const;

private:
    std::int64_t& Bag(CoralLoot kind);
    std::int64_t m_gold = 0, m_normal = 0, m_boss = 0;
    bool m_hasShip = false;
    std::array<CargoSlot, EconomyRules::CargoKinds> m_cargo = {};
};
