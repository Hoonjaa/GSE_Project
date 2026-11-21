#include "stdafx.h"
#include "GameUi.h"
#include "MonsterSystem.h"
#include "QuestRules.h"
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace UiTheme;

namespace
{
    const Mesh& Disc()
    {
        static const Mesh mesh = []
        {
            Mesh result;
            for (unsigned i = 0; i < 64; ++i)
            {
                const float a = i * 6.2831853f / 64, b = (i + 1) * 6.2831853f / 64;
                Geometry::Triangle(
                    result,
                    {.5f, .5f, 0},
                    {.5f + .5f * std::cos(a), .5f + .5f * std::sin(a), 0},
                    {.5f + .5f * std::cos(b), .5f + .5f * std::sin(b), 0},
                    {1, 1, 1});
            }
            return result;
        }();
        return mesh;
    }

    void ChartArrow(UiCanvas& ui, float x, float y, float heading)
    {
        if (!ui.draw)
        {
            return;
        }
        static const Mesh arrow = []
        {
            Mesh result;
            Geometry::Triangle(result, {.5f, .05f, 0}, {.15f, .86f, 0}, {.85f, .86f, 0}, {1, 1, 1});
            return result;
        }();
        Mesh rotated = arrow;
        const float c = -std::cos(heading), s = -std::sin(heading);
        for (auto& vertex : rotated)
        {
            const float dx = vertex.position.x - .5f, dy = vertex.position.y - .5f;
            vertex.position.x = .5f + c * dx - s * dy;
            vertex.position.y = .5f + s * dx + c * dy;
        }
        ui.draw->Icon(rotated, (x - 10) * ui.scale, (y - 10) * ui.scale, 20 * ui.scale, 20 * ui.scale, Gold);
    }
}

void GameUi::RenderHud(Renderer& r)
{
    UiCanvas ui(r.Width(), r.Height(), &r);
    BuildUi(ui);
}

