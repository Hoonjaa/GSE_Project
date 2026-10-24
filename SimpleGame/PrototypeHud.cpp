#include "stdafx.h"
#include "Prototype.h"
#include "KoreanText.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace
{
    const Color Paper = {.96f, .90f, .72f}, Ink = {.07f, .19f, .23f}, Gold = {.98f, .71f, .32f};
}

void Prototype::RenderHud(Renderer& r)
{
    if (m_hud && !PlayerPanelOpen())
    {
        const float W = static_cast<float>(r.Width()), H = static_cast<float>(r.Height());
        const float font = W < 950 ? 1.5f : 2.f;
        r.Panel(20, 20, 330, 88, Ink);
        r.Panel(20, 20, 4, 88, Gold);
        r.Text(38, 30, KoreanText::Title, Paper, 3);
        r.Text(38, 65, KoreanText::Subtitle, Gold, 1.7f);
        r.Text(38, 86, KoreanText::Version, Paper, 1.5f);
        if (m_postHud)
        {
            r.Panel(20, 118, 420, 107, Ink);
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
        if (W > 760)
        {
            const float left = W - 192, top = 20, chart = 160;
            r.Panel(left, top, 172, 200, Ink);
            r.Text(left + 12, top + 8, KoreanText::Chart, Gold, 1.8f);
            r.Panel(left + 6, top + 34, chart, chart, {.09f, .32f, .38f});
            const auto* island = m_world.CurrentIsland(ActivePosition());
            if (island && island->Info().starter)
            {
                WorldPosition origin;
                origin.x = origin.z = 0;
                m_levelOne.Map(r, left + 6, top + 34, chart, ActivePosition().RelativeTo(origin));
            }
            else
            {
                m_world.DrawMap(r, ActivePosition(), left + 6, top + 34, chart);
            }
            r.Text(left + 145, top + 37, KoreanText::North, Gold, 1.5f);
        }
        if (const auto* island = m_world.CurrentIsland(ActivePosition()))
        {
            const auto relative = ActivePosition().RelativeTo(island->Info().center);
            const auto& entry = MapCatalog::Island(island->Info().shape);
            r.Panel(20, 265, 430, 86, Ink);
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
                    ? "마을 안전 구역 / 산호 전리품 매입 5·20골드 / 특산품 무역 준비 중"
                    : "마을 밖 탐험 구역",
                Paper,
                1.3f);
        }
        r.Panel(20, 360, 430, 84, Ink);
        r.Text(32, 365, "보유 골드: " + std::to_string(m_economy.Gold()), Gold, 1.6f);
        r.Text(
            32,
            389,
            "전리품: 일반 " + std::to_string(m_economy.LootCount(CoralLoot::Normal)) + " / 보스 " +
                std::to_string(m_economy.LootCount(CoralLoot::Boss)),
            Paper,
            1.4f,
            402);
        const auto facility = m_world.ServiceAt(Player().location);
        const std::string shipInfo =
            m_economy.HasShip()
                ? ("8m 돛단배 · 동료 정원 1명 · 적재 " + std::to_string(m_economy.UsedCargoKinds()) + "/5종")
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
        const auto active = ActivePosition();
        std::ostringstream status;
        status << (m_playerMode == PlayerMode::PirateDeck
                       ? "해적선 약탈"
                       : (Sailing() ? KoreanText::Sailing : (Aboard() ? "갑판 이동" : KoreanText::Walking)))
               << KoreanText::Chunk << active.chunkX << " : " << active.chunkZ;
        r.Panel(20, H - 122, W - 40, 102, Ink);
        r.Panel(20, H - 122, 4, 102, Gold);
        r.Text(38, H - 108, status.str(), Gold, font);
        r.Text(38, H - 85, KoreanText::Movement, Paper, font);
        r.Text(38, H - 62, KoreanText::Tools, Paper, font);
        std::ostringstream debug;
        debug << KoreanText::Seed << m_world.Tutorial().Seed() << KoreanText::LoadedChunks
              << m_world.Chunks().size() << " / 활성 " << m_world.VisibleChunks() << " / 대기 "
              << m_world.PendingChunks() << KoreanText::Triangles << r.TriangleCount() << " / "
              << static_cast<int>(1 / m_frameAverage) << KoreanText::Frames;
        r.Text(38, H - 40, debug.str(), {.49f, .71f, .69f}, 1.5f);
        if (m_messageTime > 0)
        {
            const float messageWidth = (std::min)(W - 40, 700.f);
            r.Panel(20, H - 185, messageWidth, 55, Ink);
            r.Text(34, H - 180, m_message, Paper, 1.8f, messageWidth - 28);
        }
    }
    RenderServiceMenu(r);
    RenderPlayerPanel(r);
}
