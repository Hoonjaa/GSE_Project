#include "stdafx.h"
#include "Actor.h"
#include <cmath>

Vec3 ActorTransform::Apply(Vec3 p) const
{
    const float c = std::cos(rotation), s = std::sin(rotation);
    return {p.x * c - p.z * s + position.x, p.y + position.y, p.x * s + p.z * c + position.z};
}

ActorTransform ActorTransform::Combine(const ActorTransform& child) const
{
    ActorTransform result;
    result.position = Apply(child.position);
    result.rotation = rotation + child.rotation;
    return result;
}

Actor::Actor(std::string name) : m_name(std::move(name))
{
}

MeshActor::MeshActor(CachedMesh resource, std::string name)
    : Actor(std::move(name)), mesh(std::move(resource))
{
}

void MeshActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    renderer.Submit(mesh, world.position, world.rotation);
}

void CallbackActor::Update(float dt)
{
    if (tick)
    {
        tick(dt);
    }
}

void ScaledMeshActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    renderer.Submit(mesh, world.position, world.rotation, scale, MeshAnimation::None, 0);
}

void CallbackActor::Render(Renderer& renderer, const ActorTransform& world) const
{
    if (draw)
    {
        draw(renderer, world);
    }
}

void CallbackActor::Overlay(Renderer& renderer, const ActorTransform& world) const
{
    if (overlay)
    {
        overlay(renderer, world);
    }
}
