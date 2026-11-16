#pragma once
#include "World.h"
#include "PlayerEconomy.h"
#include "NpcDialogue.h"
#include "NpcState.h"

struct NpcSelection
{
    bool valid = false;
    NpcKey key;
    IslandShape shape = IslandShape::Round;
};

class NpcSystem
{
public:
    NpcSystem(SceneGraph& scene, World& world, PlayerEconomy& economy, NpcState& relations);
    ~NpcSystem();
    void SetActive(bool active);
    void Update(
        float dt,
        const WorldPosition& player,
        bool aboard,
        const WorldPosition& ship,
        float heading,
        bool paused);
    void Prepare(const WorldPosition& camera, bool showLabels);
    NpcSelection Nearby(const WorldPosition& player) const;
    void Open(const NpcSelection& selection);
    void Close();

    const std::string& Dialogue() const
    {
        return m_dialogue.Text();
    }

    std::string Name(const NpcKey& key) const;
    static const char* Role(unsigned role);
    static std::string Quest(unsigned role);
    int Progress(const NpcSelection& selection) const;
    bool VillageNpcActive(const NpcKey& key) const;

    void SetDialogueProvider(NpcDialogue::Provider provider)
    {
        m_dialogue.SetProvider(std::move(provider));
    }

private:
    struct Resident
    {
        std::shared_ptr<const IslandLayout> layout;
        ActorId root = NoActor;
        std::array<ActorId, NpcRole::Count> actors;
        std::array<Vec3, NpcRole::Count> positions;
        bool seen = false;
    };

    struct Follower
    {
        ActorId actor = NoActor;
        WorldPosition position;
        bool ready = false;
        NpcKey key;
        float height = .55f;
    };

    void Sync();
    void MoveFollower(Follower& follower, const WorldPosition& player, float dt);
    const NpcRecord& State(const NpcKey& key) const;
    SceneGraph& m_scene;
    World& m_world;
    PlayerEconomy& m_economy;
    NpcState& m_relations;
    ActorId m_root;
    std::array<CachedMesh, NpcRole::Count> m_models;
    std::array<Follower, 5> m_followers;
    std::map<IslandKey, Resident> m_residents;
    NpcDialogue m_dialogue;
    std::weak_ptr<const IslandLayout> m_followLayout;
    std::vector<int> m_followFlow;
    int m_followCell = -1;
    float m_flowTime = 0;
};
