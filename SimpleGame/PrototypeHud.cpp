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
    WorldPosition origin;
    origin.x = 0;
    origin.z = 0;
    if (m_hud)
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
            const auto mapPlayer = ActivePosition().RelativeTo(origin);
            const bool tutorialMap = std::abs(mapPlayer.x - 16) < 100 && std::abs(mapPlayer.z - 16) < 100;
            if (tutorialMap)
            {
                m_levelOne.Map(r, left + 6, top + 34, chart, mapPlayer);
            }
            else
            {
                for (const auto& entry : m_world.Chunks())
                {
                    const auto& island = entry.second;
                    if (!island.hasIsland)
                    {
                        continue;
                    }
                    WorldPosition p;
                    p.chunkX = entry.first.first;
                    p.chunkZ = entry.first.second;
                    p.x = island.centerX;
                    p.z = island.centerZ;
                    auto v = p.RelativeTo(ActivePosition());
                    float x = left + 86 + v.x * 1.1f, z = top + 114 + v.z * 1.1f;
                    float size = island.radius * 1.1f;
                    if (x - size < left + 7 || x + size > left + 165 || z - size < top + 35 ||
                        z + size > top + 193)
                    {
                        continue;
                    }
                    r.Panel(x - size, z - size, size * 2, size * 2, {.79f, .75f, .49f});
                    r.Panel(
                        x - size * .7f,
                        z - size * .7f,
                        size * 1.4f,
                        size * 1.4f,
                        island.harbor ? Color{.88f, .54f, .30f} : Color{.39f, .59f, .38f});
                }
            }
            if (!tutorialMap)
            {
                r.Panel(left + 83, top + 111, 6, 6, Paper);
            }
            r.Text(left + 145, top + 37, KoreanText::North, Gold, 1.5f);
        }
        const auto active = ActivePosition();
        std::ostringstream status;
        status << (m_sailing ? KoreanText::Sailing : KoreanText::Walking) << KoreanText::Chunk
               << active.chunkX << " : " << active.chunkZ;
        r.Panel(20, H - 122, W - 40, 102, Ink);
        r.Panel(20, H - 122, 4, 102, Gold);
        r.Text(38, H - 108, status.str(), Gold, font);
        r.Text(38, H - 85, KoreanText::Movement, Paper, font);
        r.Text(38, H - 62, KoreanText::Tools, Paper, font);
        std::ostringstream debug;
        debug << KoreanText::Seed << m_world.Tutorial().Seed() << KoreanText::LoadedChunks
              << m_world.Chunks().size() << KoreanText::Triangles << r.TriangleCount() << " / "
              << static_cast<int>(1 / m_frameAverage) << KoreanText::Frames;
        r.Text(38, H - 40, debug.str(), {.49f, .71f, .69f}, 1.5f);
        if (m_messageTime > 0)
        {
            const float messageWidth = (std::min)(W - 40, 700.f);
            r.Panel(20, H - 185, messageWidth, 55, Ink);
            r.Text(34, H - 180, m_message, Paper, 1.8f, messageWidth - 28);
        }
    }
}
