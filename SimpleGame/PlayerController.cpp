#include "stdafx.h"
#include "PlayerController.h"
#include "Diagnostics.h"
#include <algorithm>
#include <cmath>

bool PlayerController::BoardShip()
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
    m_message = "승선했습니다. 타륜 가까이에서 E: 조종 / 가장자리에서 F: 낚시 / K: 적재창";
    m_messageTime = 7;
    Diagnostics::Get().Event("player_boarded mode=deck");
    return true;
}

void PlayerController::SyncBoardedPlayer()
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

bool PlayerController::LeaveShip()
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

void PlayerController::BoardInteract()
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

void PlayerController::ToggleFishing()
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

void PlayerController::UpdateFishing(float dt, bool panelOpen)
{
    if (!m_fishing)
    {
        m_fishTime = 0;
        return;
    }
    if (m_playerMode != PlayerMode::Deck || panelOpen || m_marine->Captured())
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

float PlayerController::CurrentDeckHeading() const
{
    PirateDeckInfo info;
    return m_playerMode == PlayerMode::PirateDeck && m_marine->Raid().Deck(m_boardedPirate, info)
               ? info.heading
               : m_shipHeading;
}

float PlayerController::CurrentDeckHeight() const
{
    return m_playerMode == PlayerMode::PirateDeck ? PirateDeck::Height : ShipDeck::Height();
}

void PlayerController::PirateInteract()
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

void PlayerController::Move(double dx, double dz)
{
    auto& position = Sailing() ? ShipActor().location : Player().location;
    auto tryStep = [&](double x, double z)
    {
        if (x == 0 && z == 0)
        {
            return false;
        }
        auto next = position;
        next.Move(x, z);
        if (Sailing() ? (m_world.CanSail(next, m_shipHeading, EconomyRules::SailboatLength) &&
                         m_marine->Raid().CanSail(next, m_shipHeading))
                      : m_world.CanWalk(next))
        {
            position = next;
            return true;
        }
        return false;
    };
    // Small bounded substeps plus sliding prevent tunnelling and sticky coastlines.
    int steps = static_cast<int>(std::ceil(std::sqrt(dx * dx + dz * dz) / .12));
    if (steps < 1)
    {
        return;
    }
    dx /= steps;
    dz /= steps;
    for (int i = 0; i < steps; ++i)
    {
        if (tryStep(dx, dz))
        {
            m_moving = true;
        }
        else
        {
            const bool x = tryStep(dx, 0), z = tryStep(0, dz);
            m_moving = m_moving || x || z;
        }
    }
}

void PlayerController::UpdateMovement(float dt, bool blocked)
{
    m_messageTime = (std::max)(0.f, m_messageTime - dt);
    double horizontal = (m_keys['d'] ? 1.0 : 0) - (m_keys['a'] ? 1.0 : 0);
    double vertical = (m_keys['s'] ? 1.0 : 0) - (m_keys['w'] ? 1.0 : 0);
    if (blocked || (Aboard() && m_marine->Captured()))
    {
        horizontal = vertical = 0;
    }
    const double length = std::sqrt(horizontal * horizontal + vertical * vertical);
    m_moving = false;
    if (length > 0)
    {
        horizontal /= length;
        vertical /= length;
        double dx = (horizontal + vertical) * .70710678118;
        double dz = (-horizontal + vertical) * .70710678118;
        m_fishing = false;
        if (m_playerMode == PlayerMode::Deck || m_playerMode == PlayerMode::PirateDeck)
        {
            const auto local = ShipDeck::ToLocal({float(dx), 0, float(dz)}, CurrentDeckHeading());
            const auto previous = m_deckPosition;
            const auto moveDeck = m_playerMode == PlayerMode::PirateDeck ? PirateDeck::Move : ShipDeck::Move;
            moveDeck(
                m_deckPosition,
                local.x * 3.6f * static_cast<float>(dt),
                local.z * 3.6f * static_cast<float>(dt));
            m_deckFacing = std::atan2(local.x, -local.z);
            m_moving = std::hypot(previous.x - m_deckPosition.x, previous.z - m_deckPosition.z) > .001f;
        }
        else
        {
            if (!m_combat.Attacking())
            {
                m_heading = static_cast<float>(std::atan2(dx, -dz));
            }
            if (Sailing() && m_world.CanSail(ShipActor().location, m_heading, EconomyRules::SailboatLength) &&
                m_marine->Raid().CanSail(ShipActor().location, m_heading))
            {
                m_shipHeading = m_heading;
            }
            const double speed = (Sailing() ? 7.5 : 3.6) * (m_keys[' '] ? 1.8 : 1);
            Move(dx * speed * dt, dz * speed * dt);
        }
    }
    SyncBoardedPlayer();
}

PlayerController::PlayerController(
    SceneGraph& scene,
    World& world,
    PlayerEconomy& economy,
    PlayerCombat& combat)
    : m_scene(scene), m_world(world), m_economy(economy), m_combat(combat)
{
}

void PlayerController::BindActors(ActorId root, ActorId player, ActorId ship, ActorId slash)
{
    m_actorsRoot = root;
    m_playerActor = player;
    m_shipActor = ship;
    m_deckSlash = slash;
}

WorldActor& PlayerController::Player()
{
    return *m_scene.Get<WorldActor>(m_playerActor);
}

const WorldActor& PlayerController::Player() const
{
    return *m_scene.Get<WorldActor>(m_playerActor);
}

WorldActor& PlayerController::ShipActor()
{
    return *m_scene.Get<WorldActor>(m_shipActor);
}

const WorldActor& PlayerController::ShipActor() const
{
    return *m_scene.Get<WorldActor>(m_shipActor);
}

bool PlayerController::Press(unsigned char key, bool down)
{
    const bool pressed = down && !m_keys[key];
    m_keys[key] = down;
    return pressed;
}

void PlayerController::ClearInput()
{
    m_keys.fill(false);
}

void PlayerController::Interact()
{
    if (Aboard())
    {
        BoardInteract();
    }
    else
    {
        BoardShip();
    }
}

void PlayerController::SetShipPose(const WorldPosition& position, float heading)
{
    ShipActor().location = position;
    m_shipHeading = heading;
}

PlayerControlState PlayerController::Snapshot() const
{
    return {m_playerMode, m_deckPosition, m_boardedPirate, m_heading, m_shipHeading, m_deckFacing};
}

void PlayerController::Restore(const PlayerControlState& state)
{
    m_playerMode = state.mode;
    m_deckPosition = state.deck;
    m_boardedPirate = state.pirate;
    m_heading = state.heading;
    m_shipHeading = state.shipHeading;
    m_deckFacing = state.facing;
    StopFishing();
    m_moving = false;
    ClearInput();
    ActorId parent = m_playerMode == PlayerMode::Land ? m_actorsRoot : m_shipActor;
    if (m_playerMode == PlayerMode::PirateDeck)
    {
        PirateDeckInfo info;
        if (m_marine->Raid().Deck(m_boardedPirate, info))
        {
            parent = info.actor;
        }
        else
        {
            m_playerMode = PlayerMode::Deck;
            m_boardedPirate = -1;
            m_deckPosition = ShipDeck::HelmStand();
        }
    }
    m_scene.SetParent(m_playerActor, parent);
    m_scene.SetParent(m_deckSlash, m_playerMode == PlayerMode::PirateDeck ? parent : m_shipActor);
    m_scene.Get<SlashActor>(m_deckSlash)->Stop();
    SyncBoardedPlayer();
}

void PlayerController::Reset(const WorldPosition& player, const WorldPosition& ship, float heading)
{
    Player().location = player;
    ShipActor().location = ship;
    PlayerControlState state;
    state.shipHeading = heading;
    Restore(state);
}

bool PlayerController::Attack(Vec3 aim)
{
    if (Sailing())
    {
        Notify("E로 타륜 조종을 마친 뒤 공격하세요.", 3);
        return false;
    }
    if (Aboard())
    {
        if (m_marine->Captured())
        {
            return false;
        }
        StopFishing();
        if (m_marine->Attack(
                Player().location,
                aim,
                m_combat.Damage(),
                m_shipHeading,
                m_playerMode == PlayerMode::Deck))
        {
            const auto localAim = ShipDeck::ToLocal(aim, CurrentDeckHeading());
            m_deckFacing = std::atan2(localAim.x, -localAim.z);
            m_scene.Get<SlashActor>(m_deckSlash)->Trigger(m_deckPosition, localAim, CurrentDeckHeight());
        }
        return false;
    }
    if (!m_combat.Attack(Player().location, aim))
    {
        return false;
    }
    m_heading = std::atan2(aim.x, -aim.z);
    return true;
}
