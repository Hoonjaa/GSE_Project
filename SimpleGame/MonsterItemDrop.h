#pragma once
#include "MonsterActor.h"
#include "ItemCatalog.h"

class MonsterItemDrop : public MeshActor
{
public:
    MonsterItemDrop(CachedMesh mesh, InventoryItem item, Vec3 position);
    bool Tick(float dt, const MonsterContext& context, const std::function<bool(InventoryItem)>& collect);
    void Render(Renderer& renderer, const ActorTransform& world) const override;
    InventoryItem item;

private:
    float m_age = 0, m_speed = 0;
};