void GameUi::BuildUi(UiCanvas& ui) const
{
    if (PlayerPanelOpen())
    {
        if (ui.draw)
        {
            ui.draw->DimOverlay(.46f);
        }
        if (m_inventoryOpen)
        {
            RenderInventory(ui);
        }
        else if (m_talk.valid)
        {
            RenderNpc(ui);
        }
        else if (m_cargoOpen)
        {
            RenderCargo(ui);
        }
        else if (m_service.kind != TownServiceKind::None)
        {
            RenderServiceMenu(ui);
        }
        else
        {
            switch (m_page)
            {
                case Page::Journal:
                    RenderJournal(ui);
                    break;
                case Page::Chart:
                    RenderChart(ui);
                    break;
                case Page::Test:
                    RenderTestPanel(ui);
                    break;
                case Page::Pause:
                case Page::Help:
                    RenderPause(ui);
                    break;
                case Page::None:
                    break;
            }
        }
        return;
    }

    if (!m_hud)
    {
        return;
    }
    const HudLayout layout(ui.w, ui.h);
    const auto position = ActivePosition();
    const auto* island = m_world.CurrentIsland(position);
    const bool warning = !m_testOverview && !Aboard() && m_monsters->AttackWarning(position);
    if (!m_testOverview)
    {
        const auto state = m_combat.Snapshot();
        const auto stats = layout.stats;
        ui.Frame(stats);
        ui.Compass(stats.x + 17, stats.y + 15, 53);
        ui.Text(stats.x + 81, stats.y + 17, "Lv. " + std::to_string(state.level), 1.65f);
        ui.Text(
            stats.x + 243,
            stats.y + 17,
            std::to_string(int(std::ceil(state.health))) + " / " + std::to_string(m_combat.MaxHealth()),
            1.5f,
            Ivory,
            90,
            20);
        ui.Bar({stats.x + 81, stats.y + 45, 242, 8}, state.health / m_combat.MaxHealth(), Warning);
        ui.Bar({stats.x + 81, stats.y + 61, 242, 4}, float(state.xp) / m_combat.ExperienceTarget(), Gold);
        ui.Text(
            stats.x + 81,
            stats.y + 76,
            "공격력 " + std::to_string(m_combat.Damage()) + " · 강화석 " +
                std::to_string(m_economy.ItemCount(InventoryItem::UpgradeStone)),
            1.45f,
            Muted);
        auto economy = layout.economy;
        const std::string gold = std::to_string(m_economy.Gold());
        economy.w = (std::max)(economy.w, 75.f + gold.size() * 9.f);
        ui.Frame(economy, 12);
        ui.Text(economy.x + 14, economy.y + 10, "◉ " + gold + " 골드", 1.7f, Ivory, economy.w - 28, 23);

        const auto chart = layout.chart;
        ui.Frame(chart);
        ui.Text(chart.x + 61, chart.y + 12, "주변 해도", 1.6f, Gold);
        const float left = chart.x + 25, top = chart.y + 42, extent = 136;
        const float cx = left + extent * .5f, cy = top + extent * .5f;
        if (ui.draw)
        {
            ui.draw->Icon(
                Disc(),
                (left - 1) * ui.scale,
                (top - 1) * ui.scale,
                (extent + 2) * ui.scale,
                (extent + 2) * ui.scale,
                Gold);
            ui.draw
                ->Icon(Disc(), left * ui.scale, top * ui.scale, extent * ui.scale, extent * ui.scale, Teal);
            for (float line = -48; line <= 48; line += 24)
            {
                const float span = std::sqrt(66 * 66 - line * line);
                ui.Fill({cx + line, cy - span, 1, 2 * span}, Line);
                ui.Fill({cx - span, cy + line, 2 * span, 1}, Line);
            }
            m_world.DrawMap(*ui.draw, position, left * ui.scale, top * ui.scale, extent * ui.scale);
        }
        ui.Text(cx - 6, top + 4, "북", 1.25f, Ivory);
        if (Aboard())
        {
            ChartArrow(ui, cx, cy, m_player.ShipHeading());
            RenderPlayerStatus(ui, layout);
        }
        else
        {
            ui.Fill({cx - 2, cy - 2, 5, 5}, Gold);
        }

        if (island && !Aboard())
        {
            const auto& info = island->Info();
            const auto& entry = MapCatalog::Island(info.shape);
            const auto relative = position.RelativeTo(info.center);
            const bool village = island->Village(relative.x, relative.z);
            ui.Text(ui.w * .5f - 57, 31, entry.name, 2.65f, Deep, 220, 32);
            if (village)
            {
                const UiRect safe{ui.w * .5f - 84, 69, 168, 28};
                ui.Fill(safe, Ivory);
                ui.Hit(safe);
                ui.Text(safe.x + 9, safe.y + 5, "항구 마을 · 안전 구역", 1.5f, Deep);
            }

            const auto box = layout.side;
            ui.Frame(box);
            std::string sideTitle = "특산품", sideText;
            for (unsigned role = 0; role < NpcRole::Count; ++role)
            {
                const NpcKey key{info.key, role};
                if (m_relations.Npc(key).quest == QuestStatus::Active)
                {
                    const auto& quest = QuestRules::Get(role);
                    sideTitle = QuestRules::Title(role);
                    sideText = std::string(quest.objective) + " " +
                               std::to_string(m_relations.Progress(key, info.shape, m_economy)) + " / " +
                               std::to_string(quest.target);
                    break;
                }
            }
            if (sideText.empty())
            {
                for (const auto good : entry.specialties)
                {
                    if (!sideText.empty())
                    {
                        sideText += " · ";
                    }
                    sideText += MapCatalog::GoodName(good);
                }
            }
            ui.Text(box.x + 17, box.y + 14, sideTitle, 1.75f, Gold, box.w - 34, 22);
            ui.Text(box.x + 17, box.y + 41, sideText, 1.5f, Ivory, box.w - 34, 40);

            const auto service = m_world.ServiceAt(Player().location);
            const bool npc = service.kind == TownServiceKind::None && m_npcs->Nearby(Player().location).valid;
            if (npc || service.kind != TownServiceKind::None)
            {
                const UiRect prompt{ui.w * .5f - 82, ui.h * .62f, 164, 44};
                ui.Frame(prompt, 12);
                const char* label =
                    npc ? "E  대화하기"
                        : (service.kind == TownServiceKind::Shop ? "E  상점 이용" : "E  조선소 이용");
                ui.Text(prompt.x + 18, prompt.y + 12, label, 1.7f, Ivory, prompt.w - 25, 22);
                ui.Hit(prompt, UiAction::Key, 'e');
            }
        }

        const auto message = layout.message;
        const bool marineNotice = Aboard() && m_marine->HasNotice();
        if (warning || marineNotice || m_combat.NoticeTime() > 0 || m_player.NoticeTime() > 0)
        {
            const auto& text = marineNotice
                                   ? m_marine->Notice()
                                   : (m_player.NoticeTime() > 0 ? m_player.Notice() : m_combat.Notice());
            ui.Frame(message);
            if (warning)
            {
                ui.Text(message.x + 16, message.y + 13, "적 공격이 들어옵니다", 1.65f, Warning);
                ui.Text(message.x + 16, message.y + 40, "붉은 범위에서 벗어나세요.", 1.5f);
            }
            else
            {
                ui.Text(message.x + 16, message.y + 11, text, 1.5f, Ivory, message.w - 32, 55);
            }
        }
    }

    const float bottom = ui.h - 64;
    ui.Button({24, bottom, 90, 42}, "I  가방", UiAction::Key, 'i', false, !m_testOverview);
    ui.Button({122, bottom, 90, 42}, "J  의뢰", UiAction::Key, 'j');
    ui.Button({220, bottom, 98, 42}, "M  해도", UiAction::Key, 'm');
    if (m_economy.HasShip() && Aboard())
    {
        ui.Button({326, bottom, 112, 42}, "K  적재창", UiAction::Key, 'k');
    }
    ui.Button(layout.menu, "Esc  메뉴", UiAction::Key, 27);
    if (m_world.IsTestMap())
    {
        ui.Button(
            {ui.w - 182, m_testOverview ? 24.f : 354.f, 158, 42},
            "F10  테스트 맵",
            UiAction::TestPanel);
    }
    if (m_postHud)
    {
        const UiRect debug{ui.w - 428, ui.h - 158, 404, 76};
        ui.Frame(debug);
        std::ostringstream status;
        status << "HDR " << (m_postSettings.enabled ? "켜짐" : "꺼짐") << " · 노출 " << std::fixed
               << std::setprecision(1) << m_postSettings.exposure << " · " << int(1 / m_frameAverage)
               << " FPS";
        ui.Text(debug.x + 14, debug.y + 11, status.str(), 1.6f, Gold);
        ui.Text(
            debug.x + 14,
            debug.y + 39,
            "B 후처리 · 1/2/3 효과 · [ ] 노출 · 0 초기화",
            1.35f,
            Muted,
            debug.w - 28,
            21);
    }
}
