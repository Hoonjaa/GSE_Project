#include "stdafx.h"
#include "MarineActors.h"
#include "MeshCache.h"
#include "Diagnostics.h"
#include <algorithm>
#include <cmath>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

MarineTargetActor::MarineTargetActor(CachedMesh mesh, MarineTarget type)
    : MeshActor(std::move(mesh), type == MarineTarget::Pirate ? "해적선" : "크라켄 촉수"), kind(type)
{
    maximum = type == MarineTarget::Pirate ? 60.f : 30.f;
}

void MarineTargetActor::Render(Renderer& r, const ActorTransform& world) const
{
    if (health > 0)
    {
        MeshActor::Render(r, world);
    }
}

void MarineTargetActor::Overlay(Renderer& r, const ActorTransform& world) const
{
    if (health <= 0)
    {
        return;
    }
    auto p = world.position;
    p.y += kind == MarineTarget::Pirate ? 6.5f : 3;
    const auto screen = r.Project(p);
    if (screen.x < 0 || screen.x > r.Width() || screen.y < 0 || screen.y > r.Height())
    {
        return;
    }
    if (kind == MarineTarget::Pirate)
    {
        r.Text(
            screen.x - 60,
            screen.y - 20,
            lootTime >= 0 ? "약탈·복귀 " + std::to_string(int(std::ceil(lootTime))) + "초" : "해적선",
            {.98f, .78f, .4f},
            1.4f);
        return;
    }
    r.Panel(screen.x - 27, screen.y, 54, 5, {.12f, .16f, .20f});
    r.Panel(screen.x - 27, screen.y, 54 * health / maximum, 5, {.91f, .30f, .22f});
    r.Text(
        screen.x - 30,
        screen.y - 20,
        kind == MarineTarget::Pirate ? "해적선" : "촉수",
        {.98f, .78f, .4f},
        1.2f);
}

BoardingPirateActor::BoardingPirateActor(CachedMesh body, CachedMesh sword)
    : MeshActor(std::move(body), "빨간 두건 해적"), m_sword(std::move(sword))
{
    enabled = visible = false;
}

void BoardingPirateActor::Render(Renderer& r, const ActorTransform& world) const
{
    MeshActor::Render(r, world);
    r.Submit(m_sword, world.position, world.rotation + std::sin(swing * 12) * .8f);
}

void BoardingPirateActor::Overlay(Renderer& r, const ActorTransform& world) const
{
    auto p = world.position;
    p.y += 1.9f;
    const auto screen = r.Project(p);
    if (screen.x < 0 || screen.x > r.Width() || screen.y < 0 || screen.y > r.Height())
    {
        return;
    }
    r.Panel(screen.x - 20, screen.y, 40, 5, {.14f, .1f, .1f});
    r.Panel(screen.x - 20, screen.y, 40 * health / 30, 5, {.9f, .22f, .15f});
}

TornadoActor::TornadoActor(CachedMesh mesh) : MeshActor(std::move(mesh), "토네이도")
{
}

void TornadoActor::Render(Renderer& r, const ActorTransform& world) const
{
    r.Submit(mesh, world.position, world.rotation, {1, 1, 1}, MeshAnimation::Tornado, time);
}

SeaWeatherActor::SeaWeatherActor() : Actor("해상 폭풍")
{
    m_rain = MeshCache::LoadOrCreate(
        "storm_rain_v1",
        []
        {
            Mesh mesh;
            for (int i = 0; i < 96; ++i)
            {
                const float x = float(i % 12) * 3 - 18, z = float(i / 12) * 4 - 16;
                const size_t first = mesh.size();
                Geometry::Quad(
                    mesh,
                    {x, 20, z},
                    {x + .025f, 20, z},
                    {x + .15f, 19, z},
                    {x + .125f, 19, z},
                    {.48f, .65f, .76f});
                for (size_t j = first; j < mesh.size(); ++j)
                {
                    mesh[j].uv = {i * .37f, 0};
                }
            }
            return mesh;
        });
    m_lightning = MeshCache::LoadOrCreate(
        "storm_lightning_v1",
        []
        {
            Mesh mesh;
            const Color color = {.95f, .98f, 1};
            Geometry::Quad(mesh, {-5, 18, 2}, {-4.8f, 18, 2}, {-5.9f, 14, 2}, {-6.2f, 14, 2}, color);
            Geometry::Quad(mesh, {-6.2f, 14, 2}, {-5.9f, 14, 2}, {-4.5f, 10, 2}, {-4.8f, 10, 2}, color);
            return mesh;
        });
    auto put = [&](std::uint32_t n, int bytes)
    {
        for (int i = 0; i < bytes; ++i)
        {
            m_thunder.push_back(static_cast<unsigned char>(n >> (8 * i)));
        }
    };
    auto tag = [&](const char* s)
    {
        for (int i = 0; i < 4; ++i)
        {
            m_thunder.push_back(s[i]);
        }
    };
    const unsigned samples = 33075;
    tag("RIFF");
    put(36 + samples * 2, 4);
    tag("WAVE");
    tag("fmt ");
    put(16, 4);
    put(1, 2);
    put(1, 2);
    put(22050, 4);
    put(44100, 4);
    put(2, 2);
    put(16, 2);
    tag("data");
    put(samples * 2, 4);
    std::uint32_t noise = 731;
    float low = 0;
    for (unsigned i = 0; i < samples; ++i)
    {
        noise = noise * 1664525u + 1013904223u;
        low = low * .97f + ((noise >> 8) / 8388607.f - 1) * .03f;
        const float t = i / 22050.f;
        const float value =
            (low * 2 + .2f * std::sin(t * 270)) * std::exp(-2.5f * t) * (1 - std::exp(-40 * t));
        const auto sample =
            static_cast<std::int16_t>((std::max)(-30000.f, (std::min)(30000.f, value * 24000)));
        put(static_cast<std::uint16_t>(sample), 2);
    }
    visible = enabled = false;
}

SeaWeatherActor::~SeaWeatherActor()
{
    PlaySoundW(nullptr, nullptr, 0);
}

void SeaWeatherActor::SetStorm(bool storm)
{
    if (storm != m_storm)
    {
        m_time = m_flash = m_nextFlash = m_thunderDelay = 0;
    }
    m_storm = storm;
    visible = enabled = storm;
    if (!storm)
    {
        PlaySoundW(nullptr, nullptr, 0);
    }
}

void SeaWeatherActor::Update(float dt)
{
    if (!m_storm)
    {
        return;
    }
    m_time += dt;
    m_flash = (std::max)(0.f, m_flash - dt);
    m_nextFlash -= dt;
    if (m_thunderDelay > 0)
    {
        m_thunderDelay -= dt;
        if (m_thunderDelay <= 0)
        {
            if (!PlaySoundW(
                    reinterpret_cast<LPCWSTR>(m_thunder.data()),
                    nullptr,
                    SND_MEMORY | SND_ASYNC | SND_NODEFAULT))
            {
                Diagnostics::Get().Event("storm_thunder_audio_unavailable");
            }
        }
    }
    if (m_nextFlash <= 0)
    {
        m_flash = .12f;
        m_nextFlash = 6;
        m_thunderDelay = .4f;
    }
}

void SeaWeatherActor::Render(Renderer& r, const ActorTransform& world) const
{
    if (!m_storm)
    {
        return;
    }
    r.Submit(m_rain, world.position, 0, {1, 1, 1}, MeshAnimation::Rain, m_time);
    if (m_flash > 0)
    {
        r.Submit(m_lightning, world.position);
    }
}
