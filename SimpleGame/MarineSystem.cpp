#include "stdafx.h"
#include "MarineSystem.h"
#include "ShipDeck.h"
#include "Diagnostics.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace
{
    float Distance(const WorldPosition& a, const WorldPosition& b)
    {
        const auto d = a.RelativeTo(b);
        return std::hypot(d.x, d.z);
    }
}

MarineSystem::MarineSystem(
    SceneGraph& scene,
    World& world,
    PlayerEconomy& economy,
    ActorId shipRoot,
    std::function<bool(int)> hitPlayer)
    : m_scene(scene), m_world(world), m_economy(economy), m_shipRoot(shipRoot)
{
    const auto seed = static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count());
    m_random.seed(seed);
    Diagnostics::Get().Event("marine_rng_seed=" + std::to_string(seed));
    m_root = m_scene.Create<Actor>(NoActor, "해상 조우").Id();
    m_weather = m_scene.Create<SeaWeatherActor>(m_root).Id();
    for (auto& id : m_pirates)
    {
        id = m_scene.Create<MarineTargetActor>(m_root, m_meshes.pirate, MarineTarget::Pirate).Id();
    }
    for (auto& id : m_tentacles)
    {
        id = m_scene.Create<MarineTargetActor>(m_shipRoot, m_meshes.tentacle, MarineTarget::Tentacle).Id();
    }
    for (auto& id : m_tornadoes)
    {
        id = m_scene.Create<TornadoActor>(m_root, m_meshes.tornado).Id();
    }
    m_raid.reset(new PirateRaid(
        m_scene,
        m_world,
        m_economy,
        m_root,
        m_pirates,
        m_meshes.raider,
        m_meshes.cutlass,
        m_meshes.chest,
        m_random,
        [this](const std::string& text)
        {
            Notify(text);
        },
        std::move(hitPlayer)));
    Reset();
}

MarineSystem::~MarineSystem()
{
    m_scene.Destroy(m_root);
    for (auto id : m_tentacles)
    {
        m_scene.Destroy(id);
    }
    m_scene.CollectDestroyed();
}

MarineTargetActor* MarineSystem::Target(ActorId id)
{
    return m_scene.Get<MarineTargetActor>(id);
}

void MarineSystem::Notify(const std::string& text)
{
    m_notice = text;
    Diagnostics::Get().Event(
        "marine_event kind=" + std::to_string(static_cast<int>(m_encounter)) + " " + text);
}

void MarineSystem::Reset()
{
    m_raid->Reset();
    m_encounter = SeaEncounter::None;
    m_health = 100;
    m_havePatrol = m_captured = false;
    m_tornadoCount = m_escapeCount = 0;
    m_time = m_krakenRoll = m_stormRoll = m_escapeTime = m_stormTime = m_attackCooldown = 0;
    m_notice.clear();
    m_lastEscape = 0;
    for (auto id : m_pirates)
    {
        auto* a = Target(id);
        a->health = 0;
        a->visible = a->enabled = false;
        a->chasing = false;
    }
    for (auto id : m_tentacles)
    {
        auto* a = Target(id);
        a->health = 0;
        a->visible = a->enabled = false;
    }
    for (auto id : m_tornadoes)
    {
        auto* a = m_scene.Find(id);
        a->visible = a->enabled = false;
    }
    m_scene.Get<SeaWeatherActor>(m_weather)->SetStorm(false);
}

bool MarineSystem::SafeSea(const WorldPosition& p, float heading) const
{
    return m_world.IsActive(p) ? m_world.CanSail(p, heading, EconomyRules::SailboatLength)
                               : m_world.ConservativeSea(p, EconomyRules::SailboatLength * .5f);
}

