#include "stdafx.h"
#include "GameUi.h"
#include <algorithm>
#include <cmath>

namespace
{
    const Color Paper = {.97f, .95f, .88f}, Ink = {.21f, .23f, .25f}, Border = {.45f, .46f, .45f},
                Gold = {.82f, .64f, .25f};

    void Rounded(Renderer& r, float x, float y, float w, float h, Color color)
    {
        // Four cached unit corners; all pixels remain in the existing UI batch.
        static const std::array<Mesh, 4> corners = []
        {
            std::array<Mesh, 4> result;
            for (unsigned q = 0; q < 4; ++q)
            {
                const float cx = q % 2 ? 0.f : 1.f, cy = q / 2 ? 0.f : 1.f;
                const float start =
                    q == 0 ? 3.14159265f : (q == 1 ? 4.71238898f : (q == 2 ? 1.57079633f : 0.f));
                for (unsigned i = 0; i < 8; ++i)
                {
                    const float a = start + i * 1.57079633f / 8, b = start + (i + 1) * 1.57079633f / 8;
                    Geometry::Triangle(
                        result[q],
                        {cx, cy, 0},
                        {cx + std::cos(a), cy + std::sin(a), 0},
                        {cx + std::cos(b), cy + std::sin(b), 0},
                        {1, 1, 1});
                }
            }
            return result;
        }();
        const float radius = 6;
        r.Panel(x + radius, y, w - 2 * radius, h, color);
        r.Panel(x, y + radius, radius, h - 2 * radius, color);
        r.Panel(x + w - radius, y + radius, radius, h - 2 * radius, color);
        for (unsigned q = 0; q < 4; ++q)
        {
            r.Icon(
                corners[q],
                x + (q % 2 ? w - radius : 0),
                y + (q / 2 ? h - radius : 0),
                radius,
                radius,
                color);
        }
    }

    struct Rect
    {
        float x, y, w, h;

        bool Contains(int X, int Y) const
        {
            return X >= x && Y >= y && X < x + w && Y < y + h;
        }

        void Draw(Renderer& r, bool selected = false) const
        {
            Rounded(r, x, y, w, h, selected ? Gold : Border);
            Rounded(r, x + 3, y + 3, w - 6, h - 6, Paper);
        }
    };

    struct Layout
    {
        float x, y, w, h, cellW, cellH, gap = 12;

        Layout(int W, int H)
        {
            w = (std::min)(1180.f, W - 32.f);
            h = (std::min)(700.f, H - 32.f);
            x = (W - w) * .5f;
            y = (H - h) * .5f;
            cellW = (w * .59f - 60) / 4;
            cellH = (h - 276) / 4;
        }

        Rect Cell(unsigned n) const
        {
            return {x + 24 + (n % 4) * (cellW + gap), y + 76 + (n / 4) * (cellH + gap), cellW, cellH};
        }

        Rect Gear(unsigned slot) const
        {
            const float X = x + w * .66f, Y = y + 90, width = w * .115f, height = (h - 330) / 3;
            const float dx = width + 14, dy = height + 14;
            if (slot == 1)
            {
                return {X + dx * .5f, Y, width, height};
            }
            if (slot == 0)
            {
                return {X, Y + dy, width, height};
            }
            if (slot == 2)
            {
                return {X + dx, Y + dy, width, height};
            }
            return {X + dx * .5f, Y + dy * 2, width, height};
        }

        Rect EquipButton() const
        {
            return {x + 24, y + h - 58, w * .56f, 34};
        }

        Rect Dismiss(unsigned n) const
        {
            return {x + w * .65f, y + h - 112 + n * 28, w * .3f, 24};
        }
    };
}

std::vector<InventoryItem> GameUi::BagItems() const
{
    std::vector<InventoryItem> result;
    for (unsigned i = 0; i < ItemCatalog::Count; ++i)
    {
        const auto item = static_cast<InventoryItem>(i);
        if (m_economy.ItemCount(item) > 0)
        {
            result.push_back(item);
        }
    }
    return result;
}

void GameUi::ToggleInventory()
{
    if (m_inventoryOpen)
    {
        CloseServiceMenu();
        return;
    }
    while (CloseServiceMenu())
    {
    }
    m_inventoryOpen = true;
    m_inventorySelection = 0;
    m_serviceMessage.clear();
    m_player.StopFishing();
    ClearInput();
}

