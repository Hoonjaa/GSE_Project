#include "stdafx.h"
#include "GameUi.h"
#include "KoreanText.h"
#include <algorithm>

GameUi::GameUi(
    World& world,
    PlayerEconomy& economy,
    NpcState& relations,
    PlayerCombat& combat,
    PlayerController& player)
    : m_world(world), m_economy(economy), m_relations(relations), m_combat(combat), m_player(player)
{
}

void GameUi::SetView(
    int width,
    int height,
    const WorldPosition& camera,
    bool overview,
    unsigned island,
    double average)
{
    m_viewWidth = width;
    m_viewHeight = height;
    m_camera = camera;
    m_testOverview = overview;
    m_testIsland = island;
    m_frameAverage = average;
}

void GameUi::CloseAll()
{
    while (CloseServiceMenu())
    {
    }
    m_player.StopFishing();
}

bool GameUi::Interact()
{
    if (Aboard())
    {
        return false;
    }
    m_service = m_world.ServiceAt(Player().location);
    if (m_service.kind != TownServiceKind::None)
    {
        ClearInput();
        m_serviceMessage.clear();
        m_marketSelection = 0;
        m_buying = true;
        return true;
    }
    m_talk = m_npcs->Nearby(Player().location);
    if (!m_talk.valid)
    {
        return false;
    }
    m_npcs->Open(m_talk);
    m_serviceMessage.clear();
    ClearInput();
    return true;
}

bool GameUi::Key(unsigned char key)
{
    if (key == 'i')
    {
        ToggleInventory();
        return true;
    }
    if (m_inventoryOpen)
    {
        InventoryKey(key);
        return true;
    }
    if (m_talk.valid)
    {
        NpcKeyInput(key);
        return true;
    }
    if (m_cargoOpen)
    {
        CargoKey(key);
        return true;
    }
    if (m_service.kind != TownServiceKind::None)
    {
        ServiceKey(key);
        return true;
    }
    switch (key)
    {
        case 'k':
            ToggleCargo();
            return true;
        case 'h':
            m_hud = !m_hud;
            return true;
        case 'v':
            m_postHud = !m_postHud;
            return true;
        case 'b':
            m_postSettings.enabled = !m_postSettings.enabled;
            m_player.Notify(m_postSettings.enabled ? KoreanText::PostEnabled : KoreanText::PostDisabled);
            return true;
        case '1':
            m_postSettings.toneMapping = !m_postSettings.toneMapping;
            return true;
        case '2':
            m_postSettings.vignette = !m_postSettings.vignette;
            return true;
        case '3':
            m_postSettings.edgeBlur = !m_postSettings.edgeBlur;
            return true;
        case '[':
            m_postSettings.exposure = (std::max)(.1f, m_postSettings.exposure - .1f);
            return true;
        case ']':
            m_postSettings.exposure = (std::min)(4.f, m_postSettings.exposure + .1f);
            return true;
        case '0':
            m_postSettings = PostProcessingSettings{};
            return true;
    }
    return false;
}

bool GameUi::ConsumesPointer(int x, int y) const
{
    if (PlayerPanelOpen())
    {
        return true;
    }
    if (!m_hud)
    {
        return false;
    }
    const HudLayout layout(m_viewWidth, m_viewHeight);
    if (layout.title.Contains(x, y) || layout.footer.Contains(x, y))
    {
        return true;
    }
    if (m_world.IsTestMap() && layout.testChart.Contains(x, y))
    {
        return true;
    }
    if (!m_world.IsTestMap() && m_viewWidth > 760 && layout.chart.Contains(x, y))
    {
        return true;
    }
    if (!m_testOverview)
    {
        if ((m_postHud ? layout.postStats : layout.stats).Contains(x, y))
        {
            return true;
        }
        if (layout.economy.Contains(x, y))
        {
            return true;
        }
        if (m_world.CurrentIsland(ActivePosition()) && layout.island.Contains(x, y))
        {
            return true;
        }
        if (Aboard() && layout.marine.Contains(x, y))
        {
            return true;
        }
        if (!m_postHud && m_combat.NoticeTime() > 0 && layout.combatNotice.Contains(x, y))
        {
            return true;
        }
    }
    return m_player.NoticeTime() > 0 && layout.message.Contains(x, y);
}
