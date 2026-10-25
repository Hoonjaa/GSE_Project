#pragma once
#include "PlayerEconomy.h"

class PrototypeTradePricing : public TradePricing
{
public:
    TradeQuote Buy(IslandShape market, TradeGood item, int affinity) const override;
    TradeQuote Sell(IslandShape market, TradeGood item, int affinity) const override;
};

namespace TradeRules
{
    std::int64_t Buy(IslandShape market, InventoryItem item, int affinity);
    std::int64_t Sell(IslandShape market, InventoryItem item, int affinity);
}
