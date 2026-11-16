#include "stdafx.h"
#include "NpcState.h"
#include "QuestRules.h"
#include "Diagnostics.h"
#include <algorithm>

const NpcRecord& NpcState::Npc(const NpcKey& key) const
{
    static const NpcRecord empty;
    const auto found = m_npcs.find(key);
    return found == m_npcs.end() ? empty : found->second;
}

void NpcState::AdjustAffinity(const NpcKey& key, int amount, const std::string& reason)
{
    auto& npc = m_npcs[key];
    npc.affinity = (std::max)(-100, (std::min)(100, npc.affinity + amount));
    npc.memory[npc.memoryCursor++ % npc.memory.size()] = reason;
    Diagnostics::Get().Event(
        "npc_relation island_x=" + std::to_string(key.island.x) +
        " island_z=" + std::to_string(key.island.z) + " role=" + std::to_string(key.role) +
        " affinity=" + std::to_string(npc.affinity) + " event=" + reason);
}

EconomyResult NpcState::AcceptQuest(const NpcKey& key)
{
    auto& npc = m_npcs[key];
    if (npc.quest != QuestStatus::Available)
    {
        return {false, "이미 수락했거나 완료한 의뢰입니다."};
    }
    npc.quest = QuestStatus::Active;
    npc.progress = 0;
    AdjustAffinity(key, 0, "의뢰를 수락했다");
    return {true, "의뢰를 수락했습니다. 완료 후 다시 대화하세요."};
}

void NpcState::RecordMonsterKill(IslandKey island, bool epic)
{
    if (epic)
    {
        return;
    }
    const NpcKey key = {island, NpcRole::Hunter};
    auto found = m_npcs.find(key);
    if (found != m_npcs.end() && found->second.quest == QuestStatus::Active)
    {
        found->second.progress = (std::min)(QuestRules::Get(key.role).target, found->second.progress + 1);
    }
}

void NpcState::RecordTradePurchase(IslandKey island)
{
    const NpcKey key = {island, NpcRole::Merchant};
    auto found = m_npcs.find(key);
    if (found != m_npcs.end() && found->second.quest == QuestStatus::Active)
    {
        found->second.progress = (std::min)(QuestRules::Get(key.role).target, found->second.progress + 1);
    }
}

EconomyResult NpcState::CompleteQuest(const NpcKey& key, IslandShape shape, PlayerEconomy& inventory)
{
    auto& npc = m_npcs[key];
    const auto& quest = QuestRules::Get(key.role);
    if (npc.quest != QuestStatus::Active)
    {
        return {false, "진행 중인 의뢰가 없습니다."};
    }
    if (Progress(key, shape, inventory) < quest.target)
    {
        return {
            false,
            "의뢰 목표를 먼저 완료하세요. 진행 " + std::to_string(Progress(key, shape, inventory)) + "/" +
                std::to_string(quest.target)};
    }
    if (key.role == NpcRole::Collector && !QuestRules::DeliverMaterials(shape, quest.target, inventory))
    {
        return {false, "전리품이 부족해 의뢰를 완료할 수 없습니다."};
    }
    npc.quest = QuestStatus::Complete;
    npc.progress = quest.target;
    npc.trust = (std::min)(100, npc.trust + quest.trustReward);
    npc.respect = (std::min)(100, npc.respect + quest.respectReward);
    AdjustAffinity(key, quest.affinityReward, "의뢰를 완료했다");
    return {
        true,
        "의뢰 완료! 호감도 +" + std::to_string(quest.affinityReward) + ". R로 동료 영입을 제안하세요."};
}

int NpcState::Progress(const NpcKey& key, IslandShape shape, const PlayerEconomy& inventory) const
{
    const auto& state = Npc(key);
    const int target = QuestRules::Get(key.role).target;
    if (state.quest == QuestStatus::Complete)
    {
        return target;
    }
    if (key.role == NpcRole::Collector)
    {
        return QuestRules::MaterialCount(shape, inventory);
    }
    return (std::min)(target, state.progress);
}

EconomyResult NpcState::Recruit(const NpcKey& key, const PlayerEconomy& economy)
{
    auto& npc = m_npcs[key];
    if (npc.quest != QuestStatus::Complete)
    {
        return {false, "이 NPC의 의뢰를 먼저 완료하세요."};
    }
    if (npc.recruited)
    {
        return {false, "이미 동료입니다."};
    }
    if (!economy.HasShip())
    {
        return {false, "동료가 탈 선박을 먼저 마련하세요."};
    }
    if (m_companions.size() >= EconomyRules::CompanionCapacity)
    {
        return {false, "선박의 동료 정원이 가득 찼습니다."};
    }
    npc.recruited = true;
    m_companions.push_back(key);
    AdjustAffinity(key, 0, "함께 항해하기로 했다");
    return {true, "동료가 되었습니다. 육지에서는 따라오고 승선하면 내 배에 탑승합니다."};
}

EconomyResult NpcState::Dismiss(const NpcKey& key)
{
    auto found = m_npcs.find(key);
    if (found == m_npcs.end() || !found->second.recruited)
    {
        return {false, "영입한 동료가 아닙니다."};
    }
    found->second.recruited = false;
    m_companions.erase(std::remove(m_companions.begin(), m_companions.end(), key), m_companions.end());
    AdjustAffinity(key, 0, "동료 계약을 해제했다");
    return {true, "동료를 해제했습니다. 원래 마을에서 다시 영입할 수 있습니다."};
}
