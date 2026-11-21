#pragma once
#include "PlayerController.h"
#include "NpcSystem.h"
#include "NpcState.h"
#include "InventoryIcons.h"
#include "UiLayout.h"
#include <functional>

class MonsterSystem;

class GameUi
{
public:
    GameUi(
        World& world,
        PlayerEconomy& economy,
        NpcState& relations,
        PlayerCombat& combat,
        PlayerController& player);

    void BindSystems(MarineSystem& marine, NpcSystem& npcs, MonsterSystem& monsters)
    {
        m_marine = &marine;
        m_npcs = &npcs;
        m_monsters = &monsters;
    }

    void SetView(int width, int height, bool overview, unsigned island, double average);
    bool Key(unsigned char key);
    bool Interact();
    bool CloseServiceMenu();
    void Escape();
    void OpenTestPanel();
    void CloseAll();

    bool Paused() const
    {
        return m_page == Page::Pause || m_page == Page::Help;
    }

    bool ExitRequested() const
    {
        return m_exitRequested;
    }

    bool PlayerPanelOpen() const
    {
        return m_inventoryOpen || m_talk.valid || m_cargoOpen || m_service.kind != TownServiceKind::None ||
               m_page != Page::None;
    }

    bool PanelClick(int x, int y, const std::function<void(UiAction, unsigned)>& worldAction);
    void RenderHud(Renderer& renderer);

    const PostProcessingSettings& PostSettings() const
    {
        return m_postSettings;
    }

private:
    enum class Page
    {
        None,
        Journal,
        Chart,
        Pause,
        Help,
        Test
    };

    const WorldActor& Player() const
    {
        return m_player.Player();
    }

    WorldActor& ShipActor()
    {
        return m_player.ShipActor();
    }

    const WorldActor& ShipActor() const
    {
        return m_player.ShipActor();
    }

    bool Aboard() const
    {
        return m_player.Aboard();
    }

    WorldPosition ActivePosition() const
    {
        return m_player.ActivePosition();
    }

    void ClearInput()
    {
        m_player.ClearInput();
    }

    void RenderServiceMenu(UiCanvas& ui) const;
    void ServiceKey(unsigned char key);
    bool ShipAtService() const;
    void BuySailboat();
    void ToggleCargo();
    void CargoKey(unsigned char key);
    void ToggleInventory();
    void InventoryKey(unsigned char key);
    std::vector<InventoryItem> BagItems() const;
    std::vector<InventoryItem> CargoItems() const;
    void RenderInventory(UiCanvas& ui) const;
    void NpcKeyInput(unsigned char key);
    void RenderNpc(UiCanvas& ui) const;
    void TradeKey(unsigned char key);
    void RenderTrade(UiCanvas& ui) const;
    std::vector<InventoryItem> MarketItems() const;
    IslandShape MarketShape() const;
    void RenderCargo(UiCanvas& ui) const;
    void RenderPlayerStatus(UiCanvas& ui, const HudLayout& layout) const;
    void RenderTestPanel(UiCanvas& ui) const;
    void RenderJournal(UiCanvas& ui) const;
    void RenderChart(UiCanvas& ui) const;
    void RenderPause(UiCanvas& ui) const;
    void BuildUi(UiCanvas& ui) const;
    void PageKey(unsigned char key);
    bool VisualKey(unsigned char key);
    World& m_world;
    PlayerEconomy& m_economy;
    NpcState& m_relations;
    PlayerCombat& m_combat;
    PlayerController& m_player;
    MarineSystem* m_marine = nullptr;
    NpcSystem* m_npcs = nullptr;
    MonsterSystem* m_monsters = nullptr;
    InventoryIcons m_icons;
    TownInteraction m_service;
    NpcSelection m_talk;
    std::string m_serviceMessage;
    bool m_inventoryOpen = false, m_cargoOpen = false, m_buying = true, m_hud = true, m_postHud = false;
    Page m_page = Page::None;
    bool m_exitRequested = false;
    std::array<IslandDescriptor, 2> m_lastIsland{};
    std::array<bool, 2> m_haveLastIsland{};
    unsigned m_questSelection = 0, m_chartSelection = 0;
    unsigned m_inventorySelection = 0, m_cargoSelection = 0, m_marketSelection = 0;
    int m_viewWidth = 1280, m_viewHeight = 800;
    bool m_testOverview = false;
    unsigned m_testIsland = 0;
    double m_frameAverage = 1.0 / 60;
    PostProcessingSettings m_postSettings;
};
