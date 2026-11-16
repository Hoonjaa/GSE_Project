#include "GameState.h"
#include "QuestRules.h"
#include "TradeRules.h"
#include "MonsterCatalog.h"
#include "WorldPosition.h"
#include "UiLayout.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void InventoryTransactions()
    {
        GameState state;
        const auto initial = state.economy.Gold();
        Check(
            !state.economy.BuyItem(InventoryItem::Fish, initial + 1).success,
            "Insufficient funds accepted");
        Check(
            state.economy.Gold() == initial && state.economy.ItemCount(InventoryItem::Fish) == 0,
            "Failed purchase mutated state");
        Check(!state.economy.SellItem(InventoryItem::Fish, 2).success, "Missing item sold");
        Check(state.economy.BuyItem(InventoryItem::Fish, 2).success, "Purchase failed");
        Check(state.economy.SellItem(InventoryItem::Fish, 2).success, "Sale failed");
        Check(
            state.economy.Gold() == initial && state.economy.ItemCount(InventoryItem::Fish) == 0,
            "Transaction did not conserve state");
        state.economy.CollectItem(InventoryItem::SteelSword);
        Check(state.economy.Equip(InventoryItem::SteelSword).success, "Equip failed");
        Check(state.economy.ItemCount(InventoryItem::TrainingSword) == 1, "Previous equipment was lost");
        Check(
            state.economy.ItemCount(InventoryItem::SteelSword) == 0 && state.economy.EquipmentAttack() == 10,
            "Equipment remained in bag");
        Check(state.economy.Unequip(EquipSlot::Weapon).success, "Unequip failed");
        Check(
            state.economy.ItemCount(InventoryItem::SteelSword) == 1 && state.economy.EquipmentAttack() == 0,
            "Unequip did not restore item");
    }

    void QuestsAndCompanions()
    {
        GameState state;
        const NpcKey hunter = {{0, 0}, NpcRole::Hunter};
        const NpcKey collector = {{0, 0}, NpcRole::Collector};
        const NpcKey merchant = {{0, 0}, NpcRole::Merchant};
        const auto shape = IslandShape::Round;
        state.npcs.AcceptQuest(hunter);
        state.npcs.RecordMonsterKill(hunter.island, true);
        Check(state.npcs.Progress(hunter, shape, state.economy) == 0, "Epic counted for ordinary hunt");
        for (int i = 0; i < QuestRules::Get(hunter.role).target; ++i)
        {
            state.npcs.RecordMonsterKill(hunter.island, false);
        }
        Check(state.npcs.CompleteQuest(hunter, shape, state.economy).success, "Hunt completion failed");
        const int affinity = state.npcs.Npc(hunter).affinity;
        Check(!state.npcs.CompleteQuest(hunter, shape, state.economy).success, "Quest completed twice");
        Check(state.npcs.Npc(hunter).affinity == affinity, "Duplicate completion changed reward");
        state.npcs.AcceptQuest(collector);
        const auto material = ItemCatalog::MonsterMaterial(MonsterCatalog::ForIsland(shape, 0));
        const int target = QuestRules::Get(collector.role).target;
        state.economy.CollectItems(material, target - 1);
        Check(
            !state.npcs.CompleteQuest(collector, shape, state.economy).success,
            "Insufficient delivery accepted");
        Check(state.economy.ItemCount(material) == target - 1, "Failed delivery consumed items");
        state.economy.CollectItem(material);
        Check(state.npcs.CompleteQuest(collector, shape, state.economy).success, "Delivery failed");
        Check(state.economy.ItemCount(material) == 0, "Delivery did not consume items");
        Check(
            state.npcs.Progress(collector, shape, state.economy) == target,
            "Completed delivery progress changed");
        state.npcs.AcceptQuest(merchant);
        for (int i = 0; i < QuestRules::Get(merchant.role).target; ++i)
        {
            state.npcs.RecordTradePurchase(merchant.island);
        }
        Check(state.npcs.CompleteQuest(merchant, shape, state.economy).success, "Trade quest failed");
        Check(!state.npcs.Recruit(hunter, state.economy).success, "Recruitment without ship accepted");
        Check(state.economy.BuySailboat().success, "Initial boat purchase failed");
        Check(state.npcs.Recruit(hunter, state.economy).success, "First companion failed");
        Check(state.npcs.Recruit(collector, state.economy).success, "Second companion failed");
        Check(!state.npcs.Recruit(merchant, state.economy).success, "Companion capacity exceeded");
        GameState snapshot = state;
        state.npcs.Dismiss(hunter);
        state.economy.CollectItem(InventoryItem::Fish);
        Check(
            snapshot.npcs.Companions().size() == EconomyRules::CompanionCapacity,
            "Snapshot relation state shared");
        Check(snapshot.economy.ItemCount(InventoryItem::Fish) == 0, "Snapshot inventory state shared");
        Check(state.npcs.Recruit(merchant, state.economy).success, "Freed companion slot unusable");
    }

    void PricesAndDrops()
    {
        for (unsigned id = 0; id < ItemCatalog::Count; ++id)
        {
            const auto& item = ItemCatalog::Get(static_cast<InventoryItem>(id));
            Check(!item.name.empty() && item.stack > 0 && item.price >= 0, "Incomplete item definition");
        }
        for (const auto& market : MapCatalog::Islands())
        {
            for (const auto good : market.specialties)
            {
                const auto item = ItemCatalog::Trade(static_cast<unsigned>(good));
                Check(ItemCatalog::Get(item).price > 0, "Specialty price missing");
                for (int affinity = -100; affinity <= 100; ++affinity)
                {
                    Check(
                        TradeRules::Buy(market.shape, item, affinity) >=
                            TradeRules::Sell(market.shape, item, affinity),
                        "Local buy/sell arbitrage");
                }
                Check(
                    TradeRules::Buy(market.shape, item, 100) <= TradeRules::Buy(market.shape, item, 0),
                    "Affinity increased buy price");
                Check(
                    TradeRules::Sell(market.shape, item, 100) >= TradeRules::Sell(market.shape, item, 0),
                    "Affinity reduced sell price");
            }
        }
        for (unsigned id = 0; id < MonsterCatalog::Count; ++id)
        {
            const auto drops = MonsterDrops(id, MonsterCatalog::Get(id).epic);
            Check(drops[0].item == ItemCatalog::MonsterMaterial(id), "Species material mismatch");
            for (const auto& drop : drops)
            {
                Check(drop.chance == 50 && drop.count == 1, "Drop policy changed");
            }
        }
    }

    void CoordinatesAndHud()
    {
        for (const auto chunk : {std::int64_t(-8000), std::int64_t(0), std::int64_t(8000)})
        {
            WorldPosition camera;
            camera.chunkX = chunk;
            camera.chunkZ = -chunk;
            camera.x = 31.5;
            auto effect = camera;
            effect.Move(1.25, -.75);
            const auto position = effect.RelativeTo(camera);
            Check(
                std::abs(position.x - 1.25f) < .0001f && std::abs(position.z + .75f) < .0001f,
                "Nearby effect coordinates lost precision");
        }
        const HudLayout hud(1280, 800);
        Check(hud.economy.Contains(100, 370), "Gold HUD hit region missing");
        Check(!hud.economy.Contains(451, 370), "Gold HUD hit region extends outside panel");
    }
}

int main()
{
    try
    {
        InventoryTransactions();
        QuestsAndCompanions();
        PricesAndDrops();
        CoordinatesAndHud();
        std::cout << "Gameplay regression checks passed.\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
