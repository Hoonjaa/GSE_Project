#include "stdafx.h"
#include "CombatMeshes.h"
#include "MeshCache.h"
#include <cmath>

namespace
{
    constexpr float Pi = 3.14159265359f;

    const Color Gold = {.98f, .71f, .32f};
}

CombatMeshes::CombatMeshes()
{
    using namespace Geometry;
    slash = MeshCache::LoadOrCreate(
        "player_slash_v2",
        []
        {
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
            return mesh;
        });
}
