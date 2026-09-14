#include "stdafx.h"
#include "World.h"
#include <cmath>
#include <limits>
#include <algorithm>

namespace
{
    constexpr float Pi = 3.14159265359f;
    constexpr float Ground = .55f;
    using namespace Geometry;

    std::uint64_t Mix(std::uint64_t v)
    {
        v += 0x9e3779b97f4a7c15ULL;
        v = (v ^ (v >> 30)) * 0xbf58476d1ce4e5b9ULL;
        v = (v ^ (v >> 27)) * 0x94d049bb133111ebULL;
        return v ^ (v >> 31);
    }

    float Random(std::uint64_t& state)
    {
        state = Mix(state);
        return static_cast<float>(state >> 40) / 16777216.f;
    }

    bool Dock(const WorldPosition& p, float margin)
    {
        WorldPosition origin;
        origin.x = 0;
        origin.z = 0;
        const auto v = p.RelativeTo(origin);
        return v.x >= 58 + margin && v.x <= 69 - margin && v.z >= 17 + margin && v.z <= 18.5f - margin;
    }

    struct House
    {
        float x, z, w, d, h;
        Color roof;
    };

    const House Houses[] = {
        {11, 11, 3, 2.8f, 2.4f, {.78f, .29f, .19f}},
        {16, 10.5f, 2.7f, 2.6f, 2, {.88f, .41f, .24f}},
        {10, 16, 2.5f, 3, 1.9f, {.19f, .43f, .52f}},
        {14.5f, 20, 2.5f, 2.4f, 1.8f, {.83f, .36f, .23f}}};

    void HouseMesh(Mesh& m, const House& h)
    {
        float x = h.x, z = h.z, w = h.w, d = h.d, y = Ground + h.h;
        Box(m, {x, Ground, z}, {w, h.h, d}, {.94f, .85f, .63f});
        // Gabled roof with an overhang and separate illuminated faces.
        float X = x + w + .2f, Z = z + d + .2f;
        x -= .2f;
        z -= .2f;
        Quad(m, {x, y, z}, {X, y, z}, {X, y + 1, z + d * .5f + .2f}, {x, y + 1, z + d * .5f + .2f}, h.roof);
        Quad(
            m,
            {x, y + 1, z + d * .5f + .2f},
            {X, y + 1, z + d * .5f + .2f},
            {X, y, Z},
            {x, y, Z},
            {h.roof.r * .8f, h.roof.g * .8f, h.roof.b * .8f});
        Triangle(m, {X, y, z}, {X, y, Z}, {X, y + 1, z + d * .5f + .2f}, {.9f, .76f, .51f});
        Triangle(m, {x, y, z}, {x, y, Z}, {x, y + 1, z + d * .5f + .2f}, {.75f, .65f, .47f});
        Box(m, {h.x + .35f, Ground + .7f, h.z + h.d + .015f}, {.55f, .65f, .04f}, {.10f, .31f, .36f});
        Box(m, {h.x + h.w - .85f, Ground, h.z + h.d + .025f}, {.6f, 1.25f, .05f}, {.35f, .21f, .13f});
        Box(m, {h.x + .4f, y + .1f, h.z + .5f}, {.35f, 1.1f, .4f}, {.82f, .71f, .56f});
    }

    void Palm(Mesh& m, float x, float z, float height, float phase)
    {
        Box(m, {x - .13f, Ground, z - .13f}, {.26f, height, .26f}, {.54f, .37f, .20f});
        for (int n = 0; n < 6; ++n)
        {
            float a = phase + n * Pi / 3, dx = std::cos(a), dz = std::sin(a);
            Vec3 tip = {x + dx * 1.8f, Ground + height - .4f, z + dz * 1.8f};
            Vec3 top = {x, Ground + height + .3f, z};
            Vec3 l = {x + dx * .8f - dz * .35f, Ground + height + .18f, z + dz * .8f + dx * .35f};
            Vec3 r = {x + dx * .8f + dz * .35f, Ground + height + .18f, z + dz * .8f - dx * .35f};
            Triangle(m, top, l, tip, {.18f, .52f, .28f});
            Triangle(m, top, tip, r, {.29f, .64f, .31f});
        }
    }
}

// Definition for pre-C++17 toolchains when Seed is passed by reference.
constexpr std::uint64_t World::Seed;

void WorldPosition::Move(double dx, double dz)
{
    // Only small local steps are accepted; normalization supports negative chunks.
    auto axis = [](std::int64_t& chunk, double& local, double delta)
    {
        const double next = local + delta;
        if (!std::isfinite(next) || std::abs(delta) > ChunkSize * 1000)
        {
            return;
        }
        const auto shift = static_cast<std::int64_t>(std::floor(next / ChunkSize));
        const auto limit = (std::numeric_limits<std::int64_t>::max)() - 1024;
        if ((shift > 0 && chunk > limit - shift) || (shift < 0 && chunk < -limit - shift))
        {
            return;
        }
        chunk += shift;
        local = next - static_cast<double>(shift) * ChunkSize;
    };
    axis(chunkX, x, dx);
    axis(chunkZ, z, dz);
}

