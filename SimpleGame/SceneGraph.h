#pragma once
#include "Actor.h"
#include <map>
#include <memory>
#include <stdexcept>
#include <type_traits>

class SceneGraph
{
public:
    SceneGraph() = default;
    SceneGraph(const SceneGraph&) = delete;
    SceneGraph& operator=(const SceneGraph&) = delete;

    template <class T, class... Args> T& Create(ActorId parent, Args&&... args)
    {
        static_assert(std::is_base_of<Actor, T>::value, "Scene nodes must derive from Actor.");
        if (m_drawing)
        {
            throw std::logic_error("Create actors before rendering.");
        }
        Actor* owner = parent == NoActor ? nullptr : Find(parent);
        if (parent != NoActor && !owner)
        {
            throw std::invalid_argument("Actor parent does not exist.");
        }
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        actor->m_id = m_nextId++;
        actor->m_parent = parent;
        T& result = *actor;
        m_actors.emplace(result.Id(), std::move(actor));
        (owner ? owner->m_children : m_roots).push_back(result.Id());
        return result;
    }

    Actor* Find(ActorId id);
    const Actor* Find(ActorId id) const;

    template <class T> T* Get(ActorId id)
    {
        return dynamic_cast<T*>(Find(id));
    }

    template <class T> const T* Get(ActorId id) const
    {
        return dynamic_cast<const T*>(Find(id));
    }

    // Keeps local transform. Rejects cycles; reparent only outside traversal.
    bool SetParent(ActorId child, ActorId parent);
    void Destroy(ActorId id);
    void CollectDestroyed();
    void Update(float dt);
    void Render(Renderer& renderer) const;
    void Overlay(Renderer& renderer) const;

    size_t Size() const
    {
        return m_actors.size();
    }

private:
    bool Live(ActorId id) const;
    bool Enabled(ActorId id) const;
    void Draw(Renderer& renderer, ActorId id, const ActorTransform& parent, bool overlay) const;
    void DrawPass(Renderer& renderer, bool overlay) const;
    std::map<ActorId, std::unique_ptr<Actor>> m_actors;
    std::vector<ActorId> m_roots;
    ActorId m_nextId = 1;
    bool m_updating = false;
    mutable bool m_drawing = false;
};
