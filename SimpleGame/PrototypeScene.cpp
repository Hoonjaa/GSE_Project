#include "stdafx.h"
#include "Prototype.h"
#include "KoreanText.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace
{
    using namespace Geometry;
    constexpr float Pi = 3.14159265359f;
    const Color Gold = {.98f, .71f, .32f};

    Mesh Ship()
    {
        Mesh m;
        const Vec3 top[] = {
            {0, .6f, -2.05f},
            {.85f, .6f, -.85f},
            {.85f, .6f, 1.35f},
            {0, .6f, 1.8f},
            {-.85f, .6f, 1.35f},
            {-.85f, .6f, -.85f}};
        for (int i = 0; i < 6; ++i)
        {
            Vec3 a = top[i], b = top[(i + 1) % 6];
            Triangle(m, {0, .6f, 0}, a, b, {.79f, .59f, .32f});
            Quad(
                m,
                a,
                b,
                {b.x * .65f, .02f, b.z * .85f},
                {a.x * .65f, .02f, a.z * .85f},
                i < 3 ? Color{.37f, .22f, .14f} : Color{.49f, .29f, .17f});
        }
        for (int i = 0; i < 8; ++i)
        {
            Box(m, {-.67f, .603f, -.8f + i * .27f}, {1.34f, .01f, .025f}, {.47f, .33f, .20f});
        }
        Box(m, {-.085f, .6f, -.15f}, {.17f, 4.5f, .17f}, {.43f, .28f, .17f});
        Box(m, {-1.5f, 4.25f, -.16f}, {3, .10f, .10f}, {.53f, .35f, .20f});
        // Broad curved square sail: separate strips catch warm daylight.
        for (int i = 0; i < 8; ++i)
        {
            const float u = i / 8.f, v = (i + 1) / 8.f;
            const float x = -1.5f + 3 * u, X = -1.5f + 3 * v;
            Quad(
                m,
                {x, 4.24f, -.13f},
                {X, 4.24f, -.13f},
                {X * .82f, 2.1f, -.55f - .3f * std::sin(v * Pi)},
                {x * .82f, 2.1f, -.55f - .3f * std::sin(u * Pi)},
                i == 3 || i == 4 ? Color{.83f, .36f, .23f} : Color{.99f, .92f, .73f});
        }
        Triangle(m, {0, 4.8f, -.02f}, {0, 1.2f, -1.8f}, {0, 1.5f, -.3f}, {.93f, .83f, .59f});
        Triangle(m, {0, 5.0f, 0}, {.9f, 4.82f, 0}, {0, 4.6f, 0}, {.96f, .58f, .24f});
        Box(m, {-.46f, .6f, .65f}, {.92f, .48f, .65f}, {.21f, .40f, .43f});
        return m;
    }

    Mesh Character(Color coat)
    {
        Mesh m;
        Box(m, {-.22f, 0, -.13f}, {.16f, .45f, .24f}, {.22f, .24f, .24f});
        Box(m, {.06f, 0, -.13f}, {.16f, .45f, .24f}, {.22f, .24f, .24f});
        Box(m, {-.28f, .43f, -.17f}, {.56f, .59f, .34f}, coat);
        Box(m, {-.39f, .52f, -.11f}, {.12f, .42f, .22f}, coat);
        Box(m, {.27f, .52f, -.11f}, {.12f, .42f, .22f}, coat);
        Box(m, {-.18f, 1.02f, -.15f}, {.36f, .35f, .30f}, {.89f, .66f, .43f});
        Box(m, {-.33f, 1.37f, -.27f}, {.66f, .08f, .54f}, {.89f, .78f, .53f});
        Box(m, {-.21f, 1.44f, -.17f}, {.42f, .16f, .34f}, {.80f, .63f, .37f});
        Box(m, {-.23f, .64f, .18f}, {.46f, .4f, .18f}, {.52f, .32f, .18f});
        return m;
    }
}

WorldActor& Prototype::Player()
{
    return *m_scene.Get<WorldActor>(m_playerActor);
}

const WorldActor& Prototype::Player() const
{
    return *m_scene.Get<WorldActor>(m_playerActor);
}

WorldActor& Prototype::ShipActor()
{
    return *m_scene.Get<WorldActor>(m_shipActor);
}

const WorldActor& Prototype::ShipActor() const
{
    return *m_scene.Get<WorldActor>(m_shipActor);
}

