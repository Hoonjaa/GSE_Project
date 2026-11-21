#include "stdafx.h"
#include "MonsterSystem.h"
#include "Diagnostics.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include "MeshCache.h"

namespace
{
    float Squared(Vec3 a, Vec3 b)
    {
        const float x = a.x - b.x, z = a.z - b.z;
        return x * x + z * z;
    }

    bool VillageMargin(const IslandLayout& map, Vec3 p)
    {
        if (map.Village(p.x, p.z))
        {
            return false;
        }
        for (int i = 0; i < 8; ++i)
        {
            const float a = i * 6.2831853f / 8;
            if (map.Village(p.x + 2 * std::cos(a), p.z + 2 * std::sin(a)))
            {
                return false;
            }
        }
        return true;
    }
}

MonsterSystem::MonsterSystem(
    SceneGraph& scene,
    World& world,
    std::function<void(int)> hitPlayer,
    std::function<void(IslandKey, bool)> defeated,
    std::function<bool(InventoryItem)> collect)
    : m_scene(scene), m_world(world), m_hitPlayer(std::move(hitPlayer)), m_defeated(std::move(defeated)),
      m_collect(std::move(collect))
{
    const auto seed = static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count());
    m_random.seed(seed);
    m_dropMaterial = MeshCache::LoadOrCreate(
        "monster_item_material_v1",
        []
        {
            Mesh m;
            Geometry::Cone(m, {0, 0, 0}, .22f, .4f, {.91f, .71f, .25f}, 6);
            return m;
        });
    m_dropWeapon = MeshCache::LoadOrCreate(
        "monster_item_weapon_v1",
        []
        {
            Mesh m;
            Geometry::Box(m, {-.04f, 0, -.04f}, {.08f, .75f, .08f}, {.8f, .9f, .93f});
            Geometry::Box(m, {-.2f, .15f, -.05f}, {.4f, .08f, .1f}, {.91f, .71f, .25f});
            return m;
        });
    m_dropArmor = MeshCache::LoadOrCreate(
        "monster_item_armor_v1",
        []
        {
            Mesh m;
            Geometry::Box(m, {-.2f, 0, -.14f}, {.4f, .4f, .28f}, {.33f, .55f, .74f});
            return m;
        });
    Diagnostics::Get().Event(
        "monster_preview catalog=40 ordinary=32 epic=8 item_drop_chance=50 spawn_probability=disabled "
        "drop_seed=" +
        std::to_string(seed));
}

MonsterSystem::~MonsterSystem()
{
    for (const auto& group : m_groups)
    {
        m_scene.Destroy(group.second->root);
    }
    m_scene.CollectDestroyed();
}

WorldPosition MonsterSystem::Position(const Group& group, Vec3 local) const
{
    auto p = group.layout->Info().center;
    p.Move(local.x, local.z);
    return p;
}

void MonsterSystem::Spawn(Group& group)
{
    const auto started = std::chrono::steady_clock::now();
    const auto& map = *group.layout;
    const auto& info = map.Info();
    group.root = m_scene.Create<Actor>(NoActor, "섬 몬스터").Id();
    group.context.hitPlayer = m_hitPlayer;
    std::vector<Vec3> candidates;
    for (int i : map.SpawnCells())
    {
        const auto p = map.Center(i);
        if (VillageMargin(map, p) && map.Walk(p.x, p.z, .5f) && Squared(p, map.Spawn()) >= 25)
        {
            candidates.push_back(p);
        }
    }
    if (candidates.empty())
    {
        for (int i : map.SpawnCells())
        {
            const auto p = map.Center(i);
            if (group.context.walk[i])
            {
                candidates.push_back(p);
            }
        }
    }
    if (candidates.empty())
    {
        Diagnostics::Get().Event(
            "monster_spawn_no_legal_cells island_x=" + std::to_string(info.key.x) +
            " island_z=" + std::to_string(info.key.z));
        return;
    }
    std::vector<int> reachable;
    const auto port = map.Spawn();
    map.Flow(port.x, port.z, reachable);
    size_t epic = 0;
    float epicScore = -1;
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        const auto p = candidates[i];
        const int cell = map.Index(p.x, p.z);
        if (reachable[cell] < 0 || !map.Walk(p.x, p.z, 1))
        {
            continue;
        }
        const float score = Squared(p, port);
        if (score > epicScore)
        {
            epicScore = score;
            epic = i;
        }
    }
    auto add = [&](unsigned slot, Vec3 p)
    {
        const auto id = MonsterCatalog::ForIsland(info.shape, slot);
        const auto& definition = MonsterCatalog::Get(id);
        group.monsters.push_back(m_scene
                                     .Create<MonsterActor>(
                                         group.root,
                                         definition,
                                         m_meshes.Get(id),
                                         m_meshes.warning,
                                         m_meshes.epicRing,
                                         p)
                                     .Id());
        Diagnostics::Get().Event(
            "monster_spawn island_x=" + std::to_string(info.key.x) +
            " island_z=" + std::to_string(info.key.z) + " species=" + std::to_string(id) +
            " epic=" + std::to_string(definition.epic) + " local_x=" + std::to_string(p.x) +
            " local_z=" + std::to_string(p.z));
    };
    add(4, candidates[epic]);
    const float area = map.SpawnCells().size() * map.CellSize() * map.CellSize();
    const size_t target = (std::min)(
        candidates.size() - 1,
        size_t((std::max)(
            MonsterPreviewRules::MinimumNormalCount,
            int(std::ceil(area / MonsterPreviewRules::AreaPerNormal)))));
    std::vector<float> nearest(candidates.size());
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        nearest[i] = Squared(candidates[i], candidates[epic]);
    }
    nearest[epic] = -1;
    size_t selected = 0;
    for (; selected < target; ++selected)
    {
        size_t best = 0;
        float distance = -1;
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            if (nearest[i] > distance)
            {
                best = i;
                distance = nearest[i];
            }
        }
        if (distance < 0 || (distance < 4 && selected >= 4))
        {
            break;
        }
        add(unsigned(selected % 4), candidates[best]);
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            nearest[i] = (std::min)(nearest[i], Squared(candidates[i], candidates[best]));
        }
        nearest[best] = -1;
    }
    Diagnostics::Get().Event(
        "monster_island_ready x=" + std::to_string(info.key.x) + " z=" + std::to_string(info.key.z) +
        " width=" + std::to_string(info.width) + " spawn_area=" + std::to_string(area) +
        " ordinary=" + std::to_string(selected) + " epic=1" + " cpu_ms=" +
        std::to_string(
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count()));
}

