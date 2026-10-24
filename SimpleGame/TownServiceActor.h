#pragma once
#include "Actor.h"
#include "MapCatalog.h"

class TownServiceActor : public Actor
{
public:
    explicit TownServiceActor(TownServiceKind kind);
    void Render(Renderer& renderer, const ActorTransform& world) const override;
    void Overlay(Renderer& renderer, const ActorTransform& world) const override;

private:
    TownServiceKind m_kind;
    CachedMesh m_zone, m_model;
};
