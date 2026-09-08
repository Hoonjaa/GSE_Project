#pragma once
#include "World.h"
#include <array>
#include <string>

class Prototype
{
public:
    Prototype();
    void Update(double dt, int width, int height);
    void Render(Renderer& renderer);
    void Key(unsigned char key, bool down);
    void ClearInput();
    void Zoom(int direction);
private:
    void Reset();
    void Interact();
    void Move(double dx, double dz);
    WorldPosition ActivePosition() const { return m_sailing ? m_ship : m_player; }
    World m_world;
    WorldPosition m_player, m_ship, m_camera;
    std::array<bool, 256> m_keys = {};
    bool m_sailing = false, m_grid = false, m_hud = true, m_moving = false;
    float m_zoom = 30, m_time = 0, m_heading = 0, m_shipHeading = 0, m_messageTime = 0;
    double m_frameAverage = 1.0/60.0;
    std::string m_message;
    PostProcessingSettings m_postSettings;
    Mesh m_shipMesh, m_characterMesh, m_npcMesh;
};
