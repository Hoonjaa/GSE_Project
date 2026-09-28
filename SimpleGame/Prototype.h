#pragma once
#include "World.h"
#include "WorldActors.h"
#include "LevelOne.h"
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
    WorldActor& Player();
    const WorldActor& Player() const;
    WorldActor& ShipActor();
    const WorldActor& ShipActor() const;
    void Reset();
    void Interact();
    void Move(double dx, double dz);

    WorldPosition ActivePosition() const
    {
        return m_sailing ? ShipActor().location : Player().location;
    }

    SceneGraph m_scene;
    World m_world;
    LevelOne m_levelOne;
    WorldPosition m_camera;
    ActorId m_actorsRoot = NoActor, m_playerActor = NoActor, m_shipActor = NoActor;
    ActorId m_playerBody = NoActor, m_shipBody = NoActor, m_wake = NoActor, m_harbor = NoActor;
    std::array<bool, 256> m_keys = {};
    bool m_sailing = false, m_grid = false, m_hud = true, m_moving = false;
    bool m_postHud = false;
    float m_zoom = 30, m_time = 0, m_heading = 0, m_shipHeading = 0, m_messageTime = 0;
    double m_frameAverage = 1.0 / 60.0;
    std::string m_message;
    PostProcessingSettings m_postSettings;
};
