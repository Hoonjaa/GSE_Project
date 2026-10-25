#include "stdafx.h"
#include "Prototype.h"
#include "IslandTestMap.h"
#include "Diagnostics.h"
#include <algorithm>
#include <cmath>

void Prototype::CaptureTravelState()
{
    auto& state = m_travel[m_world.IsTestMap() ? 1 : 0];
    state.valid = true;
    state.player = Player().location;
    state.ship = ShipActor().location;
    state.camera = m_camera;
    state.mode = m_playerMode;
    state.deck = m_deckPosition;
    state.pirate = m_boardedPirate;
    state.heading = m_heading;
    state.shipHeading = m_shipHeading;
    state.facing = m_deckFacing;
    state.zoom = m_zoom;
    state.detailZoom = m_detailZoom;
    state.overview = m_testOverview;
    state.island = m_testIsland;
    state.progress = m_levelOne.Snapshot();
    state.economy.reset(new PlayerEconomy(m_economy));
}

void Prototype::RestoreTravelState()
{
    const auto& state = m_travel[m_world.IsTestMap() ? 1 : 0];
    if (state.valid)
    {
        Player().location = state.player;
        ShipActor().location = state.ship;
        m_camera = state.camera;
        m_playerMode = state.mode;
        m_deckPosition = state.deck;
        m_boardedPirate = state.pirate;
        m_heading = state.heading;
        m_shipHeading = state.shipHeading;
        m_deckFacing = state.facing;
        m_zoom = state.zoom;
        m_detailZoom = state.detailZoom;
        m_testOverview = state.overview;
        m_testIsland = state.island;
        m_economy = *state.economy;
        m_levelOne.Restore(state.progress);
    }
    else
    {
        m_economy = PlayerEconomy{};
        m_economy.BuySailboat();
        m_levelOne.Restore(LevelOne::ProgressState{});
        Player().location = m_world.TestSpawn(0);
        ShipActor().location = m_world.TestShip(0, EconomyRules::SailboatLength);
        m_camera = Player().location;
        m_playerMode = PlayerMode::Land;
        m_boardedPirate = -1;
        m_heading = m_deckFacing = 0;
        m_shipHeading = m_world.OutboundHeading({0, 0});
        m_zoom = m_detailZoom = 30;
        m_testOverview = true;
        m_testIsland = 0;
    }
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
    SyncBoardedPlayer();
}

