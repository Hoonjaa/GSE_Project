#include "stdafx.h"
#include "QuestRules.h"
#include "MonsterCatalog.h"
#include <algorithm>
#include <stdexcept>

namespace
{
    const QuestDefinition Quests[] = {
        {NpcRole::Hunter, "사냥꾼", "이 섬의 일반 몬스터", 3, 10, 5, 3},
        {NpcRole::Collector, "수집가", "이 섬 몬스터의 전리품", 3, 10, 5, 3},
        {NpcRole::Merchant, "상인", "이 섬 상점의 특산품", 3, 10, 5, 3}};
    static_assert(sizeof(Quests) / sizeof(Quests[0]) == NpcRole::Count, "Quest roles must match NPC roles");
}

const QuestDefinition& QuestRules::Get(unsigned role)
{
    if (role >= NpcRole::Count)
    {
        throw std::out_of_range("NPC role");
    }
    return Quests[role];
}

const char* QuestRules::Title(unsigned role)
{
    static const std::array<const char*, NpcRole::Count> titles{
        {"항구 주변의 위험", "섬의 전리품", "항구의 첫 거래"}};
    return titles.at(role);
}

const char* QuestRules::StatusLabel(QuestStatus status, int progress, int target)
{
    if (status == QuestStatus::Complete)
    {
        return "완료";
    }
    if (status == QuestStatus::Available)
    {
        return "수락 가능";
    }
    return progress >= target ? "보고 가능" : "진행 중";
}

std::string QuestRules::Description(unsigned role)
{
    const auto& quest = Get(role);
    const std::string count = std::to_string(quest.target);
    if (role == NpcRole::Hunter)
    {
        return std::string(quest.objective) + " " + count + "마리를 처치해 주세요.";
    }
    if (role == NpcRole::Collector)
    {
        return std::string(quest.objective) + " " + count + "개를 가져다 주세요.";
    }
    return std::string(quest.objective) + " " + count + "개를 구매해 주세요.";
}

int QuestRules::MaterialCount(IslandShape shape, const PlayerEconomy& inventory)
{
    const int target = Get(NpcRole::Collector).target;
    int total = 0;
    for (unsigned i = 0; i < MonsterCatalog::SlotsPerIsland; ++i)
    {
        total += int((std::min)(
            std::int64_t(target - total),
            inventory.ItemCount(ItemCatalog::MonsterMaterial(MonsterCatalog::ForIsland(shape, i)))));
        if (total >= target)
        {
            break;
        }
    }
    return total;
}

bool QuestRules::DeliverMaterials(IslandShape shape, int quantity, PlayerEconomy& inventory)
{
    if (quantity <= 0 || MaterialCount(shape, inventory) < quantity)
    {
        return false;
    }
    for (unsigned i = 0; i < MonsterCatalog::SlotsPerIsland && quantity > 0; ++i)
    {
        const auto item = ItemCatalog::MonsterMaterial(MonsterCatalog::ForIsland(shape, i));
        const auto used = (std::min)(std::int64_t(quantity), inventory.ItemCount(item));
        if (used > 0)
        {
            inventory.ConsumeItems(item, used);
            quantity -= int(used);
        }
    }
    return quantity == 0;
}
