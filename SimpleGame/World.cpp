#include "stdafx.h"
#include "World.h"
#include "WorldActors.h"
#include "MeshCache.h"
#include "MapState.h"
#include "TownServiceActor.h"
#include "Diagnostics.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace
{
    constexpr float Pi = 3.14159265359f;

    WorldPosition Origin(World::Key key)
    {
        WorldPosition p;
        p.chunkX = key.first;
        p.chunkZ = key.second;
        p.x = p.z = 0;
        return p;
    }

    bool InChunk(Vec3 p)
    {
        return p.x >= 0 && p.z >= 0 && p.x < ChunkSize && p.z < ChunkSize;
    }

    std::uint64_t Distance(std::int64_t a, std::int64_t b)
    {
        return a >= b ? std::uint64_t(a) - std::uint64_t(b) : std::uint64_t(b) - std::uint64_t(a);
    }

    std::uint64_t ChunkDistance(World::Key a, World::Key b)
    {
        return (std::max)(Distance(a.first, b.first), Distance(a.second, b.second));
    }
}

constexpr std::uint64_t World::Seed;

World::World(SceneGraph& scene)
    : m_scene(scene), m_start(std::make_shared<IslandLayout>(IslandGeneration::Describe({0, 0}))),
      m_tutorial(m_start)
{
    m_root = m_scene.Create<Actor>(NoActor, "월드").Id();
    m_ocean = m_scene.Create<OceanActor>(m_root).Id();
    m_chunksRoot = m_scene.Create<Actor>(m_root, "스트리밍 청크").Id();
    m_islands[{0, 0}] = {m_start, MapState::Visited({0, 0}), false};
    m_house = MeshCache::LoadOrCreate(
        "map_house_v1",
        []
        {
            Mesh m;
            Geometry::Box(m, {-1.4f, 0, -1.2f}, {2.8f, 2.2f, 2.4f}, {.94f, .85f, .63f});
            Geometry::Quad(
                m,
                {-1.5f, 2.2f, -1.3f},
                {1.5f, 2.2f, -1.3f},
                {1.5f, 3.1f, 0},
                {-1.5f, 3.1f, 0},
                {.83f, .36f, .23f});
            Geometry::Quad(
                m,
                {-1.5f, 3.1f, 0},
                {1.5f, 3.1f, 0},
                {1.5f, 2.2f, 1.3f},
                {-1.5f, 2.2f, 1.3f},
                {.69f, .29f, .19f});
            Geometry::Triangle(
                m,
                {1.5f, 2.2f, -1.3f},
                {1.5f, 2.2f, 1.3f},
                {1.5f, 3.1f, 0},
                {.9f, .76f, .51f});
            Geometry::Triangle(
                m,
                {-1.5f, 2.2f, -1.3f},
                {-1.5f, 2.2f, 1.3f},
                {-1.5f, 3.1f, 0},
                {.75f, .65f, .47f});
            Geometry::Box(m, {-.9f, .75f, 1.2f}, {.55f, .65f, .03f}, {.10f, .31f, .36f});
            Geometry::Box(m, {.55f, 0, 1.2f}, {.6f, 1.3f, .03f}, {.35f, .21f, .13f});
            return m;
        });
    m_palm = MeshCache::LoadOrCreate(
        "map_palm_v1",
        []
        {
            Mesh m;
            Geometry::Box(m, {-.13f, 0, -.13f}, {.26f, 3, .26f}, {.54f, .37f, .20f});
            for (int n = 0; n < 6; ++n)
            {
                const float a = n * Pi / 3, dx = std::cos(a), dz = std::sin(a);
                Geometry::Triangle(
                    m,
                    {0, 3.3f, 0},
                    {dx * .8f - dz * .35f, 3.1f, dz * .8f + dx * .35f},
                    {dx * 1.8f, 2.6f, dz * 1.8f},
                    {.18f, .52f, .28f});
                Geometry::Triangle(
                    m,
                    {0, 3.3f, 0},
                    {dx * 1.8f, 2.6f, dz * 1.8f},
                    {dx * .8f + dz * .35f, 3.1f, dz * .8f - dx * .35f},
                    {.29f, .64f, .31f});
            }
            return m;
        });
    m_pier = MeshCache::LoadOrCreate(
        "map_pier_v1",
        []
        {
            Mesh m;
            Geometry::Box(m, {0, .37f, -.75f}, {1, .18f, 1.5f}, {.59f, .39f, .22f});
            return m;
        });
    Diagnostics::Get().Event("map_generator=v1 island_width=max_shoreline_span gap=shore_to_shore_min_300 "
                             "sector_chunks=16 engine_chunk_m=32 queue_chunks_per_update=1");
}

