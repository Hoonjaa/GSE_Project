#pragma once
#include "WorldActors.h"
#include "ShipDeck.h"
#include "MarineSystem.h"
#include "PlayerCombat.h"
#include <array>

struct PlayerControlState
{
    PlayerMode mode = PlayerMode::Land;
    Vec3 deck = {};
    int pirate = -1;
    float heading = 0, shipHeading = 0, facing = 0;
};

class PlayerController
{
public:
    PlayerController(SceneGraph& scene, World& world, PlayerEconomy& economy, PlayerCombat& combat);
    void BindActors(ActorId root, ActorId player, ActorId ship, ActorId slash);

    void BindMarine(MarineSystem& marine)
    {
        m_marine = &marine;
    }

    WorldActor& Player();
    const WorldActor& Player() const;
    WorldActor& ShipActor();
    const WorldActor& ShipActor() const;
    bool Press(unsigned char key, bool down);
    void ClearInput();
    void UpdateMovement(float dt, bool blocked);
    void UpdateFishing(float dt, bool panelOpen);
    void Interact();
    bool Attack(Vec3 aim);
    void ToggleFishing();

    void StopFishing()
    {
        m_fishing = false;
        m_fishTime = 0;
    }

    void SyncBoardedPlayer();
    void Reset(const WorldPosition& player, const WorldPosition& ship, float heading);
    PlayerControlState Snapshot() const;
    void Restore(const PlayerControlState& state);
    void SetShipPose(const WorldPosition& position, float heading);

    bool Aboard() const
    {
        return m_playerMode != PlayerMode::Land;
    }

    bool Sailing() const
    {
        return m_playerMode == PlayerMode::Helm;
    }

    PlayerMode Mode() const
    {
        return m_playerMode;
    }

    float Heading() const
    {
        return m_heading;
    }

    float ShipHeading() const
    {
        return m_shipHeading;
    }

    float DeckFacing() const
    {
        return m_deckFacing;
    }

    Vec3 DeckPosition() const
    {
        return m_deckPosition;
    }

    bool Fishing() const
    {
        return m_fishing;
    }

    bool Moving() const
    {
        return m_moving;
    }

    float FishTime() const
    {
        return m_fishTime;
    }

    WorldPosition ActivePosition() const
    {
        return Sailing() ? ShipActor().location : Player().location;
    }

    float CurrentDeckHeading() const;
    float CurrentDeckHeight() const;

    void Notify(const std::string& text, float seconds = 5)
    {
        m_message = text;
        m_messageTime = seconds;
    }

    const std::string& Notice() const
    {
        return m_message;
    }

    float NoticeTime() const
    {
        return m_messageTime;
    }

private:
    bool BoardShip();
    bool LeaveShip();
    void BoardInteract();
    void PirateInteract();
    void Move(double dx, double dz);
    SceneGraph& m_scene;
    World& m_world;
    PlayerEconomy& m_economy;
    PlayerCombat& m_combat;
    MarineSystem* m_marine = nullptr;
    ActorId m_actorsRoot = NoActor, m_playerActor = NoActor, m_shipActor = NoActor, m_deckSlash = NoActor;
    std::array<bool, 256> m_keys = {};
    PlayerMode m_playerMode = PlayerMode::Land;
    int m_boardedPirate = -1;
    Vec3 m_deckPosition = {};
    float m_deckFacing = 0, m_fishTime = 0, m_heading = 0, m_shipHeading = 0;
    bool m_fishing = false, m_moving = false;
    float m_messageTime = 0;
    std::string m_message;
};
