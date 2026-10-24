#include "stdafx.h"
#include "TownServiceActor.h"
#include "MeshCache.h"

TownServiceActor::TownServiceActor(TownServiceKind kind)
    : Actor(kind == TownServiceKind::Shop ? "상점" : "조선소"), m_kind(kind)
{
    const bool shop = kind == TownServiceKind::Shop;
    m_zone = MeshCache::LoadOrCreate(
        shop ? "town_shop_zone_v1" : "town_shipyard_zone_v1",
        [shop]
        {
            Mesh mesh;
            Geometry::Disc(
                mesh,
                {0, .565f, 0},
                .8f,
                shop ? Color{.84f, .66f, .26f} : Color{.28f, .65f, .75f},
                24);
            return mesh;
        });
    m_model = MeshCache::LoadOrCreate(
        shop ? "town_shop_stall_v1" : "town_shipyard_crane_v1",
        [shop]
        {
            Mesh mesh;
            if (shop)
            {
                Geometry::Box(mesh, {-1, .55f, -1.8f}, {2, .7f, .6f}, {.57f, .36f, .18f});
                Geometry::Box(mesh, {-1.05f, .55f, -1.85f}, {.1f, 2, .1f}, {.40f, .28f, .15f});
                Geometry::Box(mesh, {.95f, .55f, -1.85f}, {.1f, 2, .1f}, {.40f, .28f, .15f});
                Geometry::Box(mesh, {-1.15f, 2.5f, -1.95f}, {2.3f, .12f, .9f}, {.91f, .60f, .22f});
                Geometry::Cone(mesh, {-.5f, 1.25f, -1.5f}, .19f, .35f, {.85f, .45f, .30f}, 5);
            }
            else
            {
                Geometry::Box(mesh, {-1, .55f, -1.8f}, {.18f, 3, .18f}, {.44f, .31f, .17f});
                Geometry::Box(mesh, {-.95f, 3.4f, -1.8f}, {2.2f, .18f, .18f}, {.55f, .39f, .20f});
                Geometry::Box(mesh, {1, 2.1f, -1.75f}, {.04f, 1.3f, .04f}, {.8f, .71f, .47f});
                Geometry::Box(mesh, {.85f, 1.9f, -1.9f}, {.35f, .25f, .35f}, {.34f, .40f, .44f});
            }
            return mesh;
        });
}

void TownServiceActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    renderer.Submit(m_zone, world.position, world.rotation);
    renderer.Submit(m_model, world.position, world.rotation);
}

void TownServiceActor::Overlay(Renderer& renderer, const ActorTransform& world) const
{
    auto position = world.position;
    position.y += 3.8f;
    const auto p = renderer.Project(position);
    if (p.x < 0 || p.y < 0 || p.x > renderer.Width() || p.y > renderer.Height())
    {
        return;
    }
    renderer
        .Text(p.x - 35, p.y, m_kind == TownServiceKind::Shop ? "상점" : "조선소", {.98f, .80f, .40f}, 1.5f);
}
