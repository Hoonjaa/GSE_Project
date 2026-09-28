#pragma once
#include <cstdint>
#include <fstream>
#include <string>

class Diagnostics
{
public:
    static Diagnostics& Get();
    static std::wstring ExecutableDirectory();
    static bool MakeDirectory(const std::wstring& path);
    void Event(const std::string& text);
    void Flush();
    std::ofstream samples, batches;
    std::ofstream frames;
    bool detailedFrames = false;
    std::uint64_t memoryHits = 0, diskHits = 0, generated = 0, cacheErrors = 0;

private:
    Diagnostics();
    std::ofstream m_events;
};
