#include "stdafx.h"
#include "WorldActors.h"
#include <cmath>

void WorldActor::Prepare(const WorldPosition& camera)
{
    transform.position = location.RelativeTo(camera);
}

void OceanActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    using namespace Geometry;
    const float seaExtent = extent;
    Mesh water;
    Quad(
        water,
        {-seaExtent, -.04f, -seaExtent},
        {seaExtent, -.04f, -seaExtent},
        {seaExtent, -.04f, seaExtent},
        {-seaExtent, -.04f, seaExtent},
        {.055f, .40f, .51f});
    renderer.Submit(water, world.position, world.rotation);
}

void WaveActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    using namespace Geometry;
    Mesh surface;
    // Waves remain anchored to chunk-local coordinates across origin shifts.
    for (int z = 0; z < 8; ++z)
    {
        for (int x = 0; x < 8; ++x)
        {
            const float phase = time * .8f + x * .75f + z * 1.2f;
            const float px = x * 4.f + 1 + std::sin(phase) * .35f, pz = z * 4.f + 1.5f;
            const float light = .5f + .5f * std::sin(phase);
            Quad(
                surface,
                {px, 0, pz},
                {px + 1.1f + light * .7f, 0, pz},
                {px + 1.3f + light * .7f, 0, pz + .065f},
                {px + .2f, 0, pz + .065f},
                {.09f + light * .055f, .47f + light * .08f, .57f + light * .07f});
        }
    }
    if (grid)
    {
        Quad(surface, {0, .65f, 0}, {32, .65f, 0}, {32, .65f, .035f}, {0, .65f, .035f}, {.98f, .71f, .29f});
        Quad(surface, {0, .65f, 0}, {.035f, .65f, 0}, {.035f, .65f, 32}, {0, .65f, 32}, {.98f, .71f, .29f});
    }

    renderer.Submit(surface, world.position, world.rotation);
}

void FlagActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    Mesh mesh;
    Geometry::Box(mesh, {0, .55f, 0}, {.09f, 3.4f, .09f}, {.46f, .33f, .18f});
    Geometry::Triangle(
        mesh,
        {.05f, 3.9f, 0},
        {1.3f, 3.55f, std::sin(m_time * 3) * .15f},
        {.05f, 3.2f, 0},
        {.88f, .45f, .26f});
    renderer.Submit(mesh, world.position, world.rotation);
}

void GullActor::Update(float dt)
{
    m_time += dt;
    const float angle = m_time * .22f + m_index * 2;
    transform.position = {std::cos(angle) * 8, 6 + m_index * .4f, std::sin(angle) * 5};
}

void GullActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    const float flap = std::sin(m_time * 5 + m_index) * .16f;
    const Color paper = {.96f, .90f, .72f};
    Mesh mesh;
    Geometry::Triangle(mesh, {0, 0, 0}, {-.6f, flap, -.12f}, {-.12f, 0, .08f}, paper);
    Geometry::Triangle(mesh, {0, 0, 0}, {.6f, flap, -.12f}, {.12f, 0, .08f}, paper);
    renderer.Submit(mesh, world.position, world.rotation);
}
