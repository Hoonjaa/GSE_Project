#pragma once
#include "NpcData.h"
#include <future>
#include <functional>
#include <vector>

struct NpcDialogueContext
{
    std::string name, role, quest, latestMemory;
    int affinity = 0, trust = 0, progress = 0;
    bool complete = false, recruited = false;
};

class NpcDialogue
{
public:
    using Provider = std::function<std::string(NpcDialogueContext)>;
    void SetProvider(Provider provider);
    void Open(NpcDialogueContext context);
    void Close();
    void Poll();

    const std::string& Text() const
    {
        return m_text;
    }

private:
    struct Request
    {
        unsigned id;
        std::future<std::string> result;
    };

    unsigned m_revision = 0;
    Provider m_provider;
    std::vector<Request> m_pending;
    std::string m_text;
};
