#include "stdafx.h"
#include "NpcSystem.h"
#include "MeshCache.h"
#include "ShipDeck.h"
#include "Diagnostics.h"
#include <algorithm>
#include <cmath>

namespace
{
    class QuestNpcActor : public MeshActor
    {
    public:
        QuestNpcActor(CachedMesh mesh, std::string name) : MeshActor(std::move(mesh), std::move(name))
        {
            enabled = false;
        }

        bool label = true;
        std::string status;

        void Overlay(Renderer& r, const ActorTransform& world) const override
        {
            if (!label)
            {
                return;
            }
            auto p = world.position;
            p.y += 2.1f;
            const auto screen = r.Project(p);
            if (screen.x < 0 || screen.x > r.Width() || screen.y < 0 || screen.y > r.Height())
            {
                return;
            }
            r.Text(screen.x - 45, screen.y, status, {.99f, .79f, .37f}, 1.25f);
        }
    };

    WorldPosition Position(const IslandLayout& map, Vec3 local)
    {
        auto p = map.Info().center;
        p.Move(local.x, local.z);
        return p;
    }

    float Distance(Vec3 a, Vec3 b)
    {
        return std::hypot(a.x - b.x, a.z - b.z);
    }
}

NpcSystem::NpcSystem(SceneGraph& scene, World& world, PlayerEconomy& economy)
    : m_scene(scene), m_world(world), m_economy(economy)
{
    m_root = m_scene.Create<Actor>(NoActor, "NPC와 동료").Id();
    for (unsigned role = 0; role < 3; ++role)
    {
        m_models[role] = MeshCache::LoadOrCreate(
            "quest_npc_v1_" + std::to_string(role),
            [role]
            {
                Mesh m;
                const Color c = role == 0 ? Color{.34f, .56f, .3f}
                                          : (role == 1 ? Color{.56f, .4f, .74f} : Color{.8f, .52f, .22f});
                Geometry::Box(m, {-.16f, 0, -.1f}, {.13f, .5f, .2f}, {.24f, .24f, .26f});
                Geometry::Box(m, {.03f, 0, -.1f}, {.13f, .5f, .2f}, {.24f, .24f, .26f});
                Geometry::Box(m, {-.22f, .45f, -.14f}, {.44f, .6f, .28f}, c);
                Geometry::Box(m, {-.16f, 1.05f, -.13f}, {.32f, .34f, .26f}, {.87f, .66f, .45f});
                Geometry::Box(m, {-.24f, 1.38f, -.2f}, {.48f, .12f, .4f}, c);
                return m;
            });
    }
    for (auto& follower : m_followers)
    {
        follower.actor = m_scene.Create<QuestNpcActor>(m_root, m_models[0], "동료").Id();
        m_scene.Find(follower.actor)->visible = false;
    }
}

NpcSystem::~NpcSystem()
{
    m_scene.Destroy(m_root);
    m_scene.CollectDestroyed();
}

const NpcRecord& NpcSystem::State(const NpcKey& key) const
{
    return static_cast<const PlayerEconomy&>(m_economy).Npc(key);
}

const char* NpcSystem::Role(unsigned role)
{
    return role == 0 ? "사냥꾼" : (role == 1 ? "수집가" : "상인");
}

const char* NpcSystem::Quest(unsigned role)
{
    return role == 0 ? "이 섬의 일반 몬스터 3마리를 처치해 주세요."
                     : (role == 1 ? "이 섬 몬스터의 전리품 3개를 가져다 주세요."
                                  : "이 섬 상점에서 특산품을 3개 구매해 주세요.");
}

std::string NpcSystem::Name(const NpcKey& key) const
{
    const char* names[] =
        {"로안", "미라", "세온", "루카", "다나", "테오", "하린", "유나", "레온", "루미", "이안", "소라"};
    const auto hash = IslandGeneration::Mix(
        static_cast<std::uint64_t>(key.island.x) ^
        IslandGeneration::Mix(static_cast<std::uint64_t>(key.island.z)));
    return std::string(Role(key.role)) + " " + names[(hash + key.role * 3) % 12];
}

void NpcSystem::SetActive(bool active)
{
    auto* root = m_scene.Find(m_root);
    root->visible = root->enabled = active;
    if (!active)
    {
        Close();
    }
}

