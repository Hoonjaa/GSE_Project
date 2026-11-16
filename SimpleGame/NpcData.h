#pragma once
#include "IslandKey.h"
#include <array>
#include <string>

namespace NpcRole
{
    constexpr unsigned Hunter = 0, Collector = 1, Merchant = 2, Count = 3;
}

struct NpcKey
{
    IslandKey island;
    unsigned role = NpcRole::Hunter;

    bool operator<(const NpcKey& other) const
    {
        if (island < other.island)
        {
            return true;
        }
        if (other.island < island)
        {
            return false;
        }
        return role < other.role;
    }

    bool operator==(const NpcKey& other) const
    {
        return island == other.island && role == other.role;
    }
};
enum class QuestStatus
{
    Available,
    Active,
    Complete
};

struct NpcRecord
{
    int affinity = 0, trust = 0, respect = 0, fear = 0, resentment = 0;
    QuestStatus quest = QuestStatus::Available;
    int progress = 0;
    bool recruited = false;
    std::array<std::string, 4> memory;
    unsigned memoryCursor = 0;
};