Vec3 WorldPosition::RelativeTo(const WorldPosition& origin) const
{
    // Subtract integers first for nearby chunks; never convert global positions to float.
    auto relative = [](std::int64_t a, std::int64_t b, double local)
    {
        // Unsigned subtraction also handles endpoints without signed overflow.
        const std::uint64_t distance =
            a >= b ? std::uint64_t(a) - std::uint64_t(b) : std::uint64_t(b) - std::uint64_t(a);
        if (distance > 1000)
        {
            return a >= b ? 100000.f : -100000.f;
        }
        return static_cast<float>((a >= b ? 1.0 : -1.0) * double(distance) * ChunkSize + local);
    };
    return {relative(chunkX, origin.chunkX, x - origin.x), 0, relative(chunkZ, origin.chunkZ, z - origin.z)};
}

float IslandChunk::ShoreRadius(float a) const
{
    return radius * (1 + .065f * std::sin(3 * a + phase) + .035f * std::cos(5 * a - phase));
}

IslandChunk World::Generate(std::int64_t cx, std::int64_t cz)
{
    std::uint64_t state =
        Mix(Seed ^ Mix(static_cast<std::uint64_t>(cx)) ^
            (Mix(static_cast<std::uint64_t>(cz) + 1337) * 0x9e3779b97f4a7c15ULL));
    IslandChunk c;
    Mesh terrain, scenery;
    c.harbor = (cx == 0 && cz == 0);
    c.hasIsland = c.harbor || Random(state) > .30f;
    if (!c.hasIsland)
    {
        return c;
    }
    // The tutorial spans several chunks; do not overlap it with small generated islands.
    if (!c.harbor && cx >= -2 && cx <= 2 && cz >= -2 && cz <= 2)
    {
        c.hasIsland = false;
        return c;
    }
    c.centerX = 13 + Random(state) * 6;
    c.centerZ = 13 + Random(state) * 6;
    c.radius = 5 + Random(state) * 3.5f;
    c.phase = Random(state) * 2 * Pi;
    c.ruin = Random(state) > .7f;
    if (c.harbor)
    {
        c.centerX = 16;
        c.centerZ = 16;
        c.radius = 9;
        c.phase = 0;
        c.ruin = false;
    }
    // Polygon rings share angular samples; the land collision is kept inside the edge.
    const float scales[] = {1.30f, 1.12f, 1.f, .81f};
    const float heights[] = {.012f, .025f, Ground, Ground + .01f};
    const Color colors[] = {{.13f, .61f, .64f}, {.33f, .76f, .71f}, {.90f, .80f, .56f}, {.46f, .66f, .31f}};
    for (int ring = 0; ring < 4; ++ring)
    {
        for (int i = 0; i < 64; ++i)
        {
            const float a = 2 * Pi * i / 64, b = 2 * Pi * (i + 1) / 64;
            const float ra = c.ShoreRadius(a) * scales[ring], rb = c.ShoreRadius(b) * scales[ring];
            Triangle(
                terrain,
                {c.centerX, heights[ring], c.centerZ},
                {c.centerX + std::cos(a) * ra, heights[ring], c.centerZ + std::sin(a) * ra},
                {c.centerX + std::cos(b) * rb, heights[ring], c.centerZ + std::sin(b) * rb},
                colors[ring]);
        }
    }
    // Sloped sand joins the raised land to the shallow water.
    for (int i = 0; i < 64; ++i)
    {
        const float a = 2 * Pi * i / 64, b = 2 * Pi * (i + 1) / 64;
        const float ra = c.ShoreRadius(a), rb = c.ShoreRadius(b);
        Quad(
            terrain,
            {c.centerX + std::cos(a) * ra, Ground, c.centerZ + std::sin(a) * ra},
            {c.centerX + std::cos(b) * rb, Ground, c.centerZ + std::sin(b) * rb},
            {c.centerX + std::cos(b) * rb * 1.055f, .03f, c.centerZ + std::sin(b) * rb * 1.055f},
            {c.centerX + std::cos(a) * ra * 1.055f, .03f, c.centerZ + std::sin(a) * ra * 1.055f},
            {.77f, .69f, .47f});
    }
    if (c.harbor)
    {
        Quad(
            scenery,
            {12, Ground + .022f, 17},
            {23, Ground + .022f, 17},
            {23, Ground + .022f, 18.5f},
            {12, Ground + .022f, 18.5f},
            {.84f, .74f, .51f});
        for (const auto& h : Houses)
        {
            HouseMesh(scenery, h);
        }
        for (int i = 0; i < 28; ++i)
        {
            Box(scenery, {21 + i * .32f, .37f, 17}, {.29f, .18f, 1.5f}, {.59f, .39f, .22f});
        }
        for (int i = 0; i < 5; ++i)
        {
            for (float z : {17.05f, 18.25f})
            {
                Box(scenery, {22 + i * 1.7f, -.15f, z}, {.18f, 1.02f, .18f}, {.38f, .25f, .16f});
            }
        }
        for (int i = 0; i < 3; ++i)
        {
            Box(scenery, {21.f + i * .65f, Ground, 16.2f}, {.55f, .55f, .55f}, {.64f, .44f, .24f});
        }
        // Lighthouse.
        Box(scenery, {19.8f, Ground, 12.5f}, {1.4f, 3.8f, 1.4f}, {.93f, .88f, .70f});
        Box(scenery, {19.72f, 3.1f, 12.42f}, {1.56f, .45f, 1.56f}, {.82f, .32f, .22f});
        Box(scenery, {19.95f, 4.35f, 12.65f}, {1.1f, .75f, 1.1f}, {.99f, .77f, .32f});
        Cone(scenery, {20.5f, 5.1f, 13.2f}, 1.15f, .8f, {.19f, .34f, .40f}, 4);
        Palm(scenery, 11, 21, 3.1f, 0);
        Palm(scenery, 14, 9, 3.4f, 1);
        Palm(scenery, 22, 16, 2.9f, 2);
        Palm(scenery, 18, 23, 3.1f, 3);
    }
    else
    {
        for (int i = 0; i < 7; ++i)
        {
            const float a = Random(state) * 2 * Pi, r = (.25f + Random(state) * .4f) * c.radius;
            Palm(
                scenery,
                c.centerX + std::cos(a) * r,
                c.centerZ + std::sin(a) * r,
                2.2f + Random(state) * 1.4f,
                a);
        }
        if (c.ruin)
        {
            for (int i = -1; i <= 1; i += 2)
            {
                Box(scenery,
                    {c.centerX + i * 1.25f - .25f, Ground, c.centerZ},
                    {.5f, 2.8f, .65f},
                    {.64f, .68f, .58f});
            }
            Box(scenery, {c.centerX - 1.5f, Ground + 2.8f, c.centerZ}, {3, .45f, .65f}, {.78f, .79f, .65f});
            Cone(scenery, {c.centerX, Ground, c.centerZ + 1.8f}, .55f, 1, {.34f, .79f, .77f}, 4);
        }
        else
        {
            Cone(scenery, {c.centerX, Ground, c.centerZ}, c.radius * .38f, 2, {.49f, .60f, .36f}, 7);
        }
    }
    c.terrain = CachedMesh(std::move(terrain));
    c.scenery = CachedMesh(std::move(scenery));
    return c;
}

