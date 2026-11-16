#pragma once
#include "ItemCatalog.h"
#include "GeometryTypes.h"

class InventoryIcons
{
public:
    InventoryIcons();
    const Mesh& Get(ItemIcon icon) const;

private:
    std::array<Mesh, static_cast<unsigned>(ItemIcon::Count)> m_icons;
};
