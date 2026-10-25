#pragma once
#include "Actor.h"
#include "IslandLayout.h"
#include "MonsterCatalog.h"
#include <functional>

struct MonsterContext
{
    explicit MonsterContext(const IslandLayout& value);
    bool Segment(Vec3 from, Vec3 to) const;
    void Flow(Vec3 target, std::vector<int>& result) const;
    void Step(Vec3& position, Vec3 target, float distance, const std::vector<int>& flow) const;
    const IslandLayout& layout;
    std::vector<unsigned char> walk;
    std::vector<std::array<int, 4>> links;
    std::vector<int> playerFlow;
    Vec3 player = {};
    bool active = false, safe = true;
    std::function<void(int)> hitPlayer;
};

class MonsterActor : public Actor
{
public:
    MonsterActor(
        const MonsterDefinition& definition,
        CachedMesh model,
        CachedMesh warning,
        CachedMesh epicRing,
        Vec3 spawn);
    void Tick(float dt, const MonsterContext& context);
    bool TakeDamage(int damage);
    void Render(Renderer& renderer, const ActorTransform& world) const override;
    void Overlay(Renderer& renderer, const ActorTransform& world) const override;
    const MonsterDefinition& definition;
    Vec3 home;
    float health, maximum;

private:
    CachedMesh m_model, m_warning, m_epicRing;
    Vec3 m_target = {};
    float m_cooldown = 1, m_windup = 0, m_flash = 0, m_respawn = MonsterPreviewRules::RespawnTime;
    float m_hitTime = 0;
    int m_hitAmount = 0;
    std::vector<int> m_homeFlow;
    bool m_returning = false;
};
