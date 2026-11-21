#include "stdafx.h"
#include "GameUi.h"
#include "IslandTestMap.h"
#include "QuestRules.h"
#include <iterator>

using namespace UiTheme;

void GameUi::PageKey(unsigned char key)
{
    if (key == 27 || (key == 'j' && m_page == Page::Journal) || (key == 'm' && m_page == Page::Chart))
    {
        Escape();
        return;
    }
    if (m_page == Page::Pause)
    {
        if (key == 13)
        {
            CloseServiceMenu();
        }
        return;
    }
    if (m_page == Page::Test)
    {
        VisualKey(key);
        return;
    }
    if (m_page == Page::Journal)
    {
        if (key == 'w')
        {
            m_questSelection = (m_questSelection + NpcRole::Count - 1) % NpcRole::Count;
        }
        if (key == 's')
        {
            m_questSelection = (m_questSelection + 1) % NpcRole::Count;
        }
    }
    if (m_page == Page::Chart)
    {
        const auto count = m_world.ChartIslands().size();
        if (key == 'w' && m_chartSelection > 0)
        {
            --m_chartSelection;
        }
        if (key == 's' && m_chartSelection + 1 < count)
        {
            ++m_chartSelection;
        }
    }
}

void GameUi::RenderJournal(UiCanvas& ui) const
{
    const auto box = ui.Center(848, 354);
    const float x = box.x + 28, y = box.y, right = x + 450;
    const auto mode = m_world.IsTestMap() ? 1 : 0;
    const auto& island = m_lastIsland[mode];
    const bool known = m_haveLastIsland[mode];
    ui.Header(box, "의뢰 일지", known ? MapCatalog::Island(island.shape).name : "항구 방문 기록 없음");
    if (!known)
    {
        ui.Text(x, y + 123, "섬에 도착하면 이곳에서 마을 의뢰를 확인할 수 있습니다.", 1.65f, Muted);
        ui.Footer(box);
        return;
    }

    for (unsigned role = 0; role < NpcRole::Count; ++role)
    {
        const UiRect row{x, y + 112 + role * 49.f, 430, 44};
        const NpcKey key{island.key, role};
        const auto& record = m_relations.Npc(key);
        const auto& quest = QuestRules::Get(role);
        const int progress = m_relations.Progress(key, island.shape, m_economy);
        ui.Card(row, m_questSelection == role);
        ui.Hit(row, UiAction::Quest, role);
        ui.Text(row.x + 12, row.y + 10, QuestRules::Title(role), 1.75f);
        ui.Text(
            row.x + 323,
            row.y + 11,
            QuestRules::StatusLabel(record.quest, progress, quest.target),
            1.5f,
            Muted,
            101,
            23);
    }

    const NpcKey selected{island.key, m_questSelection};
    const auto& quest = QuestRules::Get(selected.role);
    const int progress = m_relations.Progress(selected, island.shape, m_economy);
    ui.Text(right, y + 113, QuestRules::Title(selected.role), 1.95f, Gold);
    ui.Text(right, y + 146, "의뢰인 · " + m_npcs->Name(selected), 1.45f, Muted);
    ui.Text(right, y + 177, QuestRules::Description(selected.role), 1.55f, Ivory, 342, 43);
    ui.Bar({right, y + 225, 342, 8}, float(progress) / quest.target, Safe);
    ui.Text(
        right,
        y + 242,
        "진행 " + std::to_string(progress) + " / " + std::to_string(quest.target) + " · 호감도 +" +
            std::to_string(quest.affinityReward),
        1.45f,
        Muted);
    ui.Footer(box);
}

