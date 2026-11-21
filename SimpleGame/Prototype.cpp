#include "stdafx.h"
#include "Prototype.h"
#include "KoreanText.h"
#include "Diagnostics.h"
#include "IslandTestMap.h"
#include <algorithm>
#include <cmath>

Prototype::Prototype()
    : m_world(m_scene), m_combat(m_scene, m_state.economy),
      m_player(m_scene, m_world, m_state.economy, m_combat),
      m_ui(m_world, m_state.economy, m_state.npcs, m_combat, m_player)
{
    CreateActors();
    m_player.BindActors(m_actorsRoot, m_playerActor, m_shipActor, m_deckSlash);
    CreateMapSystems();
    Reset();
}

void Prototype::CreateMapSystems()
{
    if (!m_marine)
    {
        m_marine.reset(new MarineSystem(
            m_scene,
            m_world,
            m_state.economy,
            m_shipActor,
            [this](int damage)
            {
                const float previous = m_combat.Health();
                m_combat.TakeDamage(damage);
                Diagnostics::Get().Event(
                    "pirate_player_damage requested=" + std::to_string(damage) +
                    " applied=" + std::to_string(previous - m_combat.Health()) +
                    " health=" + std::to_string(m_combat.Health()));
                return m_combat.Health() <= 0;
            }));
    }
    if (!m_monsters)
    {
        m_monsters.reset(new MonsterSystem(
            m_scene,
            m_world,
            [this](int damage)
            {
                m_combat.TakeDamage(damage);
            },
            [this](IslandKey island, bool epic)
            {
                m_combat.RewardMonster(epic);
                m_state.npcs.RecordMonsterKill(island, epic);
            },
            [this](InventoryItem item)
            {
                return m_state.economy.CollectItem(item);
            }));
    }
    if (!m_npcs)
    {
        m_npcs.reset(new NpcSystem(m_scene, m_world, m_state.economy, m_state.npcs));
    }
    m_player.BindMarine(*m_marine);
    m_ui.BindSystems(*m_marine, *m_npcs, *m_monsters);
}

int Prototype::LoadRadius(float zoom) const
{
    return int(std::ceil((m_viewWidth * .354 + m_viewHeight * .613) / (zoom * ChunkSize))) + 1;
}

void Prototype::Reset()
{
    m_ui.CloseAll();
    if (m_marine)
    {
        m_marine->Reset();
    }
    m_player.Reset(
        m_world.StartSpawn(),
        m_world.StartShip(EconomyRules::SailboatLength),
        m_world.OutboundHeading({0, 0}));
    m_camera = Player().location;
    m_combat.Recover();
    m_player.Notify(KoreanText::Welcome, 10);
}

