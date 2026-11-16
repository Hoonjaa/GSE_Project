#pragma once
#include <cstdint>

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
