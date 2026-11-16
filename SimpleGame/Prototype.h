#pragma once
#include "GameState.h"
#include "GameUi.h"
#include "MonsterSystem.h"
#include <array>
#include <memory>

// Composition root: world lifecycle, system wiring, travel and frame scheduling.
class Prototype
{
public:
    Prototype();
    void Update(double dt, int width, int height);
    void Render(Renderer& renderer);
    void Key(unsigned char key, bool down);

    void ClearInput()
    {
        m_player.ClearInput();
    }

    void Zoom(int direction);
    void MouseAttack(int x, int y, const Renderer& renderer);

    bool CloseServiceMenu()
    {
        return m_ui.CloseServiceMenu();
    }

    void DebugSeaThreat(int kind);
    bool SelectTestIsland(unsigned index);
    std::string MonitorSnapshot() const;

    size_t ActorCount() const
    {
        return m_scene.Size() + m_uiScene.Size();
    }

    size_t LoadedChunkCount() const
    {
        return m_world.Chunks().size();
    }

    std::uint32_t TerrainSeed() const
    {
        return m_world.StartingTerrain().Seed();
    }

private:
    void CreateActors();
    void PrepareActors();
    void CreateMapSystems();
    int LoadRadius(float zoom) const;
    WorldActor& Player();
    const WorldActor& Player() const;
    WorldActor& ShipActor();
    const WorldActor& ShipActor() const;

    bool Aboard() const
    {
        return m_player.Aboard();
    }

    bool Sailing() const
    {
        return m_player.Sailing();
    }

    WorldPosition ActivePosition() const
    {
        return m_player.ActivePosition();
    }

    void Reset();
    void SummonSeaThreat(SeaEncounter kind);
    void ToggleTestMap();
    void ToggleTestOverview();
    void CaptureTravelState();
    void RestoreTravelState();

    SceneGraph m_scene, m_uiScene;
    World m_world;
    GameState m_state;
    PlayerCombat m_combat;
    PlayerController m_player;
    GameUi m_ui;
    std::unique_ptr<MarineSystem> m_marine, m_parkedMarine;
    std::unique_ptr<MonsterSystem> m_monsters, m_parkedMonsters;
    std::unique_ptr<NpcSystem> m_npcs, m_parkedNpcs;

    struct TravelState
    {
        bool valid = false, overview = false;
        WorldPosition player, ship, camera;
        PlayerControlState control;
        unsigned island = 0;
        float zoom = 30, detailZoom = 30;
        PlayerCombat::ProgressState progress;
        std::unique_ptr<GameState> game;
    };

    std::array<TravelState, 2> m_travel;
    bool m_testOverview = false;
    unsigned m_testIsland = 0;
    float m_detailZoom = 30;
    int m_viewWidth = 1280, m_viewHeight = 800;
    WorldPosition m_camera;
    ActorId m_actorsRoot = NoActor, m_playerActor = NoActor, m_shipActor = NoActor;
    ActorId m_playerBody = NoActor, m_shipBody = NoActor, m_shadow = NoActor, m_wake = NoActor,
            m_harbor = NoActor;
    ActorId m_playerMarker = NoActor, m_wheel = NoActor, m_deckSlash = NoActor, m_fishingRod = NoActor;
    bool m_grid = false;
    float m_zoom = 30, m_time = 0;
    double m_frameAverage = 1.0 / 60;
};
