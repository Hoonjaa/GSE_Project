#include "stdafx.h"
#include "PlayerCombat.h"
#include <cmath>
#include <algorithm>

namespace
{
    float Length(Vec3 a)
    {
        return std::hypot(a.x, a.z);
    }

}

PlayerCombat::PlayerCombat(SceneGraph& scene, PlayerEconomy& economy) : m_scene(scene), m_economy(economy)
{
    m_root = m_scene.Create<Actor>(NoActor, "플레이어 전투").Id();
    m_effectsRoot = m_scene.Create<Actor>(m_root, "전투 효과").Id();
    m_slash = m_scene.Create<SlashActor>(m_effectsRoot, m_meshes.slash).Id();
    Notify("섬별 몬스터 미리보기 · 일반 4종과 에픽 1종 / 전리품·장비 자동 습득");
}

void PlayerCombat::Notify(const std::string& text)
{
    m_notice = text;
    m_noticeTime = 5;
}

void PlayerCombat::Recover()
{
    m_health = float(MaxHealth());
    m_invincible = 2;
    m_sinceHit = 0;
    m_scene.Get<SlashActor>(m_slash)->Stop();
}

void PlayerCombat::Upgrade()
{
    if (m_economy.Equipped(EquipSlot::Weapon) < 0)
    {
        Notify("무기를 착용한 뒤 강화하세요.");
        return;
    }
    const int cost = m_upgrade + 1;
    if (m_upgrade >= 10)
    {
        Notify("무기가 최대 강화 단계에 도달했습니다.");
        return;
    }
    if (m_economy.ItemCount(InventoryItem::UpgradeStone) < cost)
    {
        Notify("강화석이 부족합니다. 필요 수량: " + std::to_string(cost));
        return;
    }
    m_economy.ConsumeItems(InventoryItem::UpgradeStone, cost);
    ++m_upgrade;
    Notify("무기 강화 성공! +" + std::to_string(m_upgrade) + " / 공격력 " + std::to_string(Damage()));
}

void PlayerCombat::GainExperience(int amount)
{
    if (m_level >= 20)
    {
        return;
    }
    m_xp += amount;
    while (m_level < 20 && m_xp >= ExperienceTarget())
    {
        m_xp -= ExperienceTarget();
        ++m_level;
        m_health = float(MaxHealth());
        Notify("레벨 업! " + std::to_string(m_level) + "레벨 · 공격력과 최대 체력이 증가했습니다.");
    }
    if (m_level == 20)
    {
        m_xp = 0;
    }
}

void PlayerCombat::RewardMonster(bool epic)
{
    ++m_kills;
    GainExperience(epic ? 160 : 18);
    if (epic)
    {
        m_bossDefeated = true;
        Notify("에픽 몬스터 처치! 전리품·장비를 확인하세요.");
    }
}

bool PlayerCombat::Attack(const WorldPosition& player, Vec3 aim)
{
    if (m_health <= 0 || m_cooldown > 0)
    {
        return false;
    }
    const float length = Length(aim);
    if (length < .01f)
    {
        return false;
    }
    m_slashPosition = player;
    m_scene.Get<SlashActor>(m_slash)->Trigger({0, 0, 0}, {aim.x / length, 0, aim.z / length});
    m_cooldown = .42f;
    return true;
}

PlayerCombat::~PlayerCombat()
{
    m_scene.Destroy(m_root);
    m_scene.CollectDestroyed();
}

bool PlayerCombat::Attacking() const
{
    const auto* slash = m_scene.Get<SlashActor>(m_slash);
    return slash && slash->Attacking();
}

void PlayerCombat::Prepare(const WorldPosition& camera)
{
    auto* slash = m_scene.Get<SlashActor>(m_slash);
    const float height = slash->transform.position.y;
    slash->transform.position = m_slashPosition.RelativeTo(camera);
    slash->transform.position.y = height;
    for (auto id : m_numbers)
    {
        if (auto* number = m_scene.Get<DamageNumberActor>(id))
        {
            number->transform.position = number->location.RelativeTo(camera);
        }
    }
}

void PlayerCombat::SetActiveRegion(const std::function<bool(const WorldPosition&)>& isActive)
{
    for (ActorId id : m_numbers)
    {
        if (auto* actor = m_scene.Get<DamageNumberActor>(id))
        {
            const bool active = isActive(actor->location);
            actor->enabled = active;
            actor->visible = active;
        }
    }
}

void PlayerCombat::PruneHandles(std::vector<ActorId>& handles)
{
    handles.erase(
        std::remove_if(
            handles.begin(),
            handles.end(),
            [this](ActorId id)
            {
                return !m_scene.Find(id);
            }),
        handles.end());
}

void PlayerCombat::AddNumber(WorldPosition position, int amount, bool player)
{
    PruneHandles(m_numbers);
    if (m_numbers.size() < 64)
    {
        m_numbers.push_back(m_scene.Create<DamageNumberActor>(m_effectsRoot, position, amount, player).Id());
    }
}

void PlayerCombat::HitPlayer(int damage)
{
    if (damage <= 0 || m_invincible > 0 || m_health <= 0)
    {
        return;
    }
    damage = (std::max)(1, damage - m_economy.EquipmentDefence());
    m_health = (std::max)(0.f, m_health - damage);
    m_invincible = .65f;
    m_sinceHit = 0;
    AddNumber(m_player, damage, true);
}

void PlayerCombat::BeginFrame(float dt, const WorldPosition& player, bool village)
{
    PruneHandles(m_numbers);
    m_invincible = (std::max)(0.f, m_invincible - dt);
    m_cooldown = (std::max)(0.f, m_cooldown - dt);
    m_noticeTime = (std::max)(0.f, m_noticeTime - dt);
    m_sinceHit += dt;
    m_player = player;
    if (m_sinceHit > 5)
    {
        m_health = (std::min)(float(MaxHealth()), m_health + dt * (village ? 12 : 2));
    }
}

bool PlayerCombat::EndFrame()
{
    if (m_health > 0)
    {
        return false;
    }
    Recover();
    Notify("쓰러졌습니다. 항구에서 회복했습니다. 경험치와 장비는 유지됩니다.");
    return true;
}

void PlayerCombat::EquipmentChanged()
{
    m_health = (std::min)(m_health, float(MaxHealth()));
}
