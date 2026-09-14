#pragma once
#include "Dependencies/glew.h"
#include "PostProcessing.h"
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <utility>

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

// Immutable geometry shared by instances; GPU storage belongs to the renderer.
class CachedMesh
{
public:
    CachedMesh() = default;

    CachedMesh(Mesh vertices) : m_vertices(std::make_shared<const Mesh>(std::move(vertices)))
    {
    }

private:
    friend class Renderer;
    std::shared_ptr<const Mesh> m_vertices;
};
struct FontAtlas;

// Y is height; the world lies in the X/Z plane.
namespace Geometry
{
    void Triangle(Mesh& mesh, Vec3 a, Vec3 b, Vec3 c, Color color);
    void Quad(Mesh& mesh, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Color color);
    void Box(Mesh& mesh, Vec3 base, Vec3 size, Color color);
    void Cone(Mesh& mesh, Vec3 base, float radius, float height, Color color, int sides = 8);
    void Disc(Mesh& mesh, Vec3 center, float radius, Color color, int sides = 32);
}

class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool IsInitialized() const
    {
        return m_program != 0 && m_worldProgram != 0 && m_vao != 0 && m_vbo != 0 && m_fontReady;
    }

    void Resize(int width, int height);
    void Begin(float zoom);
    Vec2 Project(Vec3 relativePosition) const;
    Vec3 ScreenGround(int x, int y, float height = .55f) const;
    void SetPostProcessing(const PostProcessingSettings& settings);

    bool PostProcessingAvailable() const
    {
        return m_post && m_post->Ready();
    }

    void Submit(const Mesh& mesh, Vec3 offset = {0, 0, 0}, float rotation = 0);
    void Submit(const CachedMesh& mesh, Vec3 offset = {0, 0, 0}, float rotation = 0);
    void BeginOverlay();
    void Panel(float x, float y, float width, float height, Color color);
    // UTF-8 input; maxWidth > 0 enables character wrapping for dialogue panels.
    void Text(float x, float y, const std::string& text, Color color, float scale = 2, float maxWidth = 0);
    void End();

    int Width() const
    {
        return m_width;
    }

    int Height() const
    {
        return m_height;
    }

    size_t TriangleCount() const
    {
        return m_triangleCount;
    }

private:
    void Flush();
    void PresentScene();
    GLuint m_program = 0, m_vao = 0, m_vbo = 0;

    struct GpuMesh
    {
        GLuint vao = 0, vbo = 0;
        GLsizei count = 0;
    };

    using MeshKey = std::weak_ptr<const Mesh>;
    std::map<MeshKey, GpuMesh, std::owner_less<MeshKey>> m_meshCache;
    GLuint m_worldProgram = 0;
    GLint m_worldOffset = -1, m_worldRotation = -1, m_worldProjection = -1;
    GLint m_worldHdr = -1, m_worldIntensity = -1;
    int m_width, m_height;
    float m_zoom = 28;
    Mesh m_batch;
    std::unique_ptr<FontAtlas> m_font;
    bool m_fontReady = false;
    std::unique_ptr<PostProcessing> m_post;
    PostProcessingSettings m_postSettings;
    bool m_hdrScene = false, m_overlay = false;
    size_t m_triangleCount = 0;
};
