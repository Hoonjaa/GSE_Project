#include "stdafx.h"
#include "WorldActors.h"
#include "MeshCache.h"
#include <cmath>

void WorldActor::Prepare(const WorldPosition& camera)
{
    transform.position = location.RelativeTo(camera);
}

OceanActor::OceanActor() : Actor("바다")
{
    m_mesh = MeshCache::LoadOrCreate(
        "ocean_v1",
        []
        {
            Mesh mesh;
            Geometry::Quad(
                mesh,
                {-1, -.04f, -1},
                {1, -.04f, -1},
                {1, -.04f, 1},
                {-1, -.04f, 1},
                {.055f, .40f, .51f});
            return mesh;
        });
}

void OceanActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    renderer.Submit(m_mesh, world.position, world.rotation, {extent, 1, extent}, MeshAnimation::None, 0);
}

WaveActor::WaveActor() : Actor("파도 및 청크 격자")
{
    m_mesh = MeshCache::LoadOrCreate(
        "waves_v1",
        []
        {
            Mesh mesh;
            mesh.reserve(8 * 8 * 6);
            for (int z = 0; z < 8; ++z)
            {
                for (int x = 0; x < 8; ++x)
                {
                    const float px = x * 4.f + 1, pz = z * 4.f + 1.5f;
                    const auto first = mesh.size();
                    Geometry::Quad(
                        mesh,
                        {px, 0, pz},
                        {px + 1.1f, 0, pz},
                        {px + 1.3f, 0, pz + .065f},
                        {px + .2f, 0, pz + .065f},
                        {.09f, .47f, .57f});
                    const float right[] = {0, 1, 1, 0, 1, 0};
                    for (size_t i = 0; i < 6; ++i)
                    {
                        mesh[first + i].uv = {x * .75f + z * 1.2f, right[i]};
                    }
                }
            }
            return mesh;
        });
    m_grid = MeshCache::LoadOrCreate(
        "chunk_grid_v1",
        []
        {
            Mesh mesh;
            Geometry::Quad(
                mesh,
                {0, .65f, 0},
                {32, .65f, 0},
                {32, .65f, .035f},
                {0, .65f, .035f},
                {.98f, .71f, .29f});
            Geometry::Quad(
                mesh,
                {0, .65f, 0},
                {.035f, .65f, 0},
                {.035f, .65f, 32},
                {0, .65f, 32},
                {.98f, .71f, .29f});
            return mesh;
        });
}

void WaveActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    renderer.Submit(m_mesh, world.position, world.rotation, {1, 1, 1}, MeshAnimation::Waves, time);
    if (grid)
    {
        renderer.Submit(m_grid, world.position, world.rotation);
    }
}

FlagActor::FlagActor() : Actor("항구 깃발")
{
    m_mesh = MeshCache::LoadOrCreate(
        "harbor_flag_v1",
        []
        {
            Mesh mesh;
            Geometry::Box(mesh, {0, .55f, 0}, {.09f, 3.4f, .09f}, {.46f, .33f, .18f});
            Geometry::Triangle(mesh, {.05f, 3.9f, 0}, {1.3f, 3.55f, 0}, {.05f, 3.2f, 0}, {.88f, .45f, .26f});
            for (auto& v : mesh)
            {
                v.uv = {0, 0};
            }
            mesh[mesh.size() - 2].uv.y = 1;
            return mesh;
        });
}

void FlagActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    renderer.Submit(m_mesh, world.position, world.rotation, {1, 1, 1}, MeshAnimation::Flag, m_time);
}

GullActor::GullActor(int index) : Actor("갈매기"), m_index(index)
{
    m_mesh = MeshCache::LoadOrCreate(
        "gull_v1",
        []
        {
            Mesh mesh;
            const Color paper = {.96f, .90f, .72f};
            Geometry::Triangle(mesh, {0, 0, 0}, {-.6f, 0, -.12f}, {-.12f, 0, .08f}, paper);
            Geometry::Triangle(mesh, {0, 0, 0}, {.6f, 0, -.12f}, {.12f, 0, .08f}, paper);
            for (auto& v : mesh)
            {
                v.uv = {0, 0};
            }
            mesh[1].uv.y = mesh[4].uv.y = 1;
            return mesh;
        });
}

void GullActor::Update(float dt)
{
    m_time += dt;
    const float angle = m_time * .22f + m_index * 2;
    transform.position = {std::cos(angle) * 8, 6 + m_index * .4f, std::sin(angle) * 5};
}

void GullActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    renderer
        .Submit(m_mesh, world.position, world.rotation, {1, 1, 1}, MeshAnimation::Gull, m_time * 5 + m_index);
}
