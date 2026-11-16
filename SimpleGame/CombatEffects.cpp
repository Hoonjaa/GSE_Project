#include "stdafx.h"
#include "CombatEffects.h"
#include <algorithm>
#include <cmath>

namespace
{
    const Color Gold = {.98f, .71f, .32f};
}

DamageNumberActor::DamageNumberActor(WorldPosition position, int amount, bool player)
    : Actor("피해량"), location(position), m_amount(amount), m_player(player)
{
}

void DamageNumberActor::Update(float dt)
{
    m_time -= dt;
    if (m_time <= 0)
    {
        Destroy();
    }
}

void DamageNumberActor::Overlay(Renderer& renderer, const ActorTransform& world) const
{
    auto position = world.position;
    position.y += 2 + (1 - m_time) * 1.3f - .55f;
    const auto p = renderer.Project(position);
    renderer.Text(p.x, p.y, std::to_string(m_amount), m_player ? Color{1, .25f, .2f} : Gold, 2);
}

SlashActor::SlashActor(CachedMesh resource) : MeshActor(std::move(resource), "근접 공격 궤적")
{
    visible = false;
}

void SlashActor::Trigger(Vec3 position, Vec3 aim, float groundHeight)
{
    transform.position = {position.x, groundHeight - .55f, position.z};
    transform.rotation = std::atan2(aim.z, aim.x);
    m_time = .2f;
    visible = true;
}

void SlashActor::Stop()
{
    m_time = 0;
    visible = false;
}

void SlashActor::Update(float dt)
{
    m_time = (std::max)(0.f, m_time - dt);
    visible = m_time > 0;
}
