#include "stdafx.h"
#include "LevelOneTerrain.h"

LevelOneTerrain::LevelOneTerrain(std::shared_ptr<const IslandLayout> layout) : m_layout(std::move(layout))
{
    std::vector<int> reachable;
    const auto spawn = m_layout->Spawn();
    m_layout->Flow(spawn.x, spawn.z, reachable);
    for (int i : m_layout->SpawnCells())
    {
        if (reachable[i] >= 0)
        {
            m_spawnCells.push_back(i);
        }
    }
}

bool LevelOneTerrain::Land(float x, float z) const
{
    return m_layout->Land(x - 16, z - 16);
}

bool LevelOneTerrain::Walk(float x, float z) const
{
    return m_layout->Walk(x - 16, z - 16);
}

bool LevelOneTerrain::CellWalk(int x, int z) const
{
    return m_layout->CellWalk(x, z);
}

bool LevelOneTerrain::Segment(float x, float z, float X, float Z) const
{
    return m_layout->Segment(x - 16, z - 16, X - 16, Z - 16);
}

int LevelOneTerrain::Index(float x, float z) const
{
    return m_layout->Index(x - 16, z - 16);
}

Vec3 LevelOneTerrain::Center(int index) const
{
    auto p = m_layout->Center(index);
    p.x += 16;
    p.z += 16;
    return p;
}

void LevelOneTerrain::Flow(float x, float z, std::vector<int>& distance) const
{
    m_layout->Flow(x - 16, z - 16, distance);
}

bool LevelOneTerrain::IsVillage(float x, float z) const
{
    return m_layout->Village(x - 16, z - 16);
}

Vec3 LevelOneTerrain::SpawnPoint() const
{
    auto p = m_layout->Spawn();
    p.x += 16;
    p.z += 16;
    return p;
}

Vec3 LevelOneTerrain::VillageCenter() const
{
    auto p = m_layout->VillageCenter();
    p.x += 16;
    p.z += 16;
    return p;
}

void LevelOneTerrain::Map(Renderer& renderer, float left, float top, float extent) const
{
    const float size = extent / Size;
    for (int i = 0; i < Size * Size; ++i)
    {
        const auto p = m_layout->Center(i);
        if (m_layout->Land(p.x, p.z))
        {
            renderer.Panel(
                left + (i % Size) * size,
                top + (i / Size) * size,
                size,
                size,
                m_layout->Village(p.x, p.z) ? Color{.85f, .73f, .43f} : Color{.61f, .72f, .40f});
        }
    }
}