void World::Update(const WorldPosition& p, int radius)
{
    const Key center = {p.chunkX, p.chunkZ};
    if (m_radius == radius && m_center == center)
    {
        return;
    }
    m_radius = radius;
    m_center = center;
    for (auto i = m_chunks.begin(); i != m_chunks.end();)
    {
        if (i->first.first < p.chunkX - radius || i->first.first > p.chunkX + radius ||
            i->first.second < p.chunkZ - radius || i->first.second > p.chunkZ + radius)
        {
            i = m_chunks.erase(i);
        }
        else
        {
            ++i;
        }
    }
    for (int z = -radius; z <= radius; ++z)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            Key key = {p.chunkX + x, p.chunkZ + z};
            if (m_chunks.find(key) == m_chunks.end())
            {
                m_chunks.emplace(key, Generate(key.first, key.second));
            }
        }
    }
}

const IslandChunk* World::At(const WorldPosition& p) const
{
    auto found = m_chunks.find({p.chunkX, p.chunkZ});
    return found == m_chunks.end() ? nullptr : &found->second;
}

bool World::LandAt(const WorldPosition& p, float margin) const
{
    if (p.chunkX >= -2 && p.chunkX <= 2 && p.chunkZ >= -2 && p.chunkZ <= 2)
    {
        WorldPosition origin;
        origin.x = 0;
        origin.z = 0;
        const auto v = p.RelativeTo(origin);
        if (m_tutorial.Land(v.x, v.z))
        {
            return true;
        }
        if (margin > 0)
        {
            for (int i = 0; i < 8; ++i)
            {
                const float a = 2 * Pi * i / 8;
                if (m_tutorial.Land(v.x + margin * std::cos(a), v.z + margin * std::sin(a)))
                {
                    return true;
                }
            }
        }
        return false;
    }
    const auto c = At(p);
    if (!c || !c->hasIsland)
    {
        return false;
    }
    const float dx = static_cast<float>(p.x) - c->centerX, dz = static_cast<float>(p.z) - c->centerZ;
    return std::sqrt(dx * dx + dz * dz) < c->ShoreRadius(std::atan2(dz, dx)) + margin;
}

