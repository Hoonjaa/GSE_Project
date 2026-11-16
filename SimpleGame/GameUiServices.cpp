#include "stdafx.h"
#include "GameUi.h"
#include "Diagnostics.h"
#include <algorithm>
#include <cmath>
#include <string>

bool GameUi::CloseServiceMenu()
{
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

void GameUi::RenderServiceMenu(Renderer& renderer) const
{
    if (m_inventoryOpen || m_talk.valid || m_cargoOpen || m_service.kind == TownServiceKind::None)
    {
        return;
    }
    if (m_service.kind == TownServiceKind::Shop)
    {
        RenderTrade(renderer);
        return;
    }
    if (renderer.Width() < 300 || renderer.Height() < 430)
    {
        renderer.Text(
            12,
            12,
            "시설 메뉴를 보려면 창을 키워주세요. E 또는 Esc: 닫기",
            {.98f, .76f, .35f},
            1,
            static_cast<float>((std::max)(1, renderer.Width() - 24)));
        return;
    }
    const float width = (std::min)(640.f, float(renderer.Width() - 24));
    const float height = 400.f;
    const float left = (renderer.Width() - width) * .5f;
    const float top = (std::max)(12.f, (renderer.Height() - height) * .5f);
    const Color ink = {.06f, .16f, .22f}, paper = {.96f, .90f, .72f}, gold = {.98f, .76f, .35f};
    renderer.Panel(left, top, width, height, ink);
    renderer.Panel(left, top, width, 4, Color{.28f, .65f, .75f});
    renderer.Text(left + 20, top + 16, "조선소 · 선박 구매 및 업그레이드", gold, 2, width - 40);
    renderer.Text(left + 20, top + 48, "보유 골드: " + std::to_string(m_economy.Gold()), paper, 1.6f);
    renderer.Text(left + 20, top + 72, "I: 인벤토리·장비 / K: 선박 적재창", paper, 1.4f, width - 40);
    renderer.Text(left + 20, top + 100, "배 미소유 → 8m 돛단배 / 50골드", gold, 1.5f);
    renderer.Text(
        left + 20,
        top + 124,
        "동료 정원 " + std::to_string(EconomyRules::CompanionCapacity) +
            "명 / 적재 5종류 / 품목별 중첩 상한 적용",
        paper,
        1.25f,
        width - 40);
    renderer.Text(
        left + 20,
        top + 148,
        m_economy.HasShip() ? "현재 단계 구매 완료 · 상위 업그레이드는 준비 중입니다."
                            : "[B 또는 U] 돛단배 구매·업그레이드",
        paper,
        1.4f,
        width - 40);
    renderer.Text(
        left + 20,
        top + 174,
        "K: 전체 적재창 / 1·2 산호 싣기 / 3·4 산호 내리기",
        paper,
        1.2f,
        width - 40);
    const auto& cargo = m_economy.Cargo();
    for (size_t i = 0; i < cargo.size(); ++i)
    {
        const auto& slot = cargo[i];
        std::string label = std::to_string(i + 1) + ". ";
        if (slot.occupied)
        {
            label += std::string(EconomyRules::ItemName(slot.item)) + " " + std::to_string(slot.count) + "/" +
                     std::to_string(EconomyRules::ItemStack(slot.item));
        }
        else
        {
            label += "빈 적재 슬롯";
        }
        renderer.Text(left + 28, top + 202 + static_cast<float>(i) * 22, label, paper, 1.3f, width - 50);
    }
    if (!m_serviceMessage.empty())
    {
        renderer.Text(left + 20, top + height - 66, m_serviceMessage, gold, 1.2f, width - 40);
    }
    renderer.Text(left + 20, top + height - 26, "[E / Esc] 닫기", paper, 1.3f);
}
