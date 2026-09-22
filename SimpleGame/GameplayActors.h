#pragma once
#include "SceneGraph.h"
#include "LevelOneTerrain.h"
#include <functional>

// Per-frame inputs/services, independent of the LevelOne progression rules.
struct GameplayContext
{
    explicit GameplayContext(const LevelOneTerrain& map) : terrain(map)
    {
    }

    const LevelOneTerrain& terrain;
    std::vector<int> flow;
    Vec3 player = {};
    bool active = false, safe = false, playerDead = false;
    int playerLevel = 1;
    std::function<void(int)> hitPlayer, collect;
    void Step(Vec3& position, Vec3 target, float distance);
};

struct LevelMeshes
{
    LevelMeshes();
    CachedMesh enemies[4], warnings[2], drops[2], slash;
};

class EnemyActor : public Actor
{
public:
    EnemyActor(GameplayContext& context, const LevelMeshes& meshes, bool isBoss, Vec3 spawn, float health);
    void Update(float dt) override;
    void Render(Renderer& renderer, const ActorTransform& world) const override;
    void Overlay(Renderer& renderer, const ActorTransform& world) const override;
    bool TakeDamage(int damage);
    void Recover();
    Vec3 home, target;
    bool boss;
    float hp, maxHp, cooldown = 0, windup = 0, respawn = 0, flash = 0;

private:
    GameplayContext& m_context;
    CachedMesh m_normal, m_flash, m_warning;
};

class DropActor : public MeshActor
{
public:
    DropActor(GameplayContext& context, const LevelMeshes& meshes, Vec3 position, int power, bool keep);
    void Update(float dt) override;
    void Render(Renderer& renderer, const ActorTransform& world) const override;

private:
    GameplayContext& m_context;
    int weapon;
    float age = 0, speed = 0;
    bool collected = false, attracted = false, persistent;
};

class DamageNumberActor : public Actor
{
public:
    DamageNumberActor(Vec3 position, int amount, bool player);
    void Update(float dt) override;
    void Overlay(Renderer& renderer, const ActorTransform& world) const override;

private:
    int m_amount;
    bool m_player;
    float m_time = .8f;
};

class SlashActor : public MeshActor
{
public:
    explicit SlashActor(CachedMesh mesh);
    void Trigger(Vec3 position, Vec3 aim);
    void Stop();

    bool Attacking() const
    {
        return m_time > 0;
    }

    void Update(float dt) override;

private:
    float m_time = 0;
};
