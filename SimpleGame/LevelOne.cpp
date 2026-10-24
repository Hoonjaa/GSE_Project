#include "stdafx.h"
#include "LevelOne.h"
#include <cmath>
#include <algorithm>

namespace
{
    float Distance(Vec3 a, Vec3 b)
    {
        return std::hypot(a.x - b.x, a.z - b.z);
    }

    float Length(Vec3 a)
    {
        return std::hypot(a.x, a.z);
    }

}

LevelOne::LevelOne(SceneGraph& scene, const LevelOneTerrain& terrain, PlayerEconomy& economy)
    : m_scene(scene), m_economy(economy), m_terrain(terrain), m_context(terrain),
      m_random(terrain.Seed() ^ 731u)
{
    m_root = m_scene.Create<Actor>(NoActor, "레벨 1").Id();
    m_context.hitPlayer = [this](int damage)
    {
        HitPlayer(damage);
    };
    m_context.collect = [this](int weapon)
    {
        Collect(weapon);
    };
    m_context.collectTrophy = [this](CoralLoot kind)
    {
        m_economy.Collect(kind);
        Notify(std::string(EconomyRules::LootName(kind)) + " 획득! 상점에서 판매할 수 있습니다.");
    };
    const auto& cells = terrain.SpawnCells();
    float bossDistance = 1e9f;
    for (int i : cells)
    {
        const auto point = terrain.Center(i);
        const float distance = Distance(point, {16.5f, .55f, -11.5f});
        if (distance < bossDistance)
        {
            bossDistance = distance;
            m_bossSpawn = point;
        }
    }
    // Bounded sampling: every accepted position belongs to the spawn's connected component.
    for (int attempt = 0; attempt < 3000 && m_enemies.size() < 32 && !cells.empty(); ++attempt)
    {
        const auto position = terrain.Center(cells[m_random() % cells.size()]);
        if (Distance(position, terrain.SpawnPoint()) < 12 || Distance(position, m_bossSpawn) < 9)
        {
            continue;
        }
        bool crowded = false;
        for (ActorId id : m_enemies)
        {
            const auto* enemy = m_scene.Get<EnemyActor>(id);
            crowded |= enemy && Distance(enemy->home, position) < 4;
        }
        if (crowded)
        {
            continue;
        }
        m_enemies.push_back(
            m_scene
                .Create<EnemyActor>(m_root, m_context, m_meshes, false, position, 28 + float(m_random() % 13))
                .Id());
    }
    m_enemies.push_back(
        m_scene.Create<EnemyActor>(m_root, m_context, m_meshes, true, m_bossSpawn, 360.f).Id());
    m_effectsRoot = m_scene.Create<Actor>(m_root, "전투 효과 및 드랍").Id();
    m_slash = m_scene.Create<SlashActor>(m_effectsRoot, m_meshes.slash).Id();
    auto& hud = m_scene.Create<CallbackActor>(m_root, "레벨 상태 UI");
    hud.overlay = [this](Renderer& renderer, const ActorTransform&)
    {
        Overlay(renderer);
    };
    const auto spawn = terrain.SpawnPoint();
    terrain.Flow(spawn.x, spawn.z, m_context.flow);
    Notify("레벨 1 · 새싹 탐험가의 섬 — 섬의 게를 처치하고 장비를 모으세요.");
}

void LevelOne::Notify(const std::string& text)
{
    m_notice = text;
    m_noticeTime = 5;
}

void LevelOne::Recover()
{
    m_health = float(MaxHealth());
    m_invincible = 2;
    m_sinceHit = 0;
    m_scene.Get<SlashActor>(m_slash)->Stop();
    m_context.playerDead = false;
    for (ActorId id : m_enemies)
    {
        if (auto* enemy = m_scene.Get<EnemyActor>(id))
        {
            enemy->Recover();
        }
    }
}

void LevelOne::Upgrade()
{
    const int cost = m_upgrade + 1;
    if (m_upgrade >= 10)
    {
        Notify("무기가 최대 강화 단계에 도달했습니다.");
        return;
    }
    if (m_stones < cost)
    {
        Notify("강화석이 부족합니다. 필요 수량: " + std::to_string(cost));
        return;
    }
    m_stones -= cost;
    ++m_upgrade;
    Notify("무기 강화 성공! +" + std::to_string(m_upgrade) + " / 공격력 " + std::to_string(Damage()));
}

void LevelOne::GainExperience(int amount)
{
    if (m_level >= 20)
    {
        return;
    }
    m_xp += amount;
    while (m_level < 20 && m_xp >= 40 + (m_level - 1) * 25)
    {
        m_xp -= 40 + (m_level - 1) * 25;
        ++m_level;
        m_health = float(MaxHealth());
        Notify("레벨 업! " + std::to_string(m_level) + "레벨 · 공격력과 최대 체력이 증가했습니다.");
    }
    if (m_level == 20)
    {
        m_xp = 0;
    }
}

void LevelOne::Reward(const EnemyActor& enemy)
{
    ++m_kills;
    GainExperience(enemy.boss ? 160 : 18);
    if (enemy.boss)
    {
        m_bossDefeated = true;
        Notify("보스 '산호갑각 대장' 처치! 레벨 1 완료 · 영웅 무기와 강화석을 회수하세요.");
    }
    PruneHandles(m_drops);
    const auto trophy = enemy.boss ? CoralLoot::Boss : CoralLoot::Normal;
    if (m_drops.size() < 512)
    {
        m_drops.push_back(
            m_scene.Create<DropActor>(m_effectsRoot, m_context, m_meshes, enemy.transform.position, trophy)
                .Id());
    }
    else
    {
        // Preserve the guaranteed trophy without exceeding the existing Actor cap.
        m_context.collectTrophy(trophy);
    }
    auto drop = [&](int weapon)
    {
        if (m_drops.size() >= 512)
        {
            return;
        }
        m_drops.push_back(m_scene
                              .Create<DropActor>(
                                  m_effectsRoot,
                                  m_context,
                                  m_meshes,
                                  enemy.transform.position,
                                  weapon,
                                  enemy.boss)
                              .Id());
    };
    for (int i = 0; i < (enemy.boss ? 8 : 1); ++i)
    {
        drop(0);
    }
    if (enemy.boss || m_kills == 1 || m_random() % 100 < 30)
    {
        drop(enemy.boss ? 18 : 4 + int(m_random() % 8));
    }
}