void GameUi::InventoryKey(unsigned char key)
{
    if (key == 27)
    {
        CloseServiceMenu();
        return;
    }
    const auto items = BagItems();
    if (items.empty())
    {
        m_inventorySelection = 0;
        return;
    }
    const unsigned last = static_cast<unsigned>(items.size() - 1);
    m_inventorySelection = (std::min)(last, m_inventorySelection);
    if (key == 'a' && m_inventorySelection > 0)
    {
        --m_inventorySelection;
    }
    if (key == 'd' && m_inventorySelection < last)
    {
        ++m_inventorySelection;
    }
    if (key == 'w')
    {
        m_inventorySelection = m_inventorySelection >= 4 ? m_inventorySelection - 4 : 0;
    }
    if (key == 's')
    {
        m_inventorySelection = (std::min)(last, m_inventorySelection + 4);
    }
    if (key == '[')
    {
        m_inventorySelection = m_inventorySelection >= 16 ? m_inventorySelection - 16 : 0;
    }
    if (key == ']')
    {
        m_inventorySelection = (std::min)(last, m_inventorySelection + 16);
    }
    if (key == 13 || key == 'e')
    {
        m_serviceMessage = m_economy.Equip(items[m_inventorySelection]).message;
        m_combat.EquipmentChanged();
        const auto remaining = BagItems();
        m_inventorySelection =
            remaining.empty() ? 0 : (std::min)(m_inventorySelection, unsigned(remaining.size() - 1));
    }
}

bool GameUi::PanelClick(int x, int y)
{
    if (!m_inventoryOpen)
    {
        return PlayerPanelOpen();
    }
    if (m_viewWidth < 760 || m_viewHeight < 540)
    {
        return true;
    }
    const Layout layout(m_viewWidth, m_viewHeight);
    const auto items = BagItems();
    const unsigned selection =
        items.empty() ? 0 : (std::min)(m_inventorySelection, unsigned(items.size() - 1));
    const unsigned page = selection / 16;
    for (unsigned i = 0; i < 16; ++i)
    {
        if (layout.Cell(i).Contains(x, y) && page * 16 + i < items.size())
        {
            m_inventorySelection = page * 16 + i;
            return true;
        }
    }
    for (unsigned slot = 0; slot < 4; ++slot)
    {
        if (layout.Gear(slot).Contains(x, y))
        {
            m_serviceMessage = m_economy.Unequip(static_cast<EquipSlot>(slot)).message;
            m_combat.EquipmentChanged();
            return true;
        }
    }
    if (layout.EquipButton().Contains(x, y))
    {
        InventoryKey(13);
        return true;
    }
    for (unsigned i = 0; i < m_relations.Companions().size(); ++i)
    {
        if (layout.Dismiss(i).Contains(x, y))
        {
            const auto key = m_relations.Companions()[i];
            m_serviceMessage = m_relations.Dismiss(key).message;
            return true;
        }
    }
    return true;
}

