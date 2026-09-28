#include "stdafx.h"
#include "FrameProfiler.h"
#include "Diagnostics.h"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

namespace
{
    double Milliseconds(std::chrono::steady_clock::duration duration)
    {
        return std::chrono::duration<double, std::milli>(duration).count();
    }

    std::string Csv(const std::string& value)
    {
        std::string escaped = "\"";
        for (char c : value)
        {
            if (c == '"')
            {
                escaped += '"';
            }
            escaped += c;
        }
        return escaped + '"';
    }
}

void FrameProfiler::Reset()
{
    m_running = false;
    m_previousValid = false;
    m_frames = m_drawSum = m_drawMax = m_updates = 0;
    m_drawMin = (std::numeric_limits<std::uint64_t>::max)();
    m_renderSum = m_renderMax = m_swapSum = m_swapMax = m_updateSum = m_intervalMax = 0;
    m_pendingUpdateMs = 0;
    m_sum.Reset();
}

void FrameProfiler::WriteHeaders()
{
    if (m_headers)
    {
        return;
    }
    auto& log = Diagnostics::Get();
    log.samples << "end_frame,seconds,frames,fps,draw_last,draw_avg,draw_min,draw_max,"
                   "world_draw_avg,dynamic_draw_avg,ui_draw_avg,post_draw_avg,submits_avg,instances_avg,"
                   "dynamic_vertices_avg,ui_vertices_avg,static_upload_bytes_avg,instance_upload_bytes_avg,"
                   "stream_upload_bytes_avg,gpu_hits,gpu_misses,gpu_evictions,resident_meshes,"
                   "cpu_update_ms_avg,update_count,cpu_render_ms_avg,cpu_render_ms_max,"
                   "swap_ms_avg,swap_ms_max,frame_interval_ms_max,actors,chunks,width,height,hdr,blur,"
                   "mesh_memory_hits_total,mesh_disk_hits_total,mesh_generated_total,mesh_errors_total\n";
    log.batches << "frame,mesh_id,asset,vertices,instances,draw_calls,instance_bytes\n";
    if (log.detailedFrames)
    {
        log.frames << "frame,draws,world,dynamic,ui,post,submits,instances,dynamic_vertices,"
                      "static_bytes,instance_bytes,stream_bytes,cpu_update_ms_since_last_frame,"
                      "cpu_render_ms,swap_ms,interval_ms,actors,chunks\n";
    }
    m_headers = true;
}

void FrameProfiler::BeginFrame()
{
    if (!m_running)
    {
        const double pending = m_pendingUpdateMs;
        Reset();
        m_pendingUpdateMs = pending;
        m_start = Clock::now();
        m_running = true;
    }
    WriteHeaders();
    m_begin = Clock::now();
    m_renderEnd = m_begin;
}

void FrameProfiler::RenderFinished()
{
    m_renderEnd = Clock::now();
}

void FrameProfiler::RecordUpdate(double milliseconds)
{
    if (m_running)
    {
        ++m_updates;
        m_updateSum += milliseconds;
    }
    m_pendingUpdateMs += milliseconds;
}

