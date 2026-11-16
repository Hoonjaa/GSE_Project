#pragma once
#include "ItemCatalog.h"
#include "MapCatalog.h"

namespace TradeRules
{
    std::int64_t Buy(IslandShape market, InventoryItem item, int affinity);
    std::int64_t Sell(IslandShape market, InventoryItem item, int affinity);
}
