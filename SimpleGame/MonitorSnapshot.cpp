#include "stdafx.h"
#include "Prototype.h"
#include "QuestRules.h"
#include <chrono>
#include <locale>
#include <map>
#include <sstream>

namespace
{
    void WriteString(std::ostream& out, const std::string& value)
    {
        constexpr char hex[] = "0123456789abcdef";
        out << '"';
        for (const unsigned char c : value)
        {
            if (c == '"' || c == '\\')
            {
                out << '\\' << static_cast<char>(c);
            }
            else if (c < 0x20)
            {
                out << "\\u00" << hex[c >> 4] << hex[c & 15];
            }
            else
            {
                out << static_cast<char>(c);
            }
        }
        out << '"';
    }

    void WriteKey(std::ostream& out, IslandKey key)
    {
        // Decimal strings preserve the full signed 64-bit coordinates in JavaScript.
        out << "{\"x\":\"" << key.x << "\",\"z\":\"" << key.z << "\"}";
    }

    void WritePosition(std::ostream& out, const WorldPosition& position)
    {
        out << "{\"chunkX\":\"" << position.chunkX << "\",\"chunkZ\":\"" << position.chunkZ
            << "\",\"localX\":" << position.x << ",\"localZ\":" << position.z << '}';
    }

    const char* QuestName(QuestStatus status)
    {
        switch (status)
        {
            case QuestStatus::Available:
                return "available";
            case QuestStatus::Active:
                return "active";
            case QuestStatus::Complete:
                return "complete";
        }
        return "unknown";
    }

    void WriteNpc(
        std::ostream& out,
        const NpcKey& key,
        const NpcSystem& system,
        const NpcRecord& state,
        const char* presence,
        int progress)
    {
        const auto& quest = QuestRules::Get(key.role);
        out << "{\"home\":";
        WriteKey(out, key.island);
        out << ",\"roleId\":" << key.role << ",\"role\":";
        WriteString(out, quest.roleName);
        out << ",\"name\":";
        WriteString(out, system.Name(key));
        out << ",\"presence\":\"" << presence << "\",\"affinity\":" << state.affinity
            << ",\"trust\":" << state.trust << ",\"respect\":" << state.respect << ",\"fear\":" << state.fear
            << ",\"resentment\":" << state.resentment << ",\"quest\":{\"status\":\"" << QuestName(state.quest)
            << "\",\"objective\":";
        WriteString(out, quest.objective);
        out << ",\"progress\":" << progress << ",\"target\":" << quest.target << "},\"latestMemory\":";
        WriteString(
            out,
            state.memoryCursor ? state.memory[(state.memoryCursor - 1) % state.memory.size()] : "");
        out << '}';
    }

    struct IslandChunks
    {
        size_t loaded = 0, active = 0;
    };
}

std::string Prototype::MonitorSnapshot() const
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::boolalpha;
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();
    const auto position = ActivePosition();
    const auto* current = m_world.CurrentIsland(position);
    out << "{\"schemaVersion\":1,\"timestampMs\":" << timestamp << ",\"map\":\""
        << (m_world.IsTestMap() ? "test" : "adventure")
        << "\",\"overview\":" << (m_world.IsTestMap() && m_testOverview) << ",\"player\":{\"position\":";
    WritePosition(out, position);
    out << ",\"island\":";
    if (current)
    {
        WriteKey(out, current->Info().key);
    }
    else
    {
        out << "null";
    }
    out << "},\"chunks\":{\"loaded\":" << m_world.Chunks().size() << ",\"active\":" << m_world.VisibleChunks()
        << ",\"pending\":" << m_world.PendingChunks() << "},\"islands\":[";

    std::map<IslandKey, IslandChunks> chunksByIsland;
    for (const auto& entry : m_world.Chunks())
    {
        const auto& chunk = entry.second;
        if (chunk.island)
        {
            auto& count = chunksByIsland[chunk.island->Info().key];
            ++count.loaded;
            count.active += chunk.visible;
        }
    }

    bool first = true;
    for (const auto* layout : m_world.IslandLayouts())
    {
        const auto& info = layout->Info();
        const auto count = chunksByIsland[info.key];
        if (!m_world.IsTestMap() && !count.loaded)
        {
            continue;
        }
        out << (first ? "" : ",") << "{\"key\":";
        first = false;
        WriteKey(out, info.key);
        out << ",\"name\":";
        WriteString(out, MapCatalog::Island(info.shape).name);
        out << ",\"center\":";
        WritePosition(out, info.center);
        out << ",\"visited\":" << m_world.WasVisited(info.key) << ",\"starter\":" << info.starter
            << ",\"widthM\":" << info.width << ",\"landAreaM2\":" << layout->LandArea()
            << ",\"villageAreaM2\":" << layout->VillageArea() << ",\"loadedChunks\":" << count.loaded
            << ",\"activeChunks\":" << count.active << ",\"monsters\":";
        if (count.loaded)
        {
            const auto population = m_monsters->Population(info.key);
            out << "{\"spawned\":" << population.spawned << ",\"alive\":" << population.alive
                << ",\"active\":" << population.active << ",\"epicAlive\":" << population.epicAlive << '}';
        }
        else
        {
            out << "null";
        }

        size_t residentNpcs = 0, activeNpcs = 0;
        out << ",\"npcs\":[";
        for (unsigned role = 0; role < NpcRole::Count; ++role)
        {
            if (role != 0)
            {
                out << ',';
            }
            const NpcKey key{info.key, role};
            const auto& state = m_state.npcs.Npc(key);
            const bool active = m_npcs->VillageNpcActive(key);
            residentNpcs += count.loaded && !state.recruited;
            activeNpcs += active;
            const char* presence = state.recruited ? "companion"
                                   : active        ? "active"
                                   : count.loaded  ? "resident"
                                                   : "unloaded";
            WriteNpc(
                out,
                key,
                *m_npcs,
                state,
                presence,
                m_state.npcs.Progress(key, info.shape, m_state.economy));
        }
        out << "],\"residentNpcCount\":" << residentNpcs << ",\"activeNpcCount\":" << activeNpcs << '}';
    }
    out << "],\"companions\":[";
    first = true;
    for (const auto& key : m_state.npcs.Companions())
    {
        out << (first ? "" : ",");
        first = false;
        const auto& state = m_state.npcs.Npc(key);
        WriteNpc(out, key, *m_npcs, state, "companion", state.progress);
    }
    out << "]}";
    return out.str();
}
