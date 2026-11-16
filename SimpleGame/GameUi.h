#pragma once
#include "PlayerController.h"
#include "NpcSystem.h"
#include "NpcState.h"
#include "InventoryIcons.h"
#include "UiLayout.h"

class GameUi
{
public:
    GameUi(
        World& world,
        PlayerEconomy& economy,
        NpcState& relations,
        PlayerCombat& combat,
        PlayerController& player);

    void BindSystems(MarineSystem& marine, NpcSystem& npcs)
    {
        m_marine = &marine;
        m_npcs = &npcs;
    }

    void SetView(
        int width,
        int height,
        const WorldPosition& camera,
        bool overview,
        unsigned island,
        double average);
    bool Key(unsigned char key);
    bool Interact();
    bool CloseServiceMenu();
    void CloseAll();

    bool PlayerPanelOpen() const
    {
        return m_inventoryOpen || m_talk.valid || m_cargoOpen || m_service.kind != TownServiceKind::None;
    }

    bool ConsumesPointer(int x, int y) const;
    bool PanelClick(int x, int y);
    void RenderHud(Renderer& renderer);

    bool ShowCombatHud() const
    {
        return m_hud && !m_postHud && !PlayerPanelOpen() && !m_testOverview;
    }

    const PostProcessingSettings& PostSettings() const
    {
        return m_postSettings;
    }

private:
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

    bool Sailing() const
    {
        return m_player.Sailing();
    }

    WorldPosition ActivePosition() const
    {
        return m_player.ActivePosition();
    }

    void ClearInput()
    {
        m_player.ClearInput();
    }

    void RenderServiceMenu(Renderer& renderer) const;
    void ServiceKey(unsigned char key);
    bool ShipAtService() const;
    void BuySailboat();
    void ToggleCargo();
    void CargoKey(unsigned char key);
    void ToggleInventory();
    void InventoryKey(unsigned char key);
    std::vector<InventoryItem> BagItems() const;
    std::vector<InventoryItem> CargoItems() const;
    void RenderInventory(Renderer& renderer) const;
    void NpcKeyInput(unsigned char key);
    void RenderNpc(Renderer& renderer) const;
    void TradeKey(unsigned char key);
    void RenderTrade(Renderer& renderer) const;
    std::vector<InventoryItem> MarketItems() const;
    IslandShape MarketShape() const;
    void RenderPlayerPanel(Renderer& renderer) const;
    void RenderPlayerStatus(Renderer& renderer) const;
    void RenderTestMapChart(Renderer& renderer) const;
    World& m_world;
    PlayerEconomy& m_economy;
    NpcState& m_relations;
    PlayerCombat& m_combat;
    PlayerController& m_player;
    MarineSystem* m_marine = nullptr;
    NpcSystem* m_npcs = nullptr;
    InventoryIcons m_icons;
    TownInteraction m_service;
    NpcSelection m_talk;
    std::string m_serviceMessage;
    bool m_inventoryOpen = false, m_cargoOpen = false, m_buying = true, m_hud = true, m_postHud = false;
    unsigned m_inventorySelection = 0, m_cargoSelection = 0, m_marketSelection = 0;
    int m_viewWidth = 1280, m_viewHeight = 800;
    WorldPosition m_camera;
    bool m_testOverview = false;
    unsigned m_testIsland = 0;
    double m_frameAverage = 1.0 / 60;
    PostProcessingSettings m_postSettings;
};
