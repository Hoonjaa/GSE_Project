#include "stdafx.h"
#include "Prototype.h"
#include "KoreanText.h"
#include "MeshCache.h"
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
    m_shipActor = m_scene.Create<WorldActor>(m_actorsRoot, "8m 돛단배").Id();
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
    m_shadow = m_scene
                   .Create<ScaledMeshActor>(
                       m_shipActor,
                       CachedMesh(std::move(shadow), "ship_shadow"),
                       Vec3{1, 1, 1},
                       "선박 그림자")
                   .Id();
    m_wake = m_scene
                 .Create<ScaledMeshActor>(
                     m_shipActor,
                     CachedMesh(std::move(wake), "ship_wake"),
                     Vec3{1, 1, 1},
                     "항적")
                 .Id();
    m_shipBody = m_scene
                     .Create<ScaledMeshActor>(
                         m_shipActor,
                         CachedMesh(
                             []
                             {
                                 Mesh mesh = Ship();
                                 for (auto& vertex : mesh)
                                 {
                                     vertex.position.z += .125f;
                                 }
                                 return mesh;
                             }(),
                             "ship_centered"),
                         Vec3{1, 1, 1},
                         "선체")
                     .Id();

    m_playerActor = m_scene.Create<WorldActor>(m_actorsRoot, "플레이어").Id();
    Disc(marker, {0, 0, 0}, .23f, Gold, 24);
    auto& markerActor =
        m_scene.Create<MeshActor>(m_playerActor, CachedMesh(std::move(marker), "player_marker"), "발밑 표시");
    markerActor.transform.position.y = .58f;
    m_playerMarker = markerActor.Id();
    Mesh playerModel = Character({.17f, .43f, .63f});
    float bodyRadius = 0;
    for (const auto& vertex : playerModel)
    {
        bodyRadius = (std::max)(bodyRadius, std::hypot(vertex.position.x, vertex.position.z));
    }
    const float playerScale = MapCatalog::PlayerRadius / bodyRadius;
    for (auto& vertex : playerModel)
    {
        vertex.position.x *= playerScale;
        vertex.position.z *= playerScale;
    }
    m_playerBody = m_scene
                       .Create<MeshActor>(
                           m_playerActor,
                           CachedMesh(std::move(playerModel), "player_diameter_0_4m"),
                           "플레이어 모델")
                       .Id();
    Box(sword, {.32f, .5f, -.55f}, {.12f, .12f, .9f}, {.83f, .91f, .95f});
    Box(sword, {.20f, .48f, .1f}, {.36f, .16f, .1f}, Gold);
    for (auto& vertex : sword)
    {
        vertex.position.x *= playerScale;
        vertex.position.z *= playerScale;
    }
    m_scene.Create<MeshActor>(m_playerBody, CachedMesh(std::move(sword), "equipped_sword"), "장착 무기");

    const auto wheel = MeshCache::LoadOrCreate(
        "ship_wheel_v1",
        []
        {
            Mesh mesh;
            Box(mesh, {-.05f, 0, -.05f}, {.1f, .8f, .1f}, {.39f, .22f, .12f});
            for (int i = 0; i < 16; ++i)
            {
                const float a = i * 2 * Pi / 16, b = (i + 1) * 2 * Pi / 16;
                Quad(
                    mesh,
                    {.32f * std::cos(a), .8f + .32f * std::sin(a), 0},
                    {.32f * std::cos(b), .8f + .32f * std::sin(b), 0},
                    {.26f * std::cos(b), .8f + .26f * std::sin(b), 0},
                    {.26f * std::cos(a), .8f + .26f * std::sin(a), 0},
                    Gold);
            }
            Box(mesh, {-.3f, .77f, -.025f}, {.6f, .06f, .05f}, Gold);
            Box(mesh, {-.03f, .5f, -.025f}, {.06f, .6f, .05f}, Gold);
            return mesh;
        });
    m_wheel = m_scene.Create<MeshActor>(m_shipActor, wheel, "타륜").Id();
    m_scene.Find(m_wheel)->transform.position = ShipDeck::Helm();
    const auto slash = MeshCache::LoadOrCreate(
        "deck_slash_v1",
        []
        {
            Mesh mesh;
            for (int i = 0; i < 20; ++i)
            {
                const float a = -.95f + i * 1.9f / 20, b = a + 1.9f / 20;
                Quad(
                    mesh,
                    {1.8f * std::cos(a), 1.1f, 1.8f * std::sin(a)},
                    {1.8f * std::cos(b), 1.1f, 1.8f * std::sin(b)},
                    {2.7f * std::cos(b), 1.1f, 2.7f * std::sin(b)},
                    {2.7f * std::cos(a), 1.1f, 2.7f * std::sin(a)},
                    Gold);
            }
            return mesh;
        });
    m_deckSlash = m_scene.Create<SlashActor>(m_shipActor, slash).Id();
    const auto rod = MeshCache::LoadOrCreate(
        "fishing_rod_v1",
        []
        {
            Mesh mesh;
            Box(mesh, {.18f, .75f, -2}, {.03f, .03f, 2.1f}, {.57f, .37f, .18f});
            Box(mesh, {.19f, -.95f, -2}, {.012f, 1.7f, .012f}, {.8f, .87f, .8f});
            return mesh;
        });
    m_fishingRod = m_scene.Create<MeshActor>(m_playerBody, rod, "낚싯대").Id();

    auto& harbor = m_scene.Create<WorldActor>(m_actorsRoot, "항구 주민 및 장식");
    m_harbor = harbor.Id();
    harbor.location = m_world.StartVillage();
    auto& flag = m_scene.Create<FlagActor>(m_harbor);
    flag.transform.position = {2.8f, 0, -.3f};
    for (int i = 0; i < 3; ++i)
    {
        auto& gull = m_scene.Create<GullActor>(m_harbor, i);
        gull.Update(0);
    }
    auto& hud = m_uiScene.Create<CallbackActor>(NoActor, "게임 HUD");
    hud.overlay = [this](Renderer& renderer, const ActorTransform&)
    {
        m_ui.RenderHud(renderer);
    };
}