void Prototype::CreateActors()
{
    using namespace Geometry;
    m_actorsRoot = m_scene.Create<Actor>(NoActor, "플레이어 및 항구").Id();
    m_shipActor = m_scene.Create<WorldActor>(m_actorsRoot, "플레이어 선박").Id();
    Mesh shadow, wake, marker, sword;
    Disc(shadow, {0, .006f, 0}, 1.5f, {.04f, .34f, .43f}, 24);
    for (int n = 0; n < 6; ++n)
    {
        const float z = 2 + n * .42f, spread = .25f + n * .13f;
        Quad(
            wake,
            {-spread, .008f, z},
            {spread, .008f, z},
            {spread + .1f, .008f, z + .09f},
            {-spread - .1f, .008f, z + .09f},
            {.34f, .70f, .73f});
    }
    m_scene.Create<MeshActor>(m_shipActor, CachedMesh(std::move(shadow)), "선박 그림자");
    m_wake = m_scene.Create<MeshActor>(m_shipActor, CachedMesh(std::move(wake)), "항적").Id();
    m_shipBody = m_scene.Create<MeshActor>(m_shipActor, CachedMesh(Ship()), "선체").Id();

    m_playerActor = m_scene.Create<WorldActor>(m_actorsRoot, "플레이어").Id();
    Disc(marker, {0, 0, 0}, .46f, Gold, 24);
    auto& markerActor = m_scene.Create<MeshActor>(m_playerActor, CachedMesh(std::move(marker)), "발밑 표시");
    markerActor.transform.position.y = .58f;
    m_playerBody =
        m_scene.Create<MeshActor>(m_playerActor, CachedMesh(Character({.17f, .43f, .63f})), "플레이어 모델")
            .Id();
    Box(sword, {.32f, .5f, -.55f}, {.12f, .12f, .9f}, {.83f, .91f, .95f});
    Box(sword, {.20f, .48f, .1f}, {.36f, .16f, .1f}, Gold);
    m_scene.Create<MeshActor>(m_playerBody, CachedMesh(std::move(sword)), "장착 무기");

    auto& harbor = m_scene.Create<WorldActor>(m_actorsRoot, "항구 주민 및 장식");
    m_harbor = harbor.Id();
    harbor.location.x = 18.3;
    harbor.location.z = 18.9;
    const CachedMesh npc(Character({.77f, .34f, .22f}));
    auto& first = m_scene.Create<MeshActor>(m_harbor, npc, "항구 주민");
    first.transform.position = {0, .57f, 0};
    first.transform.rotation = .6f;
    auto& second = m_scene.Create<MeshActor>(m_harbor, npc, "항구 주민");
    second.transform.position = {-5, .57f, .1f};
    second.transform.rotation = -.8f;
    auto& flag = m_scene.Create<FlagActor>(m_harbor);
    flag.transform.position = {2.8f, 0, -.3f};
    for (int i = 0; i < 3; ++i)
    {
        auto& gull = m_scene.Create<GullActor>(m_harbor, i);
        gull.Update(0);
    }
    auto& hud = m_scene.Create<CallbackActor>(m_actorsRoot, "게임 HUD");
    hud.overlay = [this](Renderer& renderer, const ActorTransform&)
    {
        RenderHud(renderer);
    };
}

void Prototype::PrepareActors()
{
    auto& ship = ShipActor();
    ship.Prepare(m_camera);
    ship.transform.rotation = m_shipHeading;
    ship.visible = std::abs(ship.transform.position.x) < 500 && std::abs(ship.transform.position.z) < 500;
    m_scene.Find(m_shipBody)->transform.position.y = .045f * std::sin(m_time * 2);
    m_scene.Find(m_wake)->visible = m_sailing && m_moving;

    auto& player = Player();
    player.Prepare(m_camera);
    player.transform.rotation = m_heading;
    player.visible = !m_sailing;
    m_scene.Find(m_playerBody)->transform.position.y =
        .58f + (m_moving ? .035f * std::abs(std::sin(m_time * 12)) : 0);

    auto* harbor = m_scene.Get<WorldActor>(m_harbor);
    harbor->Prepare(m_camera);
    harbor->visible =
        std::abs(harbor->transform.position.x) < 90 && std::abs(harbor->transform.position.z) < 90;
}