void MarineSystem::Patrol(const WorldPosition& ship)
{
    m_raid->Reset();
    m_patrolAnchor = ship;
    m_havePatrol = true;
    for (size_t i = 0; i < m_pirates.size(); ++i)
    {
        auto* actor = Target(m_pirates[i]);
        actor->health = 0;
        actor->chasing = false;
        actor->cooldown = 2;
        actor->coarseTime = 0;
        for (int attempt = 0; attempt < 40; ++attempt)
        {
            const float angle = std::uniform_real_distribution<float>(0, 6.2831853f)(m_random);
            const float radius = std::uniform_real_distribution<float>(120, 150)(m_random);
            auto p = ship;
            p.Move(std::cos(angle) * radius, std::sin(angle) * radius);
            bool separated = true;
            for (size_t n = 0; n < i; ++n)
            {
                separated &= Distance(p, Target(m_pirates[n])->location) > 12;
            }
            if (!SafeSea(p, angle) || !separated)
            {
                continue;
            }
            actor->location = p;
            actor->health = 60;
            break;
        }
    }
    Diagnostics::Get().Event("pirate_patrol_repositioned radius_max=150 count_max=2");
}

void MarineSystem::Begin(SeaEncounter kind, const WorldPosition& ship)
{
    if (m_encounter != SeaEncounter::None)
    {
        return;
    }
    m_encounter = kind;
    if (kind == SeaEncounter::Kraken)
    {
        int i = 0;
        for (auto id : m_tentacles)
        {
            auto* actor = Target(id);
            actor->health = 30;
            actor->cooldown = 2;
            actor->transform.position = {
                (i % 2 ? 1 : -1) * (ShipDeck::HalfBeam() + .25f),
                0,
                (i < 2 ? -1 : 1) * EconomyRules::SailboatLength * .22f};
            actor->visible = true;
            ++i;
        }
        Notify("크라켄 출현! 조종을 풀고 갑판에서 촉수 4개를 절단하세요.");
    }
    else if (kind == SeaEncounter::Storm)
    {
        m_stormTime = 60;
        m_captured = false;
        m_tornadoCount = std::uniform_int_distribution<int>(2, 3)(m_random);
        for (int i = 0; i < m_tornadoCount; ++i)
        {
            auto* actor = m_scene.Get<TornadoActor>(m_tornadoes[i]);
            actor->location = ship;
            const float angle = i * 6.2831853f / m_tornadoCount;
            const float distance = 18.f;
            actor->location.Move(std::cos(angle) * distance, std::sin(angle) * distance);
            if (!SafeSea(actor->location, 0))
            {
                actor->location = ship;
            }
            actor->visible = true;
        }
        m_scene.Get<SeaWeatherActor>(m_weather)->SetStorm(true);
        Notify("폭풍이 몰려옵니다! 토네이도에 휘말리면 A/D를 번갈아 누르세요.");
    }
    else
    {
        Notify("해적선이 추격합니다! 접근하면 빨간 두건 해적들이 승선합니다.");
    }
}

void MarineSystem::EndStorm()
{
    m_captured = false;
    m_tornadoCount = 0;
    m_encounter = SeaEncounter::None;
    m_scene.Get<SeaWeatherActor>(m_weather)->SetStorm(false);
    for (auto id : m_tornadoes)
    {
        m_scene.Find(id)->visible = false;
    }
}

void MarineSystem::EscapeKey(unsigned char key)
{
    if (!m_captured || (key != 'a' && key != 'd') || key == m_lastEscape)
    {
        return;
    }
    m_lastEscape = key;
    ++m_escapeCount;
    Diagnostics::Get().Event(
        "tornado_escape_input key=" + std::string(1, key) + " count=" + std::to_string(m_escapeCount));
    if (m_escapeCount >= 12)
    {
        EndStorm();
        Notify("토네이도에서 탈출했습니다!");
    }
}

void MarineSystem::DamageShip(float damage)
{
    m_health = (std::max)(0.f, m_health - damage);
    Diagnostics::Get().Event(
        "ship_damage amount=" + std::to_string(damage) + " health=" + std::to_string(m_health));
}

