#include "stdafx.h"
#include "GameUi.h"
#include "TradeRules.h"
#include <algorithm>

IslandShape GameUi::MarketShape() const
{
    const auto* island = m_world.CurrentIsland(Player().location);
    return island ? island->Info().shape : IslandShape::Round;
}

std::vector<InventoryItem> GameUi::MarketItems() const
{
    if (!m_buying)
    {
        return BagItems();
    }
    std::vector<InventoryItem> items;
    for (auto good : MapCatalog::Island(MarketShape()).specialties)
    {
        items.push_back(ItemCatalog::Trade(static_cast<unsigned>(good)));
    }
    return items;
}

void GameUi::TradeKey(unsigned char key)
{
    if (key == 9)
    {
        m_buying = !m_buying;
        m_marketSelection = 0;
        m_serviceMessage.clear();
        return;
    }
    const auto items = MarketItems();
    if (items.empty())
    {
        m_marketSelection = 0;
        return;
    }
    m_marketSelection = (std::min)(m_marketSelection, unsigned(items.size() - 1));
    if (key == 'w')
    {
        if (m_marketSelection)
        {
            --m_marketSelection;
        }
        return;
    }
    if (key == 's')
    {
        m_marketSelection = (std::min)(m_marketSelection + 1, unsigned(items.size() - 1));
        return;
    }
    if (key != 13 && key != 'p')
    {
        return;
    }
    const NpcKey merchant = {m_service.island, NpcRole::Merchant};
    if (key == 'p')
    {
        if (m_buying)
        {
            return;
        }
        m_relations.AdjustAffinity(merchant, -5, "매입가의 두 배를 요구해 거래가 거절되었다");
        m_serviceMessage = "두 배 가격 요구를 거절했습니다. 호감도 -5 / 물건은 유지됩니다.";
        return;
    }
    const auto item = items[m_marketSelection];
    const int affinity = static_cast<const NpcState&>(m_relations).Npc(merchant).affinity;
    const auto price = m_buying ? TradeRules::Buy(MarketShape(), item, affinity)
                                : TradeRules::Sell(MarketShape(), item, affinity);
    const auto result = m_buying ? m_economy.BuyItem(item, price) : m_economy.SellItem(item, price);
    m_serviceMessage = result.message;
    if (result.success)
    {
        m_relations.AdjustAffinity(
            merchant,
            1,
            m_buying ? "상점에서 물건을 구매했다" : "합리적인 가격에 전리품을 판매했다");
        if (m_buying)
        {
            m_relations.RecordTradePurchase(m_service.island);
        }
    }
    const auto next = MarketItems();
    m_marketSelection = next.empty() ? 0 : (std::min)(m_marketSelection, unsigned(next.size() - 1));
}

void GameUi::RenderTrade(UiCanvas& ui) const
{
    using namespace UiTheme;
    const auto box = ui.Center(848, m_buying ? 466 : 526);
    const float x = box.x + 28, y = box.y, footer = y + box.h - 62;
    const NpcKey merchant{m_service.island, NpcRole::Merchant};
    const auto& record = m_relations.Npc(merchant);
    ui.Header(
        box,
        "항구 상점",
        "◉ " + std::to_string(m_economy.Gold()) + " 골드 · 상인 호감도 " + std::to_string(record.affinity));
    ui.Card({x, y + 112, 66, 42}, m_buying);
    ui.Card({x + 74, y + 112, 66, 42}, !m_buying);
    ui.Text(x + 13, y + 124, "구매", 1.75f);
    ui.Text(x + 87, y + 124, "판매", 1.75f);
    ui.Hit({x + (m_buying ? 74.f : 0.f), y + 112, 66, 42}, UiAction::Key, 9);
    const auto items = MarketItems();
    const unsigned selected = items.empty() ? 0 : (std::min)(m_marketSelection, unsigned(items.size() - 1));
    const unsigned pageSize = m_buying ? 4 : 5;
    const unsigned first = (selected / pageSize) * pageSize;
    for (unsigned i = first; i < first + pageSize && i < items.size(); ++i)
    {
        const UiRect row{x, y + 165 + (i - first) * 49.f, 792, 44};
        const auto item = items[i];
        const auto price = m_buying ? TradeRules::Buy(MarketShape(), item, record.affinity)
                                    : TradeRules::Sell(MarketShape(), item, record.affinity);
        ui.Card(row, i == selected);
        ui.Hit(row, UiAction::Market, i);
        ui.Icon(m_icons.Get(ItemCatalog::Get(item).icon), row.x + 13, row.y + 10, 24);
        ui.Text(row.x + 50, row.y + 11, ItemCatalog::Get(item).name, 1.8f, Ivory, 516, 23);
        ui.Text(
            row.x + 580,
            row.y + 11,
            std::to_string(price) + "골드 · 소지 " + std::to_string(m_economy.ItemCount(item)),
            1.65f,
            Gold,
            203,
            22);
    }
    if (items.empty())
    {
        ui.Text(x, y + 177, "판매할 소지품이 없습니다.", 1.6f, Muted);
    }
    if (items.size() > pageSize)
    {
        ui.Button({x + 660, y + 112, 60, 42}, "W  ↑", UiAction::Key, 'w');
        ui.Button({x + 732, y + 112, 60, 42}, "S  ↓", UiAction::Key, 's');
    }
    ui.Text(x + 148, footer + 11, m_serviceMessage, 1.45f, Gold, 355, 20);
    ui.Footer(box);
    if (!m_buying)
    {
        ui.Button({x + 522, footer, 108, 42}, "P  흥정", UiAction::Key, 'p', false, !items.empty());
    }
    ui.Button(
        {x + 638, footer, 154, 42},
        m_buying ? "Enter  1개 구매" : "Enter  1개 판매",
        UiAction::Key,
        13,
        true,
        !items.empty());
}
