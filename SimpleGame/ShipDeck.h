#pragma once
#include "Renderer.h"
#include "PlayerEconomy.h"

enum class PlayerMode
{
    Land,
    Deck,
    PirateDeck,
    Helm
};

namespace ShipDeck
{
    float Scale();
    float Height();
    float HalfBeam();
    Vec3 Helm();
    Vec3 HelmStand();
    bool Walkable(Vec3 point);
    bool NearEdge(Vec3 point);
    bool ClearAttack(Vec3 from, Vec3 to);
    void Move(Vec3& point, float dx, float dz);
    void Chase(Vec3& point, Vec3 target, float distance);
    Vec3 ClosestBoardingPoint(Vec3 outside);
    Vec3 ToWorld(Vec3 local, float heading);
    Vec3 ToLocal(Vec3 world, float heading);
}

namespace PirateDeck
{
    constexpr float Height = 1.05f;
    bool Walkable(Vec3 point);
    bool NearEdge(Vec3 point);
    void Move(Vec3& point, float dx, float dz);
    Vec3 ClosestBoardingPoint(Vec3 outside);
}
