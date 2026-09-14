#include "stdafx.h"
#include "PostProcessing.h"
#include <algorithm>
#include <iostream>

PostProcessing::PostProcessing(GLuint compositeProgram, GLuint blurProgram)
    : m_composite(compositeProgram), m_blur(blurProgram)
{
    if (m_composite && m_blur)
    {
        glGenVertexArrays(1, &m_vao);
    }
}

PostProcessing::~PostProcessing()
{
    ReleaseTargets();
    if (m_vao)
    {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_composite)
    {
        glDeleteProgram(m_composite);
    }
    if (m_blur)
    {
        glDeleteProgram(m_blur);
    }
}

void PostProcessing::ReleaseTargets()
{
    m_ready = false;
    if (m_sceneFbo)
    {
        glDeleteFramebuffers(1, &m_sceneFbo);
    }
    if (m_sceneColor)
    {
        glDeleteRenderbuffers(1, &m_sceneColor);
    }
    if (m_sceneDepth)
    {
        glDeleteRenderbuffers(1, &m_sceneDepth);
    }
    if (m_resolveFbo)
    {
        glDeleteFramebuffers(1, &m_resolveFbo);
    }
    if (m_resolveColor)
    {
        glDeleteTextures(1, &m_resolveColor);
    }
    glDeleteFramebuffers(2, m_blurFbo);
    glDeleteTextures(2, m_blurColor);
    m_sceneFbo = m_sceneColor = m_sceneDepth = 0;
    m_resolveFbo = m_resolveColor = 0;
    m_blurFbo[0] = m_blurFbo[1] = m_blurColor[0] = m_blurColor[1] = 0;
}

bool PostProcessing::MakeTarget(GLuint& fbo, GLuint& texture, int width, int height)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    return fbo && texture && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void PostProcessing::Resize(int width, int height)
{
    width = (std::max)(1, width);
    height = (std::max)(1, height);
    if (m_ready && width == m_width && height == m_height)
    {
        return;
    }
    ReleaseTargets();
    m_width = width;
    m_height = height;
    m_halfWidth = (std::max)(1, (width + 1) / 2);
    m_halfHeight = (std::max)(1, (height + 1) / 2);
    if (!m_composite || !m_blur || !m_vao)
    {
        return;
    }
    GLint maxTexture = 0, maxRenderbuffer = 0, maxSamples = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbuffer);
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    glActiveTexture(GL_TEXTURE0);
    bool ok =
        width <= maxTexture && height <= maxTexture && width <= maxRenderbuffer && height <= maxRenderbuffer;
    if (ok)
    {
        ok = MakeTarget(m_resolveFbo, m_resolveColor, width, height);
    }
    if (ok)
    {
        ok = MakeTarget(m_blurFbo[0], m_blurColor[0], m_halfWidth, m_halfHeight);
    }
    if (ok)
    {
        ok = MakeTarget(m_blurFbo[1], m_blurColor[1], m_halfWidth, m_halfHeight);
    }
    if (ok)
    {
        glGenFramebuffers(1, &m_sceneFbo);
        glGenRenderbuffers(1, &m_sceneColor);
        glGenRenderbuffers(1, &m_sceneDepth);
        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);
        m_samples = (std::min)(4, (std::max)(1, maxSamples));
        // Try multisampling first. A format/sample mismatch falls back to one sample.
        for (;;)
        {
            glBindRenderbuffer(GL_RENDERBUFFER, m_sceneColor);
            if (m_samples > 1)
            {
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_RGBA16F, width, height);
            }
            else
            {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA16F, width, height);
            }
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_sceneColor);
            glBindRenderbuffer(GL_RENDERBUFFER, m_sceneDepth);
            if (m_samples > 1)
            {
                glRenderbufferStorageMultisample(
                    GL_RENDERBUFFER,
                    m_samples,
                    GL_DEPTH_COMPONENT24,
                    width,
                    height);
            }
            else
            {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
            }
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_sceneDepth);
            ok = m_sceneFbo && m_sceneColor && m_sceneDepth &&
                 glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
            if (ok || m_samples == 1)
            {
                break;
            }
            m_samples = 1;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (!ok)
    {
        std::cerr << "Post-processing targets unavailable; using original rendering.\n";
        ReleaseTargets();
    }
    else
    {
        m_ready = true;
    }
    glViewport(0, 0, width, height);
}

void PostProcessing::BeginScene()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);
    glViewport(0, 0, m_width, m_height);
    glEnable(GL_MULTISAMPLE);
}

void PostProcessing::Composite(const PostProcessingSettings& settings)
{
    if (!m_ready)
    {
        return;
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sceneFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFbo);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FRAMEBUFFER_SRGB); // Composite shader encodes exactly once.
    glBindVertexArray(m_vao);
    glActiveTexture(GL_TEXTURE0);
    if (settings.edgeBlur && settings.blurStrength > 0 && settings.blurRadius > 0)
    {
        glUseProgram(m_blur);
        glUniform1i(glGetUniformLocation(m_blur, "u_Source"), 0);
        glViewport(0, 0, m_halfWidth, m_halfHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo[0]);
        glBindTexture(GL_TEXTURE_2D, m_resolveColor);
        glUniform2f(glGetUniformLocation(m_blur, "u_Step"), settings.blurRadius / m_width, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo[1]);
        glBindTexture(GL_TEXTURE_2D, m_blurColor[0]);
        glUniform2f(glGetUniformLocation(m_blur, "u_Step"), 0, settings.blurRadius / m_height);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glUseProgram(m_composite);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_resolveColor);
    glActiveTexture(GL_TEXTURE1);
    const bool blurred = settings.edgeBlur && settings.blurStrength > 0 && settings.blurRadius > 0;
    glBindTexture(GL_TEXTURE_2D, blurred ? m_blurColor[1] : m_resolveColor);
    glUniform1i(glGetUniformLocation(m_composite, "u_Scene"), 0);
    glUniform1i(glGetUniformLocation(m_composite, "u_Blurred"), 1);
    glUniform1i(glGetUniformLocation(m_composite, "u_ToneMapping"), settings.toneMapping);
    glUniform1f(glGetUniformLocation(m_composite, "u_Exposure"), settings.exposure);
    glUniform1f(
        glGetUniformLocation(m_composite, "u_Vignette"),
        settings.vignette ? settings.vignetteStrength : 0);
    glUniform1f(glGetUniformLocation(m_composite, "u_BlurStrength"), blurred ? settings.blurStrength : 0);
    glUniform2f(glGetUniformLocation(m_composite, "u_BlurRange"), settings.blurStart, settings.blurEnd);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
