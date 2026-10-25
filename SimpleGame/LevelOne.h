#pragma once
#include "GameplayActors.h"
#include <string>

// Level rules only: placement, progression, rewards, and player combat policy.
class LevelOne
{
public:
    LevelOne(SceneGraph& scene, const LevelOneTerrain& terrain, PlayerEconomy& economy);
    ~LevelOne();
    LevelOne(const LevelOne&) = delete;
    LevelOne& operator=(const LevelOne&) = delete;
    void BeginFrame(float dt, Vec3 player, bool sailing, bool village);
    bool EndFrame();
    void Prepare(Vec3 offset, bool showHud);
    void SetActive(bool active);
    void SetActiveRegion(const std::function<bool(Vec3)>& isActive);
    bool Attack(Vec3 player, Vec3 aim, bool sailing);
    void RewardMonster(bool epic);
    void Upgrade();
    void Recover();

    void TakeDamage(int damage)
    {
        HitPlayer(damage);
    }

    float Health() const
    {
        return m_health;
    }

    void Die()
    {
        m_health = 0;
        m_context.playerDead = true;
    }

    void Map(Renderer& renderer, float left, float top, float extent, Vec3 player) const;

    int Damage() const
    {
        return 10 + (m_level - 1) * 3 + m_weapon + m_upgrade * 2;
    }

    int MaxHealth() const
    {
        return 100 + (m_level - 1) * 18;
    }

    bool Attacking() const;

private:
    void GainExperience(int amount);
    void Notify(const std::string& text);
    void HitPlayer(int damage);
    void Collect(int weapon);
    void AddNumber(Vec3 position, int amount, bool player);
    void Overlay(Renderer& renderer) const;
    void PruneHandles(std::vector<ActorId>& handles);
    SceneGraph& m_scene;
    PlayerEconomy& m_economy;
    ActorId m_root = NoActor, m_effectsRoot = NoActor, m_slash = NoActor;
    const LevelOneTerrain& m_terrain;
    GameplayContext m_context;
    LevelMeshes m_meshes;
    std::vector<ActorId> m_numbers;
    int m_level = 1, m_xp = 0, m_weapon = 0, m_upgrade = 0, m_stones = 0, m_kills = 0;
    float m_health = 100, m_invincible = 0, m_sinceHit = 0, m_cooldown = 0;
    float m_noticeTime = 0;
    bool m_bossDefeated = false, m_showHud = true;
    std::string m_notice;
};
