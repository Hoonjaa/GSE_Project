#pragma once
#include "Dependencies/glew.h"
#include "PostProcessing.h"
#include "GeometryTypes.h"
#include "RenderStatistics.h"
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <utility>

// Immutable geometry shared by instances; GPU storage belongs to the renderer.
class CachedMesh
{
public:
    CachedMesh() = default;

    CachedMesh(Mesh vertices, std::string name = "procedural")
        : m_vertices(std::make_shared<const Mesh>(std::move(vertices))), m_name(std::move(name))
    {
    }

private:
    friend class Renderer;
    friend class MeshCache;
    std::shared_ptr<const Mesh> m_vertices;
    std::string m_name;
};

enum class MeshAnimation
{
    None = 0,
    Waves = 1,
    Flag = 2,
    Gull = 3,
    Rain = 4,
    Tornado = 5,
    HitFlash = 6
};

struct MeshInstance
{
    Vec3 offset = {0, 0, 0};
    Vec2 rotation = {1, 0};
    Vec3 scale = {1, 1, 1};
    Vec2 animation = {0, 0};
};

struct BatchRecord
{
    std::uint64_t meshId;
    std::string name;
    size_t vertices, instances;
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
        return m_program != 0 && m_worldProgram != 0 && m_vao != 0 && m_vbo != 0 && m_instanceVbo != 0 &&
               m_fontReady;
    }

    void Resize(int width, int height);
    void Begin(float zoom);
    Vec2 Project(Vec3 relativePosition) const;
    Vec3 ScreenGround(int x, int y, float height = .55f) const;
    void SetPostProcessing(const PostProcessingSettings& settings);
    void SetSceneLight(float brightness);

    bool PostProcessingAvailable() const
    {
        return m_post && m_post->Ready();
    }

    void Submit(const Mesh& mesh, Vec3 offset = {0, 0, 0}, float rotation = 0);
    void Submit(const CachedMesh& mesh, Vec3 offset = {0, 0, 0}, float rotation = 0);
    void Submit(
        const CachedMesh& mesh,
        Vec3 offset,
        float rotation,
        Vec3 scale,
        MeshAnimation animation,
        float animationTime);
    void BeginOverlay();
    void Panel(float x, float y, float width, float height, Color color);
    void Icon(const Mesh& unitMesh, float x, float y, float width, float height, Color tint = {1, 1, 1});
    // UTF-8 input; maxWidth enables wrapping, maxHeight bounds the text block (0 = viewport).
    void Text(
        float x,
        float y,
        const std::string& text,
        Color color,
        float scale = 2,
        float maxWidth = 0,
        float maxHeight = 0);
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

    std::uint64_t DrawCallCount() const
    {
        return m_statistics.DrawCallCount();
    }

    const RenderStatistics& Statistics() const
    {
        return m_statistics;
    }

    const std::vector<BatchRecord>& Batches() const
    {
        return m_batchRecords;
    }

    size_t ResidentMeshes() const
    {
        return m_meshCache.size();
    }

    bool HdrActive() const
    {
        return m_hdrScene;
    }

    float ZoomValue() const
    {
        return m_zoom;
    }

    bool BlurActive() const
    {
        return m_hdrScene && m_postSettings.edgeBlur && m_postSettings.blurStrength > 0 &&
               m_postSettings.blurRadius > 0;
    }

private:
    void Flush();
    void FlushWorld();
    void PresentScene();
    GLuint m_program = 0, m_vao = 0, m_vbo = 0, m_instanceVbo = 0;

    struct GpuMesh
    {
        GLuint vao = 0, vbo = 0;
        GLsizei count = 0;
        std::uint64_t id = 0;
        std::string name;
        std::shared_ptr<const Mesh> pendingOwner;
        std::vector<MeshInstance> instances;
    };

    using MeshKey = std::weak_ptr<const Mesh>;
    std::map<MeshKey, GpuMesh, std::owner_less<MeshKey>> m_meshCache;
    std::vector<MeshKey> m_pendingOrder;
    std::vector<BatchRecord> m_batchRecords;
    std::uint64_t m_nextMeshId = 1;
    GLuint m_worldProgram = 0;
    GLint m_worldProjection = -1;
    GLint m_worldHdr = -1, m_worldIntensity = -1, m_worldLight = -1;
    float m_sceneLight = 1;
    int m_width, m_height;
    float m_zoom = 28;
    Mesh m_batch;
    std::unique_ptr<FontAtlas> m_font;
    bool m_fontReady = false;
    std::unique_ptr<PostProcessing> m_post;
    PostProcessingSettings m_postSettings;
    bool m_hdrScene = false, m_overlay = false;
    size_t m_triangleCount = 0;
    RenderStatistics m_statistics;
};
