#include "stdafx.h"
#include "PlayerEconomy.h"
#include "Diagnostics.h"
#include "MapCatalog.h"
#include <algorithm>
#include <limits>

PlayerEconomy::PlayerEconomy()
{
    Diagnostics::Get().Event("economy_initial_gold=" + std::to_string(m_gold) + " reason=monster_preview");
}

InventoryItem EconomyRules::CoralItem(CoralLoot kind)
{
    return kind == CoralLoot::Boss ? InventoryItem::BossCoral : InventoryItem::NormalCoral;
}

const char* EconomyRules::ItemName(InventoryItem item)
{
    const auto id = static_cast<unsigned>(item);
    return id < ItemCatalog::Count ? ItemCatalog::Get(item).name.c_str() : "알 수 없는 아이템";
}

std::int64_t EconomyRules::ItemPrice(InventoryItem item)
{
    return static_cast<unsigned>(item) < ItemCatalog::Count ? ItemCatalog::Get(item).price : 0;
}

std::int64_t EconomyRules::ItemStack(InventoryItem item)
{
    return static_cast<unsigned>(item) < ItemCatalog::Count ? ItemCatalog::Get(item).stack : 0;
}

const char* EconomyRules::LootName(CoralLoot kind)
{
    return ItemName(CoralItem(kind));
}

std::int64_t EconomyRules::SellPrice(CoralLoot kind)
{
    return ItemPrice(CoralItem(kind));
}

std::int64_t EconomyRules::StackLimit(CoralLoot kind)
{
    return ItemStack(CoralItem(kind));
}

std::int64_t PlayerEconomy::LootCount(CoralLoot kind) const
{
    return ItemCount(EconomyRules::CoralItem(kind));
}

std::int64_t PlayerEconomy::ItemCount(InventoryItem item) const
{
    const auto id = static_cast<size_t>(item);
    return id < m_bag.size() ? m_bag[id] : 0;
}

bool PlayerEconomy::Collect(CoralLoot kind)
{
    return CollectItem(EconomyRules::CoralItem(kind));
}

bool PlayerEconomy::CollectItem(InventoryItem item)
{
    return CollectItems(item, 1);
}

bool PlayerEconomy::CollectItems(InventoryItem item, std::int64_t count)
{
    const auto id = static_cast<size_t>(item);
    if (id >= m_bag.size() || count <= 0 || count > (std::numeric_limits<std::int64_t>::max)() - m_bag[id])
    {
        return false;
    }
    m_bag[id] += count;
    Diagnostics::Get().Event(
        "item_collected id=" + std::to_string(id) + " count=" + std::to_string(count) +
        " bag=" + std::to_string(m_bag[id]));
    return true;
}

bool PlayerEconomy::ConsumeItems(InventoryItem item, std::int64_t count)
{
    const auto id = static_cast<size_t>(item);
    if (id >= m_bag.size() || count <= 0 || m_bag[id] < count)
    {
        return false;
    }
    m_bag[id] -= count;
    return true;
}

EconomyResult PlayerEconomy::CollectPirateLoot(int gold, int tentacles)
{
    const auto limit = (std::numeric_limits<std::int64_t>::max)();
    auto& item = m_bag[static_cast<size_t>(InventoryItem::Tentacle)];
    if (gold < 0 || tentacles < 0 || gold > limit - m_gold || tentacles > limit - item)
    {
        return {false, "보유 한도를 초과해 약탈품을 회수할 수 없습니다."};
    }
    m_gold += gold;
    item += tentacles;
    Diagnostics::Get().Event(
        "pirate_loot_claimed gold=" + std::to_string(gold) + " tentacles=" + std::to_string(tentacles));
    return {
        true,
        "약탈 완료! 골드 +" + std::to_string(gold) + " / 크라켄 촉수 +" + std::to_string(tentacles)};
}

