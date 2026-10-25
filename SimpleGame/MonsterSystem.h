#pragma once
#include "MonsterActor.h"
#include "World.h"
#include <map>
#include <memory>

class MonsterSystem
{
public:
    MonsterSystem(
        SceneGraph& scene,
        World& world,
        std::function<void(int)> hitPlayer,
        std::function<void(bool)> defeated);
    ~MonsterSystem();
    void Update(float dt, const WorldPosition& player, bool onLand, bool paused = false);
    void SetActive(bool active);
    void Prepare(const WorldPosition& camera, bool showStatus = true);
    void Attack(const WorldPosition& player, Vec3 aim, int damage);

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
    std::function<void(int)> m_hitPlayer;
    std::function<void(bool)> m_defeated;
    std::map<IslandKey, std::unique_ptr<Group>> m_groups;
};
