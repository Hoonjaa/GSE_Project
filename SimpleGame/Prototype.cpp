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
}

Prototype::Prototype() : m_world(m_scene), m_levelOne(m_scene, m_world.Tutorial())
{
    CreateActors();
    Reset();
}

void Prototype::Reset()
{
    Player().location = WorldPosition();
    Player().location.x = 22;
    Player().location.z = 17.75;
    ShipActor().location = WorldPosition();
    ShipActor().location.Move(67 - ShipActor().location.x, 0);
    ShipActor().location.z = 20.3;
    m_camera = Player().location;
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
    const auto relative = Player().location.RelativeTo(m_camera);
    const Vec3 aim = {hit.x - relative.x, 0, hit.z - relative.z};
    WorldPosition origin;
    origin.x = 0;
    origin.z = 0;
    m_levelOne.Attack(Player().location.RelativeTo(origin), aim, m_sailing);
    if (m_levelOne.Attacking())
    {
        m_heading = std::atan2(aim.x, -aim.z);
    }
}

void Prototype::Interact()
{
    if (!m_sailing)
    {
        const Vec3 d = ShipActor().location.RelativeTo(Player().location);
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
                auto candidate = ShipActor().location;
                candidate.Move(distance * std::cos(2 * Pi * i / 32), distance * std::sin(2 * Pi * i / 32));
                if (m_world.CanWalk(candidate))
                {
                    Player().location = candidate;
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
    auto& position = m_sailing ? ShipActor().location : Player().location;
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
    m_levelOne.BeginFrame(static_cast<float>(dt), Player().location.RelativeTo(origin), m_sailing);
    m_scene.Update(static_cast<float>(dt));
    if (m_levelOne.EndFrame())
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
    m_world.Prepare(m_camera, m_time, m_grid);
    WorldPosition origin;
    origin.x = 0;
    origin.z = 0;
    m_levelOne.Prepare(origin.RelativeTo(m_camera), m_hud && !m_postHud);
    PrepareActors();
    m_scene.Render(r);
    r.BeginOverlay();
    m_scene.Overlay(r);
    r.End();
}
