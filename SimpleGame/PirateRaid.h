#pragma once
#include "MarineActors.h"
#include "World.h"
#include "PlayerEconomy.h"
#include <array>
#include <functional>
#include <random>

struct RaidPlayer
{
    Vec3 deck = {};
    bool ownDeck = false;
};

struct PirateDeckInfo
{
    ActorId actor = NoActor;
    WorldPosition position;
    float heading = 0;
};

class PirateRaid
{
public:
    PirateRaid(
        SceneGraph& scene,
        World& world,
        PlayerEconomy& economy,
        ActorId root,
        const std::array<ActorId, 2>& ships,
        CachedMesh body,
        CachedMesh sword,
        CachedMesh chest,
        std::mt19937& random,
        std::function<void(const std::string&)> notify,
        std::function<bool(int)> hitPlayer);
    void Reset();
    bool Attached(int ship) const;
    bool Active() const;

    bool Failed() const
    {
        return m_failed;
    }

    void Attach(int ship, const WorldPosition& ownShip, float heading);
    void Update(float dt, float heading, const RaidPlayer& player);
    void Prepare(const WorldPosition& camera, const WorldPosition& ownShip, float heading);
    void Attack(Vec3 playerLocal, Vec3 aimLocal, int damage);
    bool Deck(int ship, PirateDeckInfo& info) const;
    bool Board(const WorldPosition& player, int& ship, Vec3& deck) const;
    bool Loot(int ship, Vec3 local);
    void Entered(int ship);
    void Returned(int ship);
    bool CanSail(const WorldPosition& ship, float heading) const;

    const std::string& Failure() const
    {
        return m_failure;
    }

private:
    struct Crew
    {
        ActorId actor;
        Vec3 local = {};
        float crossing = 0, cooldown = 2;
        int stage = 0; // 0 aboard pirate, 1 crossing, 2 fighting, 3 defeated.
    };

    struct Vessel
    {
        ActorId actor, chest;
        bool attached = false, cleared = false, finished = false, looted = false;
        bool visited = false, returned = false;
        float remaining = -1;
        int gold = 0, tentacles = 0;
    };

    MarineTargetActor* Ship(int index) const;
    BoardingPirateActor* Pirate(int index) const;
    void Cleared(int ship);
    void Fail(const std::string& reason);
    SceneGraph& m_scene;
    World& m_world;
    PlayerEconomy& m_economy;
    std::mt19937& m_random;
    std::function<void(const std::string&)> m_notify;
    std::function<bool(int)> m_hitPlayer;
    std::array<Vessel, 2> m_ships;
    std::array<Crew, 4> m_crew;
    bool m_failed = false;
    std::string m_failure;
};