void NpcSystem::Sync()
{
    for (auto& entry : m_residents)
    {
        entry.second.seen = false;
    }
    for (const auto& chunk : m_world.Chunks())
    {
        if (!chunk.second.island)
        {
            continue;
        }
        const auto key = chunk.second.island->Info().key;
        auto found = m_residents.find(key);
        if (found == m_residents.end())
        {
            Resident resident;
            resident.layout = chunk.second.island;
            resident.root = m_scene.Create<Actor>(m_root, "마을 퀘스트 NPC").Id();
            std::vector<Vec3> points;
            for (int i = 0; i < IslandLayout::Size * IslandLayout::Size; ++i)
            {
                const auto p = resident.layout->Center(i);
                if (resident.layout->Village(p.x, p.z) && resident.layout->Walk(p.x, p.z) &&
                    Distance(p, resident.layout->ShopPoint()) > 3 &&
                    Distance(p, resident.layout->ShipyardPoint()) > 3)
                {
                    points.push_back(p);
                }
            }
            if (points.size() < 3)
            {
                points.clear();
                for (int i = 0; i < IslandLayout::Size * IslandLayout::Size; ++i)
                {
                    const auto p = resident.layout->Center(i);
                    if (resident.layout->Village(p.x, p.z) && resident.layout->Walk(p.x, p.z))
                    {
                        points.push_back(p);
                    }
                }
            }
            for (unsigned role = 0; role < 3; ++role)
            {
                Vec3 best = resident.layout->Spawn();
                float score = -1;
                for (const auto& p : points)
                {
                    float spacing = Distance(p, resident.layout->Spawn());
                    for (unsigned n = 0; n < role; ++n)
                    {
                        spacing = (std::min)(spacing, Distance(p, resident.positions[n]));
                    }
                    if (spacing > score)
                    {
                        best = p;
                        score = spacing;
                    }
                }
                resident.positions[role] = best;
                auto& actor = m_scene.Create<QuestNpcActor>(resident.root, m_models[role], Name({key, role}));
                resident.actors[role] = actor.Id();
                actor.transform.position = best;
            }
            found = m_residents.emplace(key, std::move(resident)).first;
            Diagnostics::Get().Event(
                "npc_village_created x=" + std::to_string(key.x) + " z=" + std::to_string(key.z) +
                " count=3");
        }
        found->second.seen = true;
    }
    for (auto it = m_residents.begin(); it != m_residents.end();)
    {
        if (!it->second.seen)
        {
            m_scene.Destroy(it->second.root);
            it = m_residents.erase(it);
        }
        else
        {
            ++it;
        }
    }
    m_scene.CollectDestroyed();
}