World::~World()
{
    for (const auto& entry : m_islands)
    {
        if (entry.second.dirty)
        {
            MapState::SaveVisited(entry.first);
        }
    }
    m_scene.Destroy(m_root);
    m_scene.CollectDestroyed();
}

void World::RebuildQueue(const WorldPosition& player)
{
    m_pending.clear();
    std::vector<Key> pending;
    for (int z = -m_radius - 1; z <= m_radius + 1; ++z)
    {
        for (int x = -m_radius - 1; x <= m_radius + 1; ++x)
        {
            const Key key = {player.chunkX + x, player.chunkZ + z};
            if (m_chunks.find(key) == m_chunks.end())
            {
                pending.push_back(key);
            }
        }
    }
    std::stable_sort(
        pending.begin(),
        pending.end(),
        [&](Key a, Key b)
        {
            const auto da = ChunkDistance(a, m_center), db = ChunkDistance(b, m_center);
            if (da != db)
            {
                return da < db;
            }
            return Distance(a.first, m_center.first) + Distance(a.second, m_center.second) <
                   Distance(b.first, m_center.first) + Distance(b.second, m_center.second);
        });
    m_pending.assign(pending.begin(), pending.end());
    Diagnostics::Get().Event(
        "world_window x=" + std::to_string(m_center.first) + " z=" + std::to_string(m_center.second) +
        " render_radius=" + std::to_string(m_radius) + " preload_radius=" + std::to_string(m_radius + 1) +
        " unload_radius=" + std::to_string(m_radius + 2));
}

void World::Update(const WorldPosition& player, int radius)
{
    // Retain the viewport-derived engine radius. Borders are streaming policy,
    // not a newly fixed gameplay render distance.
    radius = (std::max)(1, (std::min)(64, radius));
    const Key center = {player.chunkX, player.chunkZ};
    if (m_radius != radius || m_center != center)
    {
        m_radius = radius;
        m_center = center;
        for (auto it = m_chunks.begin(); it != m_chunks.end();)
        {
            if (ChunkDistance(it->first, m_center) > static_cast<unsigned>(m_radius + 2))
            {
                m_scene.Destroy(it->second.actor);
                it = m_chunks.erase(it);
            }
            else
            {
                ++it;
            }
        }
        RebuildQueue(player);
    }
    const bool generating = !m_pending.empty();
    if (generating)
    {
        const auto start = std::chrono::steady_clock::now();
        const auto key = m_pending.front();
        m_pending.pop_front();
        Generate(key);
        Diagnostics::Get().Event(
            "chunk_generated x=" + std::to_string(key.first) + " z=" + std::to_string(key.second) +
            " pending=" + std::to_string(m_pending.size()) + " cpu_ms=" +
            std::to_string(
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()));
    }
    for (auto& entry : m_chunks)
    {
        const bool active = ChunkDistance(entry.first, m_center) <= static_cast<unsigned>(m_radius);
        entry.second.visible = active;
        auto* actor = m_scene.Find(entry.second.actor);
        actor->visible = active;
        actor->enabled = active;
    }
    for (auto it = m_islands.begin(); it != m_islands.end();)
    {
        if (!(it->first == IslandKey{0, 0}) && it->second.layout.use_count() == 1)
        {
            if (it->second.dirty && !MapState::SaveVisited(it->first))
            {
                ++it;
                continue;
            }
            it = m_islands.erase(it);
        }
        else
        {
            ++it;
        }
    }
    Visit(player);
    if (generating)
    {
        Diagnostics::Get().Event(
            "world_stream loaded=" + std::to_string(m_chunks.size()) +
            " active=" + std::to_string(VisibleChunks()) + " pending=" + std::to_string(m_pending.size()) +
            " islands=" + std::to_string(m_islands.size()));
    }
}

