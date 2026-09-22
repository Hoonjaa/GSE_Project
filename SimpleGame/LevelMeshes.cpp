#include "stdafx.h"
#include "GameplayActors.h"
#include <cmath>

namespace
{
    constexpr float Pi = 3.14159265359f;

    const Color Paper = {.96f, .90f, .72f}, Gold = {.98f, .71f, .32f};
}

LevelMeshes::LevelMeshes()
{
    using namespace Geometry;
    for (int variant = 0; variant < 4; ++variant)
    {
        Mesh mesh;
        const Vec3 p = {};
        const bool boss = variant >= 2, flash = (variant % 2) != 0;
        const float scale = boss ? 2.f : 1.f;
        const Color shell = flash ? Paper : (boss ? Color{.69f, .24f, .16f} : Color{.83f, .48f, .23f});
        Cone(mesh, {p.x, .56f, p.z}, .7f * scale, .85f * scale, shell, 8);
        for (int i = -1; i <= 1; i += 2)
        {
            Box(mesh,
                {p.x + i * .68f * scale - .13f, .6f, p.z - .5f * scale},
                {.26f * scale, .25f * scale, scale},
                {.59f, .30f, .16f});
            Cone(
                mesh,
                {p.x + i * .42f * scale, .78f, p.z - .65f * scale},
                .22f * scale,
                .55f * scale,
                Gold,
                5);
        }
        if (boss)
        {
            Cone(mesh, {p.x, 2.05f, p.z}, .45f, .65f, Gold, 5);
        }

        enemies[variant] = CachedMesh(std::move(mesh));
    }
    for (int variant = 0; variant < 2; ++variant)
    {
        Mesh mesh;
        const bool boss = variant != 0;
        const float radius = boss ? 3.6f : 1.35f;
        for (int i = 0; i < 48; ++i)
        {
            const float a = 2 * Pi * i / 48, b = 2 * Pi * (i + 1) / 48;
            const Vec3 t = {};
            Quad(
                mesh,
                {t.x + radius * std::cos(a), .59f, t.z + radius * std::sin(a)},
                {t.x + radius * std::cos(b), .59f, t.z + radius * std::sin(b)},
                {t.x + (radius - .12f) * std::cos(b), .59f, t.z + (radius - .12f) * std::sin(b)},
                {t.x + (radius - .12f) * std::cos(a), .59f, t.z + (radius - .12f) * std::sin(a)},
                {1, .24f, .10f});
        }
        warnings[variant] = CachedMesh(std::move(mesh));
    }
    for (int weapon = 0; weapon < 2; ++weapon)
    {
        Mesh mesh;
        const Vec3 p = {};
        const float y = 0;
        if (weapon)
        {
            Box(mesh, {p.x - .08f, y, p.z - .08f}, {.16f, .9f, .16f}, {.79f, .91f, .94f});
            Box(mesh, {p.x - .3f, y + .15f, p.z - .1f}, {.6f, .12f, .2f}, Gold);
        }
        else
        {
            Cone(mesh, {p.x, y, p.z}, .23f, .42f, {.24f, .79f, .96f}, 5);
        }
        drops[weapon] = CachedMesh(std::move(mesh));
    }
    Mesh mesh;
    const float heading = 0;
    for (int i = 0; i < 20; ++i)
    {
        const float a = heading - Pi / 3 + 2 * Pi / 3 * i / 20,
                    b = heading - Pi / 3 + 2 * Pi / 3 * (i + 1) / 20;
        const Vec3 p = {};
        Quad(
            mesh,
            {p.x + 2.7f * std::cos(a), 1.1f, p.z + 2.7f * std::sin(a)},
            {p.x + 2.7f * std::cos(b), 1.1f, p.z + 2.7f * std::sin(b)},
            {p.x + 2.3f * std::cos(b), 1.1f, p.z + 2.3f * std::sin(b)},
            {p.x + 2.3f * std::cos(a), 1.1f, p.z + 2.3f * std::sin(a)},
            Gold);
    }
    slash = CachedMesh(std::move(mesh));
}