bool MarineSystem::Update(
    float dt,
    const WorldPosition& ship,
    bool aboard,
    float shipHeading,
    const RaidPlayer& player)
{
    m_ship = ship;
    m_time += dt;
    m_attackCooldown = (std::max)(0.f, m_attackCooldown - dt);
    if (!m_economy.HasShip())
    {
        return false;
    }
    if (!aboard && m_encounter == SeaEncounter::None)
    {
        return false;
    }
    if (!m_world.IsActive(ship))
    {
        return false;
    }
    if (aboard)
    {
        if (!m_havePatrol || (m_encounter == SeaEncounter::None && Distance(ship, m_patrolAnchor) > 150))
        {
            Patrol(ship);
        }
        m_krakenRoll += dt;
        m_stormRoll += dt;
        if (m_krakenRoll >= 30)
        {
            m_krakenRoll -= 30;
            const int value = std::uniform_int_distribution<int>(0, 99)(m_random);
            const bool roll = value == 0;
            Diagnostics::Get().Event(
                "kraken_roll hit=" + std::to_string(roll) + " value=" + std::to_string(value) +
                " encounter=" + std::to_string(static_cast<int>(m_encounter)) + " result=" +
                (roll ? (m_encounter == SeaEncounter::None ? "started" : "blocked_busy") : "miss"));
            if (roll && m_encounter == SeaEncounter::None)
            {
                Begin(SeaEncounter::Kraken, ship);
            }
        }
        if (m_stormRoll >= 20)
        {
            m_stormRoll -= 20;
            const int value = std::uniform_int_distribution<int>(0, 99)(m_random);
            const bool roll = value == 0;
            Diagnostics::Get().Event(
                "tornado_roll hit=" + std::to_string(roll) + " value=" + std::to_string(value) +
                " encounter=" + std::to_string(static_cast<int>(m_encounter)) + " result=" +
                (roll ? (m_encounter == SeaEncounter::None ? "started" : "blocked_busy") : "miss"));
            if (roll && m_encounter == SeaEncounter::None)
            {
                Begin(SeaEncounter::Storm, ship);
            }
        }
    }
    if (m_encounter == SeaEncounter::None || m_encounter == SeaEncounter::Pirates)
    {
        bool chasing = false;
        int shipIndex = -1;
        for (auto id : m_pirates)
        {
            ++shipIndex;
            auto* actor = Target(id);
            if (actor->health <= 0)
            {
                continue;
            }
            if (m_raid->Attached(shipIndex))
            {
                continue;
            }
            const float distance = Distance(actor->location, ship);
            if (aboard && distance <= 50)
            {
                if (m_encounter == SeaEncounter::None)
                {
                    Begin(SeaEncounter::Pirates, ship);
                }
                if (!actor->chasing)
                {
                    Diagnostics::Get().Event("pirate_aggro distance=" + std::to_string(distance));
                }
                actor->chasing = true;
            }
            if (m_encounter != SeaEncounter::Pirates || !actor->chasing)
            {
                continue;
            }
            chasing = true;
            actor->coarseTime += dt;
            if (!m_world.IsActive(actor->location) && actor->coarseTime < .2f)
            {
                continue;
            }
            const float stepTime = actor->coarseTime;
            actor->coarseTime = 0;
            const auto d = ship.RelativeTo(actor->location);
            const float heading = std::atan2(d.x, -d.z);
            actor->transform.rotation = heading;
            const auto direction = ShipDeck::ToLocal(d, shipHeading);
            const float inverse = 1 / (std::max)(.001f, distance);
            const float side = ShipDeck::HalfBeam() * direction.x * inverse;
            const float fore = EconomyRules::SailboatLength * .5f * direction.z * inverse;
            const float contact = std::hypot(side, fore) + 4.65f;
            if (distance > contact)
            {
                const float move = (std::min)(distance - contact, stepTime * 7.5f);
                const int steps = (std::max)(1, int(std::ceil(move / .3f)));
                for (int i = 0; i < steps; ++i)
                {
                    auto next = actor->location;
                    next.Move(d.x * inverse * move / steps, d.z * inverse * move / steps);
                    bool separated = true;
                    for (auto otherId : m_pirates)
                    {
                        const auto* other = Target(otherId);
                        if (otherId != id && other->health > 0 && Distance(next, other->location) < 5)
                        {
                            separated = false;
                        }
                    }
                    if (!SafeSea(next, heading) || !separated)
                    {
                        break;
                    }
                    actor->location = next;
                }
            }
            if (Distance(actor->location, ship) <= contact + .15f)
            {
                m_raid->Attach(shipIndex, ship, shipHeading);
            }
        }
        m_raid->Update(dt, shipHeading, player);
        if (m_raid->Failed())
        {
            const int lost = m_economy.LoseCargoUnits(3, m_random);
            Notify(
                "사망했습니다. " + m_raid->Failure() + " 적재품 " + std::to_string(lost) +
                "개를 잃고 항구로 귀환합니다.");
            return true;
        }
        if (m_encounter == SeaEncounter::Pirates && !chasing && !m_raid->Active())
        {
            m_encounter = SeaEncounter::None;
        }
        else if (
            m_encounter == SeaEncounter::Pirates && !m_raid->Active() && Distance(ship, m_patrolAnchor) > 150)
        {
            m_encounter = SeaEncounter::None;
            Patrol(ship);
            Notify("해적의 추격 범위를 벗어났습니다.");
        }
    }
    if (m_encounter == SeaEncounter::Kraken)
    {
        int alive = 0;
        for (auto id : m_tentacles)
        {
            auto* actor = Target(id);
            if (actor->health <= 0)
            {
                continue;
            }
            ++alive;
            actor->cooldown -= dt;
            if (actor->cooldown <= 0)
            {
                DamageShip(5);
                actor->cooldown = 2;
            }
        }
        if (!alive)
        {
            m_economy.CollectItem(InventoryItem::Tentacle);
            m_encounter = SeaEncounter::None;
            Notify("촉수 4개를 절단했습니다! 크라켄 촉수 1개 획득.");
        }
    }
    if (m_encounter == SeaEncounter::Storm)
    {
        const bool previouslyCaptured = m_captured;
        m_stormTime -= dt;
        for (int i = 0; i < m_tornadoCount; ++i)
        {
            auto* actor = m_scene.Get<TornadoActor>(m_tornadoes[i]);
            const auto d = ship.RelativeTo(actor->location);
            const float distance = std::hypot(d.x, d.z);
            if (!m_captured && distance > .01f)
            {
                auto next = actor->location;
                const float move = (std::min)(distance, dt * 2);
                next.Move(d.x / distance * move, d.z / distance * move);
                if (SafeSea(next, 0))
                {
                    actor->location = next;
                }
            }
            if (!m_captured && distance < EconomyRules::SailboatLength)
            {
                m_captured = true;
                m_escapeTime = 5;
                m_escapeCount = 0;
                m_lastEscape = 0;
                Notify("휘말렸습니다! 5초 안에 A/D를 번갈아 12회 누르세요.");
            }
        }
        if (m_captured && previouslyCaptured)
        {
            m_escapeTime -= dt;
            if (m_escapeTime <= 0)
            {
                const int lost = m_economy.LoseCargoUnits(4, m_random);
                EndStorm();
                Notify("탈출 실패! 적재품 " + std::to_string(lost) + "개를 잃었습니다.");
            }
        }
        else if (!m_captured && m_stormTime <= 0)
        {
            EndStorm();
            Notify("폭풍이 지나갔습니다.");
        }
    }
    if (m_health <= 0)
    {
        const int requested = m_encounter == SeaEncounter::Kraken ? 2 : 3;
        const int lost = m_economy.LoseCargoUnits(requested, m_random);
        Notify("배가 파괴되었습니다. 적재품 " + std::to_string(lost) + "개를 잃고 항구로 귀환합니다.");
        return true;
    }
    return false;
}

