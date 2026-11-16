#include "stdafx.h"
#include "PlayerCombat.h"

PlayerCombat::ProgressState PlayerCombat::Snapshot() const
{
    ProgressState state;
    state.level = m_level;
    state.xp = m_xp;
    state.upgrade = m_upgrade;
    state.kills = m_kills;
    state.health = m_health;
    state.invincible = m_invincible;
    state.sinceHit = m_sinceHit;
    state.cooldown = m_cooldown;
    state.bossDefeated = m_bossDefeated;
    return state;
}

void PlayerCombat::Restore(const ProgressState& state)
{
    m_level = state.level;
    m_xp = state.xp;
    m_upgrade = state.upgrade;
    m_kills = state.kills;
    m_health = state.health;
    m_invincible = state.invincible;
    m_sinceHit = state.sinceHit;
    m_cooldown = state.cooldown;
    m_bossDefeated = state.bossDefeated;
    m_noticeTime = 0;
    m_scene.Get<SlashActor>(m_slash)->Stop();
    for (auto id : m_numbers)
    {
        m_scene.Destroy(id);
    }
    m_numbers.clear();
    m_scene.CollectDestroyed();
}
