#include "stdafx.h"
#include "MapState.h"
#include "Diagnostics.h"
#include <fstream>
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
    std::wstring Directory()
    {
        const auto base = Diagnostics::ExecutableDirectory();
        return base.empty() ? std::wstring{}
                            : base + L"Saves\\world_map_v1_" + std::to_wstring(IslandGeneration::WorldSeed);
    }

    std::wstring Name(IslandKey key)
    {
        return L"island_" + std::to_wstring(key.x) + L"_" + std::to_wstring(key.z) + L".state";
    }

    std::wstring SavePath(IslandKey key)
    {
        const auto base = Diagnostics::ExecutableDirectory();
        if (base.empty() || !Diagnostics::MakeDirectory(base + L"Saves"))
        {
            return {};
        }
        const auto folder = Directory();
        if (!Diagnostics::MakeDirectory(folder))
        {
            return {};
        }
        return folder + L"\\" + Name(key);
    }

    bool ReadVisited(const std::wstring& path, IslandKey& key)
    {
        std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
        const auto size = file.tellg();
        if (!file || size <= 0 || size > 256)
        {
            return false;
        }
        file.seekg(0);
        std::string magic;
        std::uint64_t seed;
        int visited;
        if (!(file >> magic >> seed >> key.x >> key.z >> visited))
        {
            return false;
        }
        file >> std::ws;
        return magic == "WORLD_MAP_V1" && seed == IslandGeneration::WorldSeed && visited == 1 && file.eof();
    }
}

std::set<IslandKey> MapState::VisitedIslands()
{
    std::set<IslandKey> result;
    auto directory = Directory();
    if (directory.empty())
    {
        return result;
    }
    directory += L"\\";
    WIN32_FIND_DATAW data{};
    const HANDLE files = FindFirstFileW((directory + L"island_*.state").c_str(), &data);
    if (files == INVALID_HANDLE_VALUE)
    {
        return result;
    }
    do
    {
        IslandKey key;
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            ReadVisited(directory + data.cFileName, key) && Name(key) == data.cFileName)
        {
            result.insert(key);
        }
    } while (FindNextFileW(files, &data));
    FindClose(files);
    return result;
}

bool MapState::SaveVisited(IslandKey key)
{
    const auto path = SavePath(key);
    if (path.empty())
    {
        Diagnostics::Get().Event(
            "map_visited_save_failed path_unavailable x=" + std::to_string(key.x) +
            " z=" + std::to_string(key.z));
        return false;
    }
    const auto temp = path + L".tmp." + std::to_wstring(GetCurrentProcessId());
    bool success = false;
    {
        std::ofstream file(temp.c_str(), std::ios::binary | std::ios::trunc);
        if (file)
        {
            file << "WORLD_MAP_V1 " << IslandGeneration::WorldSeed << ' ' << key.x << ' ' << key.z << " 1\n";
            file.flush();
            success = file.good();
            file.close();
            success &= !file.fail();
        }
    }
    if (success &&
        MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        Diagnostics::Get().Event(
            "map_visited_saved x=" + std::to_string(key.x) + " z=" + std::to_string(key.z));
        return true;
    }
    DeleteFileW(temp.c_str());
    Diagnostics::Get().Event(
        "map_visited_save_failed x=" + std::to_string(key.x) + " z=" + std::to_string(key.z));
    return false;
}
