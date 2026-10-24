#include "stdafx.h"
#include "MarineSystem.h"
#include "MeshCache.h"
#include <cmath>

MarineMeshes::MarineMeshes()
{
    raider = MeshCache::LoadOrCreate(
        "red_bandana_pirate_v1",
        []
        {
            Mesh m;
            Geometry::Box(m, {-.18f, 0, -.12f}, {.14f, .45f, .24f}, {.18f, .16f, .15f});
            Geometry::Box(m, {.04f, 0, -.12f}, {.14f, .45f, .24f}, {.18f, .16f, .15f});
            Geometry::Box(m, {-.24f, .45f, -.15f}, {.48f, .55f, .3f}, {.27f, .28f, .31f});
            Geometry::Box(m, {-.17f, 1, -.14f}, {.34f, .35f, .28f}, {.83f, .59f, .39f});
            Geometry::Box(m, {-.19f, 1.22f, -.16f}, {.38f, .16f, .32f}, {.9f, .10f, .08f});
            Geometry::Box(m, {.13f, 1.18f, .08f}, {.12f, .22f, .16f}, {.8f, .07f, .05f});
            Geometry::Box(m, {-.34f, .55f, -.12f}, {.12f, .38f, .24f}, {.83f, .59f, .39f});
            Geometry::Box(m, {.22f, .55f, -.12f}, {.12f, .38f, .24f}, {.83f, .59f, .39f});
            return m;
        });
    cutlass = MeshCache::LoadOrCreate(
        "pirate_cutlass_v1",
        []
        {
            Mesh m;
            Geometry::Box(m, {.25f, .62f, -.95f}, {.08f, .08f, .85f}, {.86f, .9f, .94f});
            Geometry::Box(m, {.17f, .60f, -.15f}, {.25f, .12f, .08f}, {.88f, .68f, .24f});
            return m;
        });
    chest = MeshCache::LoadOrCreate(
        "pirate_loot_chest_v1",
        []
        {
            Mesh m;
            Geometry::Box(m, {-.3f, 0, -.22f}, {.6f, .42f, .44f}, {.52f, .30f, .12f});
            Geometry::Box(m, {-.32f, .27f, -.24f}, {.64f, .07f, .48f}, {.94f, .72f, .24f});
            return m;
        });
    pirate = MeshCache::LoadOrCreate(
        "pirate_ship_v2_boarding",
        []
        {
            Mesh m;
            Geometry::Box(m, {-1.5f, .15f, -3.5f}, {3, .9f, 7}, {.40f, .23f, .15f});
            Geometry::Triangle(
                m,
                {-1.5f, 1.05f, -3.5f},
                {0, 1.05f, -4},
                {1.5f, 1.05f, -3.5f},
                {.60f, .38f, .2f});
            Geometry::Box(m, {-1, 1.05f, 1.3f}, {2, .8f, 1.6f}, {.55f, .2f, .16f});
            Geometry::Box(m, {-.10f, 1.05f, -.5f}, {.2f, 4.8f, .2f}, {.39f, .26f, .15f});
            Geometry::Quad(
                m,
                {-2, 5.6f, -.3f},
                {2, 5.6f, -.3f},
                {1.7f, 2.2f, -.6f},
                {-1.7f, 2.2f, -.6f},
                {.73f, .23f, .18f});
            return m;
        });
    tentacle = MeshCache::LoadOrCreate(
        "kraken_tentacle_v1",
        []
        {
            Mesh m;
            Geometry::Cone(m, {0, -.1f, 0}, .42f, 2.7f, {.48f, .28f, .62f}, 10);
            Geometry::Box(m, {-.12f, 1.4f, -.38f}, {.24f, .6f, .1f}, {.88f, .61f, .66f});
            return m;
        });
    tornado = MeshCache::LoadOrCreate(
        "sea_tornado_v1",
        []
        {
            Mesh m;
            for (int ring = 0; ring < 8; ++ring)
            {
                const float y = float(ring), Y = y + .8f, r = .35f + y * .44f, R = .35f + Y * .44f;
                for (int i = 0; i < 16; ++i)
                {
                    const float a = i * 6.2831853f / 16, b = (i + 1) * 6.2831853f / 16;
                    const Color c = {.30f + ring * .025f, .34f + ring * .025f, .39f + ring * .025f};
                    Geometry::Quad(
                        m,
                        {std::cos(a) * r, y, std::sin(a) * r},
                        {std::cos(b) * r, y, std::sin(b) * r},
                        {std::cos(b) * R, Y, std::sin(b) * R},
                        {std::cos(a) * R, Y, std::sin(a) * R},
                        c);
                }
            }
            return m;
        });
}
