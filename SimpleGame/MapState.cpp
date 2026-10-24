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
    std::wstring Path(IslandKey key)
    {
        const auto base = Diagnostics::ExecutableDirectory();
        if (base.empty() || !Diagnostics::MakeDirectory(base + L"Saves"))
        {
            return {};
        }
        const auto folder = base + L"Saves\\world_map_v1_" + std::to_wstring(IslandGeneration::WorldSeed);
        if (!Diagnostics::MakeDirectory(folder))
        {
            return {};
        }
        return folder + L"\\island_" + std::to_wstring(key.x) + L"_" + std::to_wstring(key.z) + L".state";
    }
}

bool MapState::Visited(IslandKey key)
{
    const auto path = Path(key);
    if (path.empty())
    {
        return false;
    }
    std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
    if (!file)
    {
        return false;
    }
    const std::streamoff bytes = file.tellg();
    if (bytes <= 0 || bytes > 256)
    {
        return false;
    }
    file.seekg(0);
    std::string magic;
    std::uint64_t seed;
    std::int64_t x, z;
    int visited;
    if (!(file >> magic >> seed >> x >> z >> visited))
    {
        return false;
    }
    file >> std::ws;
    const bool valid = magic == "WORLD_MAP_V1" && seed == IslandGeneration::WorldSeed && x == key.x &&
                       z == key.z && visited == 1 && file.eof();
    if (valid)
    {
        Diagnostics::Get().Event(
            "map_visited_loaded x=" + std::to_string(key.x) + " z=" + std::to_string(key.z));
    }
    return valid;
}

bool MapState::SaveVisited(IslandKey key)
{
    const auto path = Path(key);
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
