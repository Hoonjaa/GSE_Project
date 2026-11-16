#include "stdafx.h"
#include "TradeRules.h"
#include <algorithm>

namespace
{
    bool Trade(InventoryItem item)
    {
        const auto id = static_cast<unsigned>(item);
        return id >= ItemCatalog::TradeBegin && id < ItemCatalog::MaterialBegin;
    }

    int Affinity(int value)
    {
        return (std::max)(-100, (std::min)(100, value));
    }
}

std::int64_t TradeRules::Buy(IslandShape market, InventoryItem item, int affinity)
{
    const auto base = ItemCatalog::Get(item).price;
    if (base <= 0)
    {
        return 0;
    }
    const bool local =
        Trade(item) && MapCatalog::IsSpecialty(
                           market,
                           static_cast<TradeGood>(static_cast<unsigned>(item) - ItemCatalog::TradeBegin));
    // Exact integer fractions avoid a one-gold rounding error at decimal boundaries.
    const std::int64_t rate = Trade(item) ? (local ? 75 : 180) : 120;
    const auto numerator = base * rate * (1000 - Affinity(affinity) * 2);
    return (std::max)(std::int64_t(1), (numerator + 99999) / 100000);
}

std::int64_t TradeRules::Sell(IslandShape market, InventoryItem item, int affinity)
{
    const auto base = ItemCatalog::Get(item).price;
    if (base <= 0)
    {
        return 0;
    }
    const bool local =
        Trade(item) && MapCatalog::IsSpecialty(
                           market,
                           static_cast<TradeGood>(static_cast<unsigned>(item) - ItemCatalog::TradeBegin));
    const std::int64_t rate = Trade(item) ? (local ? 50 : 110) : 100;
    const auto numerator = base * rate * (1000 + Affinity(affinity) * 2);
    return (std::max)(std::int64_t(1), numerator / 100000);
}
