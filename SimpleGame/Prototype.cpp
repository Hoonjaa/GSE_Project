#include "stdafx.h"
#include "Prototype.h"
#include "KoreanText.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace
{
    constexpr float Pi = 3.14159265359f;
    const Color Ink = {.07f, .19f, .23f}, Paper = {.96f, .90f, .72f}, Gold = {.98f, .71f, .32f};
    using namespace Geometry;

    Mesh Ship()
    {
        Mesh m;
        const Vec3 top[] = {
            {0, .6f, -2.05f},
            {.85f, .6f, -.85f},
            {.85f, .6f, 1.35f},
            {0, .6f, 1.8f},
            {-.85f, .6f, 1.35f},
            {-.85f, .6f, -.85f}};
        for (int i = 0; i < 6; ++i)
        {
            Vec3 a = top[i], b = top[(i + 1) % 6];
            Triangle(m, {0, .6f, 0}, a, b, {.79f, .59f, .32f});
            Quad(
                m,
                a,
                b,
                {b.x * .65f, .02f, b.z * .85f},
                {a.x * .65f, .02f, a.z * .85f},
                i < 3 ? Color{.37f, .22f, .14f} : Color{.49f, .29f, .17f});
        }
        for (int i = 0; i < 8; ++i)
        {
            Box(m, {-.67f, .603f, -.8f + i * .27f}, {1.34f, .01f, .025f}, {.47f, .33f, .20f});
        }
        Box(m, {-.085f, .6f, -.15f}, {.17f, 4.5f, .17f}, {.43f, .28f, .17f});
        Box(m, {-1.5f, 4.25f, -.16f}, {3, .10f, .10f}, {.53f, .35f, .20f});
        // Broad curved square sail: separate strips catch warm daylight.
        for (int i = 0; i < 8; ++i)
        {
            const float u = i / 8.f, v = (i + 1) / 8.f;
            const float x = -1.5f + 3 * u, X = -1.5f + 3 * v;
            Quad(
                m,
                {x, 4.24f, -.13f},
                {X, 4.24f, -.13f},
                {X * .82f, 2.1f, -.55f - .3f * std::sin(v * Pi)},
                {x * .82f, 2.1f, -.55f - .3f * std::sin(u * Pi)},
                i == 3 || i == 4 ? Color{.83f, .36f, .23f} : Color{.99f, .92f, .73f});
        }
        Triangle(m, {0, 4.8f, -.02f}, {0, 1.2f, -1.8f}, {0, 1.5f, -.3f}, {.93f, .83f, .59f});
        Triangle(m, {0, 5.0f, 0}, {.9f, 4.82f, 0}, {0, 4.6f, 0}, {.96f, .58f, .24f});
        Box(m, {-.46f, .6f, .65f}, {.92f, .48f, .65f}, {.21f, .40f, .43f});
        return m;
    }

    Mesh Character(Color coat)
    {
        Mesh m;
        Box(m, {-.22f, 0, -.13f}, {.16f, .45f, .24f}, {.22f, .24f, .24f});
        Box(m, {.06f, 0, -.13f}, {.16f, .45f, .24f}, {.22f, .24f, .24f});
        Box(m, {-.28f, .43f, -.17f}, {.56f, .59f, .34f}, coat);
        Box(m, {-.39f, .52f, -.11f}, {.12f, .42f, .22f}, coat);
        Box(m, {.27f, .52f, -.11f}, {.12f, .42f, .22f}, coat);
        Box(m, {-.18f, 1.02f, -.15f}, {.36f, .35f, .30f}, {.89f, .66f, .43f});
        Box(m, {-.33f, 1.37f, -.27f}, {.66f, .08f, .54f}, {.89f, .78f, .53f});
        Box(m, {-.21f, 1.44f, -.17f}, {.42f, .16f, .34f}, {.80f, .63f, .37f});
        Box(m, {-.23f, .64f, .18f}, {.46f, .4f, .18f}, {.52f, .32f, .18f});
        return m;
    }
}

