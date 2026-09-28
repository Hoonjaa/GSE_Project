#include "stdafx.h"
#include "GameplayActors.h"
#include <algorithm>
#include <cmath>

namespace
{
    float Distance(Vec3 a, Vec3 b)
    {
        return std::hypot(a.x - b.x, a.z - b.z);
    }

    const Color Paper = {.96f, .90f, .72f}, Ink = {.07f, .19f, .23f}, Gold = {.98f, .71f, .32f};
}

void GameplayContext::Step(Vec3& position, Vec3 target, float distance)
{
    Step(position, target, distance, flow);
}

void GameplayContext::Step(Vec3& position, Vec3 target, float distance, const std::vector<int>& targetFlow)
{
    float length = Distance(position, target);
    if (length < .001f)
    {
        return;
    }
    if (!terrain.Segment(position.x, position.z, target.x, target.z))
    {
        const int index = terrain.Index(position.x, position.z);
        if (index < 0 ||
            targetFlow.size() != static_cast<size_t>(LevelOneTerrain::Size * LevelOneTerrain::Size))
        {
            return;
        }
        int best = index;
        for (const auto& d : {std::pair<int, int>{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
        {
            const int x = index % LevelOneTerrain::Size + d.first,
                      z = index / LevelOneTerrain::Size + d.second;
            if (!terrain.CellWalk(x, z))
            {
                continue;
            }
            const int next = z * LevelOneTerrain::Size + x;
            if (targetFlow[next] >= 0 && (targetFlow[best] < 0 || targetFlow[next] < targetFlow[best]))
            {
                best = next;
            }
        }
        target = terrain.Center(best);
        // Re-center first when clearance blocks a turn around a corner.
        if (!terrain.Segment(position.x, position.z, target.x, target.z))
        {
            target = terrain.Center(index);
        }
        length = Distance(position, target);
        if (length < .001f)
        {
            return;
        }
    }
    const float step = (std::min)(length, distance);
    const float dx = (target.x - position.x) / length * step, dz = (target.z - position.z) / length * step;
    if (terrain.Segment(position.x, position.z, position.x + dx, position.z + dz))
    {
        position.x += dx;
        position.z += dz;
    }
}

EnemyActor::EnemyActor(
    GameplayContext& context,
    const LevelMeshes& meshes,
    bool isBoss,
    Vec3 spawn,
    float health)
    : Actor(isBoss ? "산호갑각 대장" : "섬 게"), home(spawn), target(spawn), boss(isBoss), hp(health),
      maxHp(health), m_context(context), m_normal(meshes.enemies[isBoss ? 2 : 0]),
      m_flash(meshes.enemies[isBoss ? 3 : 1]), m_warning(meshes.warnings[isBoss ? 1 : 0])
{
    transform.position = spawn;
}

bool EnemyActor::TakeDamage(int damage)
{
    if (hp <= 0)
    {
        return false;
    }
    hp = (std::max)(0.f, hp - damage);
    flash = .15f;
    if (hp <= 0)
    {
        windup = 0;
        respawn = 25;
        return true;
    }
    return false;
}

void EnemyActor::Recover()
{
    m_returning = false;
    m_homeFlow.clear();
    windup = 0;
    cooldown = 1;
    if (hp > 0)
    {
        transform.position = home;
        hp = maxHp;
    }
}

void EnemyActor::ReturnHome(float dt)
{
    windup = 0;
    // Preserve the existing out-of-combat health reset; only locomotion changes.
    hp = maxHp;
    if (Distance(transform.position, home) <= .001f)
    {
        transform.position = home;
        m_returning = false;
        return;
    }
    if (!m_returning)
    {
        m_returning = true;
        m_homeFlow.clear();
    }
    // The shared flow field points to the player. Build a separate home field only
    // when direct movement is blocked, and reuse it for the rest of this return.
    if (m_homeFlow.empty() &&
        !m_context.terrain.Segment(transform.position.x, transform.position.z, home.x, home.z))
    {
        m_context.terrain.Flow(home.x, home.z, m_homeFlow);
    }
    m_context.Step(transform.position, home, (std::max)(0.f, dt) * (boss ? 1.5f : 1.9f), m_homeFlow);
    if (Distance(transform.position, home) <= .001f)
    {
        transform.position = home;
    }
}

void EnemyActor::Update(float dt)
{
    const Vec3 player = m_context.player;
    const bool active = m_context.active && !m_context.playerDead, safe = m_context.safe;
    flash = (std::max)(0.f, flash - dt);
    cooldown = (std::max)(0.f, cooldown - dt);
    if (hp <= 0)
    {
        if (!boss)
        {
            respawn -= dt;
            if (respawn <= 0 && Distance(player, home) > 12)
            {
                transform.position = home;
                hp = maxHp;
                cooldown = 1;
                m_returning = false;
                m_homeFlow.clear();
            }
        }
        return;
    }
    const bool engaged = active && !safe && Distance(player, home) < (boss ? 13 : 12);
    if (!engaged)
    {
        ReturnHome(dt);
        return;
    }
    m_returning = false;
    if (windup > 0)
    {
        windup -= dt;
        if (windup <= 0)
        {
            const float radius = boss ? 3.6f : 1.35f;
            if (Distance(player, target) <= radius &&
                m_context.terrain.Segment(transform.position.x, transform.position.z, player.x, player.z))
            {
                m_context.hitPlayer(boss ? 30 : 8);
            }
            cooldown = boss ? (hp < maxHp * .5f ? 1.3f : 2.3f) : 1.4f;
        }
        return;
    }
    if (Distance(player, transform.position) < (boss ? 4.2f : 1.8f) && cooldown <= 0)
    {
        target = player;
        windup = boss ? .95f : .65f;
    }
    else if (Distance(player, transform.position) > 1.1f)
    {
        m_context.Step(transform.position, player, dt * (boss ? 1.5f : 1.9f));
    }
}

void EnemyActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    if (hp <= 0)
    {
        return;
    }
    auto base = world.position;
    base.y -= .55f;
    renderer.Submit(flash > 0 ? m_flash : m_normal, base, world.rotation);
    if (windup > 0)
    {
        // Target is captured in the enemy's parent coordinates, not attached to its moving body.
        ActorTransform relative;
        relative.position = {target.x - transform.position.x, -.55f, target.z - transform.position.z};
        ActorTransform parent = world;
        parent.rotation -= transform.rotation;
        const auto warning = parent.Combine(relative);
        renderer.Submit(m_warning, warning.position, parent.rotation);
    }
}

void EnemyActor::Overlay(Renderer& renderer, const ActorTransform& world) const
{
    if (hp <= 0)
    {
        return;
    }
    auto position = world.position;
    position.y += (boss ? 3.3f : 1.8f) - .55f;
    const auto p = renderer.Project(position);
    if (p.x < 0 || p.x > renderer.Width() || p.y < 0 || p.y > renderer.Height())
    {
        return;
    }
    const float width = boss ? 100.f : 34.f;
    renderer.Panel(p.x - width / 2, p.y, width, 5, Ink);
    renderer.Panel(
        p.x - width / 2,
        p.y,
        width * hp / maxHp,
        5,
        boss ? Color{.91f, .29f, .18f} : Color{.52f, .82f, .34f});
    if (boss)
    {
        renderer.Text(p.x - 68, p.y - 22, "산호갑각 대장", Gold, 1.6f);
    }
}

DropActor::DropActor(GameplayContext& context, const LevelMeshes& meshes, Vec3 position, int power, bool keep)
    : MeshActor(meshes.drops[power ? 1 : 0], power ? "드랍 무기" : "강화석"), m_context(context),
      weapon(power), persistent(keep)
{
    transform.position = position;
}

void DropActor::Update(float dt)
{
    const Vec3 player = m_context.player;
    const bool active = m_context.active && !m_context.playerDead;
    if (!persistent && age + dt > 120)
    {
        Destroy();
        return;
    }
    age += dt;
    if (!active || collected)
    {
        return;
    }
    const float distance = Distance(player, transform.position);
    if (distance < 3.8f + (m_context.playerLevel - 1) * .08f)
    {
        attracted = true;
    }
    if (attracted && distance < 12)
    {
        speed = (std::min)(14.f, speed + dt * 18);
        m_context.Step(transform.position, player, dt * speed);
    }
    if (Distance(player, transform.position) < .65f &&
        m_context.terrain.Segment(player.x, player.z, transform.position.x, transform.position.z))
    {
        collected = true;
        m_context.collect(weapon);
        Destroy();
    }
}

void DropActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    auto display = world;
    display.position.y += .85f + .13f * std::sin(age * 5) - .55f;
    MeshActor::Render(renderer, display);
}

DamageNumberActor::DamageNumberActor(Vec3 position, int amount, bool player)
    : Actor("피해량"), m_amount(amount), m_player(player)
{
    transform.position = position;
}

void DamageNumberActor::Update(float dt)
{
    m_time -= dt;
    if (m_time <= 0)
    {
        Destroy();
    }
}

void DamageNumberActor::Overlay(Renderer& renderer, const ActorTransform& world) const
{
    auto position = world.position;
    position.y += 2 + (1 - m_time) * 1.3f - .55f;
    const auto p = renderer.Project(position);
    renderer.Text(p.x, p.y, std::to_string(m_amount), m_player ? Color{1, .25f, .2f} : Gold, 2);
}

SlashActor::SlashActor(CachedMesh resource) : MeshActor(std::move(resource), "근접 공격 궤적")
{
    visible = false;
}

void SlashActor::Trigger(Vec3 position, Vec3 aim)
{
    transform.position = {position.x, 0, position.z};
    transform.rotation = std::atan2(aim.z, aim.x);
    m_time = .2f;
    visible = true;
}

void SlashActor::Stop()
{
    m_time = 0;
    visible = false;
}

void SlashActor::Update(float dt)
{
    m_time = (std::max)(0.f, m_time - dt);
    visible = m_time > 0;
}
