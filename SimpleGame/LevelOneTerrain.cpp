#include "stdafx.h"
#include "LevelOneTerrain.h"
#include <random>
#include <chrono>
#include <queue>
#include <cmath>
#include <algorithm>
#include <utility>

namespace
{
    bool Dock(float x, float z)
    {
        return x >= 58 && x <= 69 && z >= 17 && z <= 18.5f;
    }
}

LevelOneTerrain::LevelOneTerrain() : m_land(Size * Size, 0), m_blocked(Size * Size, 0)
{
    Mesh mesh;
    m_seed = static_cast<std::uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    try
    {
        m_seed ^= std::random_device{}();
    }
    catch (...)
    {
    }
    std::mt19937 random(m_seed);
    const float phase = (random() % 6283) / 1000.f;
    for (int z = 0; z < Size; ++z)
    {
        for (int x = 0; x < Size; ++x)
        {
            const float X = Min + x + .5f, Z = Min + z + .5f;
            const float dx = X - 16, dz = Z - 16;
            const float a = std::atan2(dz, dx);
            const float radius = 45 + 2 * std::sin(a * 3 + phase) + std::cos(a * 5 - phase);
            const int i = z * Size + x;
            m_land[i] = dx * dx + dz * dz < radius * radius;
            // Reserve wide central corridors and the harbor/safe spawn before decorating.
            const bool reserved = std::abs(dx) < 4 || std::abs(dz) < 4 ||
                                  (X > 7 && X < 26 && Z > 7 && Z < 27) ||
                                  (dx * dx + (Z + 12) * (Z + 12) < 64);
            if (m_land[i] && !reserved && random() % 100 < 7)
            {
                m_blocked[i] = 1;
            }
        }
    }
    // Obstacles in the original harbor use the same cells for rendering and collision.
    const float buildings[][4] = {
        {11, 11, 14, 13.8f},
        {16, 10.5f, 18.7f, 13.1f},
        {10, 16, 12.5f, 19},
        {14.5f, 20, 17, 22.4f},
        {19.8f, 12.5f, 21.2f, 13.9f}};
    for (int i = 0; i < Size * Size; ++i)
    {
        const auto p = Center(i);
        for (const auto& b : buildings)
        {
            if (p.x + .5f > b[0] && p.x - .5f < b[2] && p.z + .5f > b[1] && p.z - .5f < b[3])
            {
                m_blocked[i] = 2;
            }
        }
    }
    // Validate reachability from the actual spawn. Remove random blockers if needed.
    std::vector<int> reachable;
    Flow(22, 17.75f, reachable);
    bool disconnected = false;
    for (int i = 0; i < Size * Size; ++i)
    {
        disconnected |= m_land[i] && !m_blocked[i] && reachable[i] < 0;
    }
    if (disconnected)
    {
        for (auto& block : m_blocked)
        {
            if (block == 1)
            {
                block = 0;
            }
        }
        Flow(22, 17.75f, reachable);
    }
    // Unreachable fringe cells are not playable and never receive enemies or loot.
    for (int i = 0; i < Size * Size; ++i)
    {
        if (m_land[i] && !m_blocked[i] && reachable[i] < 0)
        {
            m_blocked[i] = 1;
        }
        const auto p = Center(i);
        if (reachable[i] >= 0 && Walk(p.x, p.z))
        {
            m_spawnCells.push_back(i);
        }
    }
    using namespace Geometry;
    for (int z = 0; z < Size; ++z)
    {
        for (int x = 0; x < Size; ++x)
        {
            const int i = z * Size + x;
            if (!m_land[i])
            {
                continue;
            }
            const float X = float(Min + x), Z = float(Min + z);
            const float radius = std::sqrt((X - 16) * (X - 16) + (Z - 16) * (Z - 16));
            const float variation = (random() % 9) * .004f;
            const bool path = std::abs(X - 16) < 2 || std::abs(Z - 17) < 1.5f;
            const Color color = radius > 39 || path ? Color{.86f + variation, .77f + variation, .52f}
                                                    : Color{.43f + variation, .64f + variation, .30f};
            Quad(mesh, {X, .55f, Z}, {X + 1, .55f, Z}, {X + 1, .55f, Z + 1}, {X, .55f, Z + 1}, color);
            if (m_blocked[i] == 1)
            {
                Box(mesh, {X, .55f, Z}, {1, .7f + variation * 8, 1}, {.47f, .53f, .42f});
            }
            for (const auto& d : {std::pair<int, int>{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
            {
                const int nx = x + d.first, nz = z + d.second;
                if (nx >= 0 && nz >= 0 && nx < Size && nz < Size && m_land[nz * Size + nx])
                {
                    continue;
                }
                Box(mesh, {X - .12f, .015f, Z - .12f}, {1.24f, .03f, 1.24f}, {.26f, .68f, .66f});
                if (d.first)
                {
                    const float edge = X + (d.first > 0 ? 1 : 0);
                    Quad(
                        mesh,
                        {edge, 0, Z},
                        {edge, .55f, Z},
                        {edge, .55f, Z + 1},
                        {edge, 0, Z + 1},
                        {.72f, .64f, .43f});
                }
                else
                {
                    const float edge = Z + (d.second > 0 ? 1 : 0);
                    Quad(
                        mesh,
                        {X, 0, edge},
                        {X, .55f, edge},
                        {X + 1, .55f, edge},
                        {X + 1, 0, edge},
                        {.72f, .64f, .43f});
                }
            }
        }
    }
    for (int i = 0; i < 34; ++i)
    {
        Box(mesh, {58 + i * .32f, .37f, 17}, {.30f, .18f, 1.5f}, {.59f, .39f, .22f});
    }
    // A recognizable, obstacle-free boss clearing.
    Disc(mesh, {16, .56f, -12}, 6, {.74f, .69f, .46f}, 48);
    m_mesh = CachedMesh(std::move(mesh));
}

int LevelOneTerrain::Index(float x, float z) const
{
    const int X = static_cast<int>(std::floor(x)) - Min;
    const int Z = static_cast<int>(std::floor(z)) - Min;
    return X >= 0 && Z >= 0 && X < Size && Z < Size ? Z * Size + X : -1;
}

Vec3 LevelOneTerrain::Center(int index) const
{
    return {Min + index % Size + .5f, .55f, Min + index / Size + .5f};
}

bool LevelOneTerrain::Land(float x, float z) const
{
    const int i = Index(x, z);
    return i >= 0 && m_land[i];
}

bool LevelOneTerrain::CellWalk(int x, int z) const
{
    return x >= 0 && z >= 0 && x < Size && z < Size && m_land[z * Size + x] && !m_blocked[z * Size + x];
}

bool LevelOneTerrain::PointWalk(float x, float z) const
{
    const int i = Index(x, z);
    return Dock(x, z) || (i >= 0 && m_land[i] && !m_blocked[i]);
}

bool LevelOneTerrain::Walk(float x, float z) const
{
    constexpr float radius = .28f;
    return PointWalk(x - radius, z - radius) && PointWalk(x + radius, z - radius) &&
           PointWalk(x - radius, z + radius) && PointWalk(x + radius, z + radius);
}

bool LevelOneTerrain::Segment(float x, float z, float X, float Z) const
{
    const int steps = (std::max)(1, int(std::ceil(std::hypot(X - x, Z - z) / .15f)));
    for (int i = 0; i <= steps; ++i)
    {
        const float t = float(i) / steps;
        if (!Walk(x + (X - x) * t, z + (Z - z) * t))
        {
            return false;
        }
    }
    return true;
}

void LevelOneTerrain::Flow(float x, float z, std::vector<int>& distance) const
{
    distance.assign(Size * Size, -1);
    const int start = Index(x, z);
    if (start < 0 || !CellWalk(start % Size, start / Size))
    {
        return;
    }
    std::queue<int> queue;
    queue.push(start);
    distance[start] = 0;
    while (!queue.empty())
    {
        const int i = queue.front();
        queue.pop();
        for (const auto& d : {std::pair<int, int>{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
        {
            const int X = i % Size + d.first, Z = i / Size + d.second;
            if (!CellWalk(X, Z))
            {
                continue;
            }
            const int next = Z * Size + X;
            if (distance[next] >= 0)
            {
                continue;
            }
            distance[next] = distance[i] + 1;
            queue.push(next);
        }
    }
}

void LevelOneTerrain::Render(Renderer& renderer, Vec3 offset) const
{
    renderer.Submit(m_mesh, offset);
}

void LevelOneTerrain::Map(Renderer& renderer, float left, float top, float extent) const
{
    const float size = extent / Size;
    for (int i = 0; i < Size * Size; ++i)
    {
        if (m_land[i])
        {
            renderer.Panel(
                left + (i % Size) * size,
                top + (i / Size) * size,
                size,
                size,
                m_blocked[i] ? Color{.36f, .41f, .31f} : Color{.61f, .72f, .40f});
        }
    }
}
