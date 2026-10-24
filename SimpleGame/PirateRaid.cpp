#include "stdafx.h"
#include "PirateRaid.h"
#include "ShipDeck.h"
#include "Diagnostics.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr Vec3 Chest = {.65f, PirateDeck::Height + .02f, .4f};

    Vec3 Start(int index)
    {
        return {index % 2 ? .65f : -.65f, PirateDeck::Height, -1.1f};
    }

    WorldPosition Offset(WorldPosition p, Vec3 local, float heading)
    {
        const auto d = ShipDeck::ToWorld(local, heading);
        p.Move(d.x, d.z);
        return p;
    }

    bool HullsOverlap(Vec3 offset, float pirateHeading)
    {
        const float scale = ShipDeck::Scale();
        const Vec2 own[] = {
            {0, -1.925f * scale},
            {.85f * scale, -.725f * scale},
            {.85f * scale, 1.475f * scale},
            {0, 1.925f * scale},
            {-.85f * scale, 1.475f * scale},
            {-.85f * scale, -.725f * scale}};
        const Vec2 source[] = {{0, -4}, {1.5f, -3.5f}, {1.5f, 3.5f}, {-1.5f, 3.5f}, {-1.5f, -3.5f}};
        Vec2 pirate[5];
        for (int i = 0; i < 5; ++i)
        {
            const auto p = ShipDeck::ToWorld({source[i].x, 0, source[i].y}, pirateHeading);
            pirate[i] = {p.x + offset.x, p.z + offset.z};
        }
        auto separated = [&](const Vec2* hull, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                const auto a = hull[i], b = hull[(i + 1) % count];
                const float x = -(b.y - a.y), z = b.x - a.x;
                float ownMin = 1e9f, ownMax = -1e9f, pirateMin = 1e9f, pirateMax = -1e9f;
                for (const auto& p : own)
                {
                    const float projection = p.x * x + p.y * z;
                    ownMin = (std::min)(ownMin, projection);
                    ownMax = (std::max)(ownMax, projection);
                }
                for (const auto& p : pirate)
                {
                    const float projection = p.x * x + p.y * z;
                    pirateMin = (std::min)(pirateMin, projection);
                    pirateMax = (std::max)(pirateMax, projection);
                }
                if (ownMax < pirateMin || pirateMax < ownMin)
                {
                    return true;
                }
            }
            return false;
        };
        return !separated(own, 6) && !separated(pirate, 5);
    }
}

PirateRaid::PirateRaid(
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
    std::function<bool(int)> hitPlayer)
    : m_scene(scene), m_world(world), m_economy(economy), m_random(random), m_notify(std::move(notify)),
      m_hitPlayer(std::move(hitPlayer))
{
    for (int i = 0; i < 2; ++i)
    {
        m_ships[i].actor = ships[i];
        m_ships[i].chest = m_scene.Create<MeshActor>(ships[i], chest, "해적선 약탈 상자").Id();
        m_scene.Find(m_ships[i].chest)->transform.position = Chest;
    }
    for (auto& crew : m_crew)
    {
        crew.actor = m_scene.Create<BoardingPirateActor>(root, body, sword).Id();
    }
    auto warmup = ShipDeck::HelmStand();
    ShipDeck::Chase(warmup, warmup, 0);
    Reset();
}

MarineTargetActor* PirateRaid::Ship(int i) const
{
    return m_scene.Get<MarineTargetActor>(m_ships[i].actor);
}

BoardingPirateActor* PirateRaid::Pirate(int i) const
{
    return m_scene.Get<BoardingPirateActor>(m_crew[i].actor);
}

void PirateRaid::Reset()
{
    m_failed = false;
    m_failure.clear();
    for (int i = 0; i < 2; ++i)
    {
        const auto actor = m_ships[i].actor, chest = m_ships[i].chest;
        m_ships[i] = Vessel{};
        m_ships[i].actor = actor;
        m_ships[i].chest = chest;
        m_scene.Find(chest)->visible = false;
        Ship(i)->lootTime = -1;
    }
    for (int i = 0; i < 4; ++i)
    {
        const auto actor = m_crew[i].actor;
        m_crew[i] = Crew{};
        m_crew[i].actor = actor;
        Pirate(i)->health = 30;
        Pirate(i)->swing = 0;
        Pirate(i)->visible = false;
    }
}

bool PirateRaid::Attached(int i) const
{
    return m_ships[i].attached;
}

bool PirateRaid::Active() const
{
    for (const auto& ship : m_ships)
    {
        if (ship.attached && !ship.finished)
        {
            return true;
        }
    }
    return false;
}

