#pragma once
#include "Dependencies/glew.h"

struct PostProcessingSettings
{
    bool enabled = true;
    bool toneMapping = true;
    bool vignette = true;
    bool edgeBlur = true;
    float exposure = 1.0f;
    float sceneIntensity = 2.0f;
    float vignetteStrength = .24f;
    float blurRadius = 3.0f; // Full-resolution pixel spacing between Gaussian taps.
    float blurStrength = .85f;
    float blurStart = .42f;  // Normalized radius: center = 0, corners = 1.
    float blurEnd = .98f;
};

// Owns all supplied shader programs and offscreen attachments.
// The caller must destroy this object before destroying the GL context.
class PostProcessing
{
public:
    PostProcessing(GLuint compositeProgram, GLuint blurProgram);
    ~PostProcessing();
    PostProcessing(const PostProcessing&) = delete;
    PostProcessing& operator=(const PostProcessing&) = delete;
    void Resize(int width, int height);
    bool Ready() const { return m_ready; }
    void BeginScene();
    void Composite(const PostProcessingSettings& settings);
private:
    void ReleaseTargets();
    bool MakeTarget(GLuint& fbo, GLuint& texture, int width, int height);
    GLuint m_composite = 0, m_blur = 0, m_vao = 0;
    GLuint m_sceneFbo = 0, m_sceneColor = 0, m_sceneDepth = 0;
    GLuint m_resolveFbo = 0, m_resolveColor = 0;
    GLuint m_blurFbo[2] = {}, m_blurColor[2] = {};
    int m_width = 0, m_height = 0, m_halfWidth = 0, m_halfHeight = 0, m_samples = 1;
    bool m_ready = false;
};
