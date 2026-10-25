#pragma once
#include "IslandLayout.h"

namespace IslandTestMap
{
    constexpr int Rows = 2, Columns = 4, Count = Rows * Columns;
    constexpr int CellChunks = 3;
    constexpr float Spacing = CellChunks * float(ChunkSize), IslandWidth = 40;
    IslandDescriptor Describe(unsigned index);
    bool KeyForChunk(std::int64_t x, std::int64_t z, IslandKey& key);
    WorldPosition Center();
}
