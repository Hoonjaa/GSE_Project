#include "stdafx.h"
#include "IslandTerrainAdapter.h"

void IslandTerrainAdapter::Map(
    Renderer& renderer,
    float left,
    float top,
    float extent,
    const WorldPosition& player) const
{
    const float size = extent / IslandLayout::Size;
    for (int i = 0; i < IslandLayout::Size * IslandLayout::Size; ++i)
    {
        const auto p = m_layout->Center(i);
        if (m_layout->Land(p.x, p.z))
        {
            renderer.Panel(
                left + (i % IslandLayout::Size) * size,
                top + (i / IslandLayout::Size) * size,
                size,
                size,
                m_layout->Village(p.x, p.z) ? Color{.85f, .73f, .43f} : Color{.61f, .72f, .40f});
        }
    }
    const float cell = extent / IslandLayout::Size;
    auto marker = [&](Vec3 point, Color color)
    {
        const float x = (point.x - m_layout->Origin()) / m_layout->CellSize() * cell;
        const float z = (point.z - m_layout->Origin()) / m_layout->CellSize() * cell;
        if (x >= 0 && z >= 0 && x < extent - 5 && z < extent - 5)
        {
            renderer.Panel(left + x, top + z, 5, 5, color);
        }
    };
    marker(m_layout->Spawn(), {.98f, .71f, .32f});
    marker(player.RelativeTo(m_layout->Info().center), {.96f, .90f, .72f});
}
