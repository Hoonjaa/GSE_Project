#include "stdafx.h"
#include "Renderer.h"

void Renderer::Icon(const Mesh& mesh, float x, float y, float width, float height, Color tint)
{
    if (width <= 0 || height <= 0 || m_width <= 0 || m_height <= 0)
    {
        return;
    }
    // Topology is a cached unit icon. Reuse the existing ordered UI stream batch.
    for (auto vertex : mesh)
    {
        vertex.position = {
            2 * (x + vertex.position.x * width) / m_width - 1,
            1 - 2 * (y + vertex.position.y * height) / m_height,
            0};
        vertex.color = {vertex.color.r * tint.r, vertex.color.g * tint.g, vertex.color.b * tint.b};
        m_batch.push_back(vertex);
    }
}