Prototype::Prototype() : m_levelOne(m_world.Tutorial())
{
    Mesh marker, sword;
    Disc(marker, {0, 0, 0}, .46f, Gold, 24);
    Box(sword, {.32f, .5f, -.55f}, {.12f, .12f, .9f}, {.83f, .91f, .95f});
    Box(sword, {.20f, .48f, .1f}, {.36f, .16f, .1f}, Gold);
    m_markerMesh = CachedMesh(std::move(marker));
    m_swordMesh = CachedMesh(std::move(sword));
    m_shipMesh = Ship();
    m_characterMesh = Character({.17f, .43f, .63f});
    m_npcMesh = Character({.77f, .34f, .22f});
    Reset();
}

void Prototype::Reset()
{
    m_player = WorldPosition();
    m_player.x = 22;
    m_player.z = 17.75;
    m_ship = WorldPosition();
    m_ship.Move(67 - m_ship.x, 0);
    m_ship.z = 20.3;
    m_camera = m_player;
    m_sailing = false;
    m_heading = 0;
    m_shipHeading = 0;
    m_keys.fill(false);
    m_levelOne.Recover();
    m_message = KoreanText::Welcome;
    m_messageTime = 10;
}

void Prototype::ClearInput()
{
    m_keys.fill(false);
}

void Prototype::Key(unsigned char key, bool down)
{
    if (key >= 'A' && key <= 'Z')
    {
        key += 'a' - 'A';
    }
    const bool pressed = down && !m_keys[key];
    m_keys[key] = down;
    if (!pressed)
    {
        return;
    }
    switch (key)
    {
        case 'e':
            Interact();
            break;
        case 'g':
            m_grid = !m_grid;
            break;
        case 'h':
            m_hud = !m_hud;
            break;
        case 'r':
            Reset();
            break;
        case 'f':
            m_levelOne.Upgrade();
            break;
        case 'v':
            m_postHud = !m_postHud;
            break;
        case 'p':
            m_postSettings.enabled = !m_postSettings.enabled;
            m_message = m_postSettings.enabled ? KoreanText::PostEnabled : KoreanText::PostDisabled;
            m_messageTime = 5;
            break;
        case '1':
            m_postSettings.toneMapping = !m_postSettings.toneMapping;
            break;
        case '2':
            m_postSettings.vignette = !m_postSettings.vignette;
            break;
        case '3':
            m_postSettings.edgeBlur = !m_postSettings.edgeBlur;
            break;
        case '[':
            m_postSettings.exposure = (std::max)(.1f, m_postSettings.exposure - .1f);
            break;
        case ']':
            m_postSettings.exposure = (std::min)(4.f, m_postSettings.exposure + .1f);
            break;
        case '0':
            m_postSettings = PostProcessingSettings();
            break;
        case '+':
        case '=':
            Zoom(1);
            break;
        case '-':
        case '_':
            Zoom(-1);
            break;
    }
}

void Prototype::Zoom(int direction)
{
    m_zoom = (std::max)(16.f, (std::min)(48.f, m_zoom + direction * 2.f));
}

void Prototype::MouseAttack(int x, int y, const Renderer& renderer)
{
    if (m_hud && (y > renderer.Height() - 122 || (x < 445 && y < 255) ||
                  (renderer.Width() > 760 && x > renderer.Width() - 192 && y < 220)))
    {
        return;
    }
    const auto hit = renderer.ScreenGround(x, y);
    const auto relative = m_player.RelativeTo(m_camera);
    const Vec3 aim = {hit.x - relative.x, 0, hit.z - relative.z};
    WorldPosition origin;
    origin.x = 0;
    origin.z = 0;
    m_levelOne.Attack(m_player.RelativeTo(origin), aim, m_sailing);
    if (m_levelOne.Attacking())
    {
        m_heading = std::atan2(aim.x, -aim.z);
    }
}

