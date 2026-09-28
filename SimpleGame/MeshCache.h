#pragma once
#include "Renderer.h"
#include <functional>

// Version the key whenever geometry or animation metadata changes.
// Intended for a finite reusable asset catalog, not infinite procedural chunk seeds.
class MeshCache
{
public:
    static CachedMesh LoadOrCreate(const std::string& key, const std::function<Mesh()>& factory);
};
