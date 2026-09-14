#pragma once
#include "LevelOneTerrain.h"
#include <random>
#include <string>

class LevelOne
{
public:
    explicit LevelOne(const LevelOneTerrain& terrain);
    bool Update(float dt, Vec3 player, bool sailing);
    void Attack(Vec3 player, Vec3 aim, bool sailing);
    void Upgrade();
    void Recover();
    void Render(Renderer& renderer, Vec3 worldOffset) const;
    void Overlay(Renderer& renderer, Vec3 worldOffset, bool showHud) const;
    void Map(Renderer& renderer, float left, float top, float extent, Vec3 player) const;

    int Damage() const
    {
        return 10 + (m_level - 1) * 3 + m_weapon + m_upgrade * 2;
    }

    int MaxHealth() const
    {
        return 100 + (m_level - 1) * 18;
    }

    bool Attacking() const
    {
        return m_slashTime > 0;
    }

private:
    struct Enemy
    {
        Vec3 position, home, target;
        bool boss = false;
        float hp = 32, maxHp = 32, cooldown = 0, windup = 0, respawn = 0, flash = 0;
    };

    struct Drop
    {
        Vec3 position;
        int weapon = 0;
        float age = 0, speed = 0;
        bool collected = false, attracted = false, persistent = false;
    };

    struct Number
    {
        Vec3 position;
        int amount = 0;
        float time = 0;
        bool player = false;
    };

    void Reward(const Enemy& enemy);
    void CreateMeshes();
    CachedMesh m_enemyMeshes[4], m_warningMeshes[2], m_dropMeshes[2], m_slashMesh;
    void GainExperience(int amount);
    void Notify(const std::string& text);
    void Step(Vec3& position, Vec3 target, float distance);
    const LevelOneTerrain& m_terrain;
    std::mt19937 m_random;
    std::vector<Enemy> m_enemies;
    std::vector<Drop> m_drops;
    std::vector<Number> m_numbers;
    std::vector<int> m_flow;
    int m_level = 1, m_xp = 0, m_weapon = 0, m_upgrade = 0, m_stones = 0, m_kills = 0;
    float m_health = 100, m_invincible = 0, m_sinceHit = 0, m_cooldown = 0, m_slashTime = 0, m_flowTime = 0;
    float m_noticeTime = 0;
    bool m_bossDefeated = false;
    Vec3 m_slashPosition = {}, m_slashAim = {0, 0, -1};
    std::string m_notice;
};
