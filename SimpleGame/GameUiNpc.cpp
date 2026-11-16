#include "stdafx.h"
#include "GameUi.h"
#include <algorithm>
#include "QuestRules.h"

void GameUi::NpcKeyInput(unsigned char key)
{
    if (key == 'e' || key == 27)
    {
        CloseServiceMenu();
        return;
    }
    EconomyResult result = {false, ""};
    if (key == 'q')
    {
        result = m_relations.AcceptQuest(m_talk.key);
    }
    else if (key == 'c')
    {
        result = m_relations.CompleteQuest(m_talk.key, m_talk.shape, m_economy);
    }
    else if (key == 'r')
    {
        result = m_relations.Recruit(m_talk.key, m_economy);
    }
    else if (key == 'd')
    {
        result = m_relations.Dismiss(m_talk.key);
    }
    else
    {
        return;
    }
    m_serviceMessage = result.message;
    m_npcs->Open(m_talk);
    ClearInput();
}

void GameUi::RenderNpc(Renderer& r) const
{
    if (!m_talk.valid)
    {
        return;
    }
    const float w = (std::min)(760.f, r.Width() - 24.f), h = (std::min)(450.f, r.Height() - 24.f);
    if (w < 400 || h < 400)
    {
        r.Text(
            12,
            12,
            "대화창을 보려면 창을 키워주세요. E / Esc: 닫기",
            {.98f, .71f, .32f},
            1.3f,
            r.Width() - 24.f);
        return;
    }
    const float x = (r.Width() - w) * .5f, y = (r.Height() - h) * .5f;
    const Color paper = {.96f, .90f, .72f}, ink = {.07f, .19f, .23f}, gold = {.98f, .71f, .32f};
    r.Panel(x, y, w, h, ink);
    const auto& state = static_cast<const NpcState&>(m_relations).Npc(m_talk.key);
    r.Text(
        x + 20,
        y + 16,
        m_npcs->Name(m_talk.key) + " · 호감도 " + std::to_string(state.affinity),
        gold,
        2,
        w - 40);
    r.Text(x + 20, y + 58, m_npcs->Dialogue(), paper, 1.6f, w - 40, h - 304);
    const std::string status = state.quest == QuestStatus::Available
                                   ? "수락 가능"
                                   : (state.quest == QuestStatus::Active ? "진행 중" : "완료");
    r.Text(
        x + 20,
        y + h - 235,
        "의뢰 [" + status + "] " + NpcSystem::Quest(m_talk.key.role),
        gold,
        1.4f,
        w - 40);
    r.Text(
        x + 20,
        y + h - 190,
        "진행 " + std::to_string(m_npcs->Progress(m_talk)) + "/" +
            std::to_string(QuestRules::Get(m_talk.key.role).target) + " · 완료 시 호감도 +" +
            std::to_string(QuestRules::Get(m_talk.key.role).affinityReward) + " / 선택 영입",
        paper,
        1.35f,
        w - 40);
    r.Text(x + 20, y + h - 150, "[Q] 수락   [C] 완료   [R] 동료 영입   [D] 동료 해제", paper, 1.4f, w - 40);
    r.Text(
        x + 20,
        y + h - 116,
        "동료 " + std::to_string(m_relations.Companions().size()) + "/" +
            std::to_string(EconomyRules::CompanionCapacity) + " · 대화 종료 후 I에서도 동료 해제 가능",
        paper,
        1.25f,
        w - 40);
    r.Text(x + 20, y + h - 80, m_serviceMessage, gold, 1.3f, w - 40);
    r.Text(x + 20, y + h - 28, "[E / Esc] 대화 종료", paper, 1.3f);
}
