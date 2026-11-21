#include "stdafx.h"
#include "GameUi.h"
#include "Diagnostics.h"
#include <algorithm>
#include <cmath>
#include <string>

bool GameUi::CloseServiceMenu()
{
    if (m_page != Page::None)
    {
        m_page = m_page == Page::Help ? Page::Pause : Page::None;
        ClearInput();
        return true;
    }
    if (m_inventoryOpen || m_talk.valid)
    {
        m_inventoryOpen = false;
        m_talk = {};
        if (m_npcs)
        {
            m_npcs->Close();
        }
        ClearInput();
        return true;
    }
    if (m_cargoOpen)
    {
        m_cargoOpen = false;
        ClearInput();
        return true;
    }
    if (m_service.kind == TownServiceKind::None)
    {
        return false;
    }
    m_service = TownInteraction{};
    m_serviceMessage.clear();
    ClearInput();
    return true;
}

bool GameUi::ShipAtService() const
{
    if (!m_economy.HasShip() || m_service.kind != TownServiceKind::Shipyard)
    {
        return false;
    }
    const auto mooring = m_world.ShipyardMooring(m_service.island, EconomyRules::SailboatLength);
    const auto delta = ShipActor().location.RelativeTo(mooring);
    return std::hypot(delta.x, delta.z) <= 3.4f;
}

void GameUi::BuySailboat()
{
    if (m_economy.HasShip() || m_economy.Gold() < EconomyRules::SailboatPrice)
    {
        m_serviceMessage = m_economy.BuySailboat().message;
        return;
    }
    const auto mooring = m_world.ShipyardMooring(m_service.island, EconomyRules::SailboatLength);
    const float heading = m_world.OutboundHeading(m_service.island);
    if (!m_world.CanSail(mooring, heading, EconomyRules::SailboatLength))
    {
        m_serviceMessage = "부두 주변을 로딩 중이거나 정박 공간이 부족합니다. 잠시 후 다시 시도하세요.";
        Diagnostics::Get().Event("sailboat_purchase_waiting_for_mooring");
        return;
    }
    const auto result = m_economy.BuySailboat();
    m_serviceMessage = result.message;
    if (result.success)
    {
        m_player.SetShipPose(mooring, heading);
    }
}

void GameUi::ServiceKey(unsigned char key)
{
    if (key == 'e' || key == 27)
    {
        CloseServiceMenu();
        return;
    }
    if (m_service.kind == TownServiceKind::Shop)
    {
        TradeKey(key);
        return;
    }
    if (m_service.kind != TownServiceKind::Shipyard)
    {
        return;
    }
    if (key == 'k')
    {
        ToggleCargo();
        return;
    }
    if (key == 'b' || key == 'u')
    {
        BuySailboat();
        return;
    }
    if (key < '1' || key > '4')
    {
        return;
    }
    if (!ShipAtService())
    {
        m_serviceMessage = m_economy.HasShip() ? "이 마을의 부두에 소유 선박을 정박한 뒤 이용하세요."
                                               : "돛단배를 먼저 구매하세요.";
        return;
    }
    switch (key)
    {
        case '1':
            m_serviceMessage = m_economy.StoreItem(InventoryItem::NormalCoral).message;
            break;
        case '2':
            m_serviceMessage = m_economy.StoreItem(InventoryItem::BossCoral).message;
            break;
        case '3':
            m_serviceMessage = m_economy.UnloadItem(InventoryItem::NormalCoral).message;
            break;
        case '4':
            m_serviceMessage = m_economy.UnloadItem(InventoryItem::BossCoral).message;
            break;
    }
}

void GameUi::RenderServiceMenu(UiCanvas& ui) const
{
    using namespace UiTheme;
    if (m_service.kind == TownServiceKind::Shop)
    {
        RenderTrade(ui);
        return;
    }
    const auto box = ui.Center(848, 424);
    const float x = box.x + 28, y = box.y;
    ui.Header(
        box,
        "조선소",
        "◉ " + std::to_string(m_economy.Gold()) + " 골드 · " + MapCatalog::Island(MarketShape()).name +
            " 부두");
    ui.Fill({x, y + 112, 430, 207}, Deep);
    ui.Compass(x + 176, y + 143, 78);
    ui.Text(x + 166, y + 235, "8m 돛단배", 2, Gold);
    ui.Text(x + 153, y + 264, m_economy.HasShip() ? "보유 중" : "구매 가능", 1.55f, Muted);
    const std::array<const char*, 4> labels{{"선박 가격", "최대 내구도", "적재 공간", "동료 정원"}};
    const std::array<std::string, 4> values{
        {std::to_string(EconomyRules::SailboatPrice) + "골드",
         "100",
         "물품 " + std::to_string(EconomyRules::CargoKinds) + "종",
         std::to_string(EconomyRules::CompanionCapacity) + "명"}};
    for (unsigned i = 0; i < labels.size(); ++i)
    {
        const UiRect row{x + 450, y + 112 + i * 52.f, 342, 46};
        ui.Card(row);
        ui.Text(row.x + 12, row.y + 12, labels[i], 1.8f);
        ui.Text(row.x + 225, row.y + 12, values[i], 1.7f, i ? Ivory : Gold, 110, 24);
    }
    ui.Text(x, y + 325, m_serviceMessage, 1.4f, Gold, 790, 20);
    ui.Footer(box);
    const bool owned = m_economy.HasShip();
    ui.Button(
        {x + 625, y + 362, 167, 42},
        owned ? "K  적재창" : "B  돛단배 구매",
        UiAction::Key,
        owned ? 'k' : 'b',
        true,
        owned ? ShipAtService() : m_economy.Gold() >= EconomyRules::SailboatPrice);
}
