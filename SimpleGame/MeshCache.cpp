#include "stdafx.h"
#include "MeshCache.h"
#include "Diagnostics.h"
#include <cmath>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
    constexpr std::uint32_t Format = 1, MaxVertices = 1000000;
    constexpr size_t HeaderSize = 24, VertexBytes = 32;
    static_assert(sizeof(float) == 4, "Mesh cache requires 32-bit float.");
    using Bytes = std::vector<unsigned char>;

    void Put(Bytes& data, std::uint64_t value, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            data.push_back(static_cast<unsigned char>(value >> (i * 8)));
        }
    }

    std::uint64_t Read(const Bytes& data, size_t& position, size_t count)
    {
        std::uint64_t value = 0;
        for (size_t i = 0; i < count; ++i)
        {
            value |= std::uint64_t(data[position++]) << (i * 8);
        }
        return value;
    }

    std::uint64_t Hash(const Bytes& data, size_t begin)
    {
        std::uint64_t hash = 14695981039346656037ULL;
        for (size_t i = begin; i < data.size(); ++i)
        {
            hash = (hash ^ data[i]) * 1099511628211ULL;
        }
        return hash;
    }

    bool Valid(const Mesh& mesh)
    {
        if (mesh.empty() || mesh.size() > MaxVertices || mesh.size() % 3 != 0)
        {
            return false;
        }
        for (const auto& v : mesh)
        {
            const float values[] =
                {v.position.x, v.position.y, v.position.z, v.color.r, v.color.g, v.color.b, v.uv.x, v.uv.y};
            for (float f : values)
            {
                if (!std::isfinite(f) || std::abs(f) > 1000000.f)
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool Load(const std::wstring& path, Mesh& mesh, std::string& reason)
    {
        std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
        if (!file)
        {
            reason = "missing_or_unreadable";
            return false;
        }
        const std::streamoff size = file.tellg();
        if (size < std::streamoff(HeaderSize) ||
            size > std::streamoff(HeaderSize + MaxVertices * VertexBytes))
        {
            reason = "invalid_file_size";
            return false;
        }
        Bytes bytes(static_cast<size_t>(size));
        file.seekg(0);
        if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        {
            reason = "read_failed";
            return false;
        }
        if (std::memcmp(bytes.data(), "SGMESH01", 8) != 0)
        {
            reason = "invalid_magic";
            return false;
        }
        size_t p = 8;
        const auto version = Read(bytes, p, 4), count = Read(bytes, p, 4), checksum = Read(bytes, p, 8);
        if (version != Format || !count || count > MaxVertices || count % 3 ||
            bytes.size() != HeaderSize + count * VertexBytes || checksum != Hash(bytes, HeaderSize))
        {
            reason = "invalid_version_count_or_checksum";
            return false;
        }
        mesh.reserve(static_cast<size_t>(count));
        for (size_t i = 0; i < count; ++i)
        {
            float f[8];
            for (auto& value : f)
            {
                const auto bits = static_cast<std::uint32_t>(Read(bytes, p, 4));
                std::memcpy(&value, &bits, 4);
            }
            mesh.push_back({{f[0], f[1], f[2]}, {f[3], f[4], f[5]}, {f[6], f[7]}});
        }
        if (!Valid(mesh))
        {
            mesh.clear();
            reason = "invalid_vertex";
            return false;
        }
        return true;
    }

    bool Save(const std::wstring& path, const Mesh& mesh)
    {
        Bytes payload;
        payload.reserve(mesh.size() * VertexBytes);
        for (const auto& v : mesh)
        {
            const float values[] =
                {v.position.x, v.position.y, v.position.z, v.color.r, v.color.g, v.color.b, v.uv.x, v.uv.y};
            for (float value : values)
            {
                std::uint32_t bits;
                std::memcpy(&bits, &value, 4);
                Put(payload, bits, 4);
            }
        }
        Bytes header = {'S', 'G', 'M', 'E', 'S', 'H', '0', '1'};
        Put(header, Format, 4);
        Put(header, mesh.size(), 4);
        Put(header, Hash(payload, 0), 8);
        const auto temp = path + L".tmp." + std::to_wstring(GetCurrentProcessId());
        bool written = false;
        {
            std::ofstream file(temp.c_str(), std::ios::binary | std::ios::trunc);
            if (file)
            {
                file.write(
                    reinterpret_cast<const char*>(header.data()),
                    static_cast<std::streamsize>(header.size()));
                file.write(
                    reinterpret_cast<const char*>(payload.data()),
                    static_cast<std::streamsize>(payload.size()));
                file.flush();
                written = file.good();
                file.close();
                written = written && !file.fail();
            }
        }
        if (written &&
            MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            return true;
        }
        DeleteFileW(temp.c_str());
        return false;
    }
}

CachedMesh MeshCache::LoadOrCreate(const std::string& key, const std::function<Mesh()>& factory)
{
    if (key.empty() || key.size() > 80 ||
        key.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") !=
            std::string::npos)
    {
        throw std::invalid_argument("Mesh cache key must be a safe versioned asset name.");
    }
    static std::map<std::string, std::weak_ptr<const Mesh>> memory;
    auto& diagnostics = Diagnostics::Get();
    if (auto shared = memory[key].lock())
    {
        ++diagnostics.memoryHits;
        CachedMesh result;
        result.m_vertices = std::move(shared);
        result.m_name = key;
        return result;
    }
    Mesh mesh;
    const auto start = std::chrono::steady_clock::now();
    std::string reason = "cache_directory_unavailable";
    const auto base = Diagnostics::ExecutableDirectory();
    const auto folder = base + L"Cache\\Meshes";
    const bool writable =
        !base.empty() && Diagnostics::MakeDirectory(base + L"Cache") && Diagnostics::MakeDirectory(folder);
    const auto path = folder + L"\\" + std::wstring(key.begin(), key.end()) + L".mesh";
    if (writable && Load(path, mesh, reason))
    {
        ++diagnostics.diskHits;
        diagnostics.Event("mesh_disk_hit key=" + key + " vertices=" + std::to_string(mesh.size()));
    }
    else
    {
        if (reason != "missing_or_unreadable")
        {
            ++diagnostics.cacheErrors;
        }
        mesh = factory();
        if (!Valid(mesh))
        {
            throw std::runtime_error("Invalid generated mesh: " + key);
        }
        ++diagnostics.generated;
        diagnostics.Event(
            "mesh_generated key=" + key + " reason=" + reason + " vertices=" + std::to_string(mesh.size()));
        if (!writable || !Save(path, mesh))
        {
            ++diagnostics.cacheErrors;
            diagnostics.Event("mesh_save_failed key=" + key + " memory_fallback=1");
            std::cerr << "[MeshCache] Save failed; using memory: " << key << '\n';
        }
        else
        {
            diagnostics.Event("mesh_saved key=" + key);
        }
    }
    diagnostics.Event(
        "mesh_ready key=" + key + " cpu_ms=" +
        std::to_string(
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()));
    CachedMesh result(std::move(mesh), key);
    memory[key] = result.m_vertices;
    return result;
}
