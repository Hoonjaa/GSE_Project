#pragma once
#include "GeometryTypes.h"
#include <cstdint>

// Existing engine unit/partition. Not a newly fixed game-design chunk size.
constexpr double ChunkSize = 32.0;

struct WorldPosition
{
    std::int64_t chunkX = 0, chunkZ = 0;
    double x = 16, z = 16;
    void Move(double dx, double dz);
    Vec3 RelativeTo(const WorldPosition& origin) const;
};
