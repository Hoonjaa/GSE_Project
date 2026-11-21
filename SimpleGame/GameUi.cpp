#include "stdafx.h"
#include "GameUi.h"
#include "KoreanText.h"
#include <algorithm>
#include <iterator>

GameUi::GameUi(
    World& world,
    PlayerEconomy& economy,
    NpcState& relations,
    PlayerCombat& combat,
    PlayerController& player)
    : m_world(world), m_economy(economy), m_relations(relations), m_combat(combat), m_player(player)
{
}

void GameUi::SetView(int width, int height, bool overview, unsigned island, double average)
{
    m_viewWidth = width;
    m_viewHeight = height;
    m_testOverview = overview;
    m_testIsland = island;
    m_frameAverage = average;
    if (const auto* layout = m_world.CurrentIsland(ActivePosition()))
    {
        const auto mode = m_world.IsTestMap() ? 1 : 0;
        m_lastIsland[mode] = layout->Info();
        m_haveLastIsland[mode] = true;
    }
}

void GameUi::Escape()
{
    if (!CloseServiceMenu())
    {
        m_page = Page::Pause;
        ClearInput();
    }
}

void GameUi::OpenTestPanel()
{
    if (!m_world.IsTestMap() || Paused())
    {
        return;
    }
    const bool opening = m_page != Page::Test;
    CloseAll();
    m_page = opening ? Page::Test : Page::None;
    ClearInput();
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
    if (m_page != Page::None)
    {
        PageKey(key);
        return true;
    }
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
        case 'j':
        case 'm':
            m_page = key == 'j' ? Page::Journal : Page::Chart;
            if (m_page == Page::Chart)
            {
                const auto records = m_world.ChartIslands();
                const auto mode = m_world.IsTestMap() ? 1 : 0;
                const auto island = records.find(m_lastIsland[mode].key);
                m_chartSelection = island == records.end()
                                       ? 0u
                                       : static_cast<unsigned>(std::distance(records.begin(), island));
            }
            ClearInput();
            return true;
        case 'k':
            ToggleCargo();
            return true;
    }
    return VisualKey(key);
}

bool GameUi::VisualKey(unsigned char key)
{
    switch (key)
    {
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

bool GameUi::PanelClick(int x, int y, const std::function<void(UiAction, unsigned)>& worldAction)
{
    UiCanvas ui(m_viewWidth, m_viewHeight, nullptr, x, y);
    BuildUi(ui);
    const bool consumed = PlayerPanelOpen() || ui.consumed;
    switch (ui.action)
    {
        case UiAction::None:
            break;
        case UiAction::Key:
            if (ui.value == 27)
            {
                Escape();
            }
            else if (!m_marine->Captured())
            {
                if (ui.value == 'e')
                {
                    if (!Interact())
                    {
                        m_player.Interact();
                    }
                }
                else
                {
                    Key(static_cast<unsigned char>(ui.value));
                }
            }
            break;
        case UiAction::Close:
            CloseServiceMenu();
            break;
        case UiAction::Bag:
            m_inventorySelection = ui.value;
            break;
        case UiAction::Gear:
            if (m_economy.Equipped(static_cast<EquipSlot>(ui.value)) >= 0)
            {
                m_serviceMessage = m_economy.Unequip(static_cast<EquipSlot>(ui.value)).message;
                m_combat.EquipmentChanged();
            }
            break;
        case UiAction::Dismiss:
        {
            const auto companion = m_relations.Companions().at(ui.value);
            m_serviceMessage = m_relations.Dismiss(companion).message;
            break;
        }
        case UiAction::Market:
            m_marketSelection = ui.value;
            break;
        case UiAction::Cargo:
            m_cargoSelection = ui.value;
            break;
        case UiAction::Quest:
            m_questSelection = ui.value;
            break;
        case UiAction::Island:
            m_chartSelection = ui.value;
            break;
        case UiAction::TestPanel:
            OpenTestPanel();
            break;
        case UiAction::Help:
            m_page = Page::Help;
            break;
        case UiAction::Quit:
            m_exitRequested = true;
            break;
        default:
            worldAction(ui.action, ui.value);
            break;
    }
    return consumed;
}
