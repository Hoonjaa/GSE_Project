#include "stdafx.h"
#include "Prototype.h"
#include <algorithm>
#include <cmath>

void Prototype::RenderPlayerPanel(Renderer& r) const
{
    if (!m_cargoOpen || m_inventoryOpen || m_talk.valid)
    {
        return;
    }
    const Color ink = {.07f, .19f, .23f}, paper = {.96f, .90f, .72f}, gold = {.98f, .71f, .32f};
    const float width = (std::min)(700.f, float(r.Width()) - 24);
    const float left = (r.Width() - width) * .5f;
    const float top = (std::max)(12.f, (r.Height() - 600.f) * .5f);
    if (width < 300 || r.Height() < 620)
    {
        r.Text(12, 12, "적재창을 보려면 창을 키워주세요. K / E / Esc: 닫기", gold, 1, float(r.Width() - 24));
        return;
    }
    r.Panel(left, top, width, 600, ink);
    r.Text(left + 18, top + 14, "8m 돛단배 적재창 · 5종류 슬롯", gold, 2);
    r.Text(left + 18, top + 48, "W/S: 품목 선택  D: 싣기  A: 내리기  K/E/Esc: 닫기", paper, 1.3f);
    r.Text(left + 18, top + 80, "같은 아이템은 한 슬롯만 사용 · 품목마다 중첩 상한이 다릅니다", paper, 1.2f);
    const auto items = CargoItems();
    const unsigned selection = items.empty() ? 0 : (std::min)(m_cargoSelection, unsigned(items.size() - 1));
    const unsigned first = (selection / 4) * 4;
    for (unsigned i = first; i < first + 4 && i < items.size(); ++i)
    {
        const auto item = items[i];
        const float y = top + 115 + (i - first) * 38;
        if (i == selection)
        {
            r.Panel(left + 12, y - 4, width - 24, 34, {.16f, .32f, .34f});
        }
        r.Text(
            left + 22,
            y,
            std::string(i == selection ? "> " : "  ") + EconomyRules::ItemName(item) + " / 가방 " +
                std::to_string(m_economy.ItemCount(item)),
            paper,
            1.5f,
            width - 44);
    }
    for (size_t i = 0; i < m_economy.Cargo().size(); ++i)
    {
        const auto& slot = m_economy.Cargo()[i];
        std::string line = "슬롯 " + std::to_string(i + 1) + ": ";
        line += slot.occupied
                    ? std::string(EconomyRules::ItemName(slot.item)) + " " + std::to_string(slot.count) +
                          "/" + std::to_string(EconomyRules::ItemStack(slot.item))
                    : "비어 있음";
        r.Text(left + 22, top + 280 + i * 32, line, gold, 1.4f, width - 44);
    }
    r.Text(
        left + 18,
        top + 454,
        "가방과 선박 적재품은 별도 보관됩니다. 해상 손실은 적재품에만 적용됩니다.",
        paper,
        1.2f,
        width - 36);
    r.Text(
        left + 18,
        top + 490,
        "W/S로 보유 품목 선택 / 상점에서 특산품·전리품 판매 가능",
        paper,
        1.2f,
        width - 36);
    r.Text(left + 18, top + 530, m_serviceMessage, gold, 1.4f, width - 36);
}

void Prototype::RenderPlayerStatus(Renderer& r) const
{
    const Color ink = {.07f, .19f, .23f}, paper = {.96f, .90f, .72f}, gold = {.98f, .71f, .32f};
    const float left = (std::max)(20.f, float(r.Width()) - 450);
    const float top = 235;
    const float width = (std::min)(430.f, float(r.Width()) - 40);
    if (!Aboard())
    {
        return;
    }
    r.Panel(left, top, width, 185, ink);
    r.Text(
        left + 12,
        top + 8,
        m_playerMode == PlayerMode::PirateDeck
            ? "해적선 약탈 중 / 40초 안에 내 배로 복귀"
            : (Sailing() ? "타륜 조종 중 / E: 갑판으로" : "갑판 활동 / 타륜 앞 E: 조종"),
        gold,
        1.4f,
        width - 24);
    r.Text(
        left + 12,
        top + 34,
        "플레이어 " + std::to_string(int(std::ceil(m_levelOne.Health()))) + "/" +
            std::to_string(m_levelOne.MaxHealth()) + " · 선박 " +
            std::to_string(int(std::ceil(m_marine->ShipHealth()))) + "/100",
        paper,
        1.4f,
        width - 24);
    r.Panel(left + 12, top + 61, width - 24, 7, {.25f, .17f, .15f});
    r.Panel(left + 12, top + 61, (width - 24) * m_marine->ShipHealth() / 100, 7, {.28f, .73f, .53f});
    std::string activity = "F: 낚시 / 클릭: 공격 / O: 토네이도 / P: 크라켄";
    if (m_playerMode == PlayerMode::PirateDeck)
    {
        activity = "상자 앞 E: 약탈 / 가장자리 E: 내 배 복귀";
    }
    if (m_fishing)
    {
        activity = "낚시 중 " + std::to_string(int(m_fishTime)) + "/5초 / F: 종료";
    }
    if (m_marine->Captured())
    {
        activity = "A/D 번갈아 " + std::to_string(m_marine->EscapeCount()) + "/12회 · 남은 " +
                   std::to_string(int(std::ceil(m_marine->EscapeTime()))) + "초";
    }
    r.Text(left + 12, top + 78, activity, gold, 1.4f, width - 24);
    r.Text(left + 12, top + 108, m_marine->Notice(), paper, 1.3f, width - 24);
    r.Text(left + 12, top + 158, "I: 인벤토리·장비 / K: 선박 적재창", paper, 1.3f, width - 24);
}
