#pragma once
#include "Renderer.h"
#include <cstdint>
#include <vector>

// One persistent tutorial island spanning multiple 32-unit streaming chunks.
class LevelOneTerrain
{
public:
    enum
    {
        Size = 104,
        Min = -36
    };

    LevelOneTerrain();
    bool Land(float x, float z) const;
    bool Walk(float x, float z) const;
    bool CellWalk(int x, int z) const;
    bool Segment(float x, float z, float X, float Z) const;
    int Index(float x, float z) const;
    Vec3 Center(int index) const;
    void Flow(float x, float z, std::vector<int>& distance) const;
    void Render(Renderer& renderer, Vec3 offset) const;
    void Map(Renderer& renderer, float left, float top, float extent) const;

    std::uint32_t Seed() const
    {
        return m_seed;
    }

    const std::vector<int>& SpawnCells() const
    {
        return m_spawnCells;
    }

private:
    bool PointWalk(float x, float z) const;
    std::uint32_t m_seed = 0;
    std::vector<unsigned char> m_land, m_blocked;
    std::vector<int> m_spawnCells;
    CachedMesh m_mesh;
};
