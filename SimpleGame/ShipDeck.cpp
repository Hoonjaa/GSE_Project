#include "stdafx.h"
#include "ShipDeck.h"
#include <algorithm>
#include <cmath>

namespace
{
    float EdgeDistance(Vec3 point)
    {
        const float s = ShipDeck::Scale();
        const Vec2 hull[] =
            {{0, -1.925f}, {.85f, -.725f}, {.85f, 1.475f}, {0, 1.925f}, {-.85f, 1.475f}, {-.85f, -.725f}};
        float distance = 1e9f;
        for (int i = 0; i < 6; ++i)
        {
            const auto a = hull[i], b = hull[(i + 1) % 6];
            const float dx = (b.x - a.x) * s, dz = (b.y - a.y) * s;
            distance = (std::min)(
                distance,
                (dx * (point.z - a.y * s) - dz * (point.x - a.x * s)) / std::hypot(dx, dz));
        }
        return distance;
    }

    bool Obstacle(Vec3 p, float margin)
    {
        const float s = ShipDeck::Scale();
        const bool mast = p.x > -.085f * s - margin && p.x < .085f * s + margin &&
                          p.z > -.025f * s - margin && p.z < .145f * s + margin;
        const bool cabin = p.x > -.46f * s - margin && p.x < .46f * s + margin && p.z > .775f * s - margin &&
                           p.z < 1.425f * s + margin;
        return mast || cabin;
    }
}

float ShipDeck::Scale()
{
    return EconomyRules::SailboatLength / 3.85f;
}

float ShipDeck::Height()
{
    return .6f * Scale();
}

float ShipDeck::HalfBeam()
{
    return .85f * Scale();
}

Vec3 ShipDeck::Helm()
{
    return {0, Height(), 1.25f};
}

Vec3 ShipDeck::HelmStand()
{
    return {0, Height(), .75f};
}

bool ShipDeck::Walkable(Vec3 p)
{
    return EdgeDistance(p) >= .2f && !Obstacle(p, .2f);
}

bool ShipDeck::NearEdge(Vec3 p)
{
    return EdgeDistance(p) <= .85f;
}

bool ShipDeck::ClearAttack(Vec3 from, Vec3 to)
{
    const int steps = (std::max)(1, int(std::ceil(std::hypot(to.x - from.x, to.z - from.z) / .1f)));
    for (int i = 0; i <= steps; ++i)
    {
        const float t = float(i) / steps;
        if (Obstacle({from.x + (to.x - from.x) * t, 0, from.z + (to.z - from.z) * t}, .02f))
        {
            return false;
        }
    }
    return true;
}

void ShipDeck::Move(Vec3& point, float dx, float dz)
{
    const int steps = (std::max)(1, int(std::ceil(std::hypot(dx, dz) / .1f)));
    dx /= steps;
    dz /= steps;
    for (int i = 0; i < steps; ++i)
    {
        const Vec3 next = {point.x + dx, Height(), point.z + dz};
        if (Walkable(next))
        {
            point = next;
            continue;
        }
        const Vec3 slideX = {point.x + dx, Height(), point.z};
        if (Walkable(slideX))
        {
            point = slideX;
        }
        const Vec3 slideZ = {point.x, Height(), point.z + dz};
        if (Walkable(slideZ))
        {
            point = slideZ;
        }
    }
}

Vec3 ShipDeck::ClosestBoardingPoint(Vec3 outside)
{
    Vec3 best = HelmStand();
    float distance = 1e9f;
    for (float z = -EconomyRules::SailboatLength * .5f; z <= EconomyRules::SailboatLength * .5f; z += .15f)
    {
        for (float x = -HalfBeam(); x <= HalfBeam(); x += .15f)
        {
            const Vec3 point = {x, Height(), z};
            if (!Walkable(point))
            {
                continue;
            }
            const float d = std::hypot(x - outside.x, z - outside.z);
            if (d < distance)
            {
                distance = d;
                best = point;
            }
        }
    }
    return best;
}

Vec3 ShipDeck::ToWorld(Vec3 p, float heading)
{
    const float c = std::cos(heading), s = std::sin(heading);
    return {p.x * c - p.z * s, p.y, p.x * s + p.z * c};
}

Vec3 ShipDeck::ToLocal(Vec3 p, float heading)
{
    return ToWorld(p, -heading);
}
