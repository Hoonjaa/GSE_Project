#pragma once
#include <vector>

struct Vec3
{
    float x, y, z;
};

struct Color
{
    float r, g, b;
};

struct Vec2
{
    float x, y;
};

struct Vertex
{
    Vec3 position;
    Color color;
    Vec2 uv = {-1, -1};
};

using Mesh = std::vector<Vertex>;