void FrameProfiler::EndFrame(const Renderer& renderer, size_t actors, size_t chunks, std::ostream& output)
{
    if (!m_running)
    {
        return;
    }
    const auto now = Clock::now();
    const auto& stats = renderer.Statistics();
    const auto draws = stats.DrawCallCount();
    const double render = Milliseconds(m_renderEnd - m_begin), swap = Milliseconds(now - m_renderEnd);
    const double interval = m_previousValid ? Milliseconds(now - m_previousEnd) : Milliseconds(now - m_begin);
    m_previousEnd = now;
    m_previousValid = true;
    ++m_frameId;
    ++m_frames;
    m_drawSum += draws;
    m_drawMin = (std::min)(m_drawMin, draws);
    m_drawMax = (std::max)(m_drawMax, draws);
    m_renderSum += render;
    m_renderMax = (std::max)(m_renderMax, render);
    m_swapSum += swap;
    m_swapMax = (std::max)(m_swapMax, swap);
    m_intervalMax = (std::max)(m_intervalMax, interval);
    m_sum.worldDraws += stats.worldDraws;
    m_sum.dynamicDraws += stats.dynamicDraws;
    m_sum.uiDraws += stats.uiDraws;
    m_sum.postDraws += stats.postDraws;
    m_sum.submissions += stats.submissions;
    m_sum.instances += stats.instances;
    m_sum.dynamicVertices += stats.dynamicVertices;
    m_sum.uiVertices += stats.uiVertices;
    m_sum.staticUploadBytes += stats.staticUploadBytes;
    m_sum.instanceUploadBytes += stats.instanceUploadBytes;
    m_sum.streamUploadBytes += stats.streamUploadBytes;
    m_sum.gpuCacheHits += stats.gpuCacheHits;
    m_sum.gpuCacheMisses += stats.gpuCacheMisses;
    m_sum.gpuCacheEvictions += stats.gpuCacheEvictions;
    auto& log = Diagnostics::Get();
    std::ostringstream context;
    context << "width=" << renderer.Width() << " height=" << renderer.Height()
            << " zoom=" << renderer.ZoomValue() << " hdr=" << renderer.HdrActive()
            << " blur=" << renderer.BlurActive() << " actors=" << actors << " chunks=" << chunks;
    if (context.str() != m_context)
    {
        m_context = context.str();
        log.Event("frame=" + std::to_string(m_frameId) + " context " + m_context);
    }
    if (log.detailedFrames && log.frames)
    {
        log.frames << m_frameId << ',' << draws << ',' << stats.worldDraws << ',' << stats.dynamicDraws << ','
                   << stats.uiDraws << ',' << stats.postDraws << ',' << stats.submissions << ','
                   << stats.instances << ',' << stats.dynamicVertices << ',' << stats.staticUploadBytes << ','
                   << stats.instanceUploadBytes << ',' << stats.streamUploadBytes << ',' << m_pendingUpdateMs
                   << ',' << render << ',' << swap << ',' << interval << ',' << actors << ',' << chunks
                   << '\n';
    }
    m_pendingUpdateMs = 0;
    const double seconds = std::chrono::duration<double>(now - m_start).count();
    if (seconds < 1.0)
    {
        return;
    }
    const double frames = static_cast<double>(m_frames);
    const double update = m_updates ? m_updateSum / m_updates : 0;
    std::ostringstream line;
    line << std::fixed << std::setprecision(1) << "[Frame] FPS=" << frames / seconds
         << " | Draw calls/frame: last=" << draws << " avg=" << m_drawSum / frames << " min=" << m_drawMin
         << " max=" << m_drawMax << " | world/UI/post=" << stats.worldDraws + stats.dynamicDraws << '/'
         << stats.uiDraws << '/' << stats.postDraws << " instances=" << stats.instances
         << " | CPU update/render/swap(ms)=" << update << '/' << m_renderSum / frames << '/'
         << m_swapSum / frames;
    output << line.str() << std::endl;
    if (log.samples)
    {
        log.samples << std::fixed << std::setprecision(4) << m_frameId << ',' << seconds << ',' << m_frames
                    << ',' << frames / seconds << ',' << draws << ',' << m_drawSum / frames << ','
                    << m_drawMin << ',' << m_drawMax << ',' << m_sum.worldDraws / frames << ','
                    << m_sum.dynamicDraws / frames << ',' << m_sum.uiDraws / frames << ','
                    << m_sum.postDraws / frames << ',' << m_sum.submissions / frames << ','
                    << m_sum.instances / frames << ',' << m_sum.dynamicVertices / frames << ','
                    << m_sum.uiVertices / frames << ',' << m_sum.staticUploadBytes / frames << ','
                    << m_sum.instanceUploadBytes / frames << ',' << m_sum.streamUploadBytes / frames << ','
                    << m_sum.gpuCacheHits << ',' << m_sum.gpuCacheMisses << ',' << m_sum.gpuCacheEvictions
                    << ',' << renderer.ResidentMeshes() << ',' << update << ',' << m_updates << ','
                    << m_renderSum / frames << ',' << m_renderMax << ',' << m_swapSum / frames << ','
                    << m_swapMax << ',' << m_intervalMax << ',' << actors << ',' << chunks << ','
                    << renderer.Width() << ',' << renderer.Height() << ',' << renderer.HdrActive() << ','
                    << renderer.BlurActive() << ',' << log.memoryHits << ',' << log.diskHits << ','
                    << log.generated << ',' << log.cacheErrors << '\n';
        log.samples.flush();
    }
    if (log.batches)
    {
        // Last complete frame in the window; not an average or every-frame trace.
        for (const auto& batch : renderer.Batches())
        {
            log.batches << m_frameId << ',' << batch.meshId << ',' << Csv(batch.name) << ',' << batch.vertices
                        << ',' << batch.instances << ",1," << batch.instances * sizeof(MeshInstance) << '\n';
        }
        log.batches.flush();
    }
    if (log.frames)
    {
        log.frames.flush();
    }
    log.Flush();
    Reset();
    m_start = now;
    m_previousEnd = now;
    m_previousValid = true;
    m_running = true;
}
