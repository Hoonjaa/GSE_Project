#pragma once
#include "Renderer.h"
#include <chrono>
#include <cstdint>
#include <ostream>

class FrameProfiler
{
public:
    void Reset();
    void BeginFrame();
    void RenderFinished();
    void RecordUpdate(double milliseconds);
    void EndFrame(const Renderer& renderer, size_t actors, size_t chunks, std::ostream& output);

private:
    using Clock = std::chrono::steady_clock;
    void WriteHeaders();
    Clock::time_point m_start, m_begin, m_renderEnd, m_previousEnd;
    bool m_running = false, m_previousValid = false, m_headers = false;
    std::uint64_t m_frames = 0, m_frameId = 0, m_drawSum = 0, m_drawMin = 0, m_drawMax = 0;
    std::uint64_t m_updates = 0;
    double m_renderSum = 0, m_renderMax = 0, m_swapSum = 0, m_swapMax = 0;
    double m_updateSum = 0, m_intervalMax = 0;
    double m_pendingUpdateMs = 0;
    RenderStatistics m_sum;
    std::string m_context;
};