void NpcSystem::MoveFollower(Follower& follower, const WorldPosition& player, float dt)
{
    auto delta = player.RelativeTo(follower.position);
    const float length = std::hypot(delta.x, delta.z);
    const auto* island = m_world.CurrentIsland(player);
    if (!follower.ready || length > 20 || m_world.CurrentIsland(follower.position) != island)
    {
        follower.position = player;
        follower.ready = true;
        return;
    }
    if (!island || length < 1.2f)
    {
        return;
    }
    auto goal = player;
    const auto from = follower.position.RelativeTo(island->Info().center),
               target = player.RelativeTo(island->Info().center);
    if (!island->Segment(from.x, from.z, target.x, target.z))
    {
        const int cell = island->Index(from.x, from.z);
        if (cell < 0 || m_followFlow.size() != IslandLayout::Size * IslandLayout::Size)
        {
            return;
        }
        int best = cell;
        for (const auto& d : {std::pair<int, int>{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
        {
            const int x = cell % IslandLayout::Size + d.first, z = cell / IslandLayout::Size + d.second;
            if (x < 0 || x >= IslandLayout::Size || z < 0 || z >= IslandLayout::Size)
            {
                continue;
            }
            const int next = z * IslandLayout::Size + x;
            if (m_followFlow[next] >= 0 &&
                (m_followFlow[best] < 0 || m_followFlow[next] < m_followFlow[best]))
            {
                best = next;
            }
        }
        goal = Position(*island, island->Center(best));
        delta = goal.RelativeTo(follower.position);
    }
    const float distance = std::hypot(delta.x, delta.z);
    if (distance < .01f)
    {
        return;
    }
    const float step = (std::min)(distance, dt * 3.6f);
    auto next = follower.position;
    next.Move(delta.x / distance * step, delta.z / distance * step);
    const auto local = next.RelativeTo(island->Info().center);
    if (m_world.CanWalk(next) && island->Segment(from.x, from.z, local.x, local.z))
    {
        follower.position = next;
    }
}

void NpcSystem::Update(
    float dt,
    const WorldPosition& player,
    bool aboard,
    const WorldPosition& ship,
    float heading,
    bool paused)
{
    Sync();
    m_dialogue.Poll();
    if (paused)
    {
        return;
    }
    const auto* island = m_world.CurrentIsland(player);
    m_flowTime -= dt;
    const auto& companions = m_economy.Companions();
    if (island && !aboard && !companions.empty())
    {
        const auto p = player.RelativeTo(island->Info().center);
        const int cell = island->Index(p.x, p.z);
        if (m_followLayout.lock().get() != island || (m_flowTime <= 0 && m_followCell != cell))
        {
            const auto* chunk = m_world.At(player);
            m_followLayout = chunk ? chunk->island : nullptr;
            island->Flow(p.x, p.z, m_followFlow);
            m_followCell = cell;
            m_flowTime = .25f;
        }
    }
    if (!island || aboard || companions.empty())
    {
        m_followLayout.reset();
        m_followFlow.clear();
        m_followCell = -1;
    }
    for (unsigned i = 0; i < m_followers.size(); ++i)
    {
        auto& follower = m_followers[i];
        if (i >= companions.size())
        {
            follower.ready = false;
            continue;
        }
        if (!follower.ready || !(follower.key == companions[i]))
        {
            follower.key = companions[i];
            follower.ready = false;
        }
        if (aboard)
        {
            const Vec3 local = {i % 2 ? .65f : -.65f, ShipDeck::Height(), -1.1f - float(i / 2) * .6f};
            follower.position = ship;
            const auto d = ShipDeck::ToWorld(local, heading);
            follower.position.Move(d.x, d.z);
            follower.height = ShipDeck::Height() + .03f;
            follower.ready = true;
        }
        else
        {
            auto goal = player;
            const auto offset = ShipDeck::ToWorld({i % 2 ? .8f : -.8f, 0, 1.3f}, heading);
            goal.Move(offset.x, offset.z);
            if (!m_world.CanWalk(goal) || m_world.CurrentIsland(goal) != island)
            {
                goal = player;
            }
            MoveFollower(follower, goal, dt);
            follower.height = .55f;
        }
    }
}

void NpcSystem::Prepare(const WorldPosition& camera, bool showLabels)
{
    for (auto& entry : m_residents)
    {
        auto& resident = entry.second;
        m_scene.Find(resident.root)->transform.position = resident.layout->Info().center.RelativeTo(camera);
        for (unsigned role = 0; role < 3; ++role)
        {
            auto* actor = m_scene.Get<QuestNpcActor>(resident.actors[role]);
            const NpcKey key = {entry.first, role};
            const auto& state = State(key);
            actor->visible =
                !state.recruited && m_world.IsActive(Position(*resident.layout, resident.positions[role]));
            actor->label = showLabels;
            actor->status =
                (state.quest == QuestStatus::Available ? "[!] "
                                                       : (state.quest == QuestStatus::Active ? "[?] " : "")) +
                Name(key);
        }
    }
    const auto& companions = m_economy.Companions();
    for (unsigned i = 0; i < m_followers.size(); ++i)
    {
        auto& follower = m_followers[i];
        auto* actor = m_scene.Get<QuestNpcActor>(follower.actor);
        actor->visible = i < companions.size() && follower.ready && m_world.IsActive(follower.position);
        actor->label = showLabels;
        if (i < companions.size())
        {
            actor->mesh = m_models[companions[i].role];
            actor->status = "[동료] " + Name(companions[i]);
        }
        actor->transform.position = follower.position.RelativeTo(camera);
        actor->transform.position.y = follower.height;
    }
}

NpcSelection NpcSystem::Nearby(const WorldPosition& player) const
{
    NpcSelection result;
    float nearest = 2.1f;
    for (const auto& entry : m_residents)
    {
        for (unsigned role = 0; role < 3; ++role)
        {
            const NpcKey key = {entry.first, role};
            if (State(key).recruited)
            {
                continue;
            }
            const auto p = Position(*entry.second.layout, entry.second.positions[role]);
            if (!m_world.IsActive(p))
            {
                continue;
            }
            const auto d = p.RelativeTo(player);
            const float distance = std::hypot(d.x, d.z);
            if (distance < nearest)
            {
                nearest = distance;
                result = {true, key, entry.second.layout->Info().shape};
            }
        }
    }
    return result;
}

int NpcSystem::Progress(const NpcSelection& selection) const
{
    if (State(selection.key).quest == QuestStatus::Complete)
    {
        return 3;
    }
    if (selection.key.role != 1)
    {
        return State(selection.key).progress;
    }
    std::int64_t total = 0;
    for (unsigned i = 0; i < 5; ++i)
    {
        total += (std::min)(
            std::int64_t(3),
            m_economy.ItemCount(
                ItemCatalog::MonsterMaterial(static_cast<unsigned>(selection.shape) * 5 + i)));
    }
    return int((std::min)(std::int64_t(3), total));
}

void NpcSystem::Open(const NpcSelection& selection)
{
    const auto& state = State(selection.key);
    NpcDialogueContext context;
    context.name = Name(selection.key);
    context.role = Role(selection.key.role);
    context.quest = Quest(selection.key.role);
    context.affinity = state.affinity;
    context.trust = state.trust;
    context.progress = Progress(selection);
    context.complete = state.quest == QuestStatus::Complete;
    context.recruited = state.recruited;
    if (state.memoryCursor)
    {
        context.latestMemory = state.memory[(state.memoryCursor - 1) % state.memory.size()];
    }
    m_dialogue.Open(std::move(context));
}

void NpcSystem::Close()
{
    m_dialogue.Close();
}
