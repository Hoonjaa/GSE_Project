#include "stdafx.h"
#include "LevelOne.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace
{
    constexpr float Pi = 3.14159265359f;

    float Distance(Vec3 a, Vec3 b)
    {
        return std::hypot(a.x - b.x, a.z - b.z);
    }

    float Length(Vec3 a)
    {
        return std::hypot(a.x, a.z);
    }

    const Vec3 Safe = {22, .55f, 17.75f};
    const Color Paper = {.96f, .90f, .72f}, Ink = {.07f, .19f, .23f}, Gold = {.98f, .71f, .32f};
}

LevelOne::LevelOne(const LevelOneTerrain& terrain) : m_terrain(terrain), m_random(terrain.Seed() ^ 731u)
{
    CreateMeshes();
    const auto& cells = terrain.SpawnCells();
    // Bounded sampling: every accepted position belongs to the spawn's connected component.
    for (int attempt = 0; attempt < 3000 && m_enemies.size() < 32 && !cells.empty(); ++attempt)
    {
        const auto position = terrain.Center(cells[m_random() % cells.size()]);
        if (Distance(position, Safe) < 12 || Distance(position, {16, .55f, -12}) < 9)
        {
            continue;
        }
        bool crowded = false;
        for (const auto& enemy : m_enemies)
        {
            crowded |= Distance(enemy.home, position) < 4;
        }
        if (crowded)
        {
            continue;
        }
        Enemy enemy;
        enemy.position = enemy.home = enemy.target = position;
        enemy.hp = enemy.maxHp = 28 + float(m_random() % 13);
        m_enemies.push_back(enemy);
    }
    Enemy boss;
    boss.boss = true;
    boss.position = boss.home = boss.target = {16.5f, .55f, -11.5f};
    boss.hp = boss.maxHp = 360;
    m_enemies.push_back(boss);
    terrain.Flow(Safe.x, Safe.z, m_flow);
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
    m_slashTime = 0;
    for (auto& enemy : m_enemies)
    {
        enemy.windup = 0;
        enemy.cooldown = 1;
        if (enemy.hp > 0)
        {
            enemy.position = enemy.home;
            enemy.hp = enemy.maxHp;
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

void LevelOne::Reward(const Enemy& enemy)
{
    ++m_kills;
    GainExperience(enemy.boss ? 160 : 18);
    if (enemy.boss)
    {
        m_bossDefeated = true;
        Notify("보스 '산호갑각 대장' 처치! 레벨 1 완료 · 영웅 무기와 강화석을 회수하세요.");
    }
    auto drop = [&](int weapon)
    {
        if (m_drops.size() >= 512)
        {
            return;
        }
        Drop item;
        item.position = enemy.position;
        item.weapon = weapon;
        item.persistent = enemy.boss;
        m_drops.push_back(item);
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
    m_slashAim = {aim.x / length, 0, aim.z / length};
    m_slashPosition = player;
    m_slashTime = .20f;
    m_cooldown = .42f;
    for (auto& enemy : m_enemies)
    {
        if (enemy.hp <= 0)
        {
            continue;
        }
        const float distance = Distance(player, enemy.position);
        const float dot = distance > .001f ? ((enemy.position.x - player.x) * m_slashAim.x +
                                              (enemy.position.z - player.z) * m_slashAim.z) /
                                                 distance
                                           : 1;
        if (distance > 2.7f + (enemy.boss ? 1.f : .35f) || dot < .5f)
        {
            continue;
        }
        if (!m_terrain.Segment(player.x, player.z, enemy.position.x, enemy.position.z))
        {
            continue;
        }
        enemy.hp = (std::max)(0.f, enemy.hp - Damage());
        enemy.flash = .15f;
        if (m_numbers.size() < 64)
        {
            m_numbers.push_back({enemy.position, Damage(), .8f, false});
        }
        if (enemy.hp <= 0)
        {
            enemy.windup = 0;
            enemy.respawn = 25;
            Reward(enemy);
        }
    }
}

void LevelOne::Step(Vec3& position, Vec3 target, float distance)
{
    float length = Distance(position, target);
    if (length < .001f)
    {
        return;
    }
    if (!m_terrain.Segment(position.x, position.z, target.x, target.z))
    {
        const int index = m_terrain.Index(position.x, position.z);
        if (index < 0 || m_flow.empty())
        {
            return;
        }
        int best = index;
        for (const auto& d : {std::pair<int, int>{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
        {
            const int x = index % LevelOneTerrain::Size + d.first,
                      z = index / LevelOneTerrain::Size + d.second;
            if (!m_terrain.CellWalk(x, z))
            {
                continue;
            }
            const int next = z * LevelOneTerrain::Size + x;
            if (m_flow[next] >= 0 && (m_flow[best] < 0 || m_flow[next] < m_flow[best]))
            {
                best = next;
            }
        }
        target = m_terrain.Center(best);
        // Re-center first when clearance blocks a turn around a corner.
        if (!m_terrain.Segment(position.x, position.z, target.x, target.z))
        {
            target = m_terrain.Center(index);
        }
        length = Distance(position, target);
        if (length < .001f)
        {
            return;
        }
    }
    const float step = (std::min)(length, distance);
    const float dx = (target.x - position.x) / length * step, dz = (target.z - position.z) / length * step;
    if (m_terrain.Walk(position.x + dx, position.z + dz))
    {
        position.x += dx;
        position.z += dz;
    }
}

bool LevelOne::Update(float dt, Vec3 player, bool sailing)
{
    m_invincible = (std::max)(0.f, m_invincible - dt);
    m_cooldown = (std::max)(0.f, m_cooldown - dt);
    m_slashTime = (std::max)(0.f, m_slashTime - dt);
    m_noticeTime = (std::max)(0.f, m_noticeTime - dt);
    m_sinceHit += dt;
    m_flowTime -= dt;
    const bool safe = Distance(player, Safe) < 9;
    const bool active = !sailing && m_terrain.Walk(player.x, player.z);
    if (active && m_flowTime <= 0)
    {
        m_terrain.Flow(player.x, player.z, m_flow);
        m_flowTime = .25f;
    }
    if (m_sinceHit > 5)
    {
        m_health = (std::min)(float(MaxHealth()), m_health + dt * (safe ? 12 : 2));
    }
    for (auto& number : m_numbers)
    {
        number.time -= dt;
    }
    m_numbers.erase(
        std::remove_if(
            m_numbers.begin(),
            m_numbers.end(),
            [](const Number& n)
            {
                return n.time <= 0;
            }),
        m_numbers.end());
    for (auto& enemy : m_enemies)
    {
        enemy.flash = (std::max)(0.f, enemy.flash - dt);
        enemy.cooldown = (std::max)(0.f, enemy.cooldown - dt);
        if (enemy.hp <= 0)
        {
            if (!enemy.boss)
            {
                enemy.respawn -= dt;
                if (enemy.respawn <= 0 && Distance(player, enemy.home) > 12)
                {
                    enemy.position = enemy.home;
                    enemy.hp = enemy.maxHp;
                    enemy.cooldown = 1;
                }
            }
            continue;
        }
        const bool engaged = active && !safe && Distance(player, enemy.home) < (enemy.boss ? 13 : 12);
        if (!engaged)
        {
            enemy.windup = 0;
            enemy.position = enemy.home;
            enemy.hp = enemy.maxHp;
            continue;
        }
        if (enemy.windup > 0)
        {
            enemy.windup -= dt;
            if (enemy.windup <= 0)
            {
                const float radius = enemy.boss ? 3.6f : 1.35f;
                if (m_invincible <= 0 && Distance(player, enemy.target) <= radius &&
                    m_terrain.Segment(enemy.position.x, enemy.position.z, player.x, player.z))
                {
                    const int damage = enemy.boss ? 30 : 8;
                    m_health = (std::max)(0.f, m_health - damage);
                    m_invincible = .65f;
                    m_sinceHit = 0;
                    if (m_numbers.size() < 64)
                    {
                        m_numbers.push_back({player, damage, .8f, true});
                    }
                }
                enemy.cooldown = enemy.boss ? (enemy.hp < enemy.maxHp * .5f ? 1.3f : 2.3f) : 1.4f;
            }
            continue;
        }
        if (Distance(player, enemy.position) < (enemy.boss ? 4.2f : 1.8f) && enemy.cooldown <= 0)
        {
            enemy.target = player;
            enemy.windup = enemy.boss ? .95f : .65f;
        }
        else if (Distance(player, enemy.position) > 1.1f)
        {
            Step(enemy.position, player, dt * (enemy.boss ? 1.5f : 1.9f));
        }
    }
    if (m_health <= 0)
    {
        Recover();
        Notify("쓰러졌습니다. 항구에서 회복했습니다. 경험치와 장비는 유지됩니다.");
        return true;
    }
    for (auto& item : m_drops)
    {
        item.age += dt;
        if (!active || item.collected)
        {
            continue;
        }
        const float distance = Distance(player, item.position);
        if (distance < 3.8f + (m_level - 1) * .08f)
        {
            item.attracted = true;
        }
        if (item.attracted && distance < 12)
        {
            item.speed = (std::min)(14.f, item.speed + dt * 18);
            Step(item.position, player, dt * item.speed);
        }
        if (Distance(player, item.position) < .65f &&
            m_terrain.Segment(player.x, player.z, item.position.x, item.position.z))
        {
            item.collected = true;
            if (item.weapon > m_weapon)
            {
                m_weapon = item.weapon;
                Notify("새 무기를 자동 장착했습니다! 무기 공격력 +" + std::to_string(m_weapon));
            }
            else
            {
                ++m_stones;
            }
        }
    }
    m_drops.erase(
        std::remove_if(
            m_drops.begin(),
            m_drops.end(),
            [](const Drop& item)
            {
                return item.collected || (!item.persistent && item.age > 120);
            }),
        m_drops.end());
    return false;
}

void LevelOne::CreateMeshes()
{
    using namespace Geometry;
    for (int variant = 0; variant < 4; ++variant)
    {
        Mesh mesh;
        const Vec3 p = {};
        const bool boss = variant >= 2, flash = (variant % 2) != 0;
        const float scale = boss ? 2.f : 1.f;
        const Color shell = flash ? Paper : (boss ? Color{.69f, .24f, .16f} : Color{.83f, .48f, .23f});
        Cone(mesh, {p.x, .56f, p.z}, .7f * scale, .85f * scale, shell, 8);
        for (int i = -1; i <= 1; i += 2)
        {
            Box(mesh,
                {p.x + i * .68f * scale - .13f, .6f, p.z - .5f * scale},
                {.26f * scale, .25f * scale, scale},
                {.59f, .30f, .16f});
            Cone(
                mesh,
                {p.x + i * .42f * scale, .78f, p.z - .65f * scale},
                .22f * scale,
                .55f * scale,
                Gold,
                5);
        }
        if (boss)
        {
            Cone(mesh, {p.x, 2.05f, p.z}, .45f, .65f, Gold, 5);
        }

        m_enemyMeshes[variant] = CachedMesh(std::move(mesh));
    }
    for (int variant = 0; variant < 2; ++variant)
    {
        Mesh mesh;
        const bool boss = variant != 0;
        const float radius = boss ? 3.6f : 1.35f;
        for (int i = 0; i < 48; ++i)
        {
            const float a = 2 * Pi * i / 48, b = 2 * Pi * (i + 1) / 48;
            const Vec3 t = {};
            Quad(
                mesh,
                {t.x + radius * std::cos(a), .59f, t.z + radius * std::sin(a)},
                {t.x + radius * std::cos(b), .59f, t.z + radius * std::sin(b)},
                {t.x + (radius - .12f) * std::cos(b), .59f, t.z + (radius - .12f) * std::sin(b)},
                {t.x + (radius - .12f) * std::cos(a), .59f, t.z + (radius - .12f) * std::sin(a)},
                {1, .24f, .10f});
        }
        m_warningMeshes[variant] = CachedMesh(std::move(mesh));
    }
    for (int weapon = 0; weapon < 2; ++weapon)
    {
        Mesh mesh;
        const Vec3 p = {};
        const float y = 0;
        if (weapon)
        {
            Box(mesh, {p.x - .08f, y, p.z - .08f}, {.16f, .9f, .16f}, {.79f, .91f, .94f});
            Box(mesh, {p.x - .3f, y + .15f, p.z - .1f}, {.6f, .12f, .2f}, Gold);
        }
        else
        {
            Cone(mesh, {p.x, y, p.z}, .23f, .42f, {.24f, .79f, .96f}, 5);
        }
        m_dropMeshes[weapon] = CachedMesh(std::move(mesh));
    }
    Mesh mesh;
    const float heading = 0;
    for (int i = 0; i < 20; ++i)
    {
        const float a = heading - Pi / 3 + 2 * Pi / 3 * i / 20,
                    b = heading - Pi / 3 + 2 * Pi / 3 * (i + 1) / 20;
        const Vec3 p = {};
        Quad(
            mesh,
            {p.x + 2.7f * std::cos(a), 1.1f, p.z + 2.7f * std::sin(a)},
            {p.x + 2.7f * std::cos(b), 1.1f, p.z + 2.7f * std::sin(b)},
            {p.x + 2.3f * std::cos(b), 1.1f, p.z + 2.3f * std::sin(b)},
            {p.x + 2.3f * std::cos(a), 1.1f, p.z + 2.3f * std::sin(a)},
            Gold);
    }
    m_slashMesh = CachedMesh(std::move(mesh));
}

void LevelOne::Render(Renderer& renderer, Vec3 offset) const
{
    for (const auto& enemy : m_enemies)
    {
        if (enemy.hp <= 0)
        {
            continue;
        }
        const int variant = (enemy.boss ? 2 : 0) + (enemy.flash > 0 ? 1 : 0);
        renderer.Submit(
            m_enemyMeshes[variant],
            {enemy.position.x + offset.x, offset.y, enemy.position.z + offset.z});
        if (enemy.windup > 0)
        {
            renderer.Submit(
                m_warningMeshes[enemy.boss ? 1 : 0],
                {enemy.target.x + offset.x, offset.y, enemy.target.z + offset.z});
        }
    }
    for (const auto& item : m_drops)
    {
        renderer.Submit(
            m_dropMeshes[item.weapon ? 1 : 0],
            {item.position.x + offset.x,
             offset.y + .85f + .13f * std::sin(item.age * 5),
             item.position.z + offset.z});
    }
    if (m_slashTime > 0)
    {
        renderer.Submit(
            m_slashMesh,
            {m_slashPosition.x + offset.x, offset.y, m_slashPosition.z + offset.z},
            std::atan2(m_slashAim.z, m_slashAim.x));
    }
}

void LevelOne::Overlay(Renderer& renderer, Vec3 offset, bool showHud) const
{
    for (const auto& enemy : m_enemies)
    {
        if (enemy.hp <= 0)
        {
            continue;
        }
        const auto p = renderer.Project(
            {enemy.position.x + offset.x, enemy.boss ? 3.3f : 1.8f, enemy.position.z + offset.z});
        if (p.x < 0 || p.x > renderer.Width() || p.y < 0 || p.y > renderer.Height())
        {
            continue;
        }
        const float width = enemy.boss ? 100.f : 34.f;
        renderer.Panel(p.x - width / 2, p.y, width, 5, Ink);
        renderer.Panel(
            p.x - width / 2,
            p.y,
            width * enemy.hp / enemy.maxHp,
            5,
            enemy.boss ? Color{.91f, .29f, .18f} : Color{.52f, .82f, .34f});
        if (enemy.boss)
        {
            renderer.Text(p.x - 68, p.y - 22, "산호갑각 대장", Gold, 1.6f);
        }
    }
    for (const auto& number : m_numbers)
    {
        auto p = renderer.Project(
            {number.position.x + offset.x, 2 + (1 - number.time) * 1.3f, number.position.z + offset.z});
        renderer.Text(p.x, p.y, std::to_string(number.amount), number.player ? Color{1, .25f, .2f} : Gold, 2);
    }
    if (!showHud)
    {
        return;
    }
    renderer.Panel(20, 118, 420, 135, Ink);
    std::ostringstream stats;
    stats << "레벨 " << m_level << " / 체력 " << int(m_health) << "/" << MaxHealth() << " / 공격력 "
          << Damage();
    renderer.Text(34, 124, stats.str(), Paper, 1.7f);
    renderer.Panel(34, 150, 390, 7, {.29f, .17f, .17f});
    renderer.Panel(34, 150, 390 * m_health / MaxHealth(), 7, {.78f, .28f, .23f});
    const int required = 40 + (m_level - 1) * 25;
    renderer.Text(
        34,
        164,
        "경험치 " + std::to_string(m_xp) + "/" + std::to_string(required) + " / 강화석 " +
            std::to_string(m_stones),
        Gold,
        1.6f);
    const std::string weaponName =
        m_weapon >= 18 ? "산호 대검"
                       : (m_weapon >= 8 ? "해적 커틀러스" : (m_weapon > 0 ? "선원 단검" : "훈련용 검"));
    renderer.Text(
        34,
        189,
        weaponName + " +" + std::to_string(m_upgrade) + " / F 강화 (" + std::to_string(m_upgrade + 1) + "개)",
        Paper,
        1.6f);
    renderer.Text(
        34,
        214,
        m_bossDefeated ? "보스 처치 완료! 계속 파밍할 수 있습니다."
                       : "목표: 파밍 → 레벨 3·무기 강화 → 북쪽 보스",
        Gold,
        1.5f);
    renderer.Text(34, 234, "좌클릭 근접 공격 / 붉은 원에서 벗어나세요", Paper, 1.4f);
    if (m_noticeTime > 0)
    {
        renderer.Panel(20, float(renderer.Height() - 246), 680, 53, Ink);
        renderer.Text(32, float(renderer.Height() - 242), m_notice, Gold, 1.6f, 650);
    }
}

void LevelOne::Map(Renderer& renderer, float left, float top, float extent, Vec3 player) const
{
    m_terrain.Map(renderer, left, top, extent);
    const float scale = extent / LevelOneTerrain::Size;
    auto marker = [&](Vec3 p, Color color, float size)
    {
        const float x = (p.x - LevelOneTerrain::Min) * scale, z = (p.z - LevelOneTerrain::Min) * scale;
        if (x >= 0 && z >= 0 && x < extent - size && z < extent - size)
        {
            renderer.Panel(left + x, top + z, size, size, color);
        }
    };
    marker(Safe, Gold, 5);
    if (!m_bossDefeated)
    {
        marker({16, .55f, -12}, {.95f, .22f, .17f}, 6);
    }
    marker(player, Paper, 5);
}