void World::Generate(Key key)
{
    const auto origin = Origin(key);
    IslandChunk chunk;
    const auto sector = IslandGeneration::Sector(key.first, key.second);
    for (int dz = -1; dz <= 1 && !chunk.island; ++dz)
    {
        for (int dx = -1; dx <= 1 && !chunk.island; ++dx)
        {
            const IslandKey islandKey = {sector.x + dx, sector.z + dz};
            const auto descriptor = IslandGeneration::Describe(islandKey);
            const auto relative = descriptor.center.RelativeTo(origin);
            // Conservative bound contains land, houses and docks of every current shape.
            if (relative.x < -MapCatalog::MaximumIslandSize || relative.z < -MapCatalog::MaximumIslandSize ||
                relative.x > ChunkSize + MapCatalog::MaximumIslandSize ||
                relative.z > ChunkSize + MapCatalog::MaximumIslandSize)
            {
                continue;
            }
            auto found = m_islands.find(islandKey);
            if (found == m_islands.end())
            {
                auto layout = std::make_shared<IslandLayout>(descriptor);
                Diagnostics::Get().Event(
                    "island_generated x=" + std::to_string(islandKey.x) +
                    " z=" + std::to_string(islandKey.z) +
                    " shape=" + std::to_string(static_cast<int>(descriptor.shape)) +
                    " width=" + std::to_string(descriptor.width) +
                    " village_ratio=" + std::to_string(layout->VillageArea() / layout->LandArea()));
                found =
                    m_islands.emplace(islandKey, ResidentIsland{layout, MapState::Visited(islandKey), false})
                        .first;
            }
            chunk.island = found->second.layout;
        }
    }
    chunk.actor = m_scene.Create<Actor>(m_chunksRoot, "월드 청크").Id();
    chunk.waves = m_scene.Create<WaveActor>(chunk.actor).Id();
    if (chunk.island)
    {
        Mesh terrain;
        const auto offset = chunk.island->Info().center.RelativeTo(origin);
        chunk.island->AppendTerrain(terrain, offset, static_cast<float>(ChunkSize));
        if (!terrain.empty())
        {
            chunk.terrain = CachedMesh(
                std::move(terrain),
                "map_chunk_" + std::to_string(key.first) + "_" + std::to_string(key.second));
            m_scene.Create<MeshActor>(chunk.actor, chunk.terrain, "청크 지형");
        }
        PlaceProps(chunk, origin);
    }
    m_chunks.emplace(key, std::move(chunk));
}

void World::PlaceProps(IslandChunk& chunk, const WorldPosition& origin)
{
    const auto& layout = *chunk.island;
    const auto offset = layout.Info().center.RelativeTo(origin);
    auto relative = [&](Vec3 p)
    {
        return Vec3{p.x + offset.x, p.y, p.z + offset.z};
    };
    for (auto p : layout.Houses())
    {
        const auto q = relative(p);
        if (InChunk(q))
        {
            auto& actor = m_scene.Create<MeshActor>(chunk.actor, m_house, "항구 건물");
            actor.transform.position = q;
        }
    }
    for (auto kind : {TownServiceKind::Shop, TownServiceKind::Shipyard})
    {
        const auto p = relative(kind == TownServiceKind::Shop ? layout.ShopPoint() : layout.ShipyardPoint());
        if (InChunk(p))
        {
            auto& actor = m_scene.Create<TownServiceActor>(chunk.actor, kind);
            actor.transform.position = {p.x, 0, p.z};
        }
    }
    const auto pier = relative(layout.PierStart());
    if (InChunk(pier))
    {
        auto& actor =
            m_scene.Create<ScaledMeshActor>(chunk.actor, m_pier, Vec3{layout.PierLength(), 1, 1}, "부두");
        actor.transform.position = {pier.x, 0, pier.z};
        const auto direction = layout.PierDirection();
        actor.transform.rotation = std::atan2(direction.z, direction.x);
    }
    for (int i = 0; i < IslandLayout::Size * IslandLayout::Size; ++i)
    {
        const auto p = layout.Center(i), q = relative(p);
        if (!InChunk(q) || !layout.Land(p.x, p.z) || layout.Village(p.x, p.z))
        {
            continue;
        }
        if (IslandGeneration::Mix(layout.Info().seed ^ i) % 500 != 0)
        {
            continue;
        }
        auto& actor = m_scene.Create<MeshActor>(chunk.actor, m_palm, "야자수");
        actor.transform.position = q;
    }
}

