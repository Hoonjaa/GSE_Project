#pragma once
#include "Renderer.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

using ActorId = std::uint64_t;
constexpr ActorId NoActor = 0;

struct ActorTransform
{
    Vec3 position = {0, 0, 0};
    float rotation = 0;

    Vec3 Apply(Vec3 point) const;
    ActorTransform Combine(const ActorTransform& child) const;
};

// Actor owns state; SceneGraph owns lifetime and hierarchy. Actors are not copyable.
class Actor
{
public:
    explicit Actor(std::string name = {});
    virtual ~Actor() = default;
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;

    ActorId Id() const
    {
        return m_id;
    }

    ActorId Parent() const
    {
        return m_parent;
    }

    const std::string& Name() const
    {
        return m_name;
    }

    bool DestroyRequested() const
    {
        return m_destroy;
    }

    void Destroy()
    {
        m_destroy = true;
    }

    ActorTransform transform;
    bool enabled = true;
    bool visible = true;

    virtual void Update(float)
    {
    }

    virtual void Render(Renderer&, const ActorTransform&) const
    {
    }

    virtual void Overlay(Renderer&, const ActorTransform&) const
    {
    }

private:
    friend class SceneGraph;
    ActorId m_id = NoActor, m_parent = NoActor;
    std::string m_name;
    bool m_destroy = false;
    std::vector<ActorId> m_children;
};

class MeshActor : public Actor
{
public:
    explicit MeshActor(CachedMesh mesh = {}, std::string name = {});
    void Render(Renderer& renderer, const ActorTransform& world) const override;
    CachedMesh mesh;
};

class ScaledMeshActor : public MeshActor
{
public:
    ScaledMeshActor(CachedMesh resource, Vec3 size, std::string name = {})
        : MeshActor(std::move(resource), std::move(name)), scale(size)
    {
    }

    void Render(Renderer& renderer, const ActorTransform& world) const override;
    Vec3 scale;
};

// Custom dynamic geometry and screen-space UI can participate in the same lifecycle.
class CallbackActor : public Actor
{
public:
    explicit CallbackActor(std::string name = {}) : Actor(std::move(name))
    {
    }

    std::function<void(float)> tick;
    std::function<void(Renderer&, const ActorTransform&)> draw, overlay;
    void Update(float dt) override;
    void Render(Renderer& renderer, const ActorTransform& world) const override;
    void Overlay(Renderer& renderer, const ActorTransform& world) const override;
};