void Prototype::Interact()
{
    if (!m_sailing)
    {
        const Vec3 d = m_ship.RelativeTo(m_player);
        if (d.x * d.x + d.z * d.z < 3.4f * 3.4f)
        {
            m_sailing = true;
            m_message = KoreanText::Boarded;
        }
        else
        {
            m_message = KoreanText::TooFarToBoard;
        }
    }
    else
    {
        // Search nearby shore/dock points without moving or discarding the ship.
        bool landed = false;
        for (float distance = 1.6f; distance <= 3.25f && !landed; distance += .2f)
        {
            for (int i = 0; i < 32 && !landed; ++i)
            {
                auto candidate = m_ship;
                candidate.Move(distance * std::cos(2 * Pi * i / 32), distance * std::sin(2 * Pi * i / 32));
                if (m_world.CanWalk(candidate))
                {
                    m_player = candidate;
                    m_sailing = false;
                    landed = true;
                }
            }
        }
        m_message = landed ? KoreanText::Landed : KoreanText::TooFarToLand;
    }
    m_messageTime = 5;
}

void Prototype::Move(double dx, double dz)
{
    auto& position = m_sailing ? m_ship : m_player;
    auto tryStep = [&](double x, double z)
    {
        if (x == 0 && z == 0)
        {
            return false;
        }
        auto next = position;
        next.Move(x, z);
        if (m_sailing ? m_world.CanSail(next) : m_world.CanWalk(next))
        {
            position = next;
            return true;
        }
        return false;
    };
    // Small bounded substeps plus sliding prevent tunnelling and sticky coastlines.
    int steps = static_cast<int>(std::ceil(std::sqrt(dx * dx + dz * dz) / .12));
    if (steps < 1)
    {
        return;
    }
    dx /= steps;
    dz /= steps;
    for (int i = 0; i < steps; ++i)
    {
        if (tryStep(dx, dz))
        {
            m_moving = true;
        }
        else
        {
            const bool x = tryStep(dx, 0), z = tryStep(0, dz);
            m_moving = m_moving || x || z;
        }
    }
}

void Prototype::Update(double dt, int width, int height)
{
    if (dt > 0)
    {
        m_frameAverage += (dt - m_frameAverage) * .05;
    }
    dt = (std::max)(0.0, (std::min)(dt, .05));
    m_time += static_cast<float>(dt);
    m_messageTime = (std::max)(0.f, m_messageTime - static_cast<float>(dt));
    // Covers the inverse isometric viewport, with a margin for scenery and camera lag.
    const int radius = static_cast<int>(std::ceil((width * .354 + height * .613) / (m_zoom * ChunkSize))) + 1;
    m_world.Update(ActivePosition(), radius);
    double horizontal = (m_keys['d'] ? 1.0 : 0) - (m_keys['a'] ? 1.0 : 0);
    double vertical = (m_keys['s'] ? 1.0 : 0) - (m_keys['w'] ? 1.0 : 0);
    const double length = std::sqrt(horizontal * horizontal + vertical * vertical);
    m_moving = false;
    if (length > 0)
    {
        horizontal /= length;
        vertical /= length;
        double dx = (horizontal + vertical) * .70710678118;
        double dz = (-horizontal + vertical) * .70710678118;
        if (!m_levelOne.Attacking())
        {
            m_heading = static_cast<float>(std::atan2(dx, -dz));
        }
        if (m_sailing)
        {
            m_shipHeading = m_heading;
        }
        const double speed = (m_sailing ? 7.5 : 3.6) * (m_keys[' '] ? 1.8 : 1);
        Move(dx * speed * dt, dz * speed * dt);
    }
    WorldPosition origin;
    origin.x = 0;
    origin.z = 0;
    if (m_levelOne.Update(static_cast<float>(dt), m_player.RelativeTo(origin), m_sailing))
    {
        Reset();
    }
    const auto active = ActivePosition();
    const Vec3 distance = active.RelativeTo(m_camera);
    const double follow = 1 - std::exp(-8 * dt);
    m_camera.Move(distance.x * follow, distance.z * follow);
}

