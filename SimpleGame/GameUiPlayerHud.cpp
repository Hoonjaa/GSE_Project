#include "stdafx.h"
#include "GameUi.h"
#include <cmath>

using namespace UiTheme;

void GameUi::RenderCargo(UiCanvas& ui) const
{
    const auto box = ui.Center(848, 490);
    const float left = box.x + 28, right = left + 450, y = box.y;
    const auto items = CargoItems();
    const unsigned selected = items.empty() ? 0 : (std::min)(m_cargoSelection, unsigned(items.size() - 1));
    const unsigned first = (selected / 5) * 5;
    ui.Header(
        box,
        "선박 적재창",
        "8m 돛단배 · " + std::to_string(m_economy.UsedCargoKinds()) + " / " +
            std::to_string(m_economy.Cargo().size()) + "종 사용");
    ui.Text(left, y + 113, "가방", 1.85f);
    ui.Text(right, y + 113, "선박", 1.85f);
    for (unsigned i = first; i < first + 5 && i < items.size(); ++i)
    {
        const UiRect row{left, y + 149 + (i - first) * 46.f, 430, 42};
        const auto item = items[i];
        ui.Card(row, i == selected);
        ui.Hit(row, UiAction::Cargo, i);
        ui.Text(row.x + 12, row.y + 10, EconomyRules::ItemName(item), 1.7f, Ivory, 286, 22);
        ui.Text(
            row.x + 307,
            row.y + 10,
            std::to_string(m_economy.ItemCount(item)) + "개",
            1.65f,
            Muted,
            112,
            22);
    }
    const auto& cargo = m_economy.Cargo();
    bool ableToUnload = false;
    for (unsigned i = 0; i < cargo.size(); ++i)
    {
        const auto& slot = cargo[i];
        const auto found = slot.occupied ? std::find(items.begin(), items.end(), slot.item) : items.end();
        const bool current = found != items.end() && static_cast<unsigned>(found - items.begin()) == selected;
        const UiRect row{right, y + 149 + i * 46.f, 342, 42};
        ui.Card(row, current);
        if (found != items.end())
        {
            ui.Hit(row, UiAction::Cargo, static_cast<unsigned>(found - items.begin()));
        }
        ui.Text(
            row.x + 12,
            row.y + 10,
            slot.occupied ? EconomyRules::ItemName(slot.item) : "빈 슬롯",
            1.65f,
            slot.occupied ? Ivory : Muted,
            220,
            22);
        if (slot.occupied)
        {
            ui.Text(
                row.x + 237,
                row.y + 10,
                std::to_string(slot.count) + " / " + std::to_string(EconomyRules::ItemStack(slot.item)),
                1.55f,
                Ivory,
                94,
                20);
        }
        ableToUnload = ableToUnload || current;
    }
    const bool ableToStore = !items.empty() && m_economy.ItemCount(items[selected]) > 0;
    ui.Text(
        left,
        y + 383,
        m_serviceMessage.empty() ? "W / S 품목 선택 · 가방과 선박 적재품은 별도 보관" : m_serviceMessage,
        1.45f,
        Muted,
        792,
        23);
    ui.Footer(box);
    ui.Button({left + 536, y + 428, 122, 42}, "A  내리기", UiAction::Key, 'a', false, ableToUnload);
    ui.Button({left + 670, y + 428, 122, 42}, "D  싣기", UiAction::Key, 'd', true, ableToStore);
}

void GameUi::RenderPlayerStatus(UiCanvas& ui, const HudLayout& layout) const
{
    const auto ship = layout.side;
    const float health = m_marine->ShipHealth();
    ui.Frame(ship);
    ui.Text(ship.x + 17, ship.y + 14, "8m 돛단배", 1.85f, Gold);
    ui.Text(
        ship.x + 144,
        ship.y + 15,
        std::to_string(int(std::ceil(health))) + " / 100",
        1.35f,
        Ivory,
        67,
        18);
    ui.Bar({ship.x + 17, ship.y + 47, ship.w - 34, 8}, health / 100, Safe);
    ui.Text(
        ship.x + 17,
        ship.y + 64,
        "선체 내구도 · 적재 " + std::to_string(m_economy.UsedCargoKinds()) + " / " +
            std::to_string(m_economy.Cargo().size()),
        1.45f,
        Muted,
        ship.w - 34,
        20);

    std::string title, value, hint;
    float progress = 0;
    Color accent = Gold;
    const float loot = m_marine->Raid().LootTime();
    if (m_marine->Captured())
    {
        title = "토네이도 탈출";
        value = std::to_string(int(std::ceil(m_marine->EscapeTime()))) + "초";
        hint = std::to_string(m_marine->EscapeCount()) + " / 12회 · A / D를 번갈아 누르세요";
        progress = m_marine->EscapeCount() / 12.f;
        accent = Warning;
    }
    else if (loot >= 0 && m_marine->Encounter() == SeaEncounter::Pirates)
    {
        title = "약탈 후 내 배로 복귀";
        value = std::to_string(int(std::ceil(loot))) + "초";
        hint = "E 상자 약탈 · 내 배 쪽 가장자리에서 E 복귀";
        progress = loot / PirateRaid::LootSeconds;
    }
    else if (m_marine->Encounter() == SeaEncounter::Kraken)
    {
        title = "크라켄 출현";
        const auto remaining = m_marine->TentaclesRemaining();
        value = "촉수 " + std::to_string(remaining) + " / 4";
        hint = "타륜 조종을 풀고 갑판에서 촉수를 공격하세요.";
        progress = remaining / 4.f;
    }
    else if (m_player.Fishing())
    {
        title = "낚시 중";
        value = std::to_string(int(m_player.FishTime())) + " / 5초";
        hint = "F  낚시 종료";
        progress = m_player.FishTime() / 5.f;
    }
    else if (m_marine->Encounter() == SeaEncounter::Pirates)
    {
        title = "해적선 접근";
        hint = "갑판에 올라온 해적을 근접 공격으로 물리치세요.";
    }
    if (title.empty())
    {
        return;
    }
    const auto box = layout.activity;
    ui.Frame(box);
    ui.Text(box.x + 21, box.y + 15, title, 1.95f, accent);
    ui.Text(box.x + 338, box.y + 16, value, 1.75f, Ivory, 96, 24);
    ui.Bar({box.x + 21, box.y + 52, box.w - 42, 8}, progress, accent);
    ui.Text(box.x + 21, box.y + 70, hint, 1.5f, Muted, box.w - 42, 24);
}