void GameUi::RenderChart(UiCanvas& ui) const
{
    const auto box = ui.Center(848, 496);
    const float x = box.x + 28, y = box.y, right = x + 524;
    const auto records = m_world.ChartIslands();
    const auto visited = std::count_if(
        records.begin(),
        records.end(),
        [](const auto& entry)
        {
            return entry.second;
        });
    ui.Header(box, "전체 해도", "발견 기록 · 방문한 섬 " + std::to_string(visited) + "곳");

    const UiRect map{x, y + 112, 506, 288};
    ui.Fill(map, Teal);
    for (float dx = 13; dx < map.w; dx += 25)
    {
        ui.Fill({x + dx, map.y, 1, map.h}, Line);
    }
    for (float dy = 13; dy < map.h; dy += 25)
    {
        ui.Fill({x, map.y + dy, map.w, 1}, Line);
    }

    if (records.empty())
    {
        ui.Text(x + 18, map.y + 17, "주변에 표시할 섬이 없습니다.", 1.6f, Muted);
        ui.Footer(box);
        return;
    }
    const auto player = ActivePosition();
    const auto* current = m_world.CurrentIsland(player);
    const unsigned selection = (std::min)(m_chartSelection, static_cast<unsigned>(records.size() - 1));
    const unsigned first = (selection / 5) * 5;
    const bool test = m_world.IsTestMap();
    auto describe = [test](IslandKey key)
    {
        return test ? IslandTestMap::Describe(static_cast<unsigned>(key.z * IslandTestMap::Columns + key.x))
                    : IslandGeneration::Describe(key);
    };
    const auto selected = std::next(records.begin(), selection);
    const auto focus = test ? IslandTestMap::Center() : describe(selected->first).center;
    const float scale = test ? 1.16f : .20f;
    const Vec2 center{map.x + map.w * .5f, map.y + map.h * .5f};
    unsigned index = 0;
    ui.Text(right, y + 111, "섬 목록", 1.85f);
    if (records.size() > 5)
    {
        ui.Button({right + 152, y + 107, 54, 31}, "W ↑", UiAction::Key, 'w');
        ui.Button({right + 214, y + 107, 54, 31}, "S ↓", UiAction::Key, 's');
    }
    for (const auto& entry : records)
    {
        const auto info = describe(entry.first);
        const bool here = current && current->Info().key == entry.first;
        const auto& type = MapCatalog::Island(info.shape);
        if (index >= first && index < first + 5)
        {
            const UiRect row{right, y + 146 + (index - first) * 45.f, 268, 41};
            ui.Card(row, index == selection);
            ui.Hit(row, UiAction::Island, index);
            ui.Text(row.x + 11, row.y + 10, type.name, 1.65f, Ivory);
            ui.Text(
                row.x + 187,
                row.y + 10,
                here ? "현재" : (entry.second ? "방문" : "미방문"),
                1.45f,
                Muted);
        }
        const auto offset = info.center.RelativeTo(focus);
        const float px = center.x + offset.x * scale, py = center.y + offset.z * scale;
        if (px >= map.x + 29 && px <= map.x + map.w - 76 && py >= map.y + 23 && py <= map.y + map.h - 35)
        {
            constexpr float Size = 37;
            if (ui.draw)
            {
                if (const auto* layout = m_world.CachedIsland(entry.first))
                {
                    constexpr int Samples = 24;
                    const float localStep = -2 * layout->Origin() / Samples;
                    for (int row = 0; row < Samples; ++row)
                    {
                        for (int col = 0; col < Samples; ++col)
                        {
                            const float lx = layout->Origin() + (col + .5f) * localStep;
                            const float lz = layout->Origin() + (row + .5f) * localStep;
                            if (layout->Land(lx, lz))
                            {
                                const float cell = Size / Samples;
                                const Color land =
                                    entry.second ? Color{.60f, .69f, .42f} : Color{.37f, .55f, .44f};
                                ui.Fill(
                                    {px - Size * .5f + col * cell, py - Size * .5f + row * cell, cell, cell},
                                    land);
                            }
                        }
                    }
                }
                else
                {
                    ui.Compass(px - 9, py - 9, 18);
                }
            }
            ui.Text(px - 28, py + 16, type.name, 1.35f, here || index == selection ? Gold : Muted, 102, 19);
            ui.Hit({px - 20, py - 20, 70, 55}, UiAction::Island, index);
        }
        ++index;
    }
    const auto relative = player.RelativeTo(focus);
    const float px = center.x + relative.x * scale, py = center.y + relative.z * scale;
    if (px >= map.x + 4 && px <= map.x + map.w - 4 && py >= map.y + 4 && py <= map.y + map.h - 4)
    {
        ui.Fill({px - 3, py - 3, 6, 6}, Ivory);
    }
    const auto& type = MapCatalog::Island(describe(selected->first).shape);
    std::string goods;
    for (const auto good : type.specialties)
    {
        if (!goods.empty())
        {
            goods += " · ";
        }
        goods += MapCatalog::GoodName(good);
    }
    ui.Footer(box);
    ui.Text(x + 122, y + 446, "특산품 · " + goods, 1.45f, Muted, 662, 35);
}