void PirateRaid::Attach(int i, const WorldPosition& ownShip, float heading)
{
    auto& vessel = m_ships[i];
    if (vessel.attached || vessel.finished)
    {
        return;
    }
    vessel.attached = true;
    for (int n = i * 2; n < i * 2 + 2; ++n)
    {
        auto& crew = m_crew[n];
        crew.stage = 1;
        crew.crossing = -(n % 2) * .4f;
        const auto p = Offset(Ship(i)->location, Start(n), Ship(i)->transform.rotation);
        crew.local = ShipDeck::ClosestBoardingPoint(ShipDeck::ToLocal(p.RelativeTo(ownShip), heading));
    }
    m_notify("해적선이 접현했습니다! 빨간 두건 해적 2명이 승선합니다. E로 조종을 풀고 싸우세요.");
    Diagnostics::Get().Event(
        "pirate_boarding_begin ship=" + std::to_string(i) +
        " crew=2 contact_damage=0 vessel_motion=anchored chunk_x=" +
        std::to_string(Ship(i)->location.chunkX) + " chunk_z=" + std::to_string(Ship(i)->location.chunkZ) +
        " x=" + std::to_string(Ship(i)->location.x) + " z=" + std::to_string(Ship(i)->location.z) +
        " heading=" + std::to_string(Ship(i)->transform.rotation));
}

void PirateRaid::Fail(const std::string& reason)
{
    if (m_failed)
    {
        return;
    }
    m_failed = true;
    m_failure = reason;
    Diagnostics::Get().Event("pirate_player_dead reason=" + reason);
}

void PirateRaid::Cleared(int i)
{
    auto& vessel = m_ships[i];
    if (vessel.cleared)
    {
        return;
    }
    for (int n = i * 2; n < i * 2 + 2; ++n)
    {
        if (Pirate(n)->health > 0)
        {
            return;
        }
    }
    vessel.cleared = true;
    vessel.remaining = 40;
    vessel.gold = std::uniform_int_distribution<int>(10, 20)(m_random);
    vessel.tentacles = std::uniform_int_distribution<int>(0, 1)(m_random);
    Ship(i)->lootTime = 40;
    m_notify("해적 2명 처치! 해적선 약탈과 내 배 복귀까지 40초입니다. 가장자리 E로 건너가세요.");
    Diagnostics::Get().Event("pirate_loot_countdown_begin ship=" + std::to_string(i) + " seconds=40");
}

void PirateRaid::Update(float dt, float heading, const RaidPlayer& player)
{
    for (int i = 0; i < 2; ++i)
    {
        auto& vessel = m_ships[i];
        if (!vessel.attached || vessel.finished)
        {
            continue;
        }
        // The pirate vessel keeps its world pose at contact; only boarded crew follow our deck.
        if (vessel.cleared)
        {
            vessel.remaining = (std::max)(0.f, vessel.remaining - dt);
            Ship(i)->lootTime = vessel.remaining;
            if (vessel.remaining <= 0)
            {
                if (vessel.visited && !vessel.returned)
                {
                    Fail("40초 안에 내 배로 돌아오지 못했습니다.");
                    return;
                }
                vessel.finished = true;
                Ship(i)->health = 0;
                Ship(i)->chasing = false;
                m_scene.Find(vessel.chest)->visible = false;
                m_notify("해적선의 약탈 시간이 끝났습니다. 내 배에서 안전하게 살아남았습니다.");
                Diagnostics::Get().Event("pirate_loot_window_closed ship=" + std::to_string(i) + " safe=1");
            }
            continue;
        }
        for (int n = i * 2; n < i * 2 + 2; ++n)
        {
            auto& crew = m_crew[n];
            auto* actor = Pirate(n);
            if (actor->health <= 0)
            {
                continue;
            }
            actor->swing = (std::max)(0.f, actor->swing - dt);
            if (crew.stage == 1)
            {
                crew.crossing += dt / 1.2f;
                if (crew.crossing >= 1)
                {
                    crew.stage = 2;
                    crew.cooldown = 1;
                    Diagnostics::Get().Event("pirate_boarded_player_ship crew=" + std::to_string(n));
                }
            }
            if (crew.stage != 2 || !player.ownDeck)
            {
                continue;
            }
            const float dx = player.deck.x - crew.local.x, dz = player.deck.z - crew.local.z;
            actor->transform.rotation = heading + std::atan2(dx, -dz);
            const float distance = std::hypot(dx, dz);
            if (distance > .8f)
            {
                ShipDeck::Chase(crew.local, player.deck, dt * 2.6f);
            }
            crew.cooldown -= dt;
            if (distance <= 1.05f && ShipDeck::ClearAttack(crew.local, player.deck) && crew.cooldown <= 0)
            {
                crew.cooldown = 2;
                actor->swing = .3f;
                Diagnostics::Get().Event("pirate_sword_attack crew=" + std::to_string(n) + " damage=10");
                if (m_hitPlayer(10))
                {
                    Fail("승선한 해적에게 패배했습니다.");
                    return;
                }
            }
        }
    }
}

