#include "stdafx.h"
#include "IslandLayout.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <stdexcept>

namespace
{
    static_assert(
        IslandGeneration::SectorChunks * ChunkSize - ChunkSize - 2 * MapCatalog::MaximumIslandSize >=
            MapCatalog::MinimumIslandGap,
        "Sector spacing must preserve the minimum shoreline gap.");
    constexpr float Pi = 3.14159265359f;

    float Cross(Vec2 a, Vec2 b, Vec2 c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    float Width(std::vector<Vec2> points)
    {
        std::sort(
            points.begin(),
            points.end(),
            [](Vec2 a, Vec2 b)
            {
                return a.x < b.x || (a.x == b.x && a.y < b.y);
            });
        points.erase(
            std::unique(
                points.begin(),
                points.end(),
                [](Vec2 a, Vec2 b)
                {
                    return a.x == b.x && a.y == b.y;
                }),
            points.end());
        std::vector<Vec2> hull;
        for (auto p : points)
        {
            while (hull.size() >= 2 && Cross(hull[hull.size() - 2], hull.back(), p) <= 0)
            {
                hull.pop_back();
            }
            hull.push_back(p);
        }
        const size_t lower = hull.size();
        for (auto it = points.rbegin() + 1; it != points.rend(); ++it)
        {
            while (hull.size() > lower && Cross(hull[hull.size() - 2], hull.back(), *it) <= 0)
            {
                hull.pop_back();
            }
            hull.push_back(*it);
        }
        float maximum = 0;
        for (auto a : hull)
        {
            for (auto b : hull)
            {
                maximum = (std::max)(maximum, (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
            }
        }
        return std::sqrt(maximum);
    }

    bool Shape(IslandShape shape, float x, float z, float phase)
    {
        const float rr = x * x + z * z;
        switch (shape)
        {
            case IslandShape::Round:
                return rr < .95f * .95f;
            case IslandShape::Crescent:
                return rr < .95f * .95f && (x - .38f) * (x - .38f) + z * z > .73f * .73f;
            case IslandShape::UShape:
                return x * x / (.88f * .88f) + z * z / (.95f * .95f) < 1 && !(std::abs(x) < .43f && z < .48f);
            case IslandShape::Dumbbell:
                return (x + .53f) * (x + .53f) + z * z < .42f * .42f ||
                       (x - .53f) * (x - .53f) + z * z < .42f * .42f ||
                       (std::abs(x) < .54f && std::abs(z) < .14f);
            case IslandShape::Irregular:
            {
                const float a = std::atan2(z, x);
                const float r = .78f + .10f * std::sin(a * 3 + phase) + .07f * std::cos(a * 5 - phase);
                return rr < r * r;
            }
            case IslandShape::Teardrop:
                return x * x / (.72f * .72f) + (z + .22f) * (z + .22f) / (.68f * .68f) < 1 ||
                       (z >= -.15f && z < .95f && std::abs(x) < (.95f - z) * .65f);
            case IslandShape::Split:
                return std::abs(x) > .12f && x * x / (.9f * .9f) + z * z / (.82f * .82f) < 1;
            case IslandShape::Reef:
            {
                const Vec3 reefs[] = {
                    {-.60f, .23f, -.10f},
                    {.48f, .23f, .43f},
                    {-.15f, .21f, .63f},
                    {-.50f, .20f, -.62f},
                    {.43f, .24f, -.48f},
                    {0, .20f, 0},
                    {.70f, .17f, 0}};
                for (auto p : reefs)
                {
                    if ((x - p.x) * (x - p.x) + (z - p.z) * (z - p.z) < p.y * p.y)
                    {
                        return true;
                    }
                }
                return false;
            }
        }
        return false;
    }
}

std::uint64_t IslandGeneration::Mix(std::uint64_t v)
{
    v += 0x9e3779b97f4a7c15ULL;
    v = (v ^ (v >> 30)) * 0xbf58476d1ce4e5b9ULL;
    v = (v ^ (v >> 27)) * 0x94d049bb133111ebULL;
    return v ^ (v >> 31);
}

IslandKey IslandGeneration::Sector(std::int64_t x, std::int64_t z)
{
    auto floorDiv = [](std::int64_t value)
    {
        const auto q = value / SectorChunks, r = value % SectorChunks;
        return q - (r < 0 ? 1 : 0);
    };
    return {floorDiv(x), floorDiv(z)};
}

IslandDescriptor IslandGeneration::Describe(IslandKey key)
{
    IslandDescriptor d;
    d.key = key;
    d.seed = Mix(
        WorldSeed ^ Mix(static_cast<std::uint64_t>(key.x)) ^ Mix(static_cast<std::uint64_t>(key.z) + 1337));
    d.starter = key.x == 0 && key.z == 0;
    d.shape =
        d.starter ? IslandShape::Round : MapCatalog::SelectShape(static_cast<unsigned>(Mix(d.seed) % 100));
    d.width = d.starter ? MapCatalog::MaximumIslandSize
                        : MapCatalog::MinimumIslandSize + static_cast<float>(Mix(d.seed + 1) % 41);
    d.center.chunkX = key.x * SectorChunks;
    d.center.chunkZ = key.z * SectorChunks;
    d.center.x = d.starter ? 16 : .5 + (Mix(d.seed + 2) % 3100) / 100.0;
    d.center.z = d.starter ? 16 : .5 + (Mix(d.seed + 3) % 3100) / 100.0;
    return d;
}

IslandLayout::IslandLayout(IslandDescriptor descriptor)
    : m_info(descriptor), m_land(Size * Size, 0), m_village(Size * Size, 0), m_blocked(Size * Size, 0)
{
    std::vector<Vec2> corners;
    const unsigned rotation =
        m_info.starter ? 0 : static_cast<unsigned>(IslandGeneration::Mix(m_info.seed + 4) % 4);
    const float phase = static_cast<float>(IslandGeneration::Mix(m_info.seed + 5) % 6283) / 1000.f;
    for (int z = 0; z < Size; ++z)
    {
        for (int x = 0; x < Size; ++x)
        {
            float X = (x + .5f - Size * .5f) * 2 / Size, Z = (z + .5f - Size * .5f) * 2 / Size;
            for (unsigned r = 0; r < rotation; ++r)
            {
                const float previous = X;
                X = -Z;
                Z = previous;
            }
            if (!Shape(m_info.shape, X, Z, phase))
            {
                continue;
            }
            m_land[z * Size + x] = 1;
            ++m_landCells;
            corners.insert(
                corners.end(),
                {{float(x), float(z)},
                 {float(x + 1), float(z)},
                 {float(x + 1), float(z + 1)},
                 {float(x), float(z + 1)}});
        }
    }
    if (!m_landCells)
    {
        throw std::runtime_error("Island generator produced empty land.");
    }
    if (m_info.width < MapCatalog::MinimumIslandSize || m_info.width > MapCatalog::MaximumIslandSize)
    {
        throw std::runtime_error("Island width is outside map.md bounds.");
    }
    // Maximal distance between hull vertices, including shoreline tile corners.
    m_cellSize = m_info.width / Width(std::move(corners));

    // This is the conservative radius used by sector spacing, not the island size metric.
    for (int i = 0; i < Size * Size; ++i)
    {
        if (!m_land[i])
        {
            continue;
        }
        const auto p = Center(i);
        for (float dx : {-.5f, .5f})
        {
            for (float dz : {-.5f, .5f})
            {
                if (std::hypot(p.x + dx * m_cellSize, p.z + dz * m_cellSize) > MapCatalog::MaximumIslandSize)
                {
                    throw std::runtime_error("Island exceeds conservative placement bound.");
                }
            }
        }
    }
    std::vector<unsigned char> seen(Size * Size, 0);
    std::vector<int> largest;
    for (int i = 0; i < Size * Size; ++i)
    {
        if (!m_land[i] || seen[i])
        {
            continue;
        }
        std::vector<int> component;
        std::queue<int> queue;
        queue.push(i);
        seen[i] = 1;
        while (!queue.empty())
        {
            const int cell = queue.front();
            queue.pop();
            component.push_back(cell);
            for (const auto& delta : {std::pair<int, int>{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
            {
                const int x = cell % Size + delta.first, z = cell / Size + delta.second;
                if (!CellLand(x, z) || seen[z * Size + x])
                {
                    continue;
                }
                seen[z * Size + x] = 1;
                queue.push(z * Size + x);
            }
        }
        if (component.size() > largest.size())
        {
            largest = std::move(component);
        }
    }
    int shore = largest.front();
    float best = -1e9f;
    for (int i : largest)
    {
        const int x = i % Size, z = i / Size;
        const auto p = Center(i);
        for (const auto& d : {std::pair<int, int>{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
        {
            if (CellLand(x + d.first, z + d.second))
            {
                continue;
            }
            bool openWater = true;
            for (float t = m_cellSize * .6f; t < PierLength() + 21; t += m_cellSize * .5f)
            {
                openWater &= !Land(p.x + d.first * t, p.z + d.second * t);
            }
            if (!openWater)
            {
                continue;
            }
            const float score = p.x - std::abs(p.z) * .7f;
            if (score > best)
            {
                best = score;
                shore = i;
                m_pierDirection = {float(d.first), 0, float(d.second)};
            }
        }
    }
    m_pierStart = Center(shore);
    // Connected town flood: its actual land-cell area is never above 20%.
    std::queue<int> village;
    village.push(shore);
    m_village[shore] = 1;
    m_villageCells = 1;
    const size_t cap = m_landCells / 5; // Integer form of the confirmed 20% cap.
    while (!village.empty() && m_villageCells < cap)
    {
        const int cell = village.front();
        village.pop();
        for (const auto& delta : {std::pair<int, int>{-1, 0}, {0, 1}, {0, -1}, {1, 0}})
        {
            const int x = cell % Size + delta.first, z = cell / Size + delta.second;
            if (!CellLand(x, z) || m_village[z * Size + x] || m_villageCells >= cap)
            {
                continue;
            }
            m_village[z * Size + x] = 1;
            ++m_villageCells;
            village.push(z * Size + x);
        }
    }
    Vec3 mean = {};
    for (int i = 0; i < Size * Size; ++i)
    {
        if (m_village[i])
        {
            const auto p = Center(i);
            mean.x += p.x;
            mean.z += p.z;
        }
    }
    mean.x /= m_villageCells;
    mean.z /= m_villageCells;
    float villageDistance = 1e9f;
    for (int i = 0; i < Size * Size; ++i)
    {
        if (!m_village[i])
        {
            continue;
        }
        const auto p = Center(i);
        const float distance = std::hypot(p.x - mean.x, p.z - mean.z);
        if (distance < villageDistance)
        {
            villageDistance = distance;
            m_villageCenter = p;
        }
    }
    // Reserve two traversable service plots inside the existing village mask.
    // This does not enlarge the town beyond its confirmed 20% area cap.
    std::vector<Vec3> sites;
    for (int i = 0; i < Size * Size; ++i)
    {
        const auto p = Center(i);
        if (!Village(p.x, p.z) || !Walk(p.x, p.z))
        {
            continue;
        }
        bool fits = true;
        const int x0 = static_cast<int>(std::floor((p.x - 1.25f - Origin()) / m_cellSize));
        const int x1 = static_cast<int>(std::floor((p.x + 1.25f - Origin()) / m_cellSize));
        const int z0 = static_cast<int>(std::floor((p.z - 2.f - Origin()) / m_cellSize));
        const int z1 = static_cast<int>(std::floor((p.z + 1.f - Origin()) / m_cellSize));
        for (int z = z0; z <= z1; ++z)
        {
            for (int x = x0; x <= x1; ++x)
            {
                fits &= CellLand(x, z) && m_village[z * Size + x] != 0;
            }
        }
        if (fits)
        {
            sites.push_back(p);
        }
    }
    if (sites.size() < 2)
    {
        throw std::runtime_error("Village cannot fit two service plots.");
    }
    m_shipyard = sites.front();
    for (auto p : sites)
    {
        if (std::hypot(p.x - m_pierStart.x, p.z - m_pierStart.z) <
            std::hypot(m_shipyard.x - m_pierStart.x, m_shipyard.z - m_pierStart.z))
        {
            m_shipyard = p;
        }
    }
    m_shop = m_shipyard;
    for (auto p : sites)
    {
        if (std::hypot(p.x - m_shipyard.x, p.z - m_shipyard.z) >
            std::hypot(m_shop.x - m_shipyard.x, m_shop.z - m_shipyard.z))
        {
            m_shop = p;
        }
    }
    // Place shared house models only when the entire footprint lies inside the village.
    for (int i = 0; i < Size * Size && m_houses.size() < 4; ++i)
    {
        const auto p = Center(i);
        if (!Village(p.x, p.z) || std::hypot(p.x - m_pierStart.x, p.z - m_pierStart.z) < 4 ||
            std::hypot(p.x - m_villageCenter.x, p.z - m_villageCenter.z) < 4 ||
            std::hypot(p.x - m_shop.x, p.z - m_shop.z) < 3.5f ||
            std::hypot(p.x - m_shipyard.x, p.z - m_shipyard.z) < 3.5f)
        {
            continue;
        }
        bool allowed = true;
        const int minX = static_cast<int>(std::floor((p.x - 1.5f - Origin()) / m_cellSize));
        const int maxX = static_cast<int>(std::floor((p.x + 1.5f - Origin()) / m_cellSize));
        const int minZ = static_cast<int>(std::floor((p.z - 1.3f - Origin()) / m_cellSize));
        const int maxZ = static_cast<int>(std::floor((p.z + 1.3f - Origin()) / m_cellSize));
        for (int z = minZ; z <= maxZ; ++z)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                allowed &= CellLand(x, z) && m_village[z * Size + x] != 0;
            }
        }
        for (auto house : m_houses)
        {
            allowed &= std::hypot(p.x - house.x, p.z - house.z) > 4;
        }
        if (allowed)
        {
            m_houses.push_back(p);
        }
    }
    for (int i = 0; i < Size * Size; ++i)
    {
        const auto p = Center(i);
        m_blocked[i] = Walk(p.x, p.z) ? 0 : 1;
    }
    m_spawn = m_pierStart;
    float spawnDistance = 1e9f;
    for (int i : largest)
    {
        const auto p = Center(i);
        if (Village(p.x, p.z) && Walk(p.x, p.z) && std::hypot(p.x - m_pierStart.x, p.z - m_pierStart.z) > 2)
        {
            const float distance = std::hypot(p.x - m_pierStart.x, p.z - m_pierStart.z);
            if (distance < spawnDistance)
            {
                spawnDistance = distance;
                m_spawn = p;
            }
        }
    }
    for (int i = 0; i < Size * Size; ++i)
    {
        const auto p = Center(i);
        if (Land(p.x, p.z) && !Village(p.x, p.z) && Walk(p.x, p.z))
        {
            m_spawnCells.push_back(i);
        }
    }
}

int IslandLayout::Index(float x, float z) const
{
    const int X = static_cast<int>(std::floor((x - Origin()) / m_cellSize));
    const int Z = static_cast<int>(std::floor((z - Origin()) / m_cellSize));
    return X >= 0 && Z >= 0 && X < Size && Z < Size ? Z * Size + X : -1;
}

Vec3 IslandLayout::Center(int i) const
{
    return {Origin() + (i % Size + .5f) * m_cellSize, .55f, Origin() + (i / Size + .5f) * m_cellSize};
}

bool IslandLayout::CellLand(int x, int z) const
{
    return x >= 0 && z >= 0 && x < Size && z < Size && m_land[z * Size + x] != 0;
}

bool IslandLayout::CellWalk(int x, int z) const
{
    return CellLand(x, z) && !m_blocked[z * Size + x];
}

bool IslandLayout::Land(float x, float z) const
{
    const int i = Index(x, z);
    return i >= 0 && m_land[i] != 0;
}

bool IslandLayout::Village(float x, float z) const
{
    const int i = Index(x, z);
    return i >= 0 && m_village[i] != 0;
}

bool IslandLayout::Building(float x, float z) const
{
    for (auto p : m_houses)
    {
        if (std::abs(x - p.x) < 1.4f && std::abs(z - p.z) < 1.2f)
        {
            return true;
        }
    }
    return false;
}

bool IslandLayout::Dock(float x, float z, float margin) const
{
    const float dx = x - m_pierStart.x, dz = z - m_pierStart.z;
    const float along = dx * m_pierDirection.x + dz * m_pierDirection.z;
    const float across = std::abs(-dx * m_pierDirection.z + dz * m_pierDirection.x);
    return along >= -m_cellSize * .5f + margin && along <= PierLength() - margin && across <= .75f - margin;
}

bool IslandLayout::PointWalk(float x, float z) const
{
    return (Land(x, z) || Dock(x, z)) && !Building(x, z);
}

bool IslandLayout::Walk(float x, float z, float radius) const
{
    for (int i = 0; i < 8; ++i)
    {
        const float a = 2 * Pi * i / 8;
        if (!PointWalk(x + std::cos(a) * radius, z + std::sin(a) * radius))
        {
            return false;
        }
    }
    return PointWalk(x, z);
}

bool IslandLayout::Segment(float x, float z, float X, float Z) const
{
    const int steps = (std::max)(1, static_cast<int>(std::ceil(std::hypot(X - x, Z - z) / .15f)));
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

void IslandLayout::Flow(float x, float z, std::vector<int>& distance) const
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
        for (const auto& delta : {std::pair<int, int>{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
        {
            const int X = i % Size + delta.first, Z = i / Size + delta.second;
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

Vec3 IslandLayout::ShipSpawn(float length) const
{
    const float along = PierLength() + length * .5f + .6f;
    return {m_pierStart.x + m_pierDirection.x * along, 0, m_pierStart.z + m_pierDirection.z * along};
}

void IslandLayout::AppendTerrain(Mesh& mesh, Vec3 offset, float chunkSize) const
{
    using namespace Geometry;
    for (int z = 0; z < Size; ++z)
    {
        for (int x = 0; x < Size; ++x)
        {
            if (!CellLand(x, z))
            {
                continue;
            }
            const float X = Origin() + x * m_cellSize + offset.x, Z = Origin() + z * m_cellSize + offset.z;
            const float l = (std::max)(0.f, X), r = (std::min)(chunkSize, X + m_cellSize);
            const float t = (std::max)(0.f, Z), b = (std::min)(chunkSize, Z + m_cellSize);
            if (l >= r || t >= b)
            {
                continue;
            }
            const bool coast =
                !CellLand(x - 1, z) || !CellLand(x + 1, z) || !CellLand(x, z - 1) || !CellLand(x, z + 1);
            const float variation = (IslandGeneration::Mix(m_info.seed ^ (z * Size + x)) % 9) * .004f;
            const Color color =
                m_village[z * Size + x]
                    ? Color{.84f, .75f, .52f}
                    : (coast ? Color{.89f, .80f, .57f} : Color{.43f + variation, .64f + variation, .30f});
            Quad(mesh, {l, .55f, t}, {r, .55f, t}, {r, .55f, b}, {l, .55f, b}, color);
            if (!CellLand(x - 1, z) && X >= 0 && X < chunkSize)
            {
                Quad(mesh, {X, 0, t}, {X, .55f, t}, {X, .55f, b}, {X, 0, b}, {.77f, .69f, .47f});
            }
            if (!CellLand(x + 1, z) && X + m_cellSize > 0 && X + m_cellSize <= chunkSize)
            {
                Quad(
                    mesh,
                    {X + m_cellSize, 0, t},
                    {X + m_cellSize, .55f, t},
                    {X + m_cellSize, .55f, b},
                    {X + m_cellSize, 0, b},
                    {.77f, .69f, .47f});
            }
            if (!CellLand(x, z - 1) && Z >= 0 && Z < chunkSize)
            {
                Quad(mesh, {l, 0, Z}, {l, .55f, Z}, {r, .55f, Z}, {r, 0, Z}, {.77f, .69f, .47f});
            }
            if (!CellLand(x, z + 1) && Z + m_cellSize > 0 && Z + m_cellSize <= chunkSize)
            {
                Quad(
                    mesh,
                    {l, 0, Z + m_cellSize},
                    {l, .55f, Z + m_cellSize},
                    {r, .55f, Z + m_cellSize},
                    {r, 0, Z + m_cellSize},
                    {.77f, .69f, .47f});
            }
        }
    }
}
