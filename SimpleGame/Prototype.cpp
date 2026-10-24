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

Prototype::Prototype() : m_world(m_scene), m_levelOne(m_scene, m_world.Tutorial(), m_economy)
{
    CreateActors();
    Reset();
}

void Prototype::Reset()
{
    Player().location = m_world.StartSpawn();
    ShipActor().location = m_world.StartShip(EconomyRules::SailboatLength);
    CloseServiceMenu();
    m_camera = Player().location;
    m_sailing = false;
    m_heading = 0;
    m_shipHeading = m_world.OutboundHeading({0, 0});
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
    if (m_service.kind != TownServiceKind::None)
    {
        ServiceKey(key);
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
    if (m_service.kind != TownServiceKind::None)
    {
        return;
    }
    if (m_hud && (y > renderer.Height() - 122 || (x < 445 && y < 255) ||
                  (renderer.Width() > 760 && x > renderer.Width() - 192 && y < 220)))
    {
        return;
    }
    if (m_hud && m_world.CurrentIsland(ActivePosition()) && x >= 20 && x <= 450 && y >= 265 && y <= 351)
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
        m_service = m_world.ServiceAt(Player().location);
        if (m_service.kind != TownServiceKind::None)
        {
            ClearInput();
            m_serviceMessage.clear();
            return;
        }
        const Vec3 d = ShipActor().location.RelativeTo(Player().location);
        if (!m_economy.HasShip())
        {
            m_message = "조선소 구역에서 E를 눌러 50골드에 돛단배를 구매하세요.";
            m_messageTime = 5;
            return;
        }
        const float c = std::cos(m_shipHeading), s = std::sin(m_shipHeading);
        const float x = d.x * c + d.z * s, z = -d.x * s + d.z * c;
        const float halfLength = EconomyRules::SailboatLength * .5f;
        const float halfBeam = EconomyRules::SailboatLength * (1.7f / 3.85f) * .5f;
        const float reach = 1.2f;
        if ((x * x) / ((halfBeam + reach) * (halfBeam + reach)) +
                (z * z) / ((halfLength + reach) * (halfLength + reach)) <
            1)
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
        for (float margin = .35f; margin <= 1.25f && !landed; margin += .15f)
        {
            for (int i = 0; i < 32 && !landed; ++i)
            {
                auto candidate = ShipActor().location;
                const float a = 2 * Pi * i / 32;
                const float x = (EconomyRules::SailboatLength * (1.7f / 3.85f) * .5f + margin) * std::cos(a);
                const float z = (EconomyRules::SailboatLength * .5f + margin) * std::sin(a);
                const float c = std::cos(m_shipHeading), s = std::sin(m_shipHeading);
                candidate.Move(x * c - z * s, x * s + z * c);
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
        if (m_sailing ? m_world.CanSail(next, m_shipHeading, EconomyRules::SailboatLength)
                      : m_world.CanWalk(next))
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
    m_levelOne.SetActive(m_world.TutorialActive());
    m_levelOne.SetActiveRegion(
        [this](Vec3 position)
        {
            WorldPosition point;
            point.x = point.z = 0;
            point.Move(position.x, position.z);
            return m_world.IsActive(point);
        });
    double horizontal = (m_keys['d'] ? 1.0 : 0) - (m_keys['a'] ? 1.0 : 0);
    double vertical = (m_keys['s'] ? 1.0 : 0) - (m_keys['w'] ? 1.0 : 0);
    if (m_service.kind != TownServiceKind::None)
    {
        horizontal = vertical = 0;
    }
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
        if (m_sailing && m_world.CanSail(ShipActor().location, m_heading, EconomyRules::SailboatLength))
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
    m_levelOne.Prepare(
        origin.RelativeTo(m_camera),
        m_hud && !m_postHud && m_service.kind == TownServiceKind::None);
    PrepareActors();
    m_scene.Render(r);
    r.BeginOverlay();
    m_scene.Overlay(r);
    r.End();
}
