#pragma once
#include "SceneGraph.h"
#include "WorldPosition.h"
#include "IslandTerrainAdapter.h"
#include <deque>
#include <map>
#include <set>
#include <utility>
#include <vector>

struct IslandChunk
{
    ActorId actor = NoActor, waves = NoActor;
    std::shared_ptr<const IslandLayout> island;
    CachedMesh terrain;
    bool visible = false;
};

struct TownInteraction
{
    TownServiceKind kind = TownServiceKind::None;
    IslandKey island;
    WorldPosition position;
};

class World
{
public:
    using Key = std::pair<std::int64_t, std::int64_t>;
    static constexpr std::uint64_t Seed = IslandGeneration::WorldSeed;
    explicit World(SceneGraph& scene);
    ~World();
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    void Update(const WorldPosition& player, int radius);
    void SwitchMap();

    bool IsTestMap() const
    {
        return m_testMap;
    }

    WorldPosition TestSpawn(unsigned index) const;
    WorldPosition TestShip(unsigned index, float length) const;
    void PrepareWindow(const WorldPosition& player, int radius);
    void Prepare(const WorldPosition& camera, float time, bool grid);
    bool CanWalk(const WorldPosition& position) const;
    bool CanSail(const WorldPosition& position, float heading, float length) const;
    bool IsActive(const WorldPosition& position) const;
    bool ConservativeSea(const WorldPosition& position, float radius) const;

    const IslandTerrainAdapter& StartingTerrain() const
    {
        return m_startTerrain;
    }

    const IslandChunk* At(const WorldPosition& position) const;

    const std::map<Key, IslandChunk>& Chunks() const
    {
        return m_chunks;
    }

    const IslandLayout* CurrentIsland(const WorldPosition& position) const;
    WorldPosition StartSpawn() const;
    WorldPosition StartShip(float length = 8) const;
    WorldPosition ShipyardMooring(IslandKey key, float length) const;
    float OutboundHeading(IslandKey key) const;
    TownInteraction ServiceAt(const WorldPosition& player) const;
    WorldPosition StartVillage() const;
    void DrawMap(Renderer& renderer, const WorldPosition& center, float left, float top, float extent) const;

    size_t PendingChunks() const
    {
        return m_pending.size();
    }

    size_t VisibleChunks() const;
    bool WasVisited(IslandKey key) const;
    std::vector<const IslandLayout*> IslandLayouts() const;

private:
    struct ResidentIsland
    {
        std::shared_ptr<IslandLayout> layout;
        bool visited = false, dirty = false;
    };

    struct ParkedMap
    {
        ActorId root = NoActor;
        std::map<Key, IslandChunk> chunks;
        std::map<IslandKey, ResidentIsland> islands;
        std::deque<Key> pending;
        Key center = {0, 0};
        int radius = -1;
    };

    void Generate(Key key);
    void PlaceProps(IslandChunk& chunk, const WorldPosition& origin);
    bool LandAt(const WorldPosition& position, float margin) const;
    void Visit(const WorldPosition& position);
    void RebuildQueue(const WorldPosition& position);
    SceneGraph& m_scene;
    ActorId m_root = NoActor, m_ocean = NoActor, m_chunksRoot = NoActor;
    std::shared_ptr<IslandLayout> m_start;
    IslandTerrainAdapter m_startTerrain;
    CachedMesh m_house, m_palm, m_pier;
    std::map<Key, IslandChunk> m_chunks;
    std::map<IslandKey, ResidentIsland> m_islands;
    std::deque<Key> m_pending;
    Key m_center = {0, 0};
    int m_radius = -1;
    bool m_testMap = false;
    ParkedMap m_parked;
};
