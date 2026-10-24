#include "stdafx.h"
#include "SceneGraph.h"
#include <algorithm>

bool SceneGraph::Live(ActorId id) const
{
    while (id != NoActor)
    {
        const auto found = m_actors.find(id);
        if (found == m_actors.end() || found->second->DestroyRequested())
        {
            return false;
        }
        id = found->second->Parent();
    }
    return true;
}

Actor* SceneGraph::Find(ActorId id)
{
    const auto found = m_actors.find(id);
    return found != m_actors.end() && Live(id) ? found->second.get() : nullptr;
}

const Actor* SceneGraph::Find(ActorId id) const
{
    const auto found = m_actors.find(id);
    return found != m_actors.end() && Live(id) ? found->second.get() : nullptr;
}

bool SceneGraph::Enabled(ActorId id) const
{
    while (id != NoActor)
    {
        const auto found = m_actors.find(id);
        if (found == m_actors.end() || found->second->DestroyRequested() || !found->second->enabled)
        {
            return false;
        }
        id = found->second->Parent();
    }
    return true;
}

bool SceneGraph::SetParent(ActorId child, ActorId parent)
{
    Actor* actor = Find(child);
    Actor* owner = parent == NoActor ? nullptr : Find(parent);
    if (m_updating || m_drawing || !actor || (parent != NoActor && !owner))
    {
        return false;
    }
    for (ActorId id = parent; id != NoActor; id = Find(id)->Parent())
    {
        if (id == child)
        {
            return false;
        }
    }
    auto& siblings = actor->Parent() == NoActor ? m_roots : Find(actor->Parent())->m_children;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
    actor->m_parent = parent;
    (owner ? owner->m_children : m_roots).push_back(child);
    return true;
}

void SceneGraph::Destroy(ActorId id)
{
    if (Actor* actor = Find(id))
    {
        actor->Destroy();
    }
}

void SceneGraph::CollectDestroyed()
{
    if (m_updating || m_drawing)
    {
        return;
    }
    std::vector<ActorId> dead;
    for (const auto& entry : m_actors)
    {
        if (!Live(entry.first))
        {
            dead.push_back(entry.first);
        }
    }
    auto removeDead = [&](std::vector<ActorId>& list)
    {
        list.erase(
            std::remove_if(
                list.begin(),
                list.end(),
                [&](ActorId id)
                {
                    return std::binary_search(dead.begin(), dead.end(), id);
                }),
            list.end());
    };
    removeDead(m_roots);
    for (auto& entry : m_actors)
    {
        removeDead(entry.second->m_children);
    }
    for (ActorId id : dead)
    {
        m_actors.erase(id);
    }
}

void SceneGraph::Update(float dt)
{
    if (m_updating || m_drawing)
    {
        return;
    }
    CollectDestroyed();
    // Snapshot: spawned actors tick next frame; removal is deferred until traversal ends.
    std::vector<ActorId> snapshot;
    std::function<void(ActorId)> visit = [&](ActorId id)
    {
        const Actor* actor = Find(id);
        if (!actor || !actor->enabled)
        {
            return;
        }
        snapshot.push_back(id);
        for (ActorId child : actor->m_children)
        {
            visit(child);
        }
    };
    for (ActorId root : m_roots)
    {
        visit(root);
    }
    m_updating = true;
    try
    {
        for (ActorId id : snapshot)
        {
            if (Enabled(id))
            {
                Find(id)->Update(dt);
            }
        }
    }
    catch (...)
    {
        m_updating = false;
        throw;
    }
    m_updating = false;
    CollectDestroyed();
}

void SceneGraph::Draw(Renderer& renderer, ActorId id, const ActorTransform& parent, bool overlay) const
{
    const Actor* actor = Find(id);
    if (!actor || !actor->visible)
    {
        return;
    }
    const auto world = parent.Combine(actor->transform);
    if (overlay)
    {
        actor->Overlay(renderer, world);
    }
    else
    {
        actor->Render(renderer, world);
    }
    for (ActorId child : actor->m_children)
    {
        Draw(renderer, child, world, overlay);
    }
}

void SceneGraph::Render(Renderer& renderer) const
{
    DrawPass(renderer, false);
}

void SceneGraph::Overlay(Renderer& renderer) const
{
    DrawPass(renderer, true);
}

void SceneGraph::DrawPass(Renderer& renderer, bool overlay) const
{
    if (m_drawing || m_updating)
    {
        return;
    }
    m_drawing = true;
    try
    {
        for (ActorId root : m_roots)
        {
            Draw(renderer, root, {}, overlay);
        }
    }
    catch (...)
    {
        m_drawing = false;
        throw;
    }
    m_drawing = false;
}
