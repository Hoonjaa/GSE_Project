#include "stdafx.h"
#include "Diagnostics.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

Diagnostics& Diagnostics::Get()
{
    static Diagnostics diagnostics;
    return diagnostics;
}

std::wstring Diagnostics::ExecutableDirectory()
{
    std::vector<wchar_t> path(32768);
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!length || length >= path.size())
    {
        return {};
    }
    const std::wstring full(path.data(), length);
    return full.substr(0, full.find_last_of(L"/\\") + 1);
}

bool Diagnostics::MakeDirectory(const std::wstring& path)
{
    if (CreateDirectoryW(path.c_str(), nullptr))
    {
        return true;
    }
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

Diagnostics::Diagnostics()
{
    const auto base = ExecutableDirectory();
    if (base.empty() || !MakeDirectory(base + L"Logs"))
    {
        std::cerr << "[Profile] Log directory unavailable; console logging only.\n";
        return;
    }
    SYSTEMTIME time;
    GetSystemTime(&time);
    std::wostringstream name;
    name << base << L"Logs\\run_" << std::setfill(L'0') << std::setw(4) << time.wYear << std::setw(2)
         << time.wMonth << std::setw(2) << time.wDay << L"_" << std::setw(2) << time.wHour << std::setw(2)
         << time.wMinute << std::setw(2) << time.wSecond << L"_" << time.wMilliseconds << L"_"
         << GetCurrentProcessId();
    const auto folder = name.str();
    if (!MakeDirectory(folder))
    {
        std::cerr << "[Profile] Session directory unavailable; console logging only.\n";
        return;
    }
    m_events.open((folder + L"\\events.log").c_str(), std::ios::out);
    samples.open((folder + L"\\summary.csv").c_str(), std::ios::out);
    batches.open((folder + L"\\batches.csv").c_str(), std::ios::out);
    wchar_t detail[8] = {};
    detailedFrames = GetEnvironmentVariableW(L"GSE_PROFILE_DETAIL", detail, 8) == 1 && detail[0] == L'1';
    if (detailedFrames)
    {
        frames.open((folder + L"\\frames.csv").c_str(), std::ios::out);
        if (!frames)
        {
            std::cerr << "[Profile] Detailed frame log unavailable.\n";
        }
    }
    if (!m_events || !samples || !batches)
    {
        std::cerr << "[Profile] Some log files could not be opened.\n";
    }
    const auto relative = folder.substr(base.size());
    std::cout << "[Profile] Logs (beside executable): " << std::string(relative.begin(), relative.end())
              << '\n';
    Event("schema=2 batching=mesh_instancing animation=vertex_shader cache_format=1");
    Event(detailedFrames ? "detail_frames=1" : "detail_frames=0");
#ifdef _DEBUG
    Event("build=Debug");
#else
    Event("build=Release");
#endif
    Event("fps=completed_frames/wall_seconds cpu_times_are_not_gpu_times timer_ms=16");
}

void Diagnostics::Event(const std::string& text)
{
    if (m_events)
    {
        SYSTEMTIME time;
        GetSystemTime(&time);
        m_events << std::setfill('0') << std::setw(2) << time.wHour << ':' << std::setw(2) << time.wMinute
                 << ':' << std::setw(2) << time.wSecond << '.' << std::setw(3) << time.wMilliseconds
                 << " UTC " << text << '\n';
    }
}

void Diagnostics::Flush()
{
    if (m_events)
    {
        m_events.flush();
    }
    if (samples)
    {
        samples.flush();
    }
    if (batches)
    {
        batches.flush();
    }
    if (frames)
    {
        frames.flush();
    }
}
