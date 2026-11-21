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

void GameUi::RenderNpc(UiCanvas& ui) const
{
    using namespace UiTheme;
    const auto box = ui.Center(848, 378);
    const float x = box.x + 28, y = box.y;
    const auto& record = m_relations.Npc(m_talk.key);
    const auto& quest = QuestRules::Get(m_talk.key.role);
    const int progress = m_npcs->Progress(m_talk);
    ui.Header(
        box,
        m_npcs->Name(m_talk.key),
        std::string(MapCatalog::Island(m_talk.shape).name) + " · 호감도 " + std::to_string(record.affinity));
    ui.Fill({x, y + 110, 792, 71}, Deep);
    ui.Fill({x, y + 110, 2, 71}, Gold);
    ui.Text(x + 15, y + 119, m_npcs->Dialogue(), 1.6f, Ivory, 760, 56);
    ui.Card({x, y + 190, 792, 39});
    ui.Text(x + 12, y + 199, QuestRules::Title(m_talk.key.role), 1.8f, Gold);
    ui.Text(x + 650, y + 200, QuestRules::StatusLabel(record.quest, progress, quest.target), 1.55f, Muted);
    ui.Text(x, y + 239, QuestRules::Description(m_talk.key.role), 1.65f, Ivory, 792, 23);
    ui.Text(
        x,
        y + 271,
        "진행 " + std::to_string(progress) + " / " + std::to_string(quest.target) + " · 완료 보상 호감도 +" +
            std::to_string(quest.affinityReward),
        1.5f,
        Muted,
        792,
        20);
    ui.Footer(box);
    ui.Text(x + 114, y + 327, m_serviceMessage, 1.4f, Gold, 505, 22);
    const bool available = record.quest == QuestStatus::Available;
    const bool active = record.quest == QuestStatus::Active;
    const std::string action =
        available ? "Q  수락" : (active ? "C  보고" : (record.recruited ? "D  동료 해제" : "R  동료 영입"));
    const unsigned char key = available ? 'q' : (active ? 'c' : (record.recruited ? 'd' : 'r'));
    ui.Button(
        {x + 642, y + 316, 150, 42},
        action,
        UiAction::Key,
        key,
        true,
        !active || progress >= quest.target);
}
