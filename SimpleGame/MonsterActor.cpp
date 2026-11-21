#include "stdafx.h"
#include "MonsterActor.h"
#include "Diagnostics.h"
#include <algorithm>
#include <cmath>
#include <queue>

namespace
{
    float Distance(Vec3 a, Vec3 b)
    {
        return std::hypot(a.x - b.x, a.z - b.z);
    }
}

MonsterContext::MonsterContext(const IslandLayout& value)
    : layout(value), walk(IslandLayout::Size * IslandLayout::Size, 0)
{
    for (int i = 0; i < int(walk.size()); ++i)
    {
        const auto p = layout.Center(i);
        walk[i] = layout.Walk(p.x, p.z) && !layout.Village(p.x, p.z) && layout.Land(p.x, p.z);
    }
    links.resize(walk.size());
    for (int i = 0; i < int(walk.size()); ++i)
    {
        links[i].fill(-1);
        if (!walk[i])
        {
            continue;
        }
        int n = 0;
        for (const auto& d : {std::pair<int, int>{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
        {
            const int x = i % IslandLayout::Size + d.first, z = i / IslandLayout::Size + d.second;
            if (x >= 0 && x < IslandLayout::Size && z >= 0 && z < IslandLayout::Size)
            {
                const int next = z * IslandLayout::Size + x;
                if (walk[next] && Segment(layout.Center(i), layout.Center(next)))
                {
                    links[i][n] = next;
                }
            }
            ++n;
        }
    }
}

bool MonsterContext::Segment(Vec3 from, Vec3 to) const
{
    const int steps = (std::max)(1, int(std::ceil(Distance(from, to) / .15f)));
    for (int i = 0; i <= steps; ++i)
    {
        const float t = float(i) / steps;
        const float x = from.x + (to.x - from.x) * t, z = from.z + (to.z - from.z) * t;
        if (!layout.Walk(x, z) || layout.Village(x, z) || !layout.Land(x, z))
        {
            return false;
        }
    }
    return true;
}

void MonsterContext::Flow(Vec3 target, std::vector<int>& result) const
{
    result.assign(walk.size(), -1);
    const int start = layout.Index(target.x, target.z);
    if (start < 0 || !walk[start])
    {
        return;
    }
    std::queue<int> queue;
    queue.push(start);
    result[start] = 0;
    while (!queue.empty())
    {
        const int i = queue.front();
        queue.pop();
        for (int next : links[i])
        {
            if (next < 0 || result[next] >= 0)
            {
                continue;
            }
            result[next] = result[i] + 1;
            queue.push(next);
        }
    }
}

void MonsterContext::Step(Vec3& position, Vec3 target, float distance, const std::vector<int>& flow) const
{
    if (!Segment(position, target))
    {
        const int index = layout.Index(position.x, position.z);
        if (index < 0 || flow.size() != walk.size())
        {
            return;
        }
        int best = index;
        for (int next : links[index])
        {
            if (next < 0)
            {
                continue;
            }
            if (flow[next] >= 0 && (flow[best] < 0 || flow[next] < flow[best]))
            {
                best = next;
            }
        }
        if (best == index && flow[index] != 0)
        {
            return;
        }
        target = layout.Center(best);
        if (!Segment(position, target))
        {
            target = layout.Center(index);
        }
    }
    const float length = Distance(position, target);
    if (length < .001f)
    {
        return;
    }
    const float step = (std::min)(length, distance);
    const Vec3 next = {
        position.x + (target.x - position.x) * step / length,
        position.y,
        position.z + (target.z - position.z) * step / length};
    if (Segment(position, next))
    {
        position = next;
    }
}

MonsterActor::MonsterActor(
    const MonsterDefinition& def,
    CachedMesh model,
    CachedMesh warning,
    CachedMesh epicRing,
    Vec3 spawn)
    : Actor(def.name), definition(def), home(spawn),
      health(def.epic ? MonsterPreviewRules::EpicHealth : MonsterPreviewRules::NormalHealth), maximum(health),
      m_model(std::move(model)), m_warning(std::move(warning)), m_epicRing(std::move(epicRing))
{
    transform.position = spawn;
    enabled = false;
}

bool MonsterActor::TakeDamage(int damage)
{
    if (health <= 0 || damage <= 0)
    {
        return false;
    }
    health = (std::max)(0.f, health - damage);
    m_hitAmount = damage;
    m_hitTime = .65f;
    m_flash = .15f;
    if (health <= 0)
    {
        m_windup = 0;
        m_respawn = MonsterPreviewRules::RespawnTime;
        return true;
    }
    return false;
}

void MonsterActor::Tick(float dt, const MonsterContext& context)
{
    m_flash = (std::max)(0.f, m_flash - dt);
    m_hitTime = (std::max)(0.f, m_hitTime - dt);
    m_cooldown = (std::max)(0.f, m_cooldown - dt);
    if (health <= 0)
    {
        if (!definition.epic)
        {
            m_respawn -= dt;
            if (m_respawn <= 0 && (!context.active || Distance(context.player, home) > 12))
            {
                health = maximum;
                transform.position = home;
                m_cooldown = 1;
                m_returning = false;
                m_homeFlow.clear();
            }
        }
        return;
    }
    const bool engaged =
        context.active && !context.safe &&
        Distance(context.player, home) <
            (definition.epic ? MonsterPreviewRules::EpicAggro : MonsterPreviewRules::NormalAggro);
    const float speed = definition.epic ? MonsterPreviewRules::EpicSpeed : MonsterPreviewRules::NormalSpeed;
    if (!engaged)
    {
        m_windup = 0;
        health = maximum;
        if (Distance(transform.position, home) < .02f)
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
        if (m_homeFlow.empty() && !context.Segment(transform.position, home))
        {
            context.Flow(home, m_homeFlow);
        }
        context.Step(transform.position, home, dt * speed, m_homeFlow);
        return;
    }
    m_returning = false;
    const auto d = Vec3{context.player.x - transform.position.x, 0, context.player.z - transform.position.z};
    transform.rotation = std::atan2(d.x, -d.z);
    if (m_windup > 0)
    {
        m_windup -= dt;
        if (m_windup <= 0)
        {
            if (Distance(context.player, m_target) <= (definition.epic ? 3.6f : 1.35f) &&
                context.Segment(transform.position, context.player))
            {
                Diagnostics::Get().Event(
                    "monster_attack species=" + std::to_string(definition.id) + " damage=" +
                    std::to_string(
                        definition.epic ? MonsterPreviewRules::EpicDamage
                                        : MonsterPreviewRules::NormalDamage));
                context.hitPlayer(
                    definition.epic ? MonsterPreviewRules::EpicDamage : MonsterPreviewRules::NormalDamage);
            }
            m_cooldown = definition.epic ? (health < maximum * .5f ? 1.3f : 2.3f) : 1.4f;
        }
        return;
    }
    if (Distance(context.player, transform.position) < (definition.epic ? 4.2f : 1.8f) && m_cooldown <= 0)
    {
        m_target = context.player;
        m_windup = definition.epic ? .95f : .65f;
    }
    else if (Distance(context.player, transform.position) > 1.1f)
    {
        context.Step(transform.position, context.player, dt * speed, context.playerFlow);
    }
}

void MonsterActor::Render(Renderer& r, const ActorTransform& world) const
{
    if (health <= 0)
    {
        return;
    }
    r.Submit(m_model, world.position, world.rotation, {1, 1, 1}, MeshAnimation::HitFlash, m_flash / .15f);
    if (definition.epic)
    {
        r.Submit(m_epicRing, world.position, 0, {1.4f, 1, 1.4f}, MeshAnimation::None, 0);
    }
    if (m_windup > 0)
    {
        const Vec3 target = {
            world.position.x + m_target.x - transform.position.x,
            world.position.y,
            world.position.z + m_target.z - transform.position.z};
        const float radius = definition.epic ? 3.6f : 1.35f;
        r.Submit(m_warning, target, 0, {radius, 1, radius}, MeshAnimation::None, 0);
    }
}

void MonsterActor::Overlay(Renderer& r, const ActorTransform& world) const
{
    if (health <= 0 || !showStatus)
    {
        return;
    }
    auto p = world.position;
    p.y += definition.epic ? 3.7f : 2.2f;
    const auto screen = r.Project(p);
    if (screen.x < 0 || screen.x > r.Width() || screen.y < 0 || screen.y > r.Height())
    {
        return;
    }
    const float width = definition.epic ? 152.f : 112.f;
    const Color ivory{1, .961f, .863f};
    r.Panel(screen.x - width / 2, screen.y, width, 7, {.025f, .105f, .139f});
    r.Panel(screen.x - width / 2, screen.y, width * health / maximum, 7, {.88f, .41f, .36f});
    r.Text(
        screen.x - width * .5f,
        screen.y - 19,
        (definition.epic ? std::string("[에픽] ") : std::string()) + definition.name,
        ivory,
        definition.epic ? 1.4f : 1.15f);
    if (health < maximum || AttackWarning())
    {
        r.Text(
            screen.x - 19,
            screen.y + 10,
            std::to_string(int(health)) + " / " + std::to_string(int(maximum)),
            ivory,
            1.1f);
    }
    if (m_hitTime > 0)
    {
        r.Text(
            screen.x + 25,
            screen.y - 35 - (.65f - m_hitTime) * 24,
            std::to_string(m_hitAmount),
            {.98f, .78f, .3f},
            1.5f);
    }
}
