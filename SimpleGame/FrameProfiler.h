#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>

// Measures completed display frames, including timer pacing and buffer swaps.
// One summary per second avoids per-frame console I/O in the render hot path.
class FrameProfiler
{
public:
    void Reset()
    {
        m_running = false;
        m_frames = 0;
        m_totalDrawCalls = 0;
        m_minDrawCalls = (std::numeric_limits<std::uint64_t>::max)();
        m_maxDrawCalls = 0;
    }

    void BeginFrame()
    {
        if (!m_running)
        {
            m_start = Clock::now();
            m_running = true;
        }
    }

    void EndFrame(std::uint64_t drawCalls, std::ostream& output)
    {
        if (!m_running)
        {
            return;
        }
        ++m_frames;
        m_totalDrawCalls += drawCalls;
        m_minDrawCalls = (std::min)(m_minDrawCalls, drawCalls);
        m_maxDrawCalls = (std::max)(m_maxDrawCalls, drawCalls);
        const auto now = Clock::now();
        const double elapsed = std::chrono::duration<double>(now - m_start).count();
        if (elapsed < 1.0)
        {
            return;
        }
        std::ostringstream line;
        line << std::fixed << std::setprecision(1) << "[Frame] FPS=" << m_frames / elapsed
             << " | Draw calls/frame: last=" << drawCalls
             << " avg=" << static_cast<double>(m_totalDrawCalls) / m_frames << " min=" << m_minDrawCalls
             << " max=" << m_maxDrawCalls;
        output << line.str() << std::endl;
        Reset();
        // Carry the boundary forward so idle time and logging overhead are included.
        m_start = now;
        m_running = true;
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_start;
    bool m_running = false;
    std::uint64_t m_frames = 0, m_totalDrawCalls = 0;
    std::uint64_t m_minDrawCalls = (std::numeric_limits<std::uint64_t>::max)();
    std::uint64_t m_maxDrawCalls = 0;
};
