#pragma once
#include "NpcData.h"
#include "PlayerEconomy.h"
#include "MapCatalog.h"
#include <map>
#include <vector>

// Value state: independent of economy, NPC Actors and the active map.
class NpcState
{
public:
    const NpcRecord& Npc(const NpcKey& key) const;
    void AdjustAffinity(const NpcKey& key, int amount, const std::string& reason);
    EconomyResult AcceptQuest(const NpcKey& key);
    EconomyResult CompleteQuest(const NpcKey& key, IslandShape shape, PlayerEconomy& inventory);
    int Progress(const NpcKey& key, IslandShape shape, const PlayerEconomy& inventory) const;
    void RecordMonsterKill(IslandKey island, bool epic);
    void RecordTradePurchase(IslandKey island);
    EconomyResult Recruit(const NpcKey& key, const PlayerEconomy& economy);
    EconomyResult Dismiss(const NpcKey& key);

    const std::vector<NpcKey>& Companions() const
    {
        return m_companions;
    }

private:
    std::map<NpcKey, NpcRecord> m_npcs;
    std::vector<NpcKey> m_companions;
};
