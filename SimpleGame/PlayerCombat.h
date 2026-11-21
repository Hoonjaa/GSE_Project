#pragma once
#include "CombatEffects.h"
#include "CombatMeshes.h"
#include "PlayerEconomy.h"
#include <functional>
#include <string>

// Player combat/progression; effects retain world coordinates until render preparation.
class PlayerCombat
{
public:
    struct ProgressState
    {
        int level = 1, xp = 0, upgrade = 0, kills = 0;
        float health = 100, invincible = 0, sinceHit = 0, cooldown = 0;
        bool bossDefeated = false;
    };

    ProgressState Snapshot() const;
    void Restore(const ProgressState& state);
    PlayerCombat(SceneGraph& scene, PlayerEconomy& economy);
    ~PlayerCombat();
    PlayerCombat(const PlayerCombat&) = delete;
    PlayerCombat& operator=(const PlayerCombat&) = delete;
    void BeginFrame(float dt, const WorldPosition& player, bool village);
    bool EndFrame();
    void Prepare(const WorldPosition& camera);
    void SetActiveRegion(const std::function<bool(const WorldPosition&)>& isActive);
    bool Attack(const WorldPosition& player, Vec3 aim);
    void RewardMonster(bool epic);
    void Upgrade();
    void EquipmentChanged();
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
    }

    int Damage() const
    {
        return m_economy.EquipmentAttack() + 10 + (m_level - 1) * 3 +
               (m_economy.Equipped(EquipSlot::Weapon) >= 0 ? m_upgrade * 2 : 0);
    }

    int MaxHealth() const
    {
        return m_economy.EquipmentHealth() + 100 + (m_level - 1) * 18;
    }

    bool Attacking() const;

    float NoticeTime() const
    {
        return m_noticeTime;
    }

    const std::string& Notice() const
    {
        return m_notice;
    }

    int ExperienceTarget() const
    {
        return 40 + (m_level - 1) * 25;
    }

private:
    void GainExperience(int amount);
    void Notify(const std::string& text);
    void HitPlayer(int damage);
    void AddNumber(WorldPosition position, int amount, bool player);
    void PruneHandles(std::vector<ActorId>& handles);
    SceneGraph& m_scene;
    PlayerEconomy& m_economy;
    ActorId m_root = NoActor, m_effectsRoot = NoActor, m_slash = NoActor;
    WorldPosition m_player, m_slashPosition;
    CombatMeshes m_meshes;
    std::vector<ActorId> m_numbers;
    int m_level = 1, m_xp = 0, m_upgrade = 0, m_kills = 0;
    float m_health = 100, m_invincible = 0, m_sinceHit = 0, m_cooldown = 0;
    float m_noticeTime = 0;
    bool m_bossDefeated = false;
    std::string m_notice;
};
