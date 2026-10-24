#include "stdafx.h"
#include "MarineSystem.h"
#include "MeshCache.h"
#include <cmath>

MarineMeshes::MarineMeshes()
{
    pirate = MeshCache::LoadOrCreate(
        "pirate_ship_v1",
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
            Geometry::Box(m, {-1.25f, 1.05f, 1.3f}, {2.5f, .8f, 1.6f}, {.55f, .2f, .16f});
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
