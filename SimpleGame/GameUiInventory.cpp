#include "stdafx.h"
#include "GameUi.h"
#include <algorithm>

using namespace UiTheme;

namespace
{
    constexpr unsigned PageSize = 16;
}

std::vector<InventoryItem> GameUi::BagItems() const
{
    std::vector<InventoryItem> result;
    for (unsigned i = 0; i < ItemCatalog::Count; ++i)
    {
        const auto item = static_cast<InventoryItem>(i);
        if (m_economy.ItemCount(item) > 0)
        {
            result.push_back(item);
        }
    }
    return result;
}

void GameUi::ToggleInventory()
{
    if (m_inventoryOpen)
    {
        CloseServiceMenu();
        return;
    }
    while (CloseServiceMenu())
    {
    }
    m_inventoryOpen = true;
    m_inventorySelection = 0;
    m_serviceMessage.clear();
    m_player.StopFishing();
    ClearInput();
}

void GameUi::InventoryKey(unsigned char key)
{
    if (key == 27)
    {
        CloseServiceMenu();
        return;
    }
    const auto items = BagItems();
    if (items.empty())
    {
        m_inventorySelection = 0;
        return;
    }
    const unsigned last = static_cast<unsigned>(items.size() - 1);
    m_inventorySelection = (std::min)(last, m_inventorySelection);
    if (key == 'a' && m_inventorySelection > 0)
    {
        --m_inventorySelection;
    }
    if (key == 'd' && m_inventorySelection < last)
    {
        ++m_inventorySelection;
    }
    if (key == 'w')
    {
        m_inventorySelection = m_inventorySelection >= 4 ? m_inventorySelection - 4 : 0;
    }
    if (key == 's')
    {
        m_inventorySelection = (std::min)(last, m_inventorySelection + 4);
    }
    if (key == '[')
    {
        m_inventorySelection = m_inventorySelection >= PageSize ? m_inventorySelection - PageSize : 0;
    }
    if (key == ']')
    {
        m_inventorySelection = (std::min)(last, m_inventorySelection + PageSize);
    }
    if (key == 13 || key == 'e')
    {
        m_serviceMessage = m_economy.Equip(items[m_inventorySelection]).message;
        m_combat.EquipmentChanged();
        const auto remaining = BagItems();
        m_inventorySelection =
            remaining.empty() ? 0 : (std::min)(m_inventorySelection, unsigned(remaining.size() - 1));
    }
}

void GameUi::RenderInventory(UiCanvas& ui) const
{
    const auto box = ui.Center(848, 530);
    const float x = box.x + 28, y = box.y, gearX = x + 450;
    const auto items = BagItems();
    const unsigned selected =
        items.empty() ? 0 : (std::min)(m_inventorySelection, unsigned(items.size() - 1));
    const unsigned page = selected / PageSize;
    const unsigned pages = (std::max)(1u, unsigned((items.size() + PageSize - 1) / PageSize));
    ui.Header(box, "인벤토리", "◉ " + std::to_string(m_economy.Gold()) + " 골드");
    ui.Text(x, y + 113, "가방", 2);
    ui.Text(
        x + 261,
        y + 115,
        std::to_string(page + 1) + " / " + std::to_string(pages) + " 페이지",
        1.5f,
        Muted);
    if (pages > 1)
    {
        ui.Button({x + 366, y + 107, 28, 30}, "‹", UiAction::Key, '[', false, page > 0);
        ui.Button({x + 401, y + 107, 28, 30}, "›", UiAction::Key, ']', false, page + 1 < pages);
    }
    ui.Text(gearX, y + 113, "장비", 2);
    for (unsigned i = 0; i < PageSize; ++i)
    {
        const UiRect cell{x + i % 4 * 109.f, y + 149 + i / 4 * 56.f, 102, 50};
        const unsigned index = page * PageSize + i;
        const bool occupied = index < items.size();
        ui.Card(cell, occupied && index == selected);
        if (occupied)
        {
            const auto item = items[index];
            ui.Hit(cell, UiAction::Bag, index);
            ui.Icon(m_icons.Get(ItemCatalog::Get(item).icon), cell.x + 37, cell.y + 9, 30);
            ui.Text(cell.x + 6, cell.y + 30, std::to_string(m_economy.ItemCount(item)), 1.3f, Ivory, 30, 16);
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        const auto slot = static_cast<EquipSlot>(i);
        const UiRect cell{gearX + i % 2 * 173.f, y + 149 + i / 2 * 69.f, 169, 63};
        ui.Card(cell);
        ui.Text(cell.x + 10, cell.y + 7, ItemCatalog::SlotName(slot), 1.4f, Muted);
        const int equipped = m_economy.Equipped(slot);
        if (equipped >= 0)
        {
            ui.Hit(cell, UiAction::Gear, i);
            const auto& def = ItemCatalog::Get(static_cast<InventoryItem>(equipped));
            ui.Icon(m_icons.Get(def.icon), cell.x + 8, cell.y + 27, 26);
            ui.Text(cell.x + 42, cell.y + 32, def.name, 1.15f, Ivory, 119, 17);
        }
        else
        {
            ui.Text(cell.x + 73, cell.y + 30, "·", 2, Gold);
        }
    }
    const auto& companions = m_relations.Companions();
    ui.Text(
        gearX,
        y + 296,
        "동료 " + std::to_string(companions.size()) + " / " + std::to_string(EconomyRules::CompanionCapacity),
        1.8f);
    for (unsigned i = 0; i < companions.size(); ++i)
    {
        const float row = y + 325 + 36 * i;
        ui.Text(gearX, row + 6, m_npcs->Name(companions[i]), 1.55f, Ivory, 230, 21);
        ui.Button({gearX + 244, row, 98, 32}, "해제", UiAction::Dismiss, i);
    }

    bool equippable = false;
    if (!items.empty())
    {
        const auto& item = ItemCatalog::Get(items[selected]);
        equippable = item.equipment != EquipSlot::None;
        ui.Text(x, y + 382, item.name, 1.8f, Gold, 420, 22);
        const auto detail =
            equippable
                ? "공격력 +" + std::to_string(item.attack) + " · 방어력 +" + std::to_string(item.defence) +
                      " · 체력 +" + std::to_string(item.health)
                : "기준가 " + std::to_string(item.price) + "골드 · 적재 중첩 " + std::to_string(item.stack);
        ui.Text(x, y + 409, detail, 1.45f, Muted, 430, 20);
    }
    ui.Footer(box);
    ui.Text(
        x + 116,
        y + 478,
        m_serviceMessage.empty() ? "[ / ] 페이지 · 장비 클릭으로 해제" : m_serviceMessage,
        1.43f,
        Muted,
        510,
        22);
    ui.Button({x + 659, y + 468, 133, 42}, "Enter  착용", UiAction::Key, 13, true, equippable);
}
