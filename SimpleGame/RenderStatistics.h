#pragma once
#include "Dependencies/glew.h"
#include <cstdint>

// All geometry draw entry points go through this frame-local counter.
// State changes (glDrawBuffer[s]), clears and framebuffer blits are not draws.
class RenderStatistics
{
public:
    void Reset()
    {
        *this = RenderStatistics{};
    }

    void DrawArrays(GLenum mode, GLint first, GLsizei count)
    {
        glDrawArrays(mode, first, count);
        ++m_drawCalls;
        ++postDraws;
    }

    void DrawStream(GLenum mode, GLint first, GLsizei count, bool overlay)
    {
        glDrawArrays(mode, first, count);
        ++m_drawCalls;
        if (overlay)
        {
            ++uiDraws;
        }
        else
        {
            ++dynamicDraws;
        }
    }

    void DrawInstances(GLenum mode, GLsizei vertices, GLsizei count, bool overlay)
    {
        glDrawArraysInstanced(mode, 0, vertices, count);
        ++m_drawCalls;
        if (overlay)
        {
            ++uiDraws;
        }
        else
        {
            ++worldDraws;
        }
    }

    std::uint64_t worldDraws = 0, dynamicDraws = 0, uiDraws = 0, postDraws = 0;
    std::uint64_t submissions = 0, instances = 0, dynamicVertices = 0, uiVertices = 0;
    std::uint64_t staticUploadBytes = 0, instanceUploadBytes = 0, streamUploadBytes = 0;
    std::uint64_t gpuCacheHits = 0, gpuCacheMisses = 0, gpuCacheEvictions = 0;

    std::uint64_t DrawCallCount() const
    {
        return m_drawCalls;
    }

private:
    std::uint64_t m_drawCalls = 0;
};
