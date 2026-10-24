#include "stdafx.h"
#include "Prototype.h"
#include <cmath>

float Prototype::CurrentDeckHeading() const
{
    PirateDeckInfo info;
    return m_playerMode == PlayerMode::PirateDeck && m_marine->Raid().Deck(m_boardedPirate, info)
               ? info.heading
               : m_shipHeading;
}

float Prototype::CurrentDeckHeight() const
{
    return m_playerMode == PlayerMode::PirateDeck ? PirateDeck::Height : ShipDeck::Height();
}

void Prototype::PirateInteract()
{
    if (m_marine->Raid().Loot(m_boardedPirate, m_deckPosition))
    {
        m_message = m_marine->Notice();
        m_messageTime = 4;
        return;
    }
    const auto outside = ShipDeck::ToLocal(Player().location.RelativeTo(ShipActor().location), m_shipHeading);
    const auto destination = ShipDeck::ClosestBoardingPoint(outside);
    if (PirateDeck::NearEdge(m_deckPosition) &&
        std::hypot(outside.x - destination.x, outside.z - destination.z) <= 2.3f)
    {
        m_deckFacing += CurrentDeckHeading() - m_shipHeading;
        m_marine->Raid().Returned(m_boardedPirate);
        m_boardedPirate = -1;
        m_playerMode = PlayerMode::Deck;
        m_deckPosition = destination;
        m_scene.SetParent(m_playerActor, m_shipActor);
        m_scene.Get<SlashActor>(m_deckSlash)->Stop();
        m_scene.SetParent(m_deckSlash, m_shipActor);
        SyncBoardedPlayer();
        ClearInput();
        m_message = "내 배에 무사히 복귀했습니다!";
        m_messageTime = 4;
        return;
    }
    m_message = "상자 앞 E: 약탈 / 내 배 쪽 가장자리 E: 복귀. 선박 위 남은 시간을 확인하세요.";
    m_messageTime = 4;
}
