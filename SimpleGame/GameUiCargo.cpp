#include "stdafx.h"
#include "GameUi.h"
#include <algorithm>

void GameUi::ToggleCargo()
{
    if (m_player.Mode() == PlayerMode::PirateDeck)
    {
        m_player.Notify("내 배에 돌아온 뒤 적재창을 이용하세요.", 4);
        return;
    }
    if (!m_cargoOpen && (!m_economy.HasShip() || (!Aboard() && !ShipAtService())))
    {
        m_serviceMessage = "적재창은 배 위 또는 소유 선박이 정박한 마을의 조선소에서 이용하세요.";
        m_player.Notify(m_serviceMessage, 4);
        return;
    }
    m_cargoOpen = !m_cargoOpen;
    m_serviceMessage.clear();
    m_player.StopFishing();
    ClearInput();
}

void GameUi::CargoKey(unsigned char key)
{
    if (key == 'k' || key == 'e' || key == 27)
    {
        m_cargoOpen = false;
        ClearInput();
        return;
    }
    const auto items = CargoItems();
    if (items.empty())
    {
        m_cargoSelection = 0;
        return;
    }
    m_cargoSelection = (std::min)(m_cargoSelection, unsigned(items.size() - 1));
    if (key == 'w')
    {
        m_cargoSelection = (m_cargoSelection + unsigned(items.size()) - 1) % unsigned(items.size());
    }
    else if (key == 's')
    {
        m_cargoSelection = (m_cargoSelection + 1) % unsigned(items.size());
    }
    else if (key == 'a' || key == 'd')
    {
        if (!Aboard() && !ShipAtService())
        {
            m_serviceMessage = "선박에 접근한 뒤 다시 이용하세요.";
            return;
        }
        const auto item = items[m_cargoSelection];
        m_serviceMessage = (key == 'd' ? m_economy.StoreItem(item) : m_economy.UnloadItem(item)).message;
    }
}

std::vector<InventoryItem> GameUi::CargoItems() const
{
    auto items = BagItems();
    for (const auto& slot : m_economy.Cargo())
    {
        if (slot.occupied && std::find(items.begin(), items.end(), slot.item) == items.end())
        {
            items.push_back(slot.item);
        }
    }
    return items;
}
