#pragma once
#include "IslandLayout.h"

class IslandTerrainAdapter
{
public:
    explicit IslandTerrainAdapter(std::shared_ptr<const IslandLayout> layout) : m_layout(std::move(layout))
    {
    }

    std::uint32_t Seed() const
    {
        return static_cast<std::uint32_t>(m_layout->Info().seed);
    }

    void Map(Renderer& renderer, float left, float top, float extent, const WorldPosition& player) const;

private:
    std::shared_ptr<const IslandLayout> m_layout;
};