void MonsterSystem::Sync()
{
    for (auto& group : m_groups)
    {
        group.second->seen = false;
    }
    for (const auto& chunk : m_world.Chunks())
    {
        if (!chunk.second.island)
        {
            continue;
        }
        const auto key = chunk.second.island->Info().key;
        auto found = m_groups.find(key);
        if (found == m_groups.end())
        {
            const auto start = std::chrono::steady_clock::now();
            auto group = std::make_unique<Group>(chunk.second.island);
            Spawn(*group);
            found = m_groups.emplace(key, std::move(group)).first;
            Diagnostics::Get().Event(
                "monster_resident_created x=" + std::to_string(key.x) + " z=" + std::to_string(key.z) +
                " total_cpu_ms=" +
                std::to_string(
                    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                        .count()));
        }
        found->second->seen = true;
    }
    for (auto it = m_groups.begin(); it != m_groups.end();)
    {
        if (!it->second->seen)
        {
            Diagnostics::Get().Event(
                "monster_island_unloaded x=" + std::to_string(it->first.x) +
                " z=" + std::to_string(it->first.z));
            m_scene.Destroy(it->second->root);
            it = m_groups.erase(it);
        }
        else
        {
            ++it;
        }
    }
    m_scene.CollectDestroyed();
}

void MonsterSystem::Update(float dt, const WorldPosition& player, bool onLand, bool paused)
{
    Sync();
    if (paused)
    {
        return;
    }
    const auto* current = m_world.CurrentIsland(player);
    for (auto& item : m_groups)
    {
        auto& group = *item.second;
        group.context.player = player.RelativeTo(group.layout->Info().center);
        group.context.active = onLand && current == group.layout.get();
        group.context.safe = group.layout->Village(group.context.player.x, group.context.player.z);
        group.flowTime -= dt;
        group.coarseTime += dt;
        const int cell = group.layout->Index(group.context.player.x, group.context.player.z);
        if (group.context.active && !group.context.safe && group.flowTime <= 0 && group.flowCell != cell)
        {
            group.context.Flow(group.context.player, group.context.playerFlow);
            group.flowCell = cell;
            group.flowTime = MonsterPreviewRules::CoarseInterval;
        }
        const bool coarse = group.coarseTime >= MonsterPreviewRules::CoarseInterval;
        for (auto id : group.monsters)
        {
            auto* actor = m_scene.Get<MonsterActor>(id);
            if (!m_world.IsActive(Position(group, actor->transform.position)))
            {
                continue;
            }
            const bool near = group.context.active &&
                              Squared(actor->transform.position, group.context.player) <
                                  MonsterPreviewRules::DetailedRadius * MonsterPreviewRules::DetailedRadius;
            if (near || coarse)
            {
                actor->Tick(near ? dt : group.coarseTime, group.context);
            }
        }
        if (coarse)
        {
            group.coarseTime = 0;
        }
        for (auto it = group.drops.begin(); it != group.drops.end();)
        {
            auto* drop = m_scene.Get<MonsterItemDrop>(*it);
            if (!drop)
            {
                it = group.drops.erase(it);
                continue;
            }
            if (m_world.IsActive(Position(group, drop->transform.position)) &&
                drop->Tick(dt, group.context, m_collect))
            {
                drop->Destroy();
                it = group.drops.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

void MonsterSystem::SetActive(bool active)
{
    for (const auto& item : m_groups)
    {
        auto* root = m_scene.Find(item.second->root);
        root->visible = root->enabled = active;
    }
}

MonsterPopulation MonsterSystem::Population(IslandKey island) const
{
    MonsterPopulation result;
    const auto found = m_groups.find(island);
    if (found == m_groups.end())
    {
        return result;
    }
    const auto& group = *found->second;
    for (const auto id : group.monsters)
    {
        const auto* monster = m_scene.Get<MonsterActor>(id);
        if (!monster)
        {
            continue;
        }
        ++result.spawned;
        if (monster->health > 0)
        {
            ++result.alive;
            result.epicAlive += monster->definition.epic;
            result.active += m_world.IsActive(Position(group, monster->transform.position));
        }
    }
    return result;
}

bool MonsterSystem::AttackWarning(const WorldPosition& player) const
{
    const auto* island = m_world.CurrentIsland(player);
    if (!island)
    {
        return false;
    }
    const auto group = m_groups.find(island->Info().key);
    if (group == m_groups.end())
    {
        return false;
    }
    const auto local = player.RelativeTo(island->Info().center);
    for (const auto id : group->second->monsters)
    {
        const auto* actor = m_scene.Get<MonsterActor>(id);
        if (actor && actor->AttackWarning() && Squared(local, actor->transform.position) <= 64)
        {
            return true;
        }
    }
    return false;
}

void MonsterSystem::Prepare(const WorldPosition& camera, bool showStatus)
{
    for (auto& item : m_groups)
    {
        auto& group = *item.second;
        m_scene.Find(group.root)->transform.position = group.layout->Info().center.RelativeTo(camera);
        for (auto id : group.monsters)
        {
            auto* actor = m_scene.Get<MonsterActor>(id);
            actor->showStatus = showStatus;
            actor->visible =
                actor->health > 0 && m_world.IsActive(Position(group, actor->transform.position));
        }
        for (auto id : group.drops)
        {
            auto* drop = m_scene.Get<MonsterItemDrop>(id);
            if (drop)
            {
                drop->visible = m_world.IsActive(Position(group, drop->transform.position));
            }
        }
    }
}

void MonsterSystem::Attack(const WorldPosition& player, Vec3 aim, int damage)
{
    const auto* layout = m_world.CurrentIsland(player);
    if (!layout)
    {
        return;
    }
    const auto found = m_groups.find(layout->Info().key);
    if (found == m_groups.end())
    {
        return;
    }
    auto& group = *found->second;
    const auto local = player.RelativeTo(layout->Info().center);
    if (layout->Village(local.x, local.z))
    {
        return;
    }
    const float length = std::hypot(aim.x, aim.z);
    if (length < .001f)
    {
        return;
    }
    for (auto id : group.monsters)
    {
        auto* actor = m_scene.Get<MonsterActor>(id);
        if (actor->health <= 0 || !m_world.IsActive(Position(group, actor->transform.position)))
        {
            continue;
        }
        const auto target = actor->transform.position;
        const float distance = std::sqrt(Squared(local, target));
        const float dot = distance > .001f ? ((target.x - local.x) * aim.x + (target.z - local.z) * aim.z) /
                                                 (distance * length)
                                           : 1;
        if (distance > 2.7f + (actor->definition.epic ? 1.f : .35f) || dot < .5f ||
            !group.context.Segment(local, target))
        {
            continue;
        }
        const bool killed = actor->TakeDamage(damage);
        Diagnostics::Get().Event(
            "monster_hit species=" + std::to_string(actor->definition.id) +
            " hp=" + std::to_string(actor->health) + " damage=" + std::to_string(damage));
        if (killed)
        {
            Diagnostics::Get().Event("monster_defeated species=" + std::to_string(actor->definition.id));
            for (const auto& entry : MonsterDrops(actor->definition.id, actor->definition.epic))
            {
                const unsigned roll = std::uniform_int_distribution<unsigned>(0, 99)(m_random);
                const bool hit = roll < entry.chance;
                Diagnostics::Get().Event(
                    "monster_drop_roll species=" + std::to_string(actor->definition.id) +
                    " item=" + std::to_string(static_cast<unsigned>(entry.item)) +
                    " chance=" + std::to_string(entry.chance) + " roll=" + std::to_string(roll) +
                    " hit=" + std::to_string(hit));
                if (!hit)
                {
                    continue;
                }
                for (int unit = 0; unit < entry.count; ++unit)
                {
                    if (group.drops.size() >= 128)
                    {
                        m_collect(entry.item);
                        continue;
                    }
                    const auto& def = ItemCatalog::Get(entry.item);
                    const auto mesh = def.icon == ItemIcon::Sword
                                          ? m_dropWeapon
                                          : (def.equipment != EquipSlot::None ? m_dropArmor : m_dropMaterial);
                    group.drops.push_back(
                        m_scene
                            .Create<MonsterItemDrop>(group.root, mesh, entry.item, actor->transform.position)
                            .Id());
                }
            }
            m_defeated(layout->Info().key, actor->definition.epic);
        }
    }
}
