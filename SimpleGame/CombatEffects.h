#pragma once
#include "SceneGraph.h"
#include "WorldPosition.h"

class DamageNumberActor : public Actor
{
public:
    DamageNumberActor(WorldPosition position, int amount, bool player);
    void Update(float dt) override;
    void Overlay(Renderer& renderer, const ActorTransform& world) const override;
    WorldPosition location;

private:
    int m_amount;
    bool m_player;
    float m_time = .8f;
};

class SlashActor : public MeshActor
{
public:
    explicit SlashActor(CachedMesh mesh);
    void Trigger(Vec3 position, Vec3 aim, float groundHeight = .55f);
    void Stop();

    bool Attacking() const
    {
        return m_time > 0;
    }

    void Update(float dt) override;

private:
    float m_time = 0;
};
