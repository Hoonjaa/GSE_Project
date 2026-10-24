#pragma once
#include "IslandLayout.h"

// Adapter keeping level combat/navigation on the same grid used by the world.
class LevelOneTerrain
{
public:
    enum
    {
        Size = IslandLayout::Size
    };

    explicit LevelOneTerrain(std::shared_ptr<const IslandLayout> layout);
    bool Land(float x, float z) const;
    bool Walk(float x, float z) const;
    bool CellWalk(int x, int z) const;
    bool Segment(float x, float z, float X, float Z) const;
    int Index(float x, float z) const;
    Vec3 Center(int index) const;
    void Flow(float x, float z, std::vector<int>& distance) const;
    void Map(Renderer& renderer, float left, float top, float extent) const;

    std::uint32_t Seed() const
    {
        return static_cast<std::uint32_t>(m_layout->Info().seed);
    }

    const std::vector<int>& SpawnCells() const
    {
        return m_spawnCells;
    }

    bool IsVillage(float x, float z) const;
    Vec3 SpawnPoint() const;
    Vec3 VillageCenter() const;

    float MapOrigin() const
    {
        return m_layout->Origin() + 16;
    }

    float CellSize() const
    {
        return m_layout->CellSize();
    }

private:
    std::shared_ptr<const IslandLayout> m_layout;
    std::vector<int> m_spawnCells;
};
