#pragma once
#include "IslandLayout.h"
#include <set>

namespace MapState
{
    std::set<IslandKey> VisitedIslands();
    bool SaveVisited(IslandKey key);
}
