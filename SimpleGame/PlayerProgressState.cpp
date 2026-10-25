#include "stdafx.h"
#include "LevelOne.h"

LevelOne::ProgressState LevelOne::Snapshot() const
{
    ProgressState state;
    state.level = m_level;
    state.xp = m_xp;
    state.weapon = m_weapon;
    state.upgrade = m_upgrade;
    state.stones = m_stones;
    state.kills = m_kills;
    state.health = m_health;
    state.invincible = m_invincible;
    state.sinceHit = m_sinceHit;
    state.cooldown = m_cooldown;
    state.bossDefeated = m_bossDefeated;
    return state;
}

void LevelOne::Restore(const ProgressState& state)
{
    m_level = state.level;
    m_xp = state.xp;
    m_weapon = state.weapon;
    m_upgrade = state.upgrade;
    m_stones = state.stones;
    m_kills = state.kills;
    m_health = state.health;
    m_invincible = state.invincible;
    m_sinceHit = state.sinceHit;
    m_cooldown = state.cooldown;
    m_bossDefeated = state.bossDefeated;
    m_context.playerDead = m_health <= 0;
    m_noticeTime = 0;
    m_scene.Get<SlashActor>(m_slash)->Stop();
    for (auto id : m_numbers)
    {
        m_scene.Destroy(id);
    }
    m_numbers.clear();
    m_scene.CollectDestroyed();
}