void PirateRaid::Attack(Vec3 player, Vec3 aim, int damage)
{
    const float length = std::hypot(aim.x, aim.z);
    if (length < .001f)
    {
        return;
    }
    for (int i = 0; i < 4; ++i)
    {
        auto& crew = m_crew[i];
        auto* actor = Pirate(i);
        if (crew.stage != 2 || actor->health <= 0)
        {
            continue;
        }
        const float dx = crew.local.x - player.x, dz = crew.local.z - player.z, distance = std::hypot(dx, dz);
        if (distance > 2.9f || (distance > .001f && (dx * aim.x + dz * aim.z) / (length * distance) < .5f) ||
            !ShipDeck::ClearAttack(player, crew.local))
        {
            continue;
        }
        actor->health = (std::max)(0.f, actor->health - damage);
        Diagnostics::Get().Event(
            "pirate_crew_hit crew=" + std::to_string(i) + " health=" + std::to_string(actor->health));
        if (actor->health <= 0)
        {
            crew.stage = 3;
            actor->visible = false;
            Cleared(i / 2);
        }
    }
}

void PirateRaid::Prepare(const WorldPosition& camera, const WorldPosition& ownShip, float heading)
{
    for (int i = 0; i < 2; ++i)
    {
        const auto& ship = m_ships[i];
        m_scene.Find(ship.chest)->visible = ship.cleared && !ship.finished && !ship.looted;
    }
    for (int i = 0; i < 4; ++i)
    {
        const auto& crew = m_crew[i];
        const auto* ship = Ship(i / 2);
        auto* actor = Pirate(i);
        float height = PirateDeck::Height;
        auto position = Offset(ship->location, Start(i), ship->transform.rotation);
        if (crew.stage == 1)
        {
            const float t = (std::max)(0.f, (std::min)(1.f, crew.crossing));
            const auto destination = Offset(ownShip, crew.local, heading);
            const auto d = destination.RelativeTo(position);
            position.Move(d.x * t, d.z * t);
            height += (ShipDeck::Height() - PirateDeck::Height) * t + std::sin(t * 3.14159265f) * .6f;
        }
        else if (crew.stage == 2)
        {
            position = Offset(ownShip, crew.local, heading);
            height = ShipDeck::Height();
        }
        actor->transform.position = position.RelativeTo(camera);
        actor->transform.position.y = height + .03f;
        if (crew.stage < 2)
        {
            actor->transform.rotation = ship->transform.rotation;
        }
        actor->visible =
            actor->health > 0 && ship->health > 0 && !m_ships[i / 2].finished && m_world.IsActive(position);
    }
}

bool PirateRaid::Deck(int i, PirateDeckInfo& info) const
{
    if (i < 0 || i >= 2 || !m_ships[i].cleared || m_ships[i].finished || m_failed)
    {
        return false;
    }
    info.actor = m_ships[i].actor;
    info.position = Ship(i)->location;
    info.heading = Ship(i)->transform.rotation;
    return true;
}

bool PirateRaid::Board(const WorldPosition& player, int& index, Vec3& deck) const
{
    float nearest = 2.3f;
    bool found = false;
    for (int i = 0; i < 2; ++i)
    {
        PirateDeckInfo info;
        if (!Deck(i, info))
        {
            continue;
        }
        const auto outside = ShipDeck::ToLocal(player.RelativeTo(info.position), info.heading);
        const auto point = PirateDeck::ClosestBoardingPoint(outside);
        const float distance = std::hypot(outside.x - point.x, outside.z - point.z);
        if (distance < nearest)
        {
            nearest = distance;
            deck = point;
            index = i;
            found = true;
        }
    }
    return found;
}

bool PirateRaid::Loot(int i, Vec3 local)
{
    PirateDeckInfo info;
    if (!Deck(i, info))
    {
        return false;
    }
    auto& ship = m_ships[i];
    if (ship.looted || std::hypot(local.x - Chest.x, local.z - Chest.z) > 1.2f)
    {
        return false;
    }
    const auto result = m_economy.CollectPirateLoot(ship.gold, ship.tentacles);
    if (result.success)
    {
        ship.looted = true;
        m_scene.Find(ship.chest)->visible = false;
    }
    m_notify(result.message);
    return true;
}

bool PirateRaid::CanSail(const WorldPosition& ownShip, float heading) const
{
    for (int i = 0; i < int(m_ships.size()); ++i)
    {
        const auto& ship = m_ships[i];
        if (ship.attached && !ship.finished &&
            HullsOverlap(
                ShipDeck::ToLocal(Ship(i)->location.RelativeTo(ownShip), heading),
                Ship(i)->transform.rotation - heading))
        {
            return false;
        }
    }
    return true;
}

void PirateRaid::Entered(int i)
{
    PirateDeckInfo info;
    if (!Deck(i, info))
    {
        return;
    }
    m_ships[i].visited = true;
    m_ships[i].returned = false;
    Diagnostics::Get().Event("player_boarded_pirate_ship ship=" + std::to_string(i));
}

void PirateRaid::Returned(int i)
{
    PirateDeckInfo info;
    if (!Deck(i, info))
    {
        return;
    }
    m_ships[i].returned = true;
    Diagnostics::Get().Event(
        "player_returned_from_pirate_ship ship=" + std::to_string(i) +
        " seconds_left=" + std::to_string(m_ships[i].remaining));
}
