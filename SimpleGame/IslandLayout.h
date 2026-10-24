#pragma once
#include "WorldPosition.h"
#include "MapCatalog.h"
#include <memory>
#include <vector>

struct IslandKey
{
    std::int64_t x = 0, z = 0;

    bool operator<(const IslandKey& other) const
    {
        return x < other.x || (x == other.x && z < other.z);
    }

    bool operator==(const IslandKey& other) const
    {
        return x == other.x && z == other.z;
    }
};

struct IslandDescriptor
{
    IslandKey key;
    WorldPosition center;
    IslandShape shape = IslandShape::Round;
    float width = 80;
    std::uint64_t seed = 0;
    bool starter = false;
};

// Land, village, collisions and visible coast all use this one immutable grid.
class IslandLayout
{
public:
    enum
    {
        Size = 64
    };

    explicit IslandLayout(IslandDescriptor descriptor);

    const IslandDescriptor& Info() const
    {
        return m_info;
    }

    float CellSize() const
    {
        return m_cellSize;
    }

    float LandArea() const
    {
        return m_landCells * m_cellSize * m_cellSize;
    }

    float VillageArea() const
    {
        return m_villageCells * m_cellSize * m_cellSize;
    }

    int Index(float x, float z) const;
    Vec3 Center(int index) const;
    bool CellLand(int x, int z) const;
    bool CellWalk(int x, int z) const;
    bool Land(float x, float z) const;
    bool Walk(float x, float z, float radius = MapCatalog::PlayerRadius) const;
    bool Village(float x, float z) const;
    bool Building(float x, float z) const;
    bool Dock(float x, float z, float margin = 0) const;
    bool Segment(float x, float z, float X, float Z) const;
    void Flow(float x, float z, std::vector<int>& distance) const;
    void AppendTerrain(Mesh& mesh, Vec3 chunkOffset, float chunkSize) const;

    const std::vector<Vec3>& Houses() const
    {
        return m_houses;
    }

    Vec3 VillageCenter() const
    {
        return m_villageCenter;
    }

    Vec3 Spawn() const
    {
        return m_spawn;
    }

    Vec3 PierStart() const
    {
        return m_pierStart;
    }

    Vec3 PierDirection() const
    {
        return m_pierDirection;
    }

    Vec3 ShipSpawn(float length = 8) const;

    Vec3 ShopPoint() const
    {
        return m_shop;
    }

    Vec3 ShipyardPoint() const
    {
        return m_shipyard;
    }

    float PierLength() const
    {
        return 11;
    }

    float Origin() const
    {
        return -Size * m_cellSize * .5f;
    }

    const std::vector<int>& SpawnCells() const
    {
        return m_spawnCells;
    }

private:
    bool PointWalk(float x, float z) const;
    IslandDescriptor m_info;
    std::vector<unsigned char> m_land, m_village, m_blocked;
    std::vector<Vec3> m_houses;
    std::vector<int> m_spawnCells;
    float m_cellSize = 1;
    size_t m_landCells = 0, m_villageCells = 0;
    Vec3 m_villageCenter = {}, m_spawn = {}, m_pierStart = {}, m_pierDirection = {1, 0, 0};
    Vec3 m_shop = {}, m_shipyard = {};
};

namespace IslandGeneration
{
    constexpr std::uint64_t WorldSeed = 20260908;
    // Derived conservative placement bound; islands are not confined to one chunk.
    constexpr int SectorChunks = 16;
    std::uint64_t Mix(std::uint64_t value);
    IslandKey Sector(std::int64_t chunkX, std::int64_t chunkZ);
    IslandDescriptor Describe(IslandKey key);
}