void GameUi::RenderPause(UiCanvas& ui) const
{
    const bool help = m_page == Page::Help;
    const auto box = ui.Center(help ? 648.f : 504.f, help ? 348.f : 274.f);
    const float x = box.x + 28, y = box.y;
    ui.Frame(box, 19);
    ui.Text(x, y + 20, help ? "조작 안내" : "일시정지", 3.05f);
    ui.Button({box.x + box.w - 70, y + 20, 42, 42}, "×", UiAction::Close);
    ui.Fill({x, y + 76, box.w - 56, 1}, Line);
    if (help)
    {
        ui.Text(
            x,
            y + 94,
            "WASD 이동 · Space 가속 · 좌클릭 근접 공격\n"
            "E 시설·NPC 이용 / 승선·하선 · F 강화 / 낚시\n"
            "I 가방 · J 의뢰 · M 해도 · K 선박 적재창\n"
            "T 테스트 맵 · Y 전체/상세 · F1–F8 섬 이동 · F10 개발 도구\n"
            "H HUD · V 렌더링 상태 · B 후처리 · G 격자 · R 귀환",
            1.7f,
            Ivory,
            box.w - 56,
            165);
        ui.Button({x, y + 266, 140, 42}, "Esc  돌아가기", UiAction::Close);
        return;
    }
    ui.Button({x, y + 94, 448, 42}, "이어하기", UiAction::Close, 0, true);
    ui.Button({x, y + 146, 448, 42}, "조작 안내", UiAction::Help);
    ui.Button({x, y + 198, 448, 42}, "종료", UiAction::Quit);
}

void GameUi::RenderTestPanel(UiCanvas& ui) const
{
    const auto box = ui.Center(848, 526);
    const float x = box.x + 28, y = box.y;
    ui.Header(box, "섬 테스트 맵", "개발 도구 · T 월드 왕복 · Y 전체 / 상세");
    for (unsigned i = 0; i < IslandTestMap::Count; ++i)
    {
        const UiRect cell{x + i % 4 * 199.f, y + 112 + i / 4 * 81.f, 193, 74};
        ui.Card(cell, i == m_testIsland);
        ui.Hit(cell, UiAction::TestIsland, i);
        ui.Text(cell.x + 82, cell.y + 9, "F" + std::to_string(i + 1), 1.55f, Gold);
        ui.Text(cell.x + 35, cell.y + 41, MapCatalog::Island(static_cast<IslandShape>(i)).name, 1.8f);
    }
    ui.Text(
        x + 12,
        y + 280,
        std::string("선택된 테스트 섬 · ") + MapCatalog::Island(static_cast<IslandShape>(m_testIsland)).name,
        1.8f,
        Ivory);
    ui.Text(
        x + 12,
        y + 318,
        "활성 청크 " + std::to_string(m_world.VisibleChunks()) + " · " +
            std::to_string(int(1 / m_frameAverage)) + " FPS",
        1.7f,
        Muted);
    ui.Button({x, y + 369, 119, 42}, "G  격자", UiAction::Grid);
    ui.Button(
        {x + 128, y + 369, 230, 42},
        m_postSettings.enabled ? "B  후처리 켜짐" : "B  후처리 꺼짐",
        UiAction::Key,
        'b');
    ui.Button({x + 368, y + 369, 130, 42}, "[  노출 낮춤", UiAction::Key, '[');
    ui.Button({x + 508, y + 369, 130, 42}, "]  노출 높임", UiAction::Key, ']');
    ui.Footer(box);
    ui.Button(
        {x + 495, y + 464, 143, 42},
        m_testOverview ? "Y  상세 보기" : "Y  전체 보기",
        UiAction::TestOverview);
    ui.Button({x + 648, y + 464, 144, 42}, "T  월드 복귀", UiAction::TestTravel);
}
