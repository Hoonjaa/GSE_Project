#pragma once
#include "IslandLayout.h"
#include <array>
#include <string>

struct NpcKey
{
    IslandKey island;
    unsigned role = 0;

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