void GameUi::RenderInventory(Renderer& r) const
{
    if (!m_inventoryOpen)
    {
        return;
    }
    if (r.Width() < 760 || r.Height() < 540)
    {
        r.Panel(8, 8, r.Width() - 16.f, 90, Paper);
        r.Text(20, 20, "인벤토리는 760×540 이상에서 표시됩니다. I / Esc: 닫기", Ink, 1.4f, r.Width() - 40.f);
        return;
    }
    const Layout layout(r.Width(), r.Height());
    const float x = layout.x, y = layout.y, w = layout.w, h = layout.h;
    Rect{x, y, w, h}.Draw(r);
    r.Text(x + 24, y + 16, "인벤토리", Ink, 2.3f);
    r.Text(x + w * .65f, y + 20, "골드 " + std::to_string(m_economy.Gold()), Ink, 1.6f);
    const auto items = BagItems();
    const unsigned selection =
        items.empty() ? 0 : (std::min)(m_inventorySelection, unsigned(items.size() - 1));
    const unsigned page = selection / 16, pages = (std::max)(1u, unsigned((items.size() + 15) / 16));
    r.Text(
        x + 24,
        y + 48,
        "WASD / 클릭: 선택 · [ ]: 페이지 " + std::to_string(page + 1) + "/" + std::to_string(pages),
        Ink,
        1.15f);
    for (unsigned i = 0; i < 16; ++i)
    {
        const auto cell = layout.Cell(i);
        const unsigned index = page * 16 + i;
        cell.Draw(r, index == selection && !items.empty());
        if (index >= items.size())
        {
            continue;
        }
        const auto item = items[index];
        const auto& def = ItemCatalog::Get(item);
        const float size = (std::min)(cell.w - 22, cell.h - 42);
        r.Icon(m_icons.Get(def.icon), cell.x + (cell.w - size) * .5f, cell.y + 7, size, size);
        r.Text(cell.x + 7, cell.y + cell.h - 32, def.name, Ink, 1.05f, cell.w - 14, 12);
        r.Text(
            cell.x + 7,
            cell.y + cell.h - 16,
            "×" + std::to_string(m_economy.ItemCount(item)),
            Ink,
            1.05f,
            cell.w - 14,
            12);
    }
    Rect{x + w * .63f, y + 76, w * .33f, h - 275}.Draw(r);
    r.Text(x + w * .65f, y + 56, "장비 · 슬롯 클릭으로 해제", Ink, 1.15f);
    for (unsigned slot = 0; slot < 4; ++slot)
    {
        const auto cell = layout.Gear(slot);
        cell.Draw(r);
        const auto equipped = m_economy.Equipped(static_cast<EquipSlot>(slot));
        r.Text(cell.x + 6, cell.y + 5, ItemCatalog::SlotName(static_cast<EquipSlot>(slot)), Ink, 1.05f);
        if (equipped >= 0)
        {
            const auto& def = ItemCatalog::Get(static_cast<InventoryItem>(equipped));
            const float size = (std::min)(cell.w - 20, cell.h - 38);
            r.Icon(m_icons.Get(def.icon), cell.x + (cell.w - size) * .5f, cell.y + 20, size, size);
            r.Text(cell.x + 6, cell.y + cell.h - 18, def.name, Ink, 1.0f, cell.w - 12, 12);
        }
    }
    if (!items.empty())
    {
        const auto& def = ItemCatalog::Get(items[selection]);
        std::string info = def.name + " ×" + std::to_string(m_economy.ItemCount(items[selection])) +
                           " · 기준가 " + std::to_string(def.price) + "골드 · 적재 중첩 " +
                           std::to_string(def.stack);
        if (def.equipment != EquipSlot::None)
        {
            info += " · 공격 +" + std::to_string(def.attack) + " / 방어 +" + std::to_string(def.defence) +
                    " / 체력 +" + std::to_string(def.health);
        }
        r.Text(x + 24, y + h - 118, info, Ink, 1.1f, w * .58f, 36);
    }
    const auto button = layout.EquipButton();
    r.Panel(button.x, button.y, button.w, button.h, Border);
    r.Text(button.x + 10, button.y + 8, "E / Enter / 클릭: 선택 장비 착용", Paper, 1.25f);
    r.Text(
        x + w * .65f,
        y + h - 176,
        "공격 " + std::to_string(m_combat.Damage()) + " / 방어 " +
            std::to_string(m_economy.EquipmentDefence()) + " / 최대 체력 " +
            std::to_string(m_combat.MaxHealth()),
        Ink,
        1.2f,
        w * .3f);
    r.Text(
        x + w * .65f,
        y + h - 140,
        "동료 " + std::to_string(m_relations.Companions().size()) + "/" +
            std::to_string(EconomyRules::CompanionCapacity),
        Ink,
        1.2f);
    for (unsigned i = 0; i < m_relations.Companions().size(); ++i)
    {
        const auto button = layout.Dismiss(i);
        r.Panel(button.x, button.y, button.w, button.h, Border);
        r.Text(
            button.x + 6,
            button.y + 5,
            m_npcs->Name(m_relations.Companions()[i]) + " · 해제",
            Paper,
            1.05f,
            button.w - 12);
    }
    r.Text(x + 24, y + h - 87, m_serviceMessage, Ink, 1.1f, w * .58f, 24);
    r.Text(x + 24, y + h - 21, "I / Esc: 닫기 · 가방은 16칸씩 페이지 표시 · 적재품은 K", Ink, 1.05f, w - 48);
}