const IslandChunk* World::At(const WorldPosition& p) const
{
    const auto found = m_chunks.find({p.chunkX, p.chunkZ});
    return found == m_chunks.end() ? nullptr : &found->second;
}

bool World::IsActive(const WorldPosition& p) const
{
    const auto* chunk = At(p);
    return chunk && chunk->visible;
}

bool World::ConservativeSea(const WorldPosition& p, float radius) const
{
    const auto sector = IslandGeneration::Sector(p.chunkX, p.chunkZ);
    for (int z = -1; z <= 1; ++z)
    {
        for (int x = -1; x <= 1; ++x)
        {
            const auto descriptor = IslandGeneration::Describe({sector.x + x, sector.z + z});
            const auto distance = descriptor.center.RelativeTo(p);
            if (std::hypot(distance.x, distance.z) <= descriptor.width + radius)
            {
                return false;
            }
        }
    }
    return true;
}

bool World::TutorialActive() const
{
    // This also covers the starter shore and dock outside its center chunk.
    const auto p = m_start->Info().center.RelativeTo(Origin(m_center));
    const float extent = static_cast<float>((m_radius + 1) * ChunkSize);
    return std::abs(p.x) <= extent + MapCatalog::MaximumIslandSize &&
           std::abs(p.z) <= extent + MapCatalog::MaximumIslandSize;
}

bool World::LandAt(const WorldPosition& p, float margin) const
{
    const auto* chunk = At(p);
    if (!chunk || !chunk->island)
    {
        return false;
    }
    const auto q = p.RelativeTo(chunk->island->Info().center);
    if (chunk->island->Land(q.x, q.z))
    {
        return true;
    }
    for (int i = 0; i < 12 && margin > 0; ++i)
    {
        const float a = 2 * Pi * i / 12;
        if (chunk->island->Land(q.x + margin * std::cos(a), q.z + margin * std::sin(a)))
        {
            return true;
        }
    }
    return false;
}

bool World::CanWalk(const WorldPosition& p) const
{
    const auto* chunk = At(p);
    if (!chunk || !chunk->island || !chunk->visible)
    {
        return false;
    }
    const auto q = p.RelativeTo(chunk->island->Info().center);
    return chunk->island->Walk(q.x, q.z);
}

bool World::CanSail(const WorldPosition& p, float heading, float length) const
{
    if (!IsActive(p) || LandAt(p, .2f))
    {
        return false;
    }
    const auto* chunk = At(p);
    if (chunk->island)
    {
        const auto q = p.RelativeTo(chunk->island->Info().center);
        if (chunk->island->Dock(q.x, q.z, -.1f))
        {
            return false;
        }
    }
    const double c = std::cos(heading), s = std::sin(heading);
    for (int i = 0; i < 24; ++i)
    {
        auto edge = p;
        const double x = length * (1.7 / 3.85) * .5 * std::cos(2 * Pi * i / 24);
        const double z = length * .5 * std::sin(2 * Pi * i / 24);
        edge.Move(x * c - z * s, x * s + z * c);
        if (!IsActive(edge) || LandAt(edge, .15f))
        {
            return false;
        }
        const auto* cell = At(edge);
        if (cell->island)
        {
            const auto q = edge.RelativeTo(cell->island->Info().center);
            if (cell->island->Dock(q.x, q.z))
            {
                return false;
            }
        }
    }
    return true;
}

const IslandLayout* World::CurrentIsland(const WorldPosition& p) const
{
    const auto* chunk = At(p);
    if (!chunk || !chunk->island)
    {
        return nullptr;
    }
    const auto q = p.RelativeTo(chunk->island->Info().center);
    return chunk->island->Land(q.x, q.z) || chunk->island->Dock(q.x, q.z) ? chunk->island.get() : nullptr;
}

void World::Visit(const WorldPosition& p)
{
    const auto* layout = CurrentIsland(p);
    if (!layout)
    {
        return;
    }
    auto& state = m_islands.find(layout->Info().key)->second;
    if (!state.visited)
    {
        state.visited = true;
        state.dirty = !MapState::SaveVisited(layout->Info().key);
    }
}