void Prototype::PrepareActors()
{
    auto& ship = ShipActor();
    ship.Prepare(m_camera);
    ship.transform.rotation = m_player.ShipHeading();
    ship.visible = m_state.economy.HasShip() && m_world.IsActive(ship.location);
    ship.enabled = ship.visible;
    const float shipScale = EconomyRules::SailboatLength / 3.85f;
    m_scene.Get<ScaledMeshActor>(m_shipBody)->scale = {shipScale, shipScale, shipScale};
    m_scene.Get<ScaledMeshActor>(m_wake)->scale = {shipScale, 1, shipScale};
    m_scene.Get<ScaledMeshActor>(m_shadow)->scale = {
        EconomyRules::SailboatLength * (1.7f / 3.85f) / 3,
        1,
        EconomyRules::SailboatLength / 3};
    const float bob = .045f * std::sin(m_time * 2);
    ship.transform.position.y = bob;
    m_scene.Find(m_shadow)->transform.position.y = -bob;
    m_scene.Find(m_wake)->transform.position.y = -bob;
    m_scene.Find(m_wake)->visible = Sailing() && m_player.Moving();

    auto& player = Player();
    if (Aboard())
    {
        player.transform.position = m_player.DeckPosition();
        player.transform.rotation = m_player.DeckFacing();
    }
    else
    {
        player.Prepare(m_camera);
        player.transform.rotation = m_player.Heading();
    }
    player.visible = true;
    m_scene.Find(m_playerMarker)->transform.position.y = Aboard() ? .02f : .58f;
    m_scene.Find(m_playerBody)->transform.position.y =
        (Aboard() ? .03f : .58f) + (m_player.Moving() ? .035f * std::abs(std::sin(m_time * 12)) : 0);
    m_scene.Find(m_fishingRod)->visible = m_player.Fishing();

    auto* harbor = m_scene.Get<WorldActor>(m_harbor);
    harbor->Prepare(m_camera);
    harbor->visible = !m_world.IsTestMap() && m_world.IsActive(harbor->location);
    harbor->enabled = harbor->visible;
}
