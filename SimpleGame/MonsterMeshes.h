#pragma once
#include "MonsterCatalog.h"
#include "Renderer.h"

class MonsterMeshes
{
public:
    MonsterMeshes();
    CachedMesh Get(unsigned id);
    CachedMesh warning, epicRing;

private:
    std::array<CachedMesh, MonsterCatalog::Count> m_models;
    std::array<bool, MonsterCatalog::Count> m_ready = {};
};
