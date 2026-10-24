#pragma once
#include "Actor.h"
#include "WorldPosition.h"
#include <vector>

enum class MarineTarget
{
    Pirate,
    Tentacle
};

class MarineTargetActor : public MeshActor
{
public:
    MarineTargetActor(CachedMesh mesh, MarineTarget kind);
    void Render(Renderer& renderer, const ActorTransform& world) const override;
    void Overlay(Renderer& renderer, const ActorTransform& world) const override;
    WorldPosition location;
    float health = 0, maximum = 60, cooldown = 2, coarseTime = 0;
    bool chasing = false;
    MarineTarget kind;
};

class TornadoActor : public MeshActor
{
public:
    explicit TornadoActor(CachedMesh mesh);
    void Render(Renderer& renderer, const ActorTransform& world) const override;
    WorldPosition location;
    float time = 0;
};

class SeaWeatherActor : public Actor
{
public:
    SeaWeatherActor();
    ~SeaWeatherActor();
    void SetStorm(bool storm);
    void Update(float dt) override;
    void Render(Renderer& renderer, const ActorTransform& world) const override;

    float Light() const
    {
        return m_storm ? (m_flash > 0 ? 1.25f : .35f) : 1;
    }

private:
    bool m_storm = false;
    float m_time = 0, m_flash = 0, m_nextFlash = 0, m_thunderDelay = 0;
    CachedMesh m_rain, m_lightning;
    std::vector<unsigned char> m_thunder;
};