void Prototype::ToggleTestMap()
{
    CaptureTravelState();
    m_cargoOpen = false;
    CloseServiceMenu();
    m_fishing = false;
    m_moving = false;
    m_scene.Get<SlashActor>(m_deckSlash)->Stop();
    m_scene.SetParent(m_playerActor, m_actorsRoot);
    m_scene.SetParent(m_deckSlash, m_shipActor);
    m_marine->SetActive(false);
    m_monsters->SetActive(false);
    m_world.SwitchMap();
    m_marine.swap(m_parkedMarine);
    m_monsters.swap(m_parkedMonsters);
    if (!m_marine)
    {
        m_marine.reset(new MarineSystem(
            m_scene,
            m_world,
            m_economy,
            m_shipActor,
            [this](int damage)
            {
                m_levelOne.TakeDamage(damage);
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
    }
    m_marine->SetActive(true);
    m_monsters->SetActive(true);
    RestoreTravelState();
    const float detailZoom = m_world.IsTestMap() && m_testOverview ? m_detailZoom : m_zoom;
    const int radius =
        int(std::ceil((m_viewWidth * .354 + m_viewHeight * .613) / (detailZoom * ChunkSize))) + 1;
    m_world.PrepareWindow(ActivePosition(), radius);
    m_monsters->Update(0, Player().location, !Aboard(), true);
    ClearInput();
    m_message = m_world.IsTestMap()
                    ? "섬 테스트 맵: T 월드 복귀 / F1~F8 섬 이동 / Y 전체·상세 보기"
                    : "일반 월드로 복귀했습니다. 위치와 진행 상태를 복원했습니다. T: 테스트 맵";
    m_messageTime = 8;
    Diagnostics::Get().Event(
        std::string("test_map_travel mode=") + (m_world.IsTestMap() ? "test" : "adventure") + " gold=" +
        std::to_string(m_economy.Gold()) + " player_health=" + std::to_string(m_levelOne.Health()) +
        " player_chunk_x=" + std::to_string(Player().location.chunkX) + " player_chunk_z=" +
        std::to_string(Player().location.chunkZ) + " player_x=" + std::to_string(Player().location.x) +
        " player_z=" + std::to_string(Player().location.z) +
        " player_mode=" + std::to_string(static_cast<int>(m_playerMode)));
}

bool Prototype::SelectTestIsland(unsigned index)
{
    if (!m_world.IsTestMap() || index >= IslandTestMap::Count)
    {
        return false;
    }
    m_cargoOpen = false;
    CloseServiceMenu();
    m_fishing = false;
    m_moving = false;
    m_scene.SetParent(m_playerActor, m_actorsRoot);
    m_scene.SetParent(m_deckSlash, m_shipActor);
    m_scene.Get<SlashActor>(m_deckSlash)->Stop();
    m_playerMode = PlayerMode::Land;
    m_boardedPirate = -1;
    m_marine->Reset();
    m_levelOne.Recover();
    m_testIsland = index;
    m_testOverview = false;
    m_zoom = m_detailZoom;
    Player().location = m_world.TestSpawn(index);
    ShipActor().location = m_world.TestShip(index, EconomyRules::SailboatLength);
    m_shipHeading = m_world.OutboundHeading(IslandTestMap::Describe(index).key);
    m_heading = 0;
    m_camera = Player().location;
    const int radius = int(std::ceil((m_viewWidth * .354 + m_viewHeight * .613) / (m_zoom * ChunkSize))) + 1;
    m_world.PrepareWindow(ActivePosition(), radius);
    m_monsters->Update(0, Player().location, true, true);
    ClearInput();
    m_message = std::string("테스트 섬: ") + MapCatalog::Island(static_cast<IslandShape>(index)).name +
                " / Y 전체 보기 / T 월드 복귀";
    m_messageTime = 6;
    Diagnostics::Get().Event("test_island_teleport index=" + std::to_string(index));
    return true;
}

void Prototype::ToggleTestOverview()
{
    if (!m_world.IsTestMap())
    {
        return;
    }
    m_cargoOpen = false;
    CloseServiceMenu();
    m_fishing = false;
    ClearInput();
    m_testOverview = !m_testOverview;
    if (m_testOverview)
    {
        m_detailZoom = m_zoom;
        m_camera = IslandTestMap::Center();
    }
    else
    {
        m_zoom = m_detailZoom;
        m_camera = ActivePosition();
    }
}

void Prototype::RenderTestMapChart(Renderer& r) const
{
    if (!m_world.IsTestMap())
    {
        return;
    }
    const float left = (std::max)(20.f, float(r.Width()) - 370), top = 20;
    const Color ink = {.07f, .19f, .23f}, gold = {.98f, .71f, .32f}, paper = {.96f, .90f, .72f};
    r.Panel(left, top, 350, 185, ink);
    r.Text(left + 12, top + 8, "섬 테스트 맵 · 2행 × 4열", gold, 1.7f);
    const auto* current = m_world.CurrentIsland(Player().location);
    const unsigned selected =
        current ? unsigned(current->Info().key.z * IslandTestMap::Columns + current->Info().key.x)
                : m_testIsland;
    for (unsigned i = 0; i < IslandTestMap::Count; ++i)
    {
        const float x = left + 10 + (i % 4) * 83, y = top + 40 + (i / 4) * 49;
        r.Panel(x, y, 79, 44, i == selected ? Color{.24f, .43f, .42f} : Color{.12f, .28f, .31f});
        r.Text(x + 4, y + 3, "F" + std::to_string(i + 1), gold, 1.2f);
        r.Text(x + 4, y + 23, MapCatalog::Island(static_cast<IslandShape>(i)).name, paper, 1.15f, 73);
    }
    r.Text(left + 12, top + 143, "T: 월드 왕복 / Y: 전체·상세 보기", gold, 1.3f);
    r.Text(
        left + 12,
        top + 165,
        m_testOverview ? "전체 보기 중: 전투·타이머 일시 정지" : "F1~F8: 각 섬 항구로 즉시 이동",
        paper,
        1.25f);
}
