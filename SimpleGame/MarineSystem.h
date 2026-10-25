#pragma once
#include "MarineActors.h"
#include "PlayerEconomy.h"
#include "World.h"
#include "PirateRaid.h"
#include <array>
#include <random>
#include <string>

enum class SeaEncounter
{
    None,
    Pirates,
    Kraken,
    Storm
};

struct MarineMeshes
{
    MarineMeshes();
    CachedMesh pirate, tentacle, tornado, raider, cutlass, chest;
};

class MarineSystem
{
public:
    MarineSystem(
        SceneGraph& scene,
        World& world,
        PlayerEconomy& economy,
        ActorId shipRoot,
        std::function<bool(int)> hitPlayer);
    ~MarineSystem();
    void Reset();
    void SetActive(bool active);
    // Returns true when a destroyed ship needs the existing home/recovery flow.
    bool Update(
        float dt,
        const WorldPosition& ship,
        bool aboard,
        float shipHeading,
        const RaidPlayer& player);

    PirateRaid& Raid()
    {
        return *m_raid;
    }

    const PirateRaid& Raid() const
    {
        return *m_raid;
    }

    void Prepare(const WorldPosition& camera, const WorldPosition& ship);
    bool Attack(const WorldPosition& player, Vec3 aim, int damage, float shipHeading, bool aboard);
    void EscapeKey(unsigned char key);
    // Test shortcut: replace the current encounter without rewards or cargo loss.
    bool ForceStart(SeaEncounter kind, const WorldPosition& ship);

    float ShipHealth() const
    {
        return m_health;
    }

    bool Captured() const
    {
        return m_captured;
    }

    int EscapeCount() const
    {
        return m_escapeCount;
    }

    float EscapeTime() const
    {
        return m_escapeTime;
    }

    float Light() const;

    SeaEncounter Encounter() const
    {
        return m_encounter;
    }

    const std::string& Notice() const
    {
        return m_notice;
    }

private:
    MarineTargetActor* Target(ActorId id);
    void Begin(SeaEncounter kind, const WorldPosition& ship);
    void EndStorm();
    void Patrol(const WorldPosition& ship);
    bool SafeSea(const WorldPosition& position, float heading) const;
    void DamageShip(float damage);
    void Notify(const std::string& message);
    SceneGraph& m_scene;
    World& m_world;
    PlayerEconomy& m_economy;
    ActorId m_root, m_weather, m_shipRoot;
    std::array<ActorId, 2> m_pirates;
    std::array<ActorId, 4> m_tentacles;
    std::array<ActorId, 3> m_tornadoes;
    MarineMeshes m_meshes;
    std::mt19937 m_random;
    std::unique_ptr<PirateRaid> m_raid;
    WorldPosition m_patrolAnchor, m_ship;
    SeaEncounter m_encounter = SeaEncounter::None;
    bool m_havePatrol = false, m_captured = false;
    int m_tornadoCount = 0, m_escapeCount = 0;
    unsigned char m_lastEscape = 0;
    float m_health = 100, m_time = 0, m_krakenRoll = 0, m_stormRoll = 0;
    float m_escapeTime = 0, m_stormTime = 0, m_attackCooldown = 0;
    std::string m_notice;
};
