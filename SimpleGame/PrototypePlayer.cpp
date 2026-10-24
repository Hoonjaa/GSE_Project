#include "stdafx.h"
#include "Prototype.h"
#include "Diagnostics.h"
#include <algorithm>
#include <cmath>

bool Prototype::BoardShip()
{
    if (!m_economy.HasShip())
    {
        m_message = "조선소에서 50골드로 돛단배를 구매하세요.";
        m_messageTime = 4;
        return false;
    }
    const auto local = ShipDeck::ToLocal(Player().location.RelativeTo(ShipActor().location), m_shipHeading);
    const auto deck = ShipDeck::ClosestBoardingPoint(local);
    if (!m_world.IsActive(ShipActor().location) || std::hypot(local.x - deck.x, local.z - deck.z) > 1.7f)
    {
        m_message = "정박한 배의 가장자리로 다가가 E를 누르세요.";
        m_messageTime = 4;
        return false;
    }
    m_deckPosition = deck;
    m_deckFacing = m_heading - m_shipHeading;
    m_playerMode = PlayerMode::Deck;
    m_scene.SetParent(m_playerActor, m_shipActor);
    SyncBoardedPlayer();
    ClearInput();
    m_message = "승선했습니다. 타륜 가까이에서 E: 조종 / 가장자리에서 F: 낚시 / I: 적재창";
    m_messageTime = 7;
    Diagnostics::Get().Event("player_boarded mode=deck");
    return true;
}

void Prototype::SyncBoardedPlayer()
{
    if (!Aboard())
    {
        return;
    }
    auto ship = ShipActor().location;
    PirateDeckInfo info;
    if (m_playerMode == PlayerMode::PirateDeck && m_marine->Raid().Deck(m_boardedPirate, info))
    {
        ship = info.position;
    }
    const auto offset = ShipDeck::ToWorld(m_deckPosition, CurrentDeckHeading());
    Player().location = ship;
    Player().location.Move(offset.x, offset.z);
}

bool Prototype::LeaveShip()
{
    if (!ShipDeck::NearEdge(m_deckPosition))
    {
        m_message = "하선하려면 육지 쪽 갑판 가장자리로 이동하세요.";
        m_messageTime = 3;
        return false;
    }
    // Pick reachable nearby shore only. Never teleport across the ship or through a town wall.
    for (float radius = .35f; radius <= 1.7f; radius += .15f)
    {
        for (int i = 0; i < 32; ++i)
        {
            const float angle = i * 6.2831853f / 32;
            auto point = Player().location;
            point.Move(std::cos(angle) * radius, std::sin(angle) * radius);
            if (!m_world.CanWalk(point))
            {
                continue;
            }
            Player().location = point;
            m_playerMode = PlayerMode::Land;
            m_heading = m_shipHeading + m_deckFacing;
            m_scene.SetParent(m_playerActor, m_actorsRoot);
            m_fishing = false;
            m_scene.Get<SlashActor>(m_deckSlash)->Stop();
            ClearInput();
            m_message = "육지에 내렸습니다. 배는 이곳에 정박합니다.";
            m_messageTime = 4;
            Diagnostics::Get().Event("player_disembarked mode=land");
            return true;
        }
    }
    m_message = "이쪽에는 내릴 수 있는 육지가 없습니다. 해안 가까이 정박하세요.";
    m_messageTime = 4;
    return false;
}

void Prototype::BoardInteract()
{
    if (m_playerMode == PlayerMode::PirateDeck)
    {
        PirateInteract();
        return;
    }
    if (m_marine->Captured())
    {
        return;
    }
    m_fishing = false;
    if (Sailing())
    {
        m_playerMode = PlayerMode::Deck;
        m_message = "조종을 마쳤습니다. WASD: 갑판 이동 / 마우스 클릭: 공격";
        Diagnostics::Get().Event("player_helm_released mode=deck");
    }
    else
    {
        int pirate = -1;
        Vec3 deck;
        if (ShipDeck::NearEdge(m_deckPosition) && m_marine->Raid().Board(Player().location, pirate, deck))
        {
            PirateDeckInfo info;
            m_marine->Raid().Deck(pirate, info);
            m_deckFacing += m_shipHeading - info.heading;
            m_deckPosition = deck;
            m_playerMode = PlayerMode::PirateDeck;
            m_boardedPirate = pirate;
            m_scene.SetParent(m_playerActor, info.actor);
            m_scene.Get<SlashActor>(m_deckSlash)->Stop();
            m_scene.SetParent(m_deckSlash, info.actor);
            m_marine->Raid().Entered(pirate);
            SyncBoardedPlayer();
            ClearInput();
            m_message = "해적선 승선! 상자 앞 E로 약탈하고, 가장자리 E로 내 배에 복귀하세요.";
            m_messageTime = 5;
            return;
        }
        const auto helm = ShipDeck::Helm();
        if (std::hypot(m_deckPosition.x - helm.x, m_deckPosition.z - helm.z) > .9f)
        {
            LeaveShip();
            return;
        }
        m_deckPosition = ShipDeck::HelmStand();
        m_deckFacing = 3.14159265f;
        m_playerMode = PlayerMode::Helm;
        m_heading = m_shipHeading;
        m_scene.Get<SlashActor>(m_deckSlash)->Stop();
        m_message = "타륜 조종 중: WASD 항해 / E: 조종 해제";
        Diagnostics::Get().Event("player_helm_engaged mode=helm");
    }
    SyncBoardedPlayer();
    ClearInput();
    m_messageTime = 5;
}

