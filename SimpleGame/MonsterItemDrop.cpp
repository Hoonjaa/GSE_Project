#include "stdafx.h"
#include "MonsterItemDrop.h"
#include <algorithm>
#include <cmath>

MonsterItemDrop::MonsterItemDrop(CachedMesh mesh, InventoryItem kind, Vec3 position)
    : MeshActor(std::move(mesh), ItemCatalog::Get(kind).name), item(kind)
{
    transform.position = position;
    enabled = false;
}

bool MonsterItemDrop::Tick(
    float dt,
    const MonsterContext& context,
    const std::function<bool(InventoryItem)>& collect)
{
    m_age += dt;
    if (m_age >= 120)
    {
        return true;
    }
    if (!context.active)
    {
        return false;
    }
    const float distance =
        std::hypot(context.player.x - transform.position.x, context.player.z - transform.position.z);
    if (distance < 3.8f)
    {
        m_speed = (std::min)(14.f, m_speed + dt * 18);
        context.Step(transform.position, context.player, dt * m_speed, context.playerFlow);
    }
    if (distance < .65f && context.Segment(transform.position, context.player))
    {
        return collect(item);
    }
    return false;
}

void MonsterItemDrop::Render(Renderer& r, const ActorTransform& world) const
{
    auto p = world.position;
    p.y += .25f + .08f * std::sin(m_age * 5);
    MeshActor::Render(r, {p, world.rotation});
}
