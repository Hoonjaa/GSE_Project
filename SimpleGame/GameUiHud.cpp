#include "stdafx.h"
#include "GameUi.h"
#include "IslandTestMap.h"
#include "KoreanText.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace
{
    const Color Paper = {.96f, .90f, .72f}, Ink = {.07f, .19f, .23f}, Gold = {.98f, .71f, .32f};
}

void GameUi::RenderHud(Renderer& r)
{
    const HudLayout layout(r.Width(), r.Height());
    if (m_hud && !PlayerPanelOpen())
    {
        const bool overview = m_world.IsTestMap() && m_testOverview;
        const float W = static_cast<float>(r.Width()), H = static_cast<float>(r.Height());
        const float font = W < 950 ? 1.5f : 2.f;
        layout.title.Draw(r, Ink);
        r.Panel(20, 20, 4, 88, Gold);
        r.Text(38, 30, KoreanText::Title, Paper, 3);
        r.Text(38, 65, KoreanText::Subtitle, Gold, 1.7f);
        r.Text(38, 86, KoreanText::Version, Paper, 1.5f);
        if (m_postHud && !overview)
        {
            layout.postStats.Draw(r, Ink);
            std::ostringstream postStatus;
            postStatus << KoreanText::PostLabel
                       << (r.PostProcessingAvailable()
                               ? (m_postSettings.enabled ? KoreanText::On : KoreanText::Off)
                               : KoreanText::Unavailable)
                       << KoreanText::Exposure << std::fixed << std::setprecision(1)
                       << m_postSettings.exposure;
            r.Text(34, 125, postStatus.str(), Gold, 1.6f);
            r.Text(34, 149, KoreanText::PostControls, Paper, 1.4f);
            r.Text(34, 171, KoreanText::ExposureControls, Paper, 1.4f);
            std::ostringstream effectStatus;
            effectStatus << KoreanText::ToneLabel
                         << (m_postSettings.toneMapping ? KoreanText::On : KoreanText::Off)
                         << KoreanText::VignetteLabel
                         << (m_postSettings.vignette ? KoreanText::On : KoreanText::Off)
                         << KoreanText::BlurLabel
                         << (m_postSettings.edgeBlur ? KoreanText::On : KoreanText::Off);
            r.Text(34, 195, effectStatus.str(), Paper, 1.4f);
        }

        // The chart shows loaded geography, not permanent discoveries.
        if (W > 760 && !m_world.IsTestMap())
        {
            const float left = layout.chart.x, top = layout.chart.y, chart = 160;
            layout.chart.Draw(r, Ink);
            r.Text(left + 12, top + 8, KoreanText::Chart, Gold, 1.8f);
            r.Panel(left + 6, top + 34, chart, chart, {.09f, .32f, .38f});
            const auto* island = m_world.CurrentIsland(ActivePosition());
            if (island && island->Info().starter)
            {
                m_world.StartingTerrain().Map(r, left + 6, top + 34, chart, ActivePosition());
            }
            else
            {
                m_world.DrawMap(r, ActivePosition(), left + 6, top + 34, chart);
            }
            r.Text(left + 145, top + 37, KoreanText::North, Gold, 1.5f);
        }
        if (m_world.IsTestMap())
        {
            RenderTestMapChart(r);
        }
        if (!overview)
        {
            if (const auto* island = m_world.CurrentIsland(ActivePosition()))
            {
                const auto relative = ActivePosition().RelativeTo(island->Info().center);
                const auto& entry = MapCatalog::Island(island->Info().shape);
                layout.island.Draw(r, Ink);
                r.Text(32, 270, std::string(entry.name) + " / 항구 마을", Gold, 1.6f);
                std::string goods = "특산품: ";
                for (size_t i = 0; i < entry.specialties.size(); ++i)
                {
                    if (i)
                    {
                        goods += " · ";
                    }
                    goods += MapCatalog::GoodName(entry.specialties[i]);
                }
                r.Text(32, 294, goods, Paper, 1.35f, 402);
                r.Text(
                    32,
                    327,
                    island->Village(relative.x, relative.z)
                        ? "마을 안전 구역 / E: 시설·NPC 이용 / 특산품 거래 가능"
                        : "마을 밖 탐험 구역",
                    Paper,
                    1.3f);
            }
            layout.economy.Draw(r, Ink);
            r.Text(32, 365, "보유 골드: " + std::to_string(m_economy.Gold()), Gold, 1.6f);
            r.Text(32, 389, "I: 인벤토리 · 장비 / K: 선박 적재창", Paper, 1.4f, 402);
            const auto facility = m_world.ServiceAt(Player().location);
            const std::string shipInfo =
                m_economy.HasShip() ? ("8m 돛단배 · 동료 " + std::to_string(m_relations.Companions().size()) +
                                       "/" + std::to_string(EconomyRules::CompanionCapacity) + "명 · 적재 " +
                                       std::to_string(m_economy.UsedCargoKinds()) + "/5종")
                                    : "배 미소유 · 조선소에서 50골드로 구매";
            r.Text(
                32,
                414,
                facility.kind == TownServiceKind::None
                    ? shipInfo
                    : (facility.kind == TownServiceKind::Shop ? "[E] 상점 이용" : "[E] 조선소 이용"),
                Paper,
                1.3f,
                402);
            RenderPlayerStatus(r);
        }
        const auto active = ActivePosition();
        std::ostringstream status;
        status << (m_player.Mode() == PlayerMode::PirateDeck
                       ? "해적선 약탈"
                       : (Sailing() ? KoreanText::Sailing : (Aboard() ? "갑판 이동" : KoreanText::Walking)))
               << KoreanText::Chunk << active.chunkX << " : " << active.chunkZ;
        layout.footer.Draw(r, Ink);
        r.Panel(20, H - 122, 4, 102, Gold);
        r.Text(38, H - 108, status.str(), Gold, font);
        r.Text(38, H - 85, KoreanText::Movement, Paper, font);
        r.Text(38, H - 62, KoreanText::Tools, Paper, font);
        std::ostringstream debug;
        debug << KoreanText::Seed << m_world.StartingTerrain().Seed() << KoreanText::LoadedChunks
              << m_world.Chunks().size() << " / 활성 " << m_world.VisibleChunks() << " / 대기 "
              << m_world.PendingChunks() << KoreanText::Triangles << r.TriangleCount() << " / "
              << static_cast<int>(1 / m_frameAverage) << KoreanText::Frames;
        r.Text(38, H - 40, debug.str(), {.49f, .71f, .69f}, 1.5f);
        if (m_player.NoticeTime() > 0)
        {
            const float messageWidth = (std::min)(W - 40, 700.f);
            layout.message.Draw(r, Ink);
            r.Text(34, H - 180, m_player.Notice(), Paper, 1.8f, messageWidth - 28);
        }
    }
    RenderServiceMenu(r);
    RenderPlayerPanel(r);
    RenderInventory(r);
    RenderNpc(r);
}

