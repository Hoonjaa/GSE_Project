#include "stdafx.h"
#include "PlayerEconomy.h"
#include "Diagnostics.h"
#include <limits>

EconomyResult PlayerEconomy::BuyItem(InventoryItem item, std::int64_t price)
{
    if (price <= 0 || static_cast<unsigned>(item) >= ItemCatalog::Count)
    {
        return {false, "구매할 수 없는 상품입니다."};
    }
    if (m_gold < price)
    {
        return {false, "골드가 부족합니다."};
    }
    if (!CollectItem(item))
    {
        return {false, "아이템 보유 한도를 초과합니다."};
    }
    m_gold -= price;
    Diagnostics::Get().Event(
        "market_buy item=" + std::to_string(static_cast<unsigned>(item)) + " price=" + std::to_string(price));
    return {
        true,
        std::string(EconomyRules::ItemName(item)) + " 1개 구매 / -" + std::to_string(price) + "골드"};
}

EconomyResult PlayerEconomy::SellItem(InventoryItem item, std::int64_t price)
{
    if (price <= 0 || static_cast<unsigned>(item) >= ItemCatalog::Count)
    {
        return {false, "판매할 수 없는 아이템입니다."};
    }
    if (price > (std::numeric_limits<std::int64_t>::max)() - m_gold)
    {
        return {false, "골드 한도를 초과합니다."};
    }
    if (!ConsumeItems(item, 1))
    {
        return {false, "소지품에 해당 아이템이 없습니다."};
    }
    m_gold += price;
    Diagnostics::Get().Event(
        "market_sell item=" + std::to_string(static_cast<unsigned>(item)) +
        " price=" + std::to_string(price));
    return {
        true,
        std::string(EconomyRules::ItemName(item)) + " 1개 판매 / +" + std::to_string(price) + "골드"};
}

EconomyResult PlayerEconomy::Equip(InventoryItem item)
{
    if (static_cast<unsigned>(item) >= ItemCatalog::Count)
    {
        return {false, "알 수 없는 아이템입니다."};
    }
    const auto& definition = ItemCatalog::Get(item);
    const int slot = static_cast<int>(definition.equipment);
    if (slot < 0 || slot >= 4)
    {
        return {false, "장착할 수 없는 아이템입니다."};
    }
    if (m_equipment[slot] == static_cast<int>(item))
    {
        return {false, "이미 같은 장비를 착용하고 있습니다."};
    }
    if (ItemCount(item) <= 0)
    {
        return {false, "가방에 해당 장비가 없습니다."};
    }
    const int old = m_equipment[slot];
    if (old >= 0 && m_bag[old] == (std::numeric_limits<std::int64_t>::max)())
    {
        return {false, "기존 장비를 보관할 공간이 없습니다."};
    }
    --m_bag[static_cast<unsigned>(item)];
    if (old >= 0)
    {
        ++m_bag[old];
    }
    m_equipment[slot] = static_cast<int>(item);
    Diagnostics::Get().Event(
        "equipment_changed slot=" + std::to_string(slot) +
        " item=" + std::to_string(static_cast<unsigned>(item)));
    return {true, definition.name + " 장착 완료"};
}

EconomyResult PlayerEconomy::Unequip(EquipSlot slot)
{
    const int index = static_cast<int>(slot);
    if (index < 0 || index >= 4 || m_equipment[index] < 0)
    {
        return {false, "장착한 장비가 없습니다."};
    }
    const int id = m_equipment[index];
    if (m_bag[id] == (std::numeric_limits<std::int64_t>::max)())
    {
        return {false, "보유 한도를 초과합니다."};
    }
    ++m_bag[id];
    m_equipment[index] = -1;
    Diagnostics::Get().Event("equipment_removed slot=" + std::to_string(index));
    return {true, ItemCatalog::Get(static_cast<InventoryItem>(id)).name + " 장착 해제"};
}

int PlayerEconomy::Equipped(EquipSlot slot) const
{
    const int index = static_cast<int>(slot);
    return index >= 0 && index < 4 ? m_equipment[index] : -1;
}

int PlayerEconomy::EquipmentAttack() const
{
    int result = 0;
    for (int id : m_equipment)
    {
        if (id >= 0)
        {
            result += ItemCatalog::Get(static_cast<InventoryItem>(id)).attack;
        }
    }
    return result;
}

int PlayerEconomy::EquipmentDefence() const
{
    int result = 0;
    for (int id : m_equipment)
    {
        if (id >= 0)
        {
            result += ItemCatalog::Get(static_cast<InventoryItem>(id)).defence;
        }
    }
    return result;
}

int PlayerEconomy::EquipmentHealth() const
{
    int result = 0;
    for (int id : m_equipment)
    {
        if (id >= 0)
        {
            result += ItemCatalog::Get(static_cast<InventoryItem>(id)).health;
        }
    }
    return result;
}
