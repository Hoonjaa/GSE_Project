#pragma once
#include "NpcData.h"
#include "MapCatalog.h"
#include "PlayerEconomy.h"

struct QuestDefinition
{
    unsigned role;
    const char* roleName;
    const char* objective;
    int target, affinityReward, trustReward, respectReward;
};

namespace QuestRules
{
    const QuestDefinition& Get(unsigned role);
    const char* Title(unsigned role);
    const char* StatusLabel(QuestStatus status, int progress, int target);
    std::string Description(unsigned role);
    int MaterialCount(IslandShape shape, const PlayerEconomy& inventory);
    bool DeliverMaterials(IslandShape shape, int quantity, PlayerEconomy& inventory);
}