void GameUi::RenderTestMapChart(Renderer& r) const
{
    if (!m_world.IsTestMap())
    {
        return;
    }
    const HudLayout layout(r.Width(), r.Height());
    const float left = layout.testChart.x, top = layout.testChart.y;
    const Color ink = {.07f, .19f, .23f}, gold = {.98f, .71f, .32f}, paper = {.96f, .90f, .72f};
    layout.testChart.Draw(r, ink);
    r.Text(left + 12, top + 8, "섬 테스트 맵 · 2행 × 4열", gold, 1.7f);
    const auto* current = m_world.CurrentIsland(Player().location);
    const unsigned selected =
        current ? unsigned(current->Info().key.z * IslandTestMap::Columns + current->Info().key.x)
                : m_testIsland;
    for (unsigned i = 0; i < IslandTestMap::Count; ++i)
    {
        const float x = left + 10 + (i % 4) * 83, y = top + 40 + (i / 4) * 49;
        r.Panel(x, y, 79, 44, i == selected ? Color{.24f, .43f, .42f} : Color{.12f, .28f, .31f});
        r.Text(x + 4, y + 3, "F" + std::to_string(i + 1), gold, 1.2f);
        r.Text(x + 4, y + 23, MapCatalog::Island(static_cast<IslandShape>(i)).name, paper, 1.15f, 73);
    }
    r.Text(left + 12, top + 143, "T: 월드 왕복 / Y: 전체·상세 보기", gold, 1.3f);
    r.Text(
        left + 12,
        top + 165,
        m_testOverview ? "전체 보기 중: 전투·타이머 일시 정지" : "F1~F8: 각 섬 항구로 즉시 이동",
        paper,
        1.25f);
}
