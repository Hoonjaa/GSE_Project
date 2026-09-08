#pragma once
#include "Renderer.h"
#include <cstdint>
#include <map>
#include <utility>

constexpr double ChunkSize = 32.0;

struct WorldPosition
{
    std::int64_t chunkX = 0, chunkZ = 0;
    double x = 16, z = 16;
    void Move(double dx, double dz);
    Vec3 RelativeTo(const WorldPosition& origin) const;
};

struct IslandChunk
{
    bool hasIsland = false, harbor = false, ruin = false;
    float centerX = 16, centerZ = 16, radius = 8, phase = 0;
    Mesh terrain;
    Mesh scenery;
    float ShoreRadius(float angle) const;
};

class World
{
public:
    using Key = std::pair<std::int64_t, std::int64_t>;
    static constexpr std::uint64_t Seed = 20260908;
    void Update(const WorldPosition& player, int radius);
    void Render(Renderer& renderer, const WorldPosition& camera, float time, bool grid) const;
    bool CanWalk(const WorldPosition& position) const;
    bool CanSail(const WorldPosition& position) const;
    const IslandChunk* At(const WorldPosition& position) const;
    const std::map<Key, IslandChunk>& Chunks() const { return m_chunks; }
private:
    static IslandChunk Generate(std::int64_t x, std::int64_t z);
    bool LandAt(const WorldPosition& position, float margin) const;
    std::map<Key, IslandChunk> m_chunks;
    Key m_center = {0, 0};
    int m_radius = -1;
};
