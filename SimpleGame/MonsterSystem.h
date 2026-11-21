#pragma once
#include "MonsterActor.h"
#include "MonsterMeshes.h"
#include "World.h"
#include <map>
#include <memory>
#include "MonsterItemDrop.h"
#include <random>

struct MonsterPopulation
{
    size_t spawned = 0, alive = 0, active = 0, epicAlive = 0;
};

class MonsterSystem
{
public:
    MonsterSystem(
        SceneGraph& scene,
        World& world,
        std::function<void(int)> hitPlayer,
        std::function<void(IslandKey, bool)> defeated,
        std::function<bool(InventoryItem)> collect);
    ~MonsterSystem();
    void Update(float dt, const WorldPosition& player, bool onLand, bool paused = false);
    void SetActive(bool active);
    void Prepare(const WorldPosition& camera, bool showStatus = true);
    void Attack(const WorldPosition& player, Vec3 aim, int damage);
    MonsterPopulation Population(IslandKey island) const;
    bool AttackWarning(const WorldPosition& player) const;

private:
    struct Group
    {
        explicit Group(std::shared_ptr<const IslandLayout> map) : layout(std::move(map)), context(*layout)
        {
        }

        std::shared_ptr<const IslandLayout> layout;
        MonsterContext context;
        ActorId root = NoActor;
        std::vector<ActorId> monsters;
        std::vector<ActorId> drops;
        bool seen = false;
        float flowTime = 0, coarseTime = 0;
        int flowCell = -1;
    };

    void Spawn(Group& group);
    void Sync();
    WorldPosition Position(const Group& group, Vec3 local) const;
    SceneGraph& m_scene;
    World& m_world;
    MonsterMeshes m_meshes;
    CachedMesh m_dropMaterial, m_dropWeapon, m_dropArmor;
    std::mt19937 m_random;
    std::function<void(int)> m_hitPlayer;
    std::function<void(IslandKey, bool)> m_defeated;
    std::function<bool(InventoryItem)> m_collect;
    std::map<IslandKey, std::unique_ptr<Group>> m_groups;
};
