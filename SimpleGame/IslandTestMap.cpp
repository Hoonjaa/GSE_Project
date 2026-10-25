#include "stdafx.h"
#include "IslandTestMap.h"
#include <stdexcept>

IslandDescriptor IslandTestMap::Describe(unsigned index)
{
    if (index >= Count)
    {
        throw std::out_of_range("Test island index");
    }
    IslandDescriptor descriptor;
    descriptor.key = {index % Columns, index / Columns};
    descriptor.center.chunkX = descriptor.key.x * CellChunks;
    descriptor.center.chunkZ = descriptor.key.z * CellChunks;
    descriptor.center.x = descriptor.center.z = 16;
    descriptor.shape = static_cast<IslandShape>(index);
    descriptor.width = IslandWidth;
    descriptor.seed = IslandGeneration::Mix(IslandGeneration::WorldSeed ^ (0x544553540000ULL + index));
    descriptor.starter = false;
    return descriptor;
}

bool IslandTestMap::KeyForChunk(std::int64_t x, std::int64_t z, IslandKey& key)
{
    // Tile edges align to chunk edges: [-1,1], [2,4], [5,7], [8,10].
    if (x < -1 || x >= Columns * CellChunks - 1 || z < -1 || z >= Rows * CellChunks - 1)
    {
        return false;
    }
    key = {(x + 1) / CellChunks, (z + 1) / CellChunks};
    return true;
}

WorldPosition IslandTestMap::Center()
{
    auto center = Describe(0).center;
    center.Move((Columns - 1) * Spacing * .5f, (Rows - 1) * Spacing * .5f);
    return center;
}