EconomyResult PlayerEconomy::BuySailboat()
{
    if (m_hasShip)
    {
        return {false, "이미 8m 돛단배를 소유하고 있습니다. 상위 단계는 준비 중입니다."};
    }
    if (m_gold < EconomyRules::SailboatPrice)
    {
        return {false, "돛단배 구매에는 50골드가 필요합니다."};
    }
    m_gold -= EconomyRules::SailboatPrice;
    m_hasShip = true;
    Diagnostics::Get().Event("sailboat_purchased price=50 length=8 companions=2 cargo_kinds=5");
    return {true, "8m 돛단배 구매 완료! 부두에서 E로 승선하세요."};
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
    return StoreItem(EconomyRules::CoralItem(kind));
}

EconomyResult PlayerEconomy::Unload(CoralLoot kind)
{
    return UnloadItem(EconomyRules::CoralItem(kind));
}

EconomyResult PlayerEconomy::StoreItem(InventoryItem item)
{
    if (!m_hasShip)
    {
        return {false, "돛단배를 먼저 구매하세요."};
    }
    const auto id = static_cast<size_t>(item), limit = static_cast<size_t>(EconomyRules::ItemStack(item));
    if (id >= m_bag.size() || !limit)
    {
        return {false, "이 아이템의 중첩 한도가 아직 설정되지 않았습니다."};
    }
    auto& bag = m_bag[id];
    if (!bag)
    {
        return {false, "소지품에 해당 아이템이 없습니다."};
    }
    CargoSlot* selected = nullptr;
    for (auto& slot : m_cargo)
    {
        if (slot.occupied && slot.item == item)
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
        return {false, "5종류 적재 슬롯을 모두 사용 중입니다."};
    }
    const auto existing = selected->occupied ? selected->count : 0;
    const auto moved = (std::min)(bag, static_cast<std::int64_t>(limit) - existing);
    if (moved <= 0)
    {
        return {false, "종류별 최대 중첩 수에 도달했습니다."};
    }
    selected->occupied = true;
    selected->item = item;
    selected->count = existing + moved;
    bag -= moved;
    Diagnostics::Get().Event("cargo_store item=" + std::to_string(id) + " count=" + std::to_string(moved));
    return {true, std::string(EconomyRules::ItemName(item)) + " " + std::to_string(moved) + "개 적재"};
}

EconomyResult PlayerEconomy::UnloadItem(InventoryItem item)
{
    if (!m_hasShip)
    {
        return {false, "돛단배를 먼저 구매하세요."};
    }
    const auto id = static_cast<size_t>(item);
    if (id >= m_bag.size())
    {
        return {false, "알 수 없는 아이템입니다."};
    }
    for (auto& slot : m_cargo)
    {
        if (!slot.occupied || slot.item != item)
        {
            continue;
        }
        if (slot.count > (std::numeric_limits<std::int64_t>::max)() - m_bag[id])
        {
            return {false, "소지품 보유 한도를 초과합니다."};
        }
        const auto moved = slot.count;
        m_bag[id] += moved;
        slot = CargoSlot{};
        Diagnostics::Get().Event(
            "cargo_unload item=" + std::to_string(id) + " count=" + std::to_string(moved));
        return {true, std::string(EconomyRules::ItemName(item)) + " " + std::to_string(moved) + "개 내리기"};
    }
    return {false, "배에 해당 아이템이 없습니다."};
}

std::int64_t PlayerEconomy::CargoUnits() const
{
    std::int64_t total = 0;
    for (const auto& slot : m_cargo)
    {
        if (slot.occupied)
        {
            total += slot.count;
        }
    }
    return total;
}

int PlayerEconomy::LoseCargoUnits(int amount, std::mt19937& random)
{
    int lost = 0;
    while (lost < amount)
    {
        const auto total = CargoUnits();
        if (total <= 0)
        {
            break;
        }
        auto pick = std::uniform_int_distribution<std::int64_t>(0, total - 1)(random);
        for (auto& slot : m_cargo)
        {
            if (!slot.occupied)
            {
                continue;
            }
            if (pick >= slot.count)
            {
                pick -= slot.count;
                continue;
            }
            Diagnostics::Get().Event(
                "cargo_lost item=" + std::to_string(static_cast<unsigned>(slot.item)) + " count=1");
            --slot.count;
            if (!slot.count)
            {
                slot = CargoSlot{};
            }
            ++lost;
            break;
        }
    }
    return lost;
}