void Prototype::ToggleFishing()
{
    if (m_playerMode != PlayerMode::Deck || !ShipDeck::NearEdge(m_deckPosition) || m_marine->Captured())
    {
        m_message = "조종을 풀고 갑판 가장자리로 이동한 뒤 F를 누르세요.";
        m_messageTime = 4;
        return;
    }
    m_fishing = !m_fishing;
    m_fishTime = 0;
    if (m_fishing)
    {
        m_deckFacing = std::atan2(m_deckPosition.x, -m_deckPosition.z);
        m_scene.Get<SlashActor>(m_deckSlash)->Stop();
    }
    m_message = m_fishing ? "낚시 중: 5초마다 물고기 1마리 / 이동·공격·F로 종료" : "낚시를 마쳤습니다.";
    m_messageTime = 5;
    Diagnostics::Get().Event("fishing_toggle active=" + std::to_string(m_fishing));
    ClearInput();
}

void Prototype::UpdateFishing(float dt)
{
    if (!m_fishing)
    {
        m_fishTime = 0;
        return;
    }
    if (m_playerMode != PlayerMode::Deck || PlayerPanelOpen() || m_marine->Captured())
    {
        m_fishing = false;
        m_fishTime = 0;
        return;
    }
    m_fishTime += dt;
    if (m_fishTime >= 5)
    {
        m_fishTime -= 5;
        if (m_economy.CollectItem(InventoryItem::Fish))
        {
            m_message = "물고기 1마리를 낚았습니다! 상점 판매가 2골드.";
            Diagnostics::Get().Event("fishing_catch count=1");
        }
        else
        {
            m_fishing = false;
            m_message = "소지 수량 한도에 도달해 낚시를 종료했습니다.";
        }
        m_messageTime = 3;
    }
}

void Prototype::ToggleCargo()
{
    if (m_playerMode == PlayerMode::PirateDeck)
    {
        m_message = "내 배에 돌아온 뒤 적재창을 이용하세요.";
        m_messageTime = 4;
        return;
    }
    if (!m_cargoOpen && (!m_economy.HasShip() || (!Aboard() && !ShipAtService())))
    {
        m_message = "적재창은 배 위 또는 소유 선박이 정박한 마을의 조선소에서 이용하세요.";
        m_messageTime = 4;
        m_serviceMessage = m_message;
        return;
    }
    m_cargoOpen = !m_cargoOpen;
    m_serviceMessage.clear();
    m_fishing = false;
    ClearInput();
}

void Prototype::CargoKey(unsigned char key)
{
    if (key == 'i' || key == 'e' || key == 27)
    {
        m_cargoOpen = false;
        ClearInput();
        return;
    }
    if (key == 'w')
    {
        m_cargoSelection = (m_cargoSelection + 3) % 4;
    }
    else if (key == 's')
    {
        m_cargoSelection = (m_cargoSelection + 1) % 4;
    }
    else if (key == 'a' || key == 'd')
    {
        if (!Aboard() && !ShipAtService())
        {
            m_serviceMessage = "선박에 접근한 뒤 다시 이용하세요.";
            return;
        }
        const auto item = static_cast<InventoryItem>(m_cargoSelection);
        m_serviceMessage = (key == 'd' ? m_economy.StoreItem(item) : m_economy.UnloadItem(item)).message;
    }
}

void Prototype::SummonSeaThreat(SeaEncounter kind)
{
    if (m_playerMode == PlayerMode::PirateDeck)
    {
        m_message = "내 배로 돌아온 뒤 해상 소환을 이용하세요.";
        m_messageTime = 4;
        return;
    }
    if (!Aboard() || !m_economy.HasShip())
    {
        Diagnostics::Get().Event("marine_force_rejected reason=not_aboard");
        m_message = "해상 소환은 배에 승선한 상태에서 이용하세요. O: 토네이도 / P: 크라켄";
        m_messageTime = 5;
        return;
    }
    if (PlayerPanelOpen() || !m_marine->ForceStart(kind, ShipActor().location))
    {
        m_message = "선박 주변의 바다 로딩이 끝나면 다시 소환하세요.";
        m_messageTime = 4;
        return;
    }
    m_fishing = false;
    m_scene.Get<SlashActor>(m_deckSlash)->Stop();
    ClearInput();
    m_message = m_marine->Notice();
    m_messageTime = 6;
}

void Prototype::DebugSeaThreat(int kind)
{
#ifdef _DEBUG
    if (Aboard() && !PlayerPanelOpen() && kind >= 1 && kind <= 3)
    {
        SummonSeaThreat(static_cast<SeaEncounter>(kind));
    }
#else
    (void)kind;
#endif
}
