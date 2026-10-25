#include "stdafx.h"
#include "Prototype.h"
#include "KoreanText.h"
#include "Diagnostics.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace
{
    constexpr float Pi = 3.14159265359f;
}

Prototype::Prototype() : m_world(m_scene), m_levelOne(m_scene, m_world.Tutorial(), m_economy)
{
    CreateActors();
    m_marine.reset(new MarineSystem(
        m_scene,
        m_world,
        m_economy,
        m_shipActor,
        [this](int damage)
        {
            const float previous = m_levelOne.Health();
            m_levelOne.TakeDamage(damage);
            Diagnostics::Get().Event(
                "pirate_player_damage requested=" + std::to_string(damage) +
                " applied=" + std::to_string(previous - m_levelOne.Health()) +
                " health=" + std::to_string(m_levelOne.Health()));
            return m_levelOne.Health() <= 0;
        }));
    m_monsters.reset(new MonsterSystem(
        m_scene,
        m_world,
        [this](int damage)
        {
            m_levelOne.TakeDamage(damage);
        },
        [this](bool epic)
        {
            m_levelOne.RewardMonster(epic);
        }));
    Reset();
}

void Prototype::Reset()
{
    Player().location = m_world.StartSpawn();
    ShipActor().location = m_world.StartShip(EconomyRules::SailboatLength);
    m_cargoOpen = m_fishing = false;
    CloseServiceMenu();
    m_playerMode = PlayerMode::Land;
    m_boardedPirate = -1;
    m_scene.SetParent(m_playerActor, m_actorsRoot);
    m_scene.SetParent(m_deckSlash, m_shipActor);
    if (m_marine)
    {
        m_marine->Reset();
    }
    m_scene.Get<SlashActor>(m_deckSlash)->Stop();
    m_camera = Player().location;
    m_heading = 0;
    m_shipHeading = m_world.OutboundHeading({0, 0});
    m_keys.fill(false);
    m_levelOne.Recover();
    m_message = KoreanText::Welcome;
    m_messageTime = 10;
}

void Prototype::ClearInput()
{
    m_keys.fill(false);
}

void Prototype::Key(unsigned char key, bool down)
{
    if (key >= 'A' && key <= 'Z')
    {
        key += 'a' - 'A';
    }
    const bool pressed = down && !m_keys[key];
    m_keys[key] = down;
    if (!pressed)
    {
        return;
    }
    if ((key == 'o' || key == 'p') && !PlayerPanelOpen())
    {
        SummonSeaThreat(key == 'o' ? SeaEncounter::Storm : SeaEncounter::Kraken);
        return;
    }
    if (m_marine && m_marine->Captured() && Aboard() && key != 'r')
    {
        m_fishing = false;
        m_marine->EscapeKey(key);
        if (!m_marine->Captured())
        {
            ClearInput();
        }
        return;
    }
    if (m_cargoOpen)
    {
        CargoKey(key);
        return;
    }
    if (m_service.kind != TownServiceKind::None)
    {
        ServiceKey(key);
        return;
    }
    switch (key)
    {
        case 'e':
            Interact();
            break;
        case 'g':
            m_grid = !m_grid;
            break;
        case 'h':
            m_hud = !m_hud;
            break;
        case 'r':
            Reset();
            break;
        case 'i':
            ToggleCargo();
            break;
        case 'f':
            if (Aboard())
            {
                ToggleFishing();
            }
            else
            {
                m_levelOne.Upgrade();
            }
            break;
        case 'v':
            m_postHud = !m_postHud;
            break;
        case 'b':
            m_postSettings.enabled = !m_postSettings.enabled;
            m_message = m_postSettings.enabled ? KoreanText::PostEnabled : KoreanText::PostDisabled;
            m_messageTime = 5;
            break;
        case '1':
            m_postSettings.toneMapping = !m_postSettings.toneMapping;
            break;
        case '2':
            m_postSettings.vignette = !m_postSettings.vignette;
            break;
        case '3':
            m_postSettings.edgeBlur = !m_postSettings.edgeBlur;
            break;
        case '[':
            m_postSettings.exposure = (std::max)(.1f, m_postSettings.exposure - .1f);
            break;
        case ']':
            m_postSettings.exposure = (std::min)(4.f, m_postSettings.exposure + .1f);
            break;
        case '0':
            m_postSettings = PostProcessingSettings();
            break;
        case '+':
        case '=':
            Zoom(1);
            break;
        case '-':
        case '_':
            Zoom(-1);
            break;
    }
}

void Prototype::Zoom(int direction)
{
    m_zoom = (std::max)(16.f, (std::min)(48.f, m_zoom + direction * 2.f));
}

