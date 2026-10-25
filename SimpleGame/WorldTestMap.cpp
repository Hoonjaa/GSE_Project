#include "stdafx.h"
#include "World.h"
#include "IslandTestMap.h"
#include "Diagnostics.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>

void World::SwitchMap()
{
    auto* oldRoot = m_scene.Find(m_chunksRoot);
    oldRoot->visible = oldRoot->enabled = false;
    std::swap(m_chunksRoot, m_parked.root);
    m_chunks.swap(m_parked.chunks);
    m_islands.swap(m_parked.islands);
    m_pending.swap(m_parked.pending);
    std::swap(m_center, m_parked.center);
    std::swap(m_radius, m_parked.radius);
    m_testMap = !m_testMap;
    if (m_chunksRoot == NoActor)
    {
        Diagnostics::Get().Event("test_map_layout rows=2 columns=4 spacing=96 island_width=40");
        m_chunksRoot = m_scene.Create<Actor>(m_root, "테스트 맵 청크").Id();
        for (unsigned i = 0; i < IslandTestMap::Count; ++i)
        {
            auto layout = std::make_shared<IslandLayout>(IslandTestMap::Describe(i));
            const auto key = layout->Info().key;
            // Every island and its pier must fit the assigned 3x3 chunk tile.
            const float half = IslandTestMap::Spacing * .5f;
            for (int cell = 0; cell < IslandLayout::Size * IslandLayout::Size; ++cell)
            {
                const auto p = layout->Center(cell);
                if (layout->Land(p.x, p.z) && (std::abs(p.x) + layout->CellSize() * .5f >= half ||
                                               std::abs(p.z) + layout->CellSize() * .5f >= half))
                {
                    throw std::logic_error("Test island extends beyond its tile");
                }
            }
            const auto start = layout->PierStart(), direction = layout->PierDirection();
            if (std::abs(start.x + direction.x * layout->PierLength()) + .75f >= half ||
                std::abs(start.z + direction.z * layout->PierLength()) + .75f >= half)
            {
                throw std::logic_error("Test pier extends beyond its tile");
            }
            m_islands.emplace(key, ResidentIsland{layout, false, false});
            Diagnostics::Get().Event(
                "test_island_created index=" + std::to_string(i) + " row=" + std::to_string(key.z) +
                " column=" + std::to_string(key.x) + " width=" + std::to_string(layout->Info().width));
        }
    }
    auto* root = m_scene.Find(m_chunksRoot);
    root->visible = root->enabled = true;
    Diagnostics::Get().Event(
        std::string("world_map_switch mode=") + (m_testMap ? "test" : "adventure") +
        " chunks=" + std::to_string(m_chunks.size()) + " islands=" + std::to_string(m_islands.size()));
}

WorldPosition World::TestSpawn(unsigned index) const
{
    if (!m_testMap)
    {
        throw std::logic_error("Test map is not active");
    }
    const auto descriptor = IslandTestMap::Describe(index);
    const auto& layout = *m_islands.at(descriptor.key).layout;
    auto p = descriptor.center;
    const auto local = layout.Spawn();
    p.Move(local.x, local.z);
    return p;
}

WorldPosition World::TestShip(unsigned index, float length) const
{
    if (!m_testMap)
    {
        throw std::logic_error("Test map is not active");
    }
    const auto descriptor = IslandTestMap::Describe(index);
    return ShipyardMooring(descriptor.key, length);
}

void World::PrepareWindow(const WorldPosition& player, int radius)
{
    Update(player, radius);
    // Make transition/teleport collision queries valid immediately; remaining scenery streams normally.
    for (int z = -1; z <= 1; ++z)
    {
        for (int x = -1; x <= 1; ++x)
        {
            const Key key = {player.chunkX + x, player.chunkZ + z};
            if (m_testMap &&
                (key.first < -2 || key.first > IslandTestMap::Columns * IslandTestMap::CellChunks - 1 ||
                 key.second < -2 || key.second > IslandTestMap::Rows * IslandTestMap::CellChunks - 1))
            {
                continue;
            }
            if (m_chunks.find(key) == m_chunks.end())
            {
                Generate(key);
            }
        }
    }
    RebuildQueue(player);
    Update(player, radius);
}
