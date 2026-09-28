#pragma once
#include "Actor.h"
#include "World.h"

class WorldActor : public Actor
{
public:
    explicit WorldActor(std::string name) : Actor(std::move(name))
    {
    }

    WorldPosition location;
    void Prepare(const WorldPosition& camera);
};

class OceanActor : public Actor
{
public:
    OceanActor();

    float extent = 128;
    void Render(Renderer& renderer, const ActorTransform& world) const override;

private:
    CachedMesh m_mesh;
};

class WaveActor : public Actor
{
public:
    WaveActor();

    float time = 0;
    bool grid = false;
    void Render(Renderer& renderer, const ActorTransform& world) const override;

private:
    CachedMesh m_mesh, m_grid;
};

class FlagActor : public Actor
{
public:
    FlagActor();

    void Update(float dt) override
    {
        m_time += dt;
    }

    void Render(Renderer& renderer, const ActorTransform& world) const override;

private:
    float m_time = 0;
    CachedMesh m_mesh;
};

class GullActor : public Actor
{
public:
    explicit GullActor(int index);

    void Update(float dt) override;
    void Render(Renderer& renderer, const ActorTransform& world) const override;

private:
    int m_index;
    float m_time = 0;
    CachedMesh m_mesh;
};