void Prototype::Key(unsigned char key, bool down)
{
    if (key >= 'A' && key <= 'Z')
    {
        key += 'a' - 'A';
    }
    if (!m_player.Press(key, down))
    {
        return;
    }
    if (m_ui.Paused())
    {
        m_ui.Key(key);
        return;
    }
    if (key == 't')
    {
        ToggleTestMap();
        return;
    }
    if (key == 'y' && m_world.IsTestMap())
    {
        ToggleTestOverview();
        return;
    }
    if (m_testOverview && m_world.IsTestMap() &&
        (key == 'e' || key == 'f' || key == 'i' || key == 'k' || key == 'o' || key == 'p'))
    {
        return;
    }
    if ((key == 'o' || key == 'p') && !m_ui.PlayerPanelOpen())
    {
        SummonSeaThreat(key == 'o' ? SeaEncounter::Storm : SeaEncounter::Kraken);
        return;
    }
    if (m_marine->Captured() && Aboard() && key != 'r')
    {
        m_player.StopFishing();
        m_marine->EscapeKey(key);
        if (!m_marine->Captured())
        {
            ClearInput();
        }
        return;
    }
    if (m_ui.Key(key))
    {
        return;
    }
    switch (key)
    {
        case 'e':
            if (!m_ui.Interact())
            {
                m_player.Interact();
            }
            break;
        case 'g':
            m_grid = !m_grid;
            break;
        case 'r':
            Reset();
            break;
        case 'f':
            if (Aboard())
            {
                m_player.ToggleFishing();
            }
            else
            {
                m_combat.Upgrade();
            }
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
    if (m_ui.Paused() || (m_world.IsTestMap() && m_testOverview))
    {
        return;
    }
    m_zoom = (std::max)(16.f, (std::min)(48.f, m_zoom + direction * 2.f));
}

void Prototype::MouseAttack(int x, int y, const Renderer& renderer)
{
    m_ui.SetView(
        renderer.Width(),
        renderer.Height(),
        m_world.IsTestMap() && m_testOverview,
        m_testIsland,
        m_frameAverage);
    if (m_ui.PanelClick(
            x,
            y,
            [this](UiAction action, unsigned value)
            {
                switch (action)
                {
                    case UiAction::TestIsland:
                        SelectTestIsland(value);
                        break;
                    case UiAction::TestOverview:
                        ToggleTestOverview();
                        break;
                    case UiAction::TestTravel:
                        ToggleTestMap();
                        break;
                    case UiAction::Grid:
                        m_grid = !m_grid;
                        break;
                    default:
                        break;
                }
            }))
    {
        return;
    }
    if ((m_world.IsTestMap() && m_testOverview) || m_marine->Captured())
    {
        return;
    }
    const auto hit = renderer.ScreenGround(x, y, Aboard() ? m_player.CurrentDeckHeight() + .5f : .55f);
    const auto relative = Player().location.RelativeTo(m_camera);
    const Vec3 aim = {hit.x - relative.x, 0, hit.z - relative.z};
    if (m_player.Attack(aim))
    {
        m_monsters->Attack(Player().location, aim, m_combat.Damage());
    }
}

void Prototype::Update(double dt, int width, int height)
{
    m_viewWidth = width;
    m_viewHeight = height;
    const bool overview = m_world.IsTestMap() && m_testOverview;
    if (overview)
    {
        const float span = (IslandTestMap::Columns + IslandTestMap::Rows - 2) * IslandTestMap::Spacing +
                           2 * IslandTestMap::IslandWidth;
        const float fit = (std::min)(
            (std::max)(100, width - 80) / (span * .70710678f),
            (std::max)(100, height - 220) / (span * .40824829f + 10));
        m_zoom = (std::max)(.5f, (std::min)(6.f, fit * .92f));
        m_camera = IslandTestMap::Center();
    }
    if (dt > 0)
    {
        m_frameAverage += (dt - m_frameAverage) * .05;
    }
    if (m_ui.Paused())
    {
        m_ui.SetView(width, height, overview, m_testIsland, m_frameAverage);
        return;
    }
    dt = (std::max)(0.0, (std::min)(dt, .05));
    const float frameDt = static_cast<float>(dt);
    m_time += frameDt;
    const int radius = LoadRadius(m_zoom);
    m_world.Update(overview ? IslandTestMap::Center() : ActivePosition(), radius);
    m_combat.SetActiveRegion(
        [this](const WorldPosition& point)
        {
            return m_world.IsActive(point);
        });
    m_player.UpdateMovement(frameDt, overview || m_ui.PlayerPanelOpen());
    const auto* island = m_world.CurrentIsland(Player().location);
    const auto local = island ? Player().location.RelativeTo(island->Info().center) : Vec3{};
    const bool village = island && island->Village(local.x, local.z);
    m_combat.BeginFrame(overview ? 0 : frameDt, Player().location, village);
    RaidPlayer raidPlayer;
    raidPlayer.deck = m_player.DeckPosition();
    raidPlayer.ownDeck = m_player.Mode() == PlayerMode::Deck || Sailing();
    const bool captured = m_marine->Captured();
    if (!overview &&
        m_marine->Update(frameDt, ShipActor().location, Aboard(), m_player.ShipHeading(), raidPlayer))
    {
        const auto notice = m_marine->Notice();
        if (m_marine->Raid().Failed())
        {
            m_combat.Die();
        }
        Reset();
        m_player.Notify(notice, 8);
        m_world.Update(ActivePosition(), radius);
    }
    if (Aboard() && captured != m_marine->Captured())
    {
        m_ui.CloseAll();
        m_player.StopFishing();
        ClearInput();
    }
    if (!overview)
    {
        m_player.UpdateFishing(frameDt, m_ui.PlayerPanelOpen());
    }
    m_player.SyncBoardedPlayer();
    m_monsters->Update(frameDt, Player().location, !Aboard() && m_combat.Health() > 0, overview);
    m_npcs->Update(
        frameDt,
        Player().location,
        Aboard(),
        ShipActor().location,
        Aboard() ? m_player.ShipHeading() : m_player.Heading(),
        overview);
    m_scene.Update(overview ? 0 : frameDt);
    if (m_combat.EndFrame())
    {
        Reset();
    }
    const auto distance = ActivePosition().RelativeTo(m_camera);
    const double follow = 1 - std::exp(-8 * dt);
    if (!overview)
    {
        m_camera.Move(distance.x * follow, distance.z * follow);
    }
    m_ui.SetView(width, height, overview, m_testIsland, m_frameAverage);
}

void Prototype::Render(Renderer& r)
{
    const bool overview = m_world.IsTestMap() && m_testOverview;
    m_ui.SetView(r.Width(), r.Height(), overview, m_testIsland, m_frameAverage);
    PrepareActors();
    m_marine->Prepare(m_camera, ShipActor().location);
    m_monsters->Prepare(m_camera, !overview && !m_ui.PlayerPanelOpen());
    m_npcs->Prepare(m_camera, !overview && !m_ui.PlayerPanelOpen());
    r.SetSceneLight(m_marine->Light());
    r.SetPostProcessing(m_ui.PostSettings());
    r.Begin(m_zoom);
    m_world.Prepare(m_camera, m_time, m_grid);
    m_combat.Prepare(m_camera);
    m_scene.Render(r);
    r.BeginOverlay();
    m_scene.Overlay(r);
    m_uiScene.Overlay(r);
    r.End();
}

void Prototype::SummonSeaThreat(SeaEncounter kind)
{
    if (m_player.Mode() == PlayerMode::PirateDeck)
    {
        m_player.Notify("내 배로 돌아온 뒤 해상 소환을 이용하세요.", 4);
        return;
    }
    if (!Aboard() || !m_state.economy.HasShip())
    {
        Diagnostics::Get().Event("marine_force_rejected reason=not_aboard");
        m_player.Notify("해상 소환은 배에 승선한 상태에서 이용하세요. O: 토네이도 / P: 크라켄", 5);
        return;
    }
    if (m_ui.PlayerPanelOpen() || !m_marine->ForceStart(kind, ShipActor().location))
    {
        m_player.Notify("선박 주변의 바다 로딩이 끝나면 다시 소환하세요.", 4);
        return;
    }
    m_player.StopFishing();
    m_scene.Get<SlashActor>(m_deckSlash)->Stop();
    ClearInput();
    m_player.Notify(m_marine->Notice(), 6);
}

void Prototype::DebugSeaThreat(int kind)
{
#ifdef _DEBUG
    if (Aboard() && !m_ui.PlayerPanelOpen() && kind >= 1 && kind <= 3)
    {
        SummonSeaThreat(static_cast<SeaEncounter>(kind));
    }
#else
    (void)kind;
#endif
}
