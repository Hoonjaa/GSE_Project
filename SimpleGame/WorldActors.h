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
    OceanActor() : Actor("바다")
    {
    }

    float extent = 128;
    void Render(Renderer& renderer, const ActorTransform& world) const override;
};

class WaveActor : public Actor
{
public:
    WaveActor() : Actor("파도 및 청크 격자")
    {
    }

    float time = 0;
    bool grid = false;
    void Render(Renderer& renderer, const ActorTransform& world) const override;
};

class FlagActor : public Actor
{
public:
    FlagActor() : Actor("항구 깃발")
    {
    }

    void Update(float dt) override
    {
        m_time += dt;
    }

    void Render(Renderer& renderer, const ActorTransform& world) const override;

private:
    float m_time = 0;
};

class GullActor : public Actor
{
public:
    explicit GullActor(int index) : Actor("갈매기"), m_index(index)
    {
    }

    void Update(float dt) override;
    void Render(Renderer& renderer, const ActorTransform& world) const override;

private:
    int m_index;
    float m_time = 0;
};