void LevelOne::Attack(Vec3 player, Vec3 aim, bool sailing)
{
    if (sailing || m_health <= 0 || m_cooldown > 0)
    {
        return;
    }
    const float length = Length(aim);
    if (length < .01f)
    {
        return;
    }
    const Vec3 slashAim = {aim.x / length, 0, aim.z / length};
    m_scene.Get<SlashActor>(m_slash)->Trigger(player, slashAim);
    m_cooldown = .42f;
    for (ActorId id : m_enemies)
    {
        auto* found = m_scene.Get<EnemyActor>(id);
        if (!found)
        {
            continue;
        }
        auto& enemy = *found;
        if (!enemy.enabled || enemy.hp <= 0)
        {
            continue;
        }
        const float distance = Distance(player, enemy.transform.position);
        const float dot = distance > .001f ? ((enemy.transform.position.x - player.x) * slashAim.x +
                                              (enemy.transform.position.z - player.z) * slashAim.z) /
                                                 distance
                                           : 1;
        if (distance > 2.7f + (enemy.boss ? 1.f : .35f) || dot < .5f)
        {
            continue;
        }
        if (!m_terrain.Segment(player.x, player.z, enemy.transform.position.x, enemy.transform.position.z))
        {
            continue;
        }
        AddNumber(enemy.transform.position, Damage(), false);
        if (enemy.TakeDamage(Damage()))
        {
            Reward(enemy);
        }
    }
}

LevelOne::~LevelOne()
{
    m_scene.Destroy(m_root);
    m_scene.CollectDestroyed();
}

bool LevelOne::Attacking() const
{
    const auto* slash = m_scene.Get<SlashActor>(m_slash);
    return slash && slash->Attacking();
}

void LevelOne::Prepare(Vec3 offset, bool showHud)
{
    m_scene.Find(m_root)->transform.position = offset;
    m_showHud = showHud;
}

void LevelOne::SetActive(bool active)
{
    auto* root = m_scene.Find(m_root);
    root->enabled = active;
    root->visible = active;
}

void LevelOne::SetActiveRegion(const std::function<bool(Vec3)>& isActive)
{
    for (const auto* handles : {&m_enemies, &m_drops, &m_numbers})
    {
        for (ActorId id : *handles)
        {
            if (auto* actor = m_scene.Find(id))
            {
                const bool active = isActive(actor->transform.position);
                actor->enabled = active;
                actor->visible = active;
            }
        }
    }
}

void LevelOne::PruneHandles(std::vector<ActorId>& handles)
{
    handles.erase(
        std::remove_if(
            handles.begin(),
            handles.end(),
            [this](ActorId id)
            {
                return !m_scene.Find(id);
            }),
        handles.end());
}

void LevelOne::AddNumber(Vec3 position, int amount, bool player)
{
    PruneHandles(m_numbers);
    if (m_numbers.size() < 64)
    {
        m_numbers.push_back(m_scene.Create<DamageNumberActor>(m_effectsRoot, position, amount, player).Id());
    }
}

void LevelOne::HitPlayer(int damage)
{
    if (m_invincible > 0 || m_health <= 0)
    {
        return;
    }
    m_health = (std::max)(0.f, m_health - damage);
    m_invincible = .65f;
    m_sinceHit = 0;
    m_context.playerDead = m_health <= 0;
    AddNumber(m_context.player, damage, true);
}

void LevelOne::Collect(int weapon)
{
    if (weapon > m_weapon)
    {
        m_weapon = weapon;
        Notify("새 무기를 자동 장착했습니다! 무기 공격력 +" + std::to_string(m_weapon));
    }
    else
    {
        ++m_stones;
    }
}

void LevelOne::BeginFrame(float dt, Vec3 player, bool sailing)
{
    PruneHandles(m_drops);
    PruneHandles(m_numbers);
    m_invincible = (std::max)(0.f, m_invincible - dt);
    m_cooldown = (std::max)(0.f, m_cooldown - dt);
    m_noticeTime = (std::max)(0.f, m_noticeTime - dt);
    m_sinceHit += dt;
    m_flowTime -= dt;
    m_context.player = player;
    m_context.active = !sailing && m_terrain.Walk(player.x, player.z);
    m_context.safe = m_terrain.IsVillage(player.x, player.z);
    m_context.playerLevel = m_level;
    m_context.playerDead = m_health <= 0;
    if (m_context.active && m_flowTime <= 0)
    {
        m_terrain.Flow(player.x, player.z, m_context.flow);
        m_flowTime = .25f;
    }
    if (m_sinceHit > 5)
    {
        m_health = (std::min)(float(MaxHealth()), m_health + dt * (m_context.safe ? 12 : 2));
    }
}

bool LevelOne::EndFrame()
{
    if (m_health > 0)
    {
        return false;
    }
    Recover();
    Notify("쓰러졌습니다. 항구에서 회복했습니다. 경험치와 장비는 유지됩니다.");
    return true;
}
