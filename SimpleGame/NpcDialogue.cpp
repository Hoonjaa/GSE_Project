#include "stdafx.h"
#include "NpcDialogue.h"
#include "Diagnostics.h"
#include <chrono>

void NpcDialogue::SetProvider(Provider provider)
{
    m_provider = std::move(provider);
}

void NpcDialogue::Open(NpcDialogueContext context)
{
    ++m_revision;
    m_text = context.affinity < 0 ? "아직 당신을 온전히 믿기는 어렵군요. " : "어서 오세요, 탐험가님. ";
    if (context.recruited)
    {
        m_text += "함께 미지의 바다로 향할 준비가 되어 있어요.";
    }
    else if (context.complete)
    {
        m_text += "도와주셔서 고마워요. 원한다면 동료가 되어 함께 항해하겠어요.";
    }
    else
    {
        m_text += context.quest + " 의뢰를 마치면 함께할지 이야기해 봐요.";
    }
    if (!context.latestMemory.empty())
    {
        m_text += " 기억하고 있어요: " + context.latestMemory;
    }
    // An optional provider receives a value-only fact snapshot and can return dialogue text only.
    // No game state or item/quest mutation callbacks are exposed to it.
    Poll();
    if (m_provider && m_pending.size() < 2)
    {
        const auto provider = m_provider;
        m_pending.push_back(
            {m_revision,
             std::async(
                 std::launch::async,
                 [provider, context]
                 {
                     return provider(context);
                 })});
    }
}

void NpcDialogue::Close()
{
    ++m_revision;
}

void NpcDialogue::Poll()
{
    for (auto it = m_pending.begin(); it != m_pending.end();)
    {
        if (it->result.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            ++it;
            continue;
        }
        try
        {
            const auto result = it->result.get();
            if (it->id == m_revision && !result.empty() && result.size() <= 2048)
            {
                m_text = result;
            }
        }
        catch (...)
        {
            Diagnostics::Get().Event("npc_dialogue_provider_failed fallback=1");
        }
        it = m_pending.erase(it);
    }
}
