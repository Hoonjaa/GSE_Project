#include "stdafx.h"
#include "MonsterMeshes.h"
#include "MeshCache.h"
#include <cmath>

namespace
{
    using namespace Geometry;
    constexpr float Pi = 3.14159265359f;
    const Color Dark = {.12f, .17f, .18f}, White = {.92f, .93f, .85f}, Leaf = {.25f, .59f, .25f};

    void Eyes(Mesh& m, float x, float y, float z)
    {
        for (int side : {-1, 1})
        {
            Box(m, {side * x - .07f, y, z}, {.14f, .13f, .08f}, White);
            Box(m, {side * x - .025f, y + .03f, z - .015f}, {.05f, .07f, .025f}, Dark);
        }
    }

    void Legs(Mesh& m, Color color, int pairs, float length = .42f)
    {
        for (int i = 0; i < pairs; ++i)
        {
            for (int side : {-1, 1})
            {
                Box(m,
                    {side * .4f - .07f, 0, -.4f + i * .8f / (pairs > 1 ? pairs - 1 : 1)},
                    {.14f, length, .15f},
                    color);
            }
        }
    }

    void Wings(Mesh& m, Color color, float y)
    {
        for (int side : {-1, 1})
        {
            Triangle(m, {0, y, -.25f}, {side * 1.15f, y + .15f, .05f}, {side * .65f, y - .15f, .65f}, color);
            Triangle(
                m,
                {0, y + .02f, -.25f},
                {side * .65f, y - .13f, .65f},
                {side * 1.15f, y + .17f, .05f},
                color);
        }
    }