void Prototype::Render(Renderer& r)
{
    r.SetPostProcessing(m_postSettings);
    r.Begin(m_zoom);
    m_world.Render(r, m_camera, m_time, m_grid);
    WorldPosition origin;
    origin.x = 0;
    origin.z = 0;
    const auto levelOffset = origin.RelativeTo(m_camera);
    m_levelOne.Render(r, levelOffset);
    const Vec3 shipOffset = m_ship.RelativeTo(m_camera);
    if (std::abs(shipOffset.x) < 500 && std::abs(shipOffset.z) < 500)
    {
        Mesh water;
        Disc(water, {0, .006f, 0}, 1.5f, {.04f, .34f, .43f}, 24);
        if (m_sailing && m_moving)
        {
            for (int n = 0; n < 6; ++n)
            {
                float z = 2 + n * .42f, spread = .25f + n * .13f;
                Quad(
                    water,
                    {-spread, .008f, z},
                    {spread, .008f, z},
                    {spread + .10f, .008f, z + .09f},
                    {-spread - .1f, .008f, z + .09f},
                    {.34f, .70f, .73f});
            }
        }
        r.Submit(water, shipOffset, m_shipHeading);
        auto bob = shipOffset;
        bob.y = .045f * std::sin(m_time * 2);
        r.Submit(m_shipMesh, bob, m_shipHeading);
    }
    if (!m_sailing)
    {
        auto p = m_player.RelativeTo(m_camera);
        p.y = .58f;
        r.Submit(m_markerMesh, p);
        p.y += m_moving ? .035f * std::abs(std::sin(m_time * 12)) : 0;
        r.Submit(m_characterMesh, p, m_heading);
        r.Submit(m_swordMesh, p, m_heading);
    }
    // Harbor residents are visual stand-ins, not an AI dialogue implementation.
    WorldPosition harbor;
    harbor.x = 18.3;
    harbor.z = 18.9;
    const auto h = harbor.RelativeTo(m_camera);
    if (std::abs(h.x) < 90 && std::abs(h.z) < 90)
    {
        r.Submit(m_npcMesh, {h.x, .57f, h.z}, .6f);
        r.Submit(m_npcMesh, {h.x - 5, .57f, h.z + .1f}, -.8f);
        Mesh flag;
        Box(flag, {h.x + 2.8f, .55f, h.z - .3f}, {.09f, 3.4f, .09f}, {.46f, .33f, .18f});
        Triangle(
            flag,
            {h.x + 2.85f, 3.9f, h.z - .3f},
            {h.x + 4.1f, 3.55f, h.z - .3f + std::sin(m_time * 3) * .15f},
            {h.x + 2.85f, 3.2f, h.z - .3f},
            {.88f, .45f, .26f});
        r.Submit(flag);
    }
    // Small animated gull silhouettes over the harbor.
    if (std::abs(h.x) < 90 && std::abs(h.z) < 90)
    {
        Mesh birds;
        for (int i = 0; i < 3; ++i)
        {
            const float angle = m_time * .22f + i * 2;
            const float x = h.x + std::cos(angle) * 8, z = h.z + std::sin(angle) * 5, y = 6 + i * .4f;
            float flap = std::sin(m_time * 5 + i) * .16f;
            Triangle(birds, {x, y, z}, {x - .6f, y + flap, z - .12f}, {x - .12f, y, z + .08f}, Paper);
            Triangle(birds, {x, y, z}, {x + .6f, y + flap, z - .12f}, {x + .12f, y, z + .08f}, Paper);
        }
        r.Submit(birds);
    }
    r.BeginOverlay();
    m_levelOne.Overlay(r, levelOffset, m_hud && !m_postHud);
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
    r.End();
}
