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
        m_drawCalls = 0;
    }

    void DrawArrays(GLenum mode, GLint first, GLsizei count)
    {
        glDrawArrays(mode, first, count);
        ++m_drawCalls;
    }

    std::uint64_t DrawCallCount() const
    {
        return m_drawCalls;
    }

private:
    std::uint64_t m_drawCalls = 0;
};
