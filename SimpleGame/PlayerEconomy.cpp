#include "stdafx.h"
#include "PlayerEconomy.h"
#include "Diagnostics.h"
#include <algorithm>
#include <limits>

const char* EconomyRules::LootName(CoralLoot kind)
{
    return kind == CoralLoot::Boss ? "보스 산호 전리품" : "산호 전리품";
}

std::int64_t EconomyRules::SellPrice(CoralLoot kind)
{
    return kind == CoralLoot::Boss ? 20 : 5;
}

std::int64_t EconomyRules::StackLimit(CoralLoot kind)
{
    // Per-item policy: new items can have different limits without changing cargo.
    switch (kind)
    {
        case CoralLoot::Normal:
            return 20;
        case CoralLoot::Boss:
            return 20;
    }
    return 0;
}

std::int64_t& PlayerEconomy::Bag(CoralLoot kind)
{
    return kind == CoralLoot::Boss ? m_boss : m_normal;
}

std::int64_t PlayerEconomy::LootCount(CoralLoot kind) const
{
    return kind == CoralLoot::Boss ? m_boss : m_normal;
}

bool PlayerEconomy::Collect(CoralLoot kind)
{
    auto& count = Bag(kind);
    if (count == (std::numeric_limits<std::int64_t>::max)())
    {
        return false;
    }
    ++count;
    Diagnostics::Get().Event(
        "coral_loot_collected kind=" + std::to_string(static_cast<int>(kind)) +
        " bag_count=" + std::to_string(count));
    return true;
}

EconomyResult PlayerEconomy::SellAll()
{
    if (!m_normal && !m_boss)
    {
        return {false, "판매할 산호 전리품이 없습니다."};
    }
    const auto maximum = (std::numeric_limits<std::int64_t>::max)();
    const auto normalPrice = EconomyRules::SellPrice(CoralLoot::Normal);
    const auto bossPrice = EconomyRules::SellPrice(CoralLoot::Boss);
    if (m_normal > (maximum - m_gold) / normalPrice)
    {
        return {false, "골드 보유 한도를 초과합니다."};
    }
    const auto normalValue = m_normal * normalPrice;
    if (m_boss > (maximum - m_gold - normalValue) / bossPrice)
    {
        return {false, "골드 보유 한도를 초과합니다."};
    }
    const auto revenue = normalValue + m_boss * bossPrice;
    Diagnostics::Get().Event(
        "coral_loot_sold normal=" + std::to_string(m_normal) + " boss=" + std::to_string(m_boss) +
        " revenue=" + std::to_string(revenue) + " gold_before=" + std::to_string(m_gold));
    m_gold += revenue;
    m_normal = m_boss = 0;
    return {true, "전리품을 판매했습니다. +" + std::to_string(revenue) + "골드"};
}

EconomyResult PlayerEconomy::BuySailboat()
{
    if (m_hasShip)
    {
        return {false, "이미 8m 돛단배를 소유하고 있습니다. 상위 단계는 준비 중입니다."};
    }
    if (m_gold < EconomyRules::SailboatPrice)
    {
        return {false, "골드가 부족합니다. 돛단배 구매에는 50골드가 필요합니다."};
    }
    m_gold -= EconomyRules::SailboatPrice;
    m_hasShip = true;
    Diagnostics::Get().Event(
        "sailboat_purchased price=50 length_m=8 companions=1 cargo_kinds=5 gold=" + std::to_string(m_gold));
    return {true, "8m 돛단배를 구매했습니다! 동료 1명 / 적재 5종류. 부두에서 승선하세요."};
}

size_t PlayerEconomy::UsedCargoKinds() const
{
    size_t count = 0;
    for (const auto& slot : m_cargo)
    {
        count += slot.occupied ? 1 : 0;
    }
    return count;
}

EconomyResult PlayerEconomy::Store(CoralLoot kind)
{
    if (!m_hasShip)
    {
        return {false, "돛단배를 먼저 구매하세요."};
    }
    auto& bag = Bag(kind);
    if (!bag)
    {
        return {false, "소지품에 해당 전리품이 없습니다."};
    }
    CargoSlot* selected = nullptr;
    for (auto& slot : m_cargo)
    {
        if (slot.occupied && slot.kind == kind)
        {
            selected = &slot;
            break;
        }
    }
    if (!selected)
    {
        for (auto& slot : m_cargo)
        {
            if (!slot.occupied)
            {
                selected = &slot;
                break;
            }
        }
    }
    if (!selected)
    {
        return {false, "적재 가능한 아이템 종류 5칸을 모두 사용 중입니다."};
    }
    const auto limit = EconomyRules::StackLimit(kind);
    const auto existing = selected->occupied ? selected->count : 0;
    const auto moved = (std::min)(bag, limit - existing);
    if (moved <= 0)
    {
        return {false, "이 아이템은 종류별 최대 중첩 수에 도달했습니다."};
    }
    selected->occupied = true;
    selected->kind = kind;
    selected->count = existing + moved;
    bag -= moved;
    Diagnostics::Get().Event(
        "cargo_store kind=" + std::to_string(static_cast<int>(kind)) + " count=" + std::to_string(moved));
    return {
        true,
        std::string(EconomyRules::LootName(kind)) + " " + std::to_string(moved) + "개를 배에 실었습니다."};
}

EconomyResult PlayerEconomy::Unload(CoralLoot kind)
{
    if (!m_hasShip)
    {
        return {false, "돛단배를 먼저 구매하세요."};
    }
    for (auto& slot : m_cargo)
    {
        if (!slot.occupied || slot.kind != kind)
        {
            continue;
        }
        auto& bag = Bag(kind);
        if (slot.count > (std::numeric_limits<std::int64_t>::max)() - bag)
        {
            return {false, "소지품 보유 한도를 초과합니다."};
        }
        const auto moved = slot.count;
        bag += moved;
        slot = CargoSlot{};
        Diagnostics::Get().Event(
            "cargo_unload kind=" + std::to_string(static_cast<int>(kind)) +
            " count=" + std::to_string(moved));
        return {
            true,
            std::string(EconomyRules::LootName(kind)) + " " + std::to_string(moved) + "개를 내렸습니다."};
    }
    return {false, "배에 해당 전리품이 없습니다."};
}
