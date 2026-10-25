#include "stdafx.h"
#include "LevelOne.h"
#include <sstream>

namespace
{

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
            std::to_string(m_economy.ItemCount(InventoryItem::UpgradeStone)),
        Gold,
        1.6f);
    const int weapon = m_economy.Equipped(EquipSlot::Weapon);
    const std::string weaponName =
        weapon < 0 ? "맨손" : ItemCatalog::Get(static_cast<InventoryItem>(weapon)).name;
    renderer.Text(
        34,
        189,
        weaponName + " +" + std::to_string(m_upgrade) + " / F 강화 (" + std::to_string(m_upgrade + 1) + "개)",
        Paper,
        1.6f);
    renderer.Text(
        34,
        214,
        !m_economy.HasShip() ? "몬스터 확인용 50골드 · 조선소에서 돛단배 구매"
                             : "섬마다 일반 4종·에픽 1종 / 전리품·장비 자동 습득",
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
        const float x = (p.x - m_terrain.MapOrigin()) / m_terrain.CellSize() * scale;
        const float z = (p.z - m_terrain.MapOrigin()) / m_terrain.CellSize() * scale;
        if (x >= 0 && z >= 0 && x < extent - size && z < extent - size)
        {
            renderer.Panel(left + x, top + z, size, size, color);
        }
    };
    marker(m_terrain.SpawnPoint(), Gold, 5);
    marker(player, Paper, 5);
}
