#include "stdafx.h"
#include "LevelOne.h"
#include <sstream>

namespace
{
    const Vec3 Safe = {22, .55f, 17.75f};
    const Color Paper = {.96f, .90f, .72f}, Ink = {.07f, .19f, .23f}, Gold = {.98f, .71f, .32f};
}

void LevelOne::Overlay(Renderer& renderer) const
{
    if (!m_showHud)
    {
        return;
    }
    renderer.Panel(20, 118, 420, 135, Ink);
    std::ostringstream stats;
    stats << "레벨 " << m_level << " / 체력 " << int(m_health) << "/" << MaxHealth() << " / 공격력 "
          << Damage();
    renderer.Text(34, 124, stats.str(), Paper, 1.7f);
    renderer.Panel(34, 150, 390, 7, {.29f, .17f, .17f});
    renderer.Panel(34, 150, 390 * m_health / MaxHealth(), 7, {.78f, .28f, .23f});
    const int required = 40 + (m_level - 1) * 25;
    renderer.Text(
        34,
        164,
        "경험치 " + std::to_string(m_xp) + "/" + std::to_string(required) + " / 강화석 " +
            std::to_string(m_stones),
        Gold,
        1.6f);
    const std::string weaponName =
        m_weapon >= 18 ? "산호 대검"
                       : (m_weapon >= 8 ? "해적 커틀러스" : (m_weapon > 0 ? "선원 단검" : "훈련용 검"));
    renderer.Text(
        34,
        189,
        weaponName + " +" + std::to_string(m_upgrade) + " / F 강화 (" + std::to_string(m_upgrade + 1) + "개)",
        Paper,
        1.6f);
    renderer.Text(
        34,
        214,
        m_bossDefeated ? "보스 처치 완료! 계속 파밍할 수 있습니다."
                       : "목표: 파밍 → 레벨 3·무기 강화 → 북쪽 보스",
        Gold,
        1.5f);
    renderer.Text(34, 234, "좌클릭 근접 공격 / 붉은 원에서 벗어나세요", Paper, 1.4f);
    if (m_noticeTime > 0)
    {
        renderer.Panel(20, float(renderer.Height() - 246), 680, 53, Ink);
        renderer.Text(32, float(renderer.Height() - 242), m_notice, Gold, 1.6f, 650);
    }
}

void LevelOne::Map(Renderer& renderer, float left, float top, float extent, Vec3 player) const
{
    m_terrain.Map(renderer, left, top, extent);
    const float scale = extent / LevelOneTerrain::Size;
    auto marker = [&](Vec3 p, Color color, float size)
    {
        const float x = (p.x - LevelOneTerrain::Min) * scale, z = (p.z - LevelOneTerrain::Min) * scale;
        if (x >= 0 && z >= 0 && x < extent - size && z < extent - size)
        {
            renderer.Panel(left + x, top + z, size, size, color);
        }
    };
    marker(Safe, Gold, 5);
    if (!m_bossDefeated)
    {
        marker({16, .55f, -12}, {.95f, .22f, .17f}, 6);
    }
    marker(player, Paper, 5);
}