void Prototype::MouseAttack(int x, int y, const Renderer& renderer)
{
    if (PlayerPanelOpen())
    {
        return;
    }
    if (m_hud && (y > renderer.Height() - 122 || (x < 445 && y < 255) ||
                  (renderer.Width() > 760 && x > renderer.Width() - 192 && y < 220)))
    {
        return;
    }
    if (m_hud && m_world.CurrentIsland(ActivePosition()) && x >= 20 && x <= 450 && y >= 265 && y <= 351)
    {
        return;
    }
    if (Sailing())
    {
        m_message = "E로 타륜 조종을 마친 뒤 공격하세요.";
        m_messageTime = 3;
        return;
    }
    if (Aboard() && m_marine && m_marine->Captured())
    {
        return;
    }
    if (m_hud && Aboard() && x >= (std::max)(20, renderer.Width() - 450) && y >= 235 && y < 420)
    {
        return;
    }
    const auto hit = renderer.ScreenGround(x, y, Aboard() ? CurrentDeckHeight() + .5f : .55f);
    const auto relative = Player().location.RelativeTo(m_camera);
    const Vec3 aim = {hit.x - relative.x, 0, hit.z - relative.z};
    WorldPosition origin;
    origin.x = 0;
    origin.z = 0;
    if (Aboard())
    {
        m_fishing = false;
        if (m_marine->Attack(
                Player().location,
                aim,
                m_levelOne.Damage(),
                m_shipHeading,
                m_playerMode == PlayerMode::Deck))
        {
            const auto localAim = ShipDeck::ToLocal(aim, CurrentDeckHeading());
            m_deckFacing = std::atan2(localAim.x, -localAim.z);
            m_scene.Get<SlashActor>(m_deckSlash)->Trigger(m_deckPosition, localAim, CurrentDeckHeight());
        }
        return;
    }
    if (m_levelOne.Attack(Player().location.RelativeTo(origin), aim, false))
    {
        m_monsters->Attack(Player().location, aim, m_levelOne.Damage());
        m_heading = std::atan2(aim.x, -aim.z);
    }
}

void Prototype::Interact()
{
    if (Aboard())
    {
        BoardInteract();
        return;
    }
    m_service = m_world.ServiceAt(Player().location);
    if (m_service.kind != TownServiceKind::None)
    {
        ClearInput();
        m_serviceMessage.clear();
        return;
    }
    BoardShip();
}

void Prototype::Move(double dx, double dz)
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

void Prototype::Update(double dt, int width, int height)
{
    if (dt > 0)
    {
        m_frameAverage += (dt - m_frameAverage) * .05;
    }
    dt = (std::max)(0.0, (std::min)(dt, .05));
    m_time += static_cast<float>(dt);
    m_messageTime = (std::max)(0.f, m_messageTime - static_cast<float>(dt));
    // Covers the inverse isometric viewport, with a margin for scenery and camera lag.
    const int radius = static_cast<int>(std::ceil((width * .354 + height * .613) / (m_zoom * ChunkSize))) + 1;
    m_world.Update(ActivePosition(), radius);
    m_levelOne.SetActive(true);
    m_levelOne.SetActiveRegion(
        [this](Vec3 position)
        {
            WorldPosition point;
            point.x = point.z = 0;
            point.Move(position.x, position.z);
            return m_world.IsActive(point);
        });
    double horizontal = (m_keys['d'] ? 1.0 : 0) - (m_keys['a'] ? 1.0 : 0);
    double vertical = (m_keys['s'] ? 1.0 : 0) - (m_keys['w'] ? 1.0 : 0);
    if (PlayerPanelOpen() || (Aboard() && m_marine->Captured()))
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
            if (!m_levelOne.Attacking())
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
    WorldPosition origin;
    origin.x = origin.z = 0;
    const auto* playerIsland = m_world.CurrentIsland(Player().location);
    const auto islandLocal =
        playerIsland ? Player().location.RelativeTo(playerIsland->Info().center) : Vec3{};
    const bool village = playerIsland && playerIsland->Village(islandLocal.x, islandLocal.z);
    m_levelOne.BeginFrame(static_cast<float>(dt), Player().location.RelativeTo(origin), Aboard(), village);
    RaidPlayer raidPlayer;
    raidPlayer.deck = m_deckPosition;
    raidPlayer.ownDeck = m_playerMode == PlayerMode::Deck || Sailing();
    const bool captured = m_marine->Captured();
    if (m_marine->Update(static_cast<float>(dt), ShipActor().location, Aboard(), m_shipHeading, raidPlayer))
    {
        const auto notice = m_marine->Notice();
        if (m_marine->Raid().Failed())
        {
            m_levelOne.Die();
        }
        Reset();
        m_message = notice;
        m_messageTime = 8;
        m_world.Update(ActivePosition(), radius);
    }
    if (Aboard() && captured != m_marine->Captured())
    {
        m_cargoOpen = false;
        CloseServiceMenu();
        m_fishing = false;
        ClearInput();
    }
    UpdateFishing(static_cast<float>(dt));

    SyncBoardedPlayer();
    m_monsters->Update(static_cast<float>(dt), Player().location, !Aboard() && m_levelOne.Health() > 0);
    m_scene.Update(static_cast<float>(dt));
    if (m_levelOne.EndFrame())
    {
        Reset();
    }
    const auto active = ActivePosition();
    const Vec3 distance = active.RelativeTo(m_camera);
    const double follow = 1 - std::exp(-8 * dt);
    m_camera.Move(distance.x * follow, distance.z * follow);
}

void Prototype::Render(Renderer& r)
{
    PrepareActors();
    m_marine->Prepare(m_camera, ShipActor().location);
    m_monsters->Prepare(m_camera);
    r.SetSceneLight(m_marine->Light());
    r.SetPostProcessing(m_postSettings);
    r.Begin(m_zoom);
    m_world.Prepare(m_camera, m_time, m_grid);
    WorldPosition origin;
    origin.x = 0;
    origin.z = 0;
    m_levelOne.Prepare(origin.RelativeTo(m_camera), m_hud && !m_postHud && !PlayerPanelOpen());
    m_scene.Render(r);
    r.BeginOverlay();
    m_scene.Overlay(r);
    r.End();
}