bool MarineSystem::Attack(const WorldPosition& player, Vec3 aim, int damage, float shipHeading, bool aboard)
{
    if (m_attackCooldown > 0 || m_captured)
    {
        return false;
    }
    const float length = std::hypot(aim.x, aim.z);
    if (length < .001f)
    {
        return false;
    }
    aim.x /= length;
    aim.z /= length;
    m_attackCooldown = .42f;
    auto hit = [&](MarineTargetActor* actor, const WorldPosition& target, float radius)
    {
        if (actor->health <= 0)
        {
            return;
        }
        const auto d = target.RelativeTo(player);
        const float distance = std::hypot(d.x, d.z);
        const float dot = distance > .001f ? (d.x * aim.x + d.z * aim.z) / distance : 1;
        if (distance > 2.7f + radius || dot < .5f)
        {
            return;
        }
        if (aboard)
        {
            const auto from = ShipDeck::ToLocal(player.RelativeTo(m_ship), shipHeading);
            const auto to = ShipDeck::ToLocal(target.RelativeTo(m_ship), shipHeading);
            if (!ShipDeck::ClearAttack(from, to))
            {
                return;
            }
        }
        actor->health = (std::max)(0.f, actor->health - damage);
        Diagnostics::Get().Event(
            "marine_target_hit kind=" + std::to_string(static_cast<int>(actor->kind)) +
            " damage=" + std::to_string(damage) + " health=" + std::to_string(actor->health));
    };
    if (m_encounter == SeaEncounter::Pirates)
    {
        if (aboard)
        {
            m_raid->Attack(
                ShipDeck::ToLocal(player.RelativeTo(m_ship), shipHeading),
                ShipDeck::ToLocal(aim, shipHeading),
                damage);
        }
    }
    if (m_encounter == SeaEncounter::Kraken)
    {
        for (auto id : m_tentacles)
        {
            auto* actor = Target(id);
            auto p = m_ship;
            const auto offset = ShipDeck::ToWorld(actor->transform.position, shipHeading);
            p.Move(offset.x, offset.z);
            hit(actor, p, .4f);
        }
    }
    return true;
}