    Mesh Build(const MonsterDefinition& def)
    {
        Mesh m;
        const auto c = def.color;
        switch (def.form)
        {
            case MonsterForm::Boar:
            case MonsterForm::Wolf:
            case MonsterForm::Rat:
            case MonsterForm::Monkey:
                Legs(m, c, 2);
                Box(m, {-.42f, .35f, -.65f}, {.84f, .5f, 1.25f}, c);
                Box(m, {-.3f, .6f, -.95f}, {.6f, .5f, .45f}, c);
                Cone(m, {-.25f, 1.05f, -.75f}, .16f, .3f, c, 4);
                Cone(m, {.25f, 1.05f, -.75f}, .16f, .3f, c, 4);
                Eyes(m, .16f, .86f, -.975f);
                Box(m, {-.07f, .5f, .55f}, {.14f, .12f, def.form == MonsterForm::Rat ? .8f : .45f}, c);
                if (def.form == MonsterForm::Boar)
                {
                    for (int side : {-1, 1})
                    {
                        Cone(m, {side * .25f, .55f, -1.03f}, .08f, .35f, White, 4);
                    }
                }
                if (def.form == MonsterForm::Monkey)
                {
                    Box(m, {-.28f, .65f, -1.01f}, {.56f, .33f, .08f}, {.88f, .68f, .46f});
                }
                break;
            case MonsterForm::Tree:
            case MonsterForm::WoodGolem:
            case MonsterForm::Vine:
            case MonsterForm::Seaweed:
            case MonsterForm::Mandrake:
                Legs(m, c, 1, .5f);
                Box(m, {-.3f, .35f, -.25f}, {.6f, 1, .5f}, c);
                for (int side : {-1, 1})
                {
                    Box(m, {side * .55f - .1f, .65f, -.1f}, {.2f, .65f, .2f}, c);
                    Cone(m, {side * .4f, 1.1f, 0}, .5f, .7f, Leaf, 6);
                }
                Cone(m, {0, 1.25f, 0}, .65f, .9f, Leaf, 7);
                Eyes(m, .15f, 1, -.275f);
                if (def.form == MonsterForm::Seaweed || def.form == MonsterForm::Vine)
                {
                    for (int i = 0; i < 5; ++i)
                    {
                        Cone(m, {-.4f + i * .2f, .8f, .1f}, .12f, 1.2f + (i % 2) * .3f, c, 5);
                    }
                }
                break;
            case MonsterForm::Bee:
            case MonsterForm::Bat:
            case MonsterForm::Butterfly:
            case MonsterForm::Harpy:
                Cone(m, {0, .45f, 0}, .4f, .85f, c, 8);
                Wings(m, def.form == MonsterForm::Bee ? White : c, 1.1f);
                Eyes(m, .16f, 1, -.37f);
                if (def.form == MonsterForm::Bee)
                {
                    Box(m, {-.31f, .65f, -.25f}, {.62f, .12f, .52f}, Dark);
                }
                if (def.form == MonsterForm::Harpy)
                {
                    Legs(m, {.78f, .57f, .25f}, 1, .55f);
                    Cone(m, {0, 1.18f, -.35f}, .13f, .3f, {.95f, .72f, .3f}, 4);
                }
                break;
            case MonsterForm::Hermit:
            case MonsterForm::Crab:
            case MonsterForm::ReefCrab:
            case MonsterForm::Pearl:
                Cone(m, {0, .25f, 0}, .65f, .55f, c, 8);
                Legs(m, c, 3, .35f);
                for (int side : {-1, 1})
                {
                    Box(m, {side * .7f - .12f, .35f, -.5f}, {.24f, .2f, .55f}, c);
                    Cone(m, {side * .65f, .55f, -.6f}, .23f, .4f, c, 5);
                }
                Eyes(m, .2f, .65f, -.52f);
                if (def.form == MonsterForm::Hermit)
                {
                    Cone(m, {0, .65f, .15f}, .5f, .9f, {.85f, .71f, .5f}, 9);
                }
                if (def.form == MonsterForm::Pearl)
                {
                    Cone(m, {0, .7f, 0}, .35f, .75f, White, 10);
                }
                break;
            case MonsterForm::Jellyfish:
            case MonsterForm::Siren:
            case MonsterForm::Ghost:
            case MonsterForm::Rope:
                Cone(m, {0, .6f, 0}, .6f, .9f, c, 10);
                for (int i = 0; i < 6; ++i)
                {
                    const float a = i * Pi / 3;
                    Box(m, {std::cos(a) * .4f - .04f, .05f, std::sin(a) * .4f - .04f}, {.08f, .75f, .08f}, c);
                }
                Eyes(m, .17f, 1, -.44f);
                if (def.form == MonsterForm::Siren)
                {
                    Wings(m, c, .8f);
                }
                if (def.form == MonsterForm::Rope)
                {
                    for (int i = 0; i < 5; ++i)
                    {
                        Box(m, {-.32f, .2f + i * .2f, -.31f}, {.64f, .08f, .62f}, {.83f, .72f, .45f});
                    }
                }
                break;
            case MonsterForm::Goblin:
            case MonsterForm::Troll:
            case MonsterForm::StoneGolem:
            case MonsterForm::Crystal:
            case MonsterForm::WaterGuardian:
                Legs(m, c, 1, .55f);
                Box(m, {-.45f, .45f, -.3f}, {.9f, .85f, .6f}, c);
                Box(m, {-.3f, 1.3f, -.25f}, {.6f, .45f, .5f}, c);
                for (int side : {-1, 1})
                {
                    Box(m, {side * .65f - .14f, .55f, -.15f}, {.28f, .7f, .3f}, c);
                }
                Eyes(m, .15f, 1.49f, -.27f);
                if (def.form == MonsterForm::Goblin)
                {
                    Cone(m, {0, 1.7f, 0}, .38f, .45f, {.48f, .29f, .18f}, 4);
                }
                if (def.form == MonsterForm::Crystal || def.form == MonsterForm::WaterGuardian)
                {
                    for (int side : {-1, 1})
                    {
                        Cone(m, {side * .5f, 1.15f, 0}, .25f, .9f, {.68f, .9f, .99f}, 5);
                    }
                }
                break;
            case MonsterForm::Chameleon:
            case MonsterForm::Lizard:
            case MonsterForm::Frog:
            case MonsterForm::Predator:
                Legs(m, c, 2, .35f);
                Box(m, {-.45f, .25f, -.6f}, {.9f, .48f, 1.2f}, c);
                Box(m, {-.35f, .45f, -.9f}, {.7f, .35f, .4f}, c);
                Eyes(m, .25f, .76f, -.7f);
                if (def.form != MonsterForm::Frog)
                {
                    Box(m, {-.12f, .3f, .55f}, {.24f, .15f, .85f}, c);
                }
                if (def.form == MonsterForm::Chameleon || def.form == MonsterForm::Predator)
                {
                    for (int i = 0; i < 5; ++i)
                    {
                        Cone(m, {0, .65f, -.3f + i * .2f}, .15f, .3f + i * .03f, c, 4);
                    }
                }
                break;
            case MonsterForm::Spider:
            case MonsterForm::Urchin:
                Cone(m, {0, .2f, 0}, .6f, .7f, c, 8);
                for (int i = 0; i < 8; ++i)
                {
                    const float a = i * Pi / 4;
                    const auto p = Vec3{std::cos(a) * .7f, .15f, std::sin(a) * .7f};
                    if (def.form == MonsterForm::Spider)
                    {
                        Box(m, {p.x - .09f, 0, p.z - .09f}, {.18f, .4f, .18f}, c);
                    }
                    else
                    {
                        Cone(m, {p.x, .25f, p.z}, .12f, .65f, c, 4);
                    }
                }
                Eyes(m, .16f, .6f, -.49f);
                break;
            case MonsterForm::Worm:
            case MonsterForm::Snake:
                for (int i = 0; i < 6; ++i)
                {
                    Cone(
                        m,
                        {std::sin(i * .8f) * .2f, .02f, -.75f + i * .3f},
                        .26f,
                        .4f + (5 - i) * .03f,
                        c,
                        7);
                }
                Eyes(m, .13f, .4f, -.95f);
                break;
            case MonsterForm::Flower:
            case MonsterForm::Mushroom:
                Box(m, {-.13f, 0, -.13f}, {.26f, .85f, .26f}, Leaf);
                if (def.form == MonsterForm::Mushroom)
                {
                    Cone(m, {0, .7f, 0}, .8f, .55f, c, 10);
                }
                else
                {
                    for (int i = 0; i < 7; ++i)
                    {
                        const float a = i * 2 * Pi / 7;
                        Cone(m, {std::cos(a) * .45f, .85f, std::sin(a) * .45f}, .35f, .3f, c, 6);
                    }
                    Cone(m, {0, .85f, 0}, .3f, .5f, {.99f, .8f, .3f}, 8);
                }
                Eyes(m, .1f, .5f, -.15f);
                break;
            case MonsterForm::Slime:
                Cone(m, {0, .02f, 0}, .7f, .95f, c, 12);
                Eyes(m, .2f, .43f, -.48f);
                break;
        }
        if (def.epic)
        {
            for (int side : {-1, 1})
            {
                Cone(m, {side * .25f, 1.8f, 0}, .13f, .4f, {.94f, .65f, .96f}, 5);
            }
            for (auto& v : m)
            {
                v.position.x *= 1.7f;
                v.position.y *= 1.5f;
                v.position.z *= 1.7f;
            }
        }
        return m;
    }
}

MonsterMeshes::MonsterMeshes()
{
    auto ring = [](Color color)
    {
        Mesh m;
        for (int i = 0; i < 40; ++i)
        {
            const float a = i * 2 * Pi / 40, b = (i + 1) * 2 * Pi / 40;
            Quad(
                m,
                {std::cos(a), .035f, std::sin(a)},
                {std::cos(b), .035f, std::sin(b)},
                {.92f * std::cos(b), .035f, .92f * std::sin(b)},
                {.92f * std::cos(a), .035f, .92f * std::sin(a)},
                color);
        }
        return m;
    };
    warning = MeshCache::LoadOrCreate(
        "monster_warning_ring_v1",
        [&]
        {
            return ring({1, .24f, .1f});
        });
    epicRing = MeshCache::LoadOrCreate(
        "monster_epic_ring_v1",
        [&]
        {
            return ring({.85f, .4f, .96f});
        });
}

CachedMesh MonsterMeshes::Get(unsigned id)
{
    if (!m_ready.at(id))
    {
        m_models[id] = MeshCache::LoadOrCreate(
            "monster_species_v1_" + std::to_string(id),
            [id]
            {
                return Build(MonsterCatalog::Get(id));
            });
        m_ready[id] = true;
    }
    return m_models[id];
}