bool World::WasVisited(IslandKey key) const
{
    const auto found = m_islands.find(key);
    return found != m_islands.end() && found->second.visited;
}

WorldPosition World::StartSpawn() const
{
    auto p = m_start->Info().center;
    const auto q = m_start->Spawn();
    p.Move(q.x, q.z);
    return p;
}

WorldPosition World::StartShip(float length) const
{
    return ShipyardMooring({0, 0}, length);
}

WorldPosition World::ShipyardMooring(IslandKey key, float length) const
{
    const auto found = m_islands.find(key);
    if (found == m_islands.end())
    {
        return WorldPosition{};
    }
    const auto& layout = *found->second.layout;
    auto position = layout.Info().center;
    const auto local = layout.ShipSpawn(length);
    position.Move(local.x, local.z);
    return position;
}

float World::OutboundHeading(IslandKey key) const
{
    const auto found = m_islands.find(key);
    if (found == m_islands.end())
    {
        return 0;
    }
    const auto d = found->second.layout->PierDirection();
    return std::atan2(d.x, -d.z);
}

TownInteraction World::ServiceAt(const WorldPosition& player) const
{
    TownInteraction result;
    float nearest = 2.2f;
    for (const auto& entry : m_islands)
    {
        const auto& layout = *entry.second.layout;
        for (auto kind : {TownServiceKind::Shop, TownServiceKind::Shipyard})
        {
            auto point = layout.Info().center;
            const auto local = kind == TownServiceKind::Shop ? layout.ShopPoint() : layout.ShipyardPoint();
            point.Move(local.x, local.z);
            if (!IsActive(point))
            {
                continue;
            }
            const auto relative = point.RelativeTo(player);
            const float distance = std::hypot(relative.x, relative.z);
            if (distance < nearest)
            {
                nearest = distance;
                result.kind = kind;
                result.island = entry.first;
                result.position = point;
            }
        }
    }
    return result;
}

WorldPosition World::StartVillage() const
{
    auto p = m_start->Info().center;
    const auto q = m_start->VillageCenter();
    p.Move(q.x, q.z);
    return p;
}

size_t World::VisibleChunks() const
{
    size_t count = 0;
    for (const auto& entry : m_chunks)
    {
        count += entry.second.visible ? 1 : 0;
    }
    return count;
}

void World::Prepare(const WorldPosition& camera, float time, bool grid)
{
    m_scene.Get<OceanActor>(m_ocean)->extent = static_cast<float>((m_radius + 2) * ChunkSize);
    for (const auto& entry : m_chunks)
    {
        if (!entry.second.visible)
        {
            continue;
        }
        m_scene.Find(entry.second.actor)->transform.position = Origin(entry.first).RelativeTo(camera);
        auto* waves = m_scene.Get<WaveActor>(entry.second.waves);
        waves->time = time;
        waves->grid = grid;
    }
}

void World::DrawMap(Renderer& r, const WorldPosition& center, float left, float top, float extent) const
{
    const float range = static_cast<float>((std::max)(2, m_radius) * ChunkSize);
    const float scale = extent / (2 * range);
    for (const auto& entry : m_islands)
    {
        const auto& layout = *entry.second.layout;
        const auto offset = layout.Info().center.RelativeTo(center);
        if (std::abs(offset.x) > range + MapCatalog::MaximumIslandSize ||
            std::abs(offset.z) > range + MapCatalog::MaximumIslandSize)
        {
            continue;
        }
        const float size = layout.CellSize() * scale;
        for (int i = 0; i < IslandLayout::Size * IslandLayout::Size; ++i)
        {
            const auto p = layout.Center(i);
            if (!layout.Land(p.x, p.z))
            {
                continue;
            }
            const float x = left + extent * .5f + (offset.x + p.x - layout.CellSize() * .5f) * scale;
            const float z = top + extent * .5f + (offset.z + p.z - layout.CellSize() * .5f) * scale;
            if (x < left || z < top || x + size > left + extent || z + size > top + extent)
            {
                continue;
            }
            r.Panel(
                x,
                z,
                size,
                size,
                layout.Village(p.x, p.z) ? Color{.85f, .73f, .43f} : Color{.61f, .72f, .40f});
        }
    }
    r.Panel(left + extent * .5f - 2, top + extent * .5f - 2, 4, 4, {.96f, .90f, .72f});
}
