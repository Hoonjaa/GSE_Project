#include "stdafx.h"
#include "LevelOne.h"
#include <cmath>
#include <algorithm>

namespace
{
    float Length(Vec3 a)
    {
        return std::hypot(a.x, a.z);
    }

}

LevelOne::LevelOne(SceneGraph& scene, const LevelOneTerrain& terrain, PlayerEconomy& economy)
    : m_scene(scene), m_economy(economy), m_terrain(terrain), m_context(terrain)
{
    m_root = m_scene.Create<Actor>(NoActor, "레벨 1").Id();
    m_effectsRoot = m_scene.Create<Actor>(m_root, "전투 효과").Id();
    m_slash = m_scene.Create<SlashActor>(m_effectsRoot, m_meshes.slash).Id();
    auto& hud = m_scene.Create<CallbackActor>(m_root, "레벨 상태 UI");
    hud.overlay = [this](Renderer& renderer, const ActorTransform&)
    {
        Overlay(renderer);
    };
    Notify("섬별 몬스터 미리보기 · 일반 4종과 에픽 1종 / 아이템 드랍 비활성");
}

void LevelOne::Notify(const std::string& text)
{
    m_notice = text;
    m_noticeTime = 5;
}

void LevelOne::Recover()
{
    m_health = float(MaxHealth());
    m_invincible = 2;
    m_sinceHit = 0;
    m_scene.Get<SlashActor>(m_slash)->Stop();
    m_context.playerDead = false;
}

void LevelOne::Upgrade()
{
    const int cost = m_upgrade + 1;
    if (m_upgrade >= 10)
    {
        Notify("무기가 최대 강화 단계에 도달했습니다.");
        return;
    }
    if (m_stones < cost)
    {
        Notify("강화석이 부족합니다. 필요 수량: " + std::to_string(cost));
        return;
    }
    m_stones -= cost;
    ++m_upgrade;
    Notify("무기 강화 성공! +" + std::to_string(m_upgrade) + " / 공격력 " + std::to_string(Damage()));
}

void LevelOne::GainExperience(int amount)
{
    if (m_level >= 20)
    {
        return;
    }
    m_xp += amount;
    while (m_level < 20 && m_xp >= 40 + (m_level - 1) * 25)
    {
        m_xp -= 40 + (m_level - 1) * 25;
        ++m_level;
        m_health = float(MaxHealth());
        Notify("레벨 업! " + std::to_string(m_level) + "레벨 · 공격력과 최대 체력이 증가했습니다.");
    }
    if (m_level == 20)
    {
        m_xp = 0;
    }
}

void LevelOne::RewardMonster(bool epic)
{
    ++m_kills;
    GainExperience(epic ? 160 : 18);
    if (epic)
    {
        m_bossDefeated = true;
        Notify("에픽 몬스터 처치! 현재 아이템 드랍은 비활성화 상태입니다.");
    }
}

bool LevelOne::Attack(Vec3 player, Vec3 aim, bool sailing)
{
    if (sailing || m_health <= 0 || m_cooldown > 0)
    {
        return false;
    }
    const float length = Length(aim);
    if (length < .01f)
    {
        return false;
    }
    m_scene.Get<SlashActor>(m_slash)->Trigger(player, {aim.x / length, 0, aim.z / length});
    m_cooldown = .42f;
    return true;
}

LevelOne::~LevelOne()
{
    m_scene.Destroy(m_root);
    m_scene.CollectDestroyed();
}

bool LevelOne::Attacking() const
{
    const auto* slash = m_scene.Get<SlashActor>(m_slash);
    return slash && slash->Attacking();
}

void LevelOne::Prepare(Vec3 offset, bool showHud)
{
    m_scene.Find(m_root)->transform.position = offset;
    m_showHud = showHud;
}

void LevelOne::SetActive(bool active)
{
    auto* root = m_scene.Find(m_root);
    root->enabled = active;
    root->visible = active;
}

void LevelOne::SetActiveRegion(const std::function<bool(Vec3)>& isActive)
{
    for (const auto* handles : {&m_numbers})
    {
        for (ActorId id : *handles)
        {
            if (auto* actor = m_scene.Find(id))
            {
                const bool active = isActive(actor->transform.position);
                actor->enabled = active;
                actor->visible = active;
            }
        }
    }
}

void LevelOne::PruneHandles(std::vector<ActorId>& handles)
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

void LevelOne::AddNumber(Vec3 position, int amount, bool player)
{
    PruneHandles(m_numbers);
    if (m_numbers.size() < 64)
    {
        m_numbers.push_back(m_scene.Create<DamageNumberActor>(m_effectsRoot, position, amount, player).Id());
    }
}

void LevelOne::HitPlayer(int damage)
{
    if (m_invincible > 0 || m_health <= 0)
    {
        return;
    }
    m_health = (std::max)(0.f, m_health - damage);
    m_invincible = .65f;
    m_sinceHit = 0;
    m_context.playerDead = m_health <= 0;
    AddNumber(m_context.player, damage, true);
}

void LevelOne::Collect(int weapon)
{
    if (weapon > m_weapon)
    {
        m_weapon = weapon;
        Notify("새 무기를 자동 장착했습니다! 무기 공격력 +" + std::to_string(m_weapon));
    }
    else
    {
        ++m_stones;
    }
}

void LevelOne::BeginFrame(float dt, Vec3 player, bool sailing, bool village)
{
    PruneHandles(m_numbers);
    m_invincible = (std::max)(0.f, m_invincible - dt);
    m_cooldown = (std::max)(0.f, m_cooldown - dt);
    m_noticeTime = (std::max)(0.f, m_noticeTime - dt);
    m_sinceHit += dt;
    m_context.player = player;
    m_context.active = !sailing && m_terrain.Walk(player.x, player.z);
    m_context.safe = village;
    m_context.playerLevel = m_level;
    m_context.playerDead = m_health <= 0;
    if (m_sinceHit > 5)
    {
        m_health = (std::min)(float(MaxHealth()), m_health + dt * (m_context.safe ? 12 : 2));
    }
}

bool LevelOne::EndFrame()
{
    if (m_health > 0)
    {
        return false;
    }
    Recover();
    Notify("쓰러졌습니다. 항구에서 회복했습니다. 경험치와 장비는 유지됩니다.");
    return true;
}
