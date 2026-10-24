#pragma once
#include "Renderer.h"
#include "PlayerEconomy.h"

enum class PlayerMode
{
    Land,
    Deck,
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
    Vec3 ClosestBoardingPoint(Vec3 outside);
    Vec3 ToWorld(Vec3 local, float heading);
    Vec3 ToLocal(Vec3 world, float heading);
}
