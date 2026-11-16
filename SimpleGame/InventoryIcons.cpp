#include "stdafx.h"
#include "InventoryIcons.h"
#include "Renderer.h"
#include <cmath>

namespace
{
    void Rect(Mesh& mesh, float x, float y, float w, float h, Color c)
    {
        Geometry::Quad(mesh, {x, y, 0}, {x + w, y, 0}, {x + w, y + h, 0}, {x, y + h, 0}, c);
    }

    void Line(Mesh& mesh, Vec2 a, Vec2 b, float width, Color c)
    {
        const float length = std::hypot(b.x - a.x, b.y - a.y);
        if (length < .001f)
        {
            return;
        }
        const float x = -(b.y - a.y) / length * width * .5f, y = (b.x - a.x) / length * width * .5f;
        Geometry::Quad(
            mesh,
            {a.x + x, a.y + y, 0},
            {b.x + x, b.y + y, 0},
            {b.x - x, b.y - y, 0},
            {a.x - x, a.y - y, 0},
            c);
    }
}

InventoryIcons::InventoryIcons()
{
    const Color metal = {.47f, .66f, .72f}, wood = {.64f, .43f, .2f}, cloth = {.27f, .49f, .65f},
                gold = {.86f, .62f, .18f};
    for (unsigned i = 0; i < m_icons.size(); ++i)
    {
        auto& mesh = m_icons[i];
        switch (static_cast<ItemIcon>(i))
        {
            case ItemIcon::Tentacle:
            {
                const Vec2 points[] =
                    {{.36f, .88f}, {.57f, .74f}, {.62f, .55f}, {.48f, .39f}, {.44f, .22f}, {.55f, .1f}};
                for (int n = 0; n < 5; ++n)
                {
                    Line(mesh, points[n], points[n + 1], .15f, {.58f, .31f, .65f});
                }
                for (int n = 0; n < 4; ++n)
                {
                    Rect(mesh, .38f + n * .035f, .3f + n * .13f, .065f, .05f, {.93f, .67f, .73f});
                }
                break;
            }
            case ItemIcon::Sword:
                Line(mesh, {.28f, .84f}, {.75f, .17f}, .11f, metal);
                Line(mesh, {.27f, .62f}, {.53f, .8f}, .08f, gold);
                Line(mesh, {.27f, .85f}, {.38f, .68f}, .09f, wood);
                break;
            case ItemIcon::Head:
                Rect(mesh, .26f, .27f, .48f, .4f, cloth);
                Rect(mesh, .15f, .61f, .7f, .1f, gold);
                break;
            case ItemIcon::Body:
                Rect(mesh, .3f, .25f, .4f, .58f, cloth);
                Rect(mesh, .15f, .28f, .7f, .2f, cloth);
                Rect(mesh, .42f, .22f, .16f, .1f, {.95f, .89f, .74f});
                break;
            case ItemIcon::Legs:
                Rect(mesh, .25f, .22f, .5f, .24f, cloth);
                Rect(mesh, .25f, .4f, .2f, .43f, cloth);
                Rect(mesh, .55f, .4f, .2f, .43f, cloth);
                break;
            case ItemIcon::Fish:
                Geometry::Triangle(mesh, {.2f, .5f, 0}, {.55f, .25f, 0}, {.8f, .5f, 0}, metal);
                Geometry::Triangle(mesh, {.2f, .5f, 0}, {.8f, .5f, 0}, {.55f, .75f, 0}, metal);
                Geometry::Triangle(mesh, {.25f, .5f, 0}, {.08f, .28f, 0}, {.08f, .72f, 0}, cloth);
                Rect(mesh, .66f, .44f, .05f, .05f, {.12f, .17f, .2f});
                break;
            case ItemIcon::Trade:
                Rect(mesh, .18f, .23f, .64f, .56f, wood);
                Rect(mesh, .27f, .23f, .08f, .56f, gold);
                Rect(mesh, .65f, .23f, .08f, .56f, gold);
                break;
            case ItemIcon::Material:
            case ItemIcon::Stone:
                Geometry::Triangle(
                    mesh,
                    {.5f, .12f, 0},
                    {.78f, .45f, 0},
                    {.5f, .88f, 0},
                    i == unsigned(ItemIcon::Stone) ? Color{.21f, .72f, .84f} : gold);
                Geometry::Triangle(
                    mesh,
                    {.5f, .12f, 0},
                    {.5f, .88f, 0},
                    {.22f, .45f, 0},
                    i == unsigned(ItemIcon::Stone) ? Color{.44f, .84f, .92f} : wood);
                break;
            default:
                break;
        }
    }
}

const Mesh& InventoryIcons::Get(ItemIcon icon) const
{
    return m_icons.at(static_cast<unsigned>(icon));
}