void MarineSystem::Prepare(const WorldPosition& camera, const WorldPosition& ship)
{
    m_raid->Prepare(camera, ship, m_scene.Find(m_shipRoot)->transform.rotation);
    for (auto id : m_pirates)
    {
        auto* a = Target(id);
        a->transform.position = a->location.RelativeTo(camera);
        a->visible = a->health > 0 && m_world.IsActive(a->location);
    }
    for (auto id : m_tentacles)
    {
        auto* a = Target(id);
        a->visible = m_encounter == SeaEncounter::Kraken && a->health > 0;
    }
    for (int i = 0; i < 3; ++i)
    {
        auto* a = m_scene.Get<TornadoActor>(m_tornadoes[i]);
        a->transform.position = a->location.RelativeTo(camera);
        a->time = m_time;
        a->visible =
            m_encounter == SeaEncounter::Storm && i < m_tornadoCount && m_world.IsActive(a->location);
    }
    auto* weather = m_scene.Get<SeaWeatherActor>(m_weather);
    weather->transform.position = ship.RelativeTo(camera);
    weather->enabled = weather->visible = m_encounter == SeaEncounter::Storm && m_world.IsActive(ship);
}

float MarineSystem::Light() const
{
    const auto* weather = m_scene.Get<SeaWeatherActor>(m_weather);
    return weather->visible ? weather->Light() : 1.f;
}

bool MarineSystem::ForceStart(SeaEncounter kind, const WorldPosition& ship)
{
    if ((kind != SeaEncounter::Storm && kind != SeaEncounter::Kraken && kind != SeaEncounter::Pirates) ||
        !m_economy.HasShip() || !m_world.IsActive(ship) || m_health <= 0)
    {
        Diagnostics::Get().Event("marine_force_rejected kind=" + std::to_string(static_cast<int>(kind)));
        return false;
    }
    const auto previous = m_encounter;
    m_raid->Reset();
    EndStorm();
    m_ship = ship;
    m_escapeTime = 0;
    m_escapeCount = 0;
    m_lastEscape = 0;
    m_attackCooldown = 0;
    for (auto id : m_tentacles)
    {
        auto* actor = Target(id);
        actor->health = 0;
        actor->visible = false;
    }
    for (auto id : m_pirates)
    {
        auto* actor = Target(id);
        actor->chasing = false;
        actor->coarseTime = 0;
        actor->cooldown = 2;
    }
    if (kind == SeaEncounter::Pirates)
    {
        Patrol(ship);
        for (size_t i = 0; i < m_pirates.size(); ++i)
        {
            auto* actor = Target(m_pirates[i]);
            actor->location = ship;
            actor->location.Move(i ? 12 : -12, 0);
            actor->health = 60;
        }
    }
    Begin(kind, ship);
    Diagnostics::Get().Event(
        "marine_forced kind=" + std::to_string(static_cast<int>(kind)) + " replaced=" +
        std::to_string(static_cast<int>(previous)) + " health=" + std::to_string(m_health) + " targets=" +
        std::to_string(
            kind == SeaEncounter::Storm ? m_tornadoCount : (kind == SeaEncounter::Kraken ? 4 : 2)));
    return m_encounter == kind;
}
