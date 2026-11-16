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

void GameUi::RenderTrade(Renderer& r) const
{
    const Color ink = {.07f, .19f, .23f}, paper = {.96f, .90f, .72f}, gold = {.98f, .71f, .32f};
    const float w = (std::min)(780.f, r.Width() - 24.f), h = (std::min)(570.f, r.Height() - 24.f);
    const float x = (r.Width() - w) * .5f, y = (r.Height() - h) * .5f;
    if (w < 400 || h < 440)
    {
        r.Text(12, 12, "상점은 424×464 이상에서 표시됩니다. E / Esc: 닫기", gold, 1.2f, r.Width() - 24.f);
        return;
    }
    r.Panel(x, y, w, h, ink);
    const NpcKey merchant = {m_service.island, 2};
    const auto& state = static_cast<const NpcState&>(m_relations).Npc(merchant);
    r.Text(
        x + 20,
        y + 16,
        std::string(m_buying ? "상점 · 특산품 구매" : "상점 · 소지품 판매") + " / 골드 " +
            std::to_string(m_economy.Gold()),
        gold,
        1.8f,
        w - 40);
    r.Text(
        x + 20,
        y + 49,
        "상인 호감도 " + std::to_string(state.affinity) + " · 높을수록 구매가↓ 판매가↑",
        paper,
        1.3f,
        w - 40);
    r.Text(x + 20, y + 78, "Tab: 구매·판매 전환 / W·S: 선택 / Enter: 1개 거래", paper, 1.25f, w - 40);
    const auto items = MarketItems();
    const unsigned selected = items.empty() ? 0 : (std::min)(m_marketSelection, unsigned(items.size() - 1));
    const unsigned first = (selected / 7) * 7;
    for (unsigned row = 0; row < 7 && first + row < items.size(); ++row)
    {
        const unsigned index = first + row;
        const auto item = items[index];
        const float Y = y + 118 + row * 39;
        if (index == selected)
        {
            r.Panel(x + 14, Y - 4, w - 28, 36, {.18f, .33f, .34f});
        }
        const auto price = m_buying ? TradeRules::Buy(MarketShape(), item, state.affinity)
                                    : TradeRules::Sell(MarketShape(), item, state.affinity);
        r.Icon(m_icons.Get(ItemCatalog::Get(item).icon), x + 22, Y, 26, 26);
        r.Text(x + 60, Y, ItemCatalog::Get(item).name, paper, 1.35f, w * .45f);
        r.Text(
            x + w * .57f,
            Y,
            std::to_string(price) + "골드 · 소지 " + std::to_string(m_economy.ItemCount(item)),
            gold,
            1.25f,
            w * .4f - 20);
    }
    if (items.empty())
    {
        r.Text(
            x + 24,
            y + 130,
            "판매할 소지품이 없습니다. 적재품은 K에서 먼저 내리세요.",
            paper,
            1.4f,
            w - 48);
    }
    r.Text(
        x + 20,
        y + h - 130,
        m_buying ? "산지 특산품을 구매해 다른 종류의 섬에 판매하면 더 높은 가격을 받습니다."
                 : "[P] 매입가 두 배 요구: 거절·호감도 -5 (판매하지 않음)",
        paper,
        1.2f,
        w - 40);
    r.Text(x + 20, y + h - 91, m_serviceMessage, gold, 1.3f, w - 40);
    r.Text(
        x + 20,
        y + h - 36,
        "E / Esc: 닫기 · I: 인벤토리 · 일반 거래 성공 시 호감도 +1",
        paper,
        1.25f,
        w - 40);
}
