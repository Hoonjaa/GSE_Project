#pragma once
#include "World.h"
#include "WorldActors.h"
#include "LevelOne.h"
#include "ShipDeck.h"
#include "MarineSystem.h"
#include "MonsterSystem.h"
#include <array>
#include <string>

class Prototype
{
public:
    Prototype();
    void Update(double dt, int width, int height);
    void Render(Renderer& renderer);
    void Key(unsigned char key, bool down);
    void ClearInput();
    void Zoom(int direction);
    void MouseAttack(int x, int y, const Renderer& renderer);
    bool CloseServiceMenu();
    void DebugSeaThreat(int kind);
    bool SelectTestIsland(unsigned index);

    size_t ActorCount() const
    {
        return m_scene.Size();
    }

    size_t LoadedChunkCount() const
    {
        return m_world.Chunks().size();
    }

    std::uint32_t TerrainSeed() const
    {
        return m_world.Tutorial().Seed();
    }

private:
    void CreateActors();
    void PrepareActors();
    void RenderHud(Renderer& renderer);
    void RenderServiceMenu(Renderer& renderer) const;
    void ServiceKey(unsigned char key);
    bool ShipAtService() const;
    void BuySailboat();
    WorldActor& Player();
    const WorldActor& Player() const;
    WorldActor& ShipActor();
    const WorldActor& ShipActor() const;
    void Reset();
    void Interact();
    void Move(double dx, double dz);
    bool BoardShip();
    bool LeaveShip();
    void BoardInteract();
    void PirateInteract();
    float CurrentDeckHeading() const;
    float CurrentDeckHeight() const;
    void SyncBoardedPlayer();
    void ToggleFishing();
    void ToggleCargo();
    void CargoKey(unsigned char key);
    void UpdateFishing(float dt);
    void SummonSeaThreat(SeaEncounter kind);
    void ToggleTestMap();
    void ToggleTestOverview();
    void RenderTestMapChart(Renderer& renderer) const;
    void CaptureTravelState();
    void RestoreTravelState();
    void RenderPlayerPanel(Renderer& renderer) const;
    void RenderPlayerStatus(Renderer& renderer) const;

    bool Aboard() const
    {
        return m_playerMode != PlayerMode::Land;
    }

    bool Sailing() const
    {
        return m_playerMode == PlayerMode::Helm;
    }

    bool PlayerPanelOpen() const
    {
        return m_cargoOpen || m_service.kind != TownServiceKind::None;
    }

    WorldPosition ActivePosition() const
    {
        return Sailing() ? ShipActor().location : Player().location;
    }

    SceneGraph m_scene;
    World m_world;
    PlayerEconomy m_economy;
    LevelOne m_levelOne;
    std::unique_ptr<MarineSystem> m_marine;
    std::unique_ptr<MonsterSystem> m_monsters;
    std::unique_ptr<MarineSystem> m_parkedMarine;
    std::unique_ptr<MonsterSystem> m_parkedMonsters;

    struct TravelState
    {
        bool valid = false, overview = false;
        WorldPosition player, ship, camera;
        PlayerMode mode = PlayerMode::Land;
        Vec3 deck = {};
        int pirate = -1;
        unsigned island = 0;
        float heading = 0, shipHeading = 0, facing = 0, zoom = 30, detailZoom = 30;
        LevelOne::ProgressState progress;
        std::unique_ptr<PlayerEconomy> economy;
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
    std::array<bool, 256> m_keys = {};
    TownInteraction m_service;
    std::string m_serviceMessage;
    PlayerMode m_playerMode = PlayerMode::Land;
    int m_boardedPirate = -1;
    Vec3 m_deckPosition = {};
    float m_deckFacing = 0, m_fishTime = 0;
    bool m_cargoOpen = false, m_fishing = false;
    unsigned m_cargoSelection = 0;
    ActorId m_playerMarker = NoActor;
    ActorId m_wheel = NoActor, m_deckSlash = NoActor, m_fishingRod = NoActor;
    bool m_grid = false, m_hud = true, m_moving = false;
    bool m_postHud = false;
    float m_zoom = 30, m_time = 0, m_heading = 0, m_shipHeading = 0, m_messageTime = 0;
    double m_frameAverage = 1.0 / 60.0;
    std::string m_message;
    PostProcessingSettings m_postSettings;
};
