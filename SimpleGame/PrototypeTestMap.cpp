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
    state.control = m_player.Snapshot();
    state.zoom = m_zoom;
    state.detailZoom = m_detailZoom;
    state.overview = m_testOverview;
    state.island = m_testIsland;
    state.progress = m_combat.Snapshot();
    state.game.reset(new GameState(m_state));
}

void Prototype::RestoreTravelState()
{
    const auto& state = m_travel[m_world.IsTestMap() ? 1 : 0];
    if (state.valid)
    {
        m_state = *state.game;
        Player().location = state.player;
        ShipActor().location = state.ship;
        m_camera = state.camera;
        m_zoom = state.zoom;
        m_detailZoom = state.detailZoom;
        m_testOverview = state.overview;
        m_testIsland = state.island;
        m_combat.Restore(state.progress);
        m_player.Restore(state.control);
    }
    else
    {
        m_state = GameState{};
        m_state.economy.BuySailboat();
        m_combat.Restore(PlayerCombat::ProgressState{});
        m_player.Reset(
            m_world.TestSpawn(0),
            m_world.TestShip(0, EconomyRules::SailboatLength),
            m_world.OutboundHeading({0, 0}));
        m_camera = Player().location;
        m_zoom = m_detailZoom = 30;
        m_testOverview = true;
        m_testIsland = 0;
    }
}

void Prototype::ToggleTestMap()
{
    CaptureTravelState();
    m_ui.CloseAll();
    auto detached = m_player.Snapshot();
    detached.mode = PlayerMode::Land;
    detached.pirate = -1;
    m_player.Restore(detached);
    m_marine->SetActive(false);
    m_monsters->SetActive(false);
    m_npcs->SetActive(false);
    m_world.SwitchMap();
    m_marine.swap(m_parkedMarine);
    m_monsters.swap(m_parkedMonsters);
    m_npcs.swap(m_parkedNpcs);
    CreateMapSystems();
    m_marine->SetActive(true);
    m_monsters->SetActive(true);
    m_npcs->SetActive(true);
    RestoreTravelState();
    m_world.PrepareWindow(
        ActivePosition(),
        LoadRadius(m_world.IsTestMap() && m_testOverview ? m_detailZoom : m_zoom));
    m_monsters->Update(0, Player().location, !Aboard(), true);
    ClearInput();
    m_player.Notify(
        m_world.IsTestMap() ? "섬 테스트 맵: T 월드 복귀 / F1~F8 섬 이동 / Y 전체·상세 보기"
                            : "일반 월드로 복귀했습니다. 위치와 진행 상태를 복원했습니다. T: 테스트 맵",
        8);
    Diagnostics::Get().Event(
        std::string("test_map_travel mode=") + (m_world.IsTestMap() ? "test" : "adventure") + " gold=" +
        std::to_string(m_state.economy.Gold()) + " player_health=" + std::to_string(m_combat.Health()) +
        " player_chunk_x=" + std::to_string(Player().location.chunkX) + " player_chunk_z=" +
        std::to_string(Player().location.chunkZ) + " player_x=" + std::to_string(Player().location.x) +
        " player_z=" + std::to_string(Player().location.z) +
        " player_mode=" + std::to_string(static_cast<int>(m_player.Mode())));
}

bool Prototype::SelectTestIsland(unsigned index)
{
    if (!m_world.IsTestMap() || index >= IslandTestMap::Count)
    {
        return false;
    }
    m_ui.CloseAll();
    m_marine->Reset();
    m_combat.Recover();
    m_player.Reset(
        m_world.TestSpawn(index),
        m_world.TestShip(index, EconomyRules::SailboatLength),
        m_world.OutboundHeading(IslandTestMap::Describe(index).key));
    m_testIsland = index;
    m_testOverview = false;
    m_zoom = m_detailZoom;
    m_camera = Player().location;
    m_world.PrepareWindow(ActivePosition(), LoadRadius(m_zoom));
    m_monsters->Update(0, Player().location, true, true);
    m_player.Notify(
        std::string("테스트 섬: ") + MapCatalog::Island(static_cast<IslandShape>(index)).name +
            " / Y 전체 보기 / T 월드 복귀",
        6);
    Diagnostics::Get().Event("test_island_teleport index=" + std::to_string(index));
    return true;
}

void Prototype::ToggleTestOverview()
{
    if (!m_world.IsTestMap())
    {
        return;
    }
    m_ui.CloseAll();
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
