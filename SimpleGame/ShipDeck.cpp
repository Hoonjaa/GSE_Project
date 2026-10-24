#include "stdafx.h"
#include "ShipDeck.h"
#include <algorithm>
#include <cmath>
#include <array>
#include <queue>

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
        auto overlaps = [&](float left, float right, float front, float back)
        {
            const float x = (std::max)(left, (std::min)(right, p.x));
            const float z = (std::max)(front, (std::min)(back, p.z));
            return std::hypot(p.x - x, p.z - z) <= margin;
        };
        const bool mast = overlaps(-.085f * s, .085f * s, -.025f * s, .145f * s);
        const bool cabin = overlaps(-.46f * s, .46f * s, .775f * s, 1.425f * s);
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

void ShipDeck::Chase(Vec3& point, Vec3 target, float distance)
{
    // All boarding pirates share a small immutable deck grid and one cached target flow.
    struct Navigation
    {
        std::array<bool, 41 * 81> walk = {};
        std::array<int, 41 * 81> flow;
        std::array<std::array<int, 8>, 41 * 81> links;
        int goal = -1;

        Navigation()
        {
            for (int i = 0; i < int(walk.size()); ++i)
            {
                walk[i] = Walkable(Position(i));
            }
            for (int i = 0; i < int(walk.size()); ++i)
            {
                links[i].fill(-1);
                if (!walk[i])
                {
                    continue;
                }
                int n = 0;
                for (int dz = -1; dz <= 1; ++dz)
                {
                    for (int dx = -1; dx <= 1; ++dx)
                    {
                        if (!dx && !dz)
                        {
                            continue;
                        }
                        const int x = i % 41 + dx, z = i / 41 + dz;
                        if (x >= 0 && x < 41 && z >= 0 && z < 81 && walk[z * 41 + x])
                        {
                            const auto p = Position(i), q = Position(z * 41 + x);
                            bool clear = true;
                            for (int sample = 1; sample < 4; ++sample)
                            {
                                const float t = sample / 4.f;
                                clear &= Walkable({p.x + (q.x - p.x) * t, Height(), p.z + (q.z - p.z) * t});
                            }
                            if (clear)
                            {
                                links[i][n] = z * 41 + x;
                            }
                        }
                        ++n;
                    }
                }
            }
            flow.fill(-1);
        }

        Vec3 Position(int i) const
        {
            return {-2 + (i % 41) * .1f, Height(), -4 + (i / 41) * .1f};
        }

        int Closest(Vec3 p) const
        {
            const int x = (std::max)(0, (std::min)(40, int(std::round((p.x + 2) * 10))));
            const int z = (std::max)(0, (std::min)(80, int(std::round((p.z + 4) * 10))));
            if (walk[z * 41 + x])
            {
                return z * 41 + x;
            }
            float best = 1e9f;
            int result = -1;
            for (int i = 0; i < int(walk.size()); ++i)
            {
                if (!walk[i])
                {
                    continue;
                }
                const auto q = Position(i);
                const float d = std::hypot(q.x - p.x, q.z - p.z);
                if (d < best)
                {
                    best = d;
                    result = i;
                }
            }
            return result;
        }

        void Update(int next)
        {
            if (next == goal)
            {
                return;
            }
            goal = next;
            flow.fill(-1);
            if (goal < 0)
            {
                return;
            }
            std::queue<int> queue;
            queue.push(goal);
            flow[goal] = 0;
            while (!queue.empty())
            {
                const int cell = queue.front();
                queue.pop();
                for (int i : links[cell])
                {
                    if (i < 0 || flow[i] >= 0)
                    {
                        continue;
                    }
                    flow[i] = flow[cell] + 1;
                    queue.push(i);
                }
            }
        }
    };

    static Navigation navigation;
    navigation.Update(navigation.Closest(target));
    const int current = navigation.Closest(point);
    if (current < 0 || navigation.flow[current] < 0)
    {
        return;
    }
    int next = current;
    const auto center = navigation.Position(current);
    if (std::hypot(center.x - point.x, center.z - point.z) < .08f)
    {
        for (int i : navigation.links[current])
        {
            if (i >= 0 && navigation.flow[i] >= 0 && navigation.flow[i] < navigation.flow[next])
            {
                next = i;
            }
        }
    }
    const auto destination = navigation.Position(next);
    const float dx = destination.x - point.x, dz = destination.z - point.z, length = std::hypot(dx, dz);
    if (length > .001f)
    {
        Move(point, dx / length * (std::min)(distance, length), dz / length * (std::min)(distance, length));
    }
}

namespace
{
    float PirateEdge(Vec3 p)
    {
        const Vec2 hull[] = {{0, -4}, {1.5f, -3.5f}, {1.5f, 3.5f}, {-1.5f, 3.5f}, {-1.5f, -3.5f}};
        float edge = 1e9f;
        for (int i = 0; i < 5; ++i)
        {
            const auto a = hull[i], b = hull[(i + 1) % 5];
            edge = (std::min)(
                edge,
                ((b.x - a.x) * (p.z - a.y) - (b.y - a.y) * (p.x - a.x)) / std::hypot(b.x - a.x, b.y - a.y));
        }
        return edge;
    }
}

bool PirateDeck::Walkable(Vec3 p)
{
    const bool mast = p.x > -.3f && p.x < .3f && p.z > -.7f && p.z < -.1f;
    const bool cabin = p.x > -1.2f && p.x < 1.2f && p.z > 1.1f && p.z < 3.1f;
    return PirateEdge(p) >= .2f && !mast && !cabin;
}

bool PirateDeck::NearEdge(Vec3 p)
{
    return PirateEdge(p) <= .85f;
}

void PirateDeck::Move(Vec3& p, float dx, float dz)
{
    const int steps = (std::max)(1, int(std::ceil(std::hypot(dx, dz) / .1f)));
    for (int i = 0; i < steps; ++i)
    {
        const Vec3 next = {p.x + dx / steps, Height, p.z + dz / steps};
        if (Walkable(next))
        {
            p = next;
            continue;
        }
        if (Walkable({next.x, Height, p.z}))
        {
            p.x = next.x;
        }
        if (Walkable({p.x, Height, next.z}))
        {
            p.z = next.z;
        }
    }
}

Vec3 PirateDeck::ClosestBoardingPoint(Vec3 outside)
{
    Vec3 best = {0, Height, 0};
    float distance = 1e9f;
    for (float z = -4; z <= 3.5f; z += .1f)
    {
        for (float x = -1.5f; x <= 1.5f; x += .1f)
        {
            const Vec3 p = {x, Height, z};
            if (!Walkable(p))
            {
                continue;
            }
            const float d = std::hypot(p.x - outside.x, p.z - outside.z);
            if (d < distance)
            {
                distance = d;
                best = p;
            }
        }
    }
    return best;
}