bool World::CanWalk(const WorldPosition& p) const
{
    if (p.chunkX >= -2 && p.chunkX <= 2 && p.chunkZ >= -2 && p.chunkZ <= 2)
    {
        WorldPosition origin;
        origin.x = 0;
        origin.z = 0;
        const auto v = p.RelativeTo(origin);
        return m_tutorial.Walk(v.x, v.z);
    }
    if (!At(p))
    {
        return false;
    }
    if (Dock(p, .18f))
    {
        return true;
    }
    if (!LandAt(p, -.3f))
    {
        return false;
    }
    const auto c = At(p);
    if (c->harbor)
    {
        for (auto h : Houses)
        {
            if (p.x > h.x - .25 && p.x < h.x + h.w + .25 && p.z > h.z - .25 && p.z < h.z + h.d + .25)
            {
                return false;
            }
        }
        if (p.x > 19.55 && p.x < 21.45 && p.z > 12.25 && p.z < 14.15)
        {
            return false;
        }
    }
    else
    {
        const float dx = static_cast<float>(p.x) - c->centerX, dz = static_cast<float>(p.z) - c->centerZ;
        if (!c->ruin && dx * dx + dz * dz < c->radius * c->radius * .16f)
        {
            return false;
        }
        if (c->ruin && std::abs(dz) < .95f && std::abs(std::abs(dx) - 1.25f) < .55f)
        {
            return false;
        }
    }
    return true;
}

bool World::CanSail(const WorldPosition& p) const
{
    if (!At(p) || LandAt(p, .2f) || Dock(p, -.1f))
    {
        return false;
    }
    for (int i = 0; i < 12; ++i)
    {
        WorldPosition edge = p;
        edge.Move(1.2 * std::cos(2 * Pi * i / 12), 1.2 * std::sin(2 * Pi * i / 12));
        if (!At(edge) || LandAt(edge, .15f) || Dock(edge, 0))
        {
            return false;
        }
    }
    return true;
}

void World::Render(Renderer& r, const WorldPosition& camera, float time, bool grid) const
{
    const float seaExtent = static_cast<float>((m_radius + 2) * ChunkSize);
    Mesh water;
    Quad(
        water,
        {-seaExtent, -.04f, -seaExtent},
        {seaExtent, -.04f, -seaExtent},
        {seaExtent, -.04f, seaExtent},
        {-seaExtent, -.04f, seaExtent},
        {.055f, .40f, .51f});
    r.Submit(water);
    for (const auto& entry : m_chunks)
    {
        WorldPosition origin;
        origin.chunkX = entry.first.first;
        origin.chunkZ = entry.first.second;
        origin.x = 0;
        origin.z = 0;
        const Vec3 offset = origin.RelativeTo(camera);
        Mesh surface;
        // Waves remain anchored to chunk-local coordinates across origin shifts.
        for (int z = 0; z < 8; ++z)
        {
            for (int x = 0; x < 8; ++x)
            {
                const float phase = time * .8f + x * .75f + z * 1.2f;
                const float px = x * 4.f + 1 + std::sin(phase) * .35f, pz = z * 4.f + 1.5f;
                const float light = .5f + .5f * std::sin(phase);
                Quad(
                    surface,
                    {px, 0, pz},
                    {px + 1.1f + light * .7f, 0, pz},
                    {px + 1.3f + light * .7f, 0, pz + .065f},
                    {px + .2f, 0, pz + .065f},
                    {.09f + light * .055f, .47f + light * .08f, .57f + light * .07f});
            }
        }
        if (grid)
        {
            Quad(
                surface,
                {0, .65f, 0},
                {32, .65f, 0},
                {32, .65f, .035f},
                {0, .65f, .035f},
                {.98f, .71f, .29f});
            Quad(
                surface,
                {0, .65f, 0},
                {.035f, .65f, 0},
                {.035f, .65f, 32},
                {0, .65f, 32},
                {.98f, .71f, .29f});
        }
        r.Submit(surface, offset);
        if (!entry.second.harbor)
        {
            r.Submit(entry.second.terrain, offset);
        }
        r.Submit(entry.second.scenery, offset);
    }
    WorldPosition origin;
    origin.x = 0;
    origin.z = 0;
    const auto offset = origin.RelativeTo(camera);
    if (std::abs(offset.x) < 400 && std::abs(offset.z) < 400)
    {
        m_tutorial.Render(r, offset);
    }
}
