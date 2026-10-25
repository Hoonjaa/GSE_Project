#include "stdafx.h"
#include "ItemCatalog.h"
#include "MapCatalog.h"
#include <stdexcept>

namespace
{
    const char* Materials[] = {
        "멧돼지 가죽",     "늑대 송곳니",    "정령 나뭇가지",  "거대 벌침",    "고대 나무 심장",
        "소라게 껍데기",   "해안 게 집게",   "해파리 결정",    "세이렌 비늘",  "수호자의 진주",
        "골렘 목재",       "뒤엉킨 밧줄",    "고블린 주머니",  "갑판 쥐 꼬리", "망령의 파편",
        "카멜레온 가죽",   "독거미 실",      "생명 덩굴",      "원숭이 털",    "환영 비늘",
        "골렘 돌조각",     "수정 박쥐 날개", "광석 벌레 껍질", "트롤 가죽",    "수정 핵",
        "맨드레이크 뿌리", "정령 꽃잎",      "나비 날개",      "독버섯 포자",  "수호 꽃술",
        "늪 도마뱀 비늘",  "슬라임 방울",    "개구리 가죽",    "수초 줄기",    "심연의 핵",
        "성게 가시",       "암초 게 껍질",   "바다뱀 비늘",    "하피 깃털",    "포식자 이빨"};
    const int Prices[32] = {5,  8,  10, 7, 35, 8,  4,  10, 12, 16, 18, 14, 22, 18, 20, 28,
                            18, 16, 40, 9, 8,  30, 12, 15, 6,  10, 12, 24, 14, 16, 38, 12};

    std::array<ItemDefinition, ItemCatalog::Count> Build()
    {
        std::array<ItemDefinition, ItemCatalog::Count> items;
        items[0] = {"산호 전리품", 5, 20, EquipSlot::None, ItemIcon::Material};
        items[1] = {"보스 산호 전리품", 20, 20, EquipSlot::None, ItemIcon::Material};
        items[2] = {"물고기", 2, 20, EquipSlot::None, ItemIcon::Fish};
        items[3] = {"크라켄 촉수", 40, 20, EquipSlot::None, ItemIcon::Tentacle};
        for (unsigned i = 0; i < 32; ++i)
        {
            items[4 + i] = {
                MapCatalog::GoodName(static_cast<TradeGood>(i)),
                Prices[i],
                20,
                EquipSlot::None,
                ItemIcon::Trade};
        }
        for (unsigned i = 0; i < 40; ++i)
        {
            const bool epic = i % 5 == 4;
            items[36 + i] = {Materials[i], epic ? 35 : 8, epic ? 5 : 30, EquipSlot::None, ItemIcon::Material};
        }
        items[76] = {"선원 커틀러스", 25, 1, EquipSlot::Weapon, ItemIcon::Sword, 4, 0, 0};
        items[77] = {"강철 커틀러스", 80, 1, EquipSlot::Weapon, ItemIcon::Sword, 10, 0, 0};
        items[78] = {"선원 모자", 12, 1, EquipSlot::Head, ItemIcon::Head, 0, 1, 5};
        items[79] = {"가죽 모자", 35, 1, EquipSlot::Head, ItemIcon::Head, 0, 3, 12};
        items[80] = {"선원 상의", 18, 1, EquipSlot::Body, ItemIcon::Body, 0, 2, 8};
        items[81] = {"가죽 갑옷", 50, 1, EquipSlot::Body, ItemIcon::Body, 0, 5, 20};
        items[82] = {"선원 하의", 15, 1, EquipSlot::Legs, ItemIcon::Legs, 0, 1, 6};
        items[83] = {"가죽 하의", 40, 1, EquipSlot::Legs, ItemIcon::Legs, 0, 3, 15};
        items[84] = {"무기 강화석", 10, 50, EquipSlot::None, ItemIcon::Stone};
        items[85] = {"훈련용 검", 0, 1, EquipSlot::Weapon, ItemIcon::Sword, 0, 0, 0};
        return items;
    }
}

const ItemDefinition& ItemCatalog::Get(InventoryItem item)
{
    static const auto items = Build();
    const auto id = static_cast<unsigned>(item);
    if (id >= items.size())
    {
        throw std::out_of_range("Item ID");
    }
    return items[id];
}

InventoryItem ItemCatalog::MonsterMaterial(unsigned species)
{
    if (species >= 40)
    {
        throw std::out_of_range("Monster material ID");
    }
    return static_cast<InventoryItem>(36 + species);
}

InventoryItem ItemCatalog::Trade(unsigned good)
{
    if (good >= 32)
    {
        throw std::out_of_range("Trade good ID");
    }
    return static_cast<InventoryItem>(4 + good);
}

const char* ItemCatalog::SlotName(EquipSlot slot)
{
    switch (slot)
    {
        case EquipSlot::Weapon:
            return "무기";
        case EquipSlot::Head:
            return "머리 보호구";
        case EquipSlot::Body:
            return "상의 보호구";
        case EquipSlot::Legs:
            return "하의 보호구";
        default:
            return "장착 불가";
    }
}

std::array<DropEntry, 3> MonsterDrops(unsigned species, bool epic)
{
    const unsigned slot = species % 4;
    const unsigned gear[] = {epic ? 77u : 76u, epic ? 79u : 78u, epic ? 81u : 80u, epic ? 83u : 82u};
    return {
        {{ItemCatalog::MonsterMaterial(species), 50, 1},
         {static_cast<InventoryItem>(gear[slot]), 50, 1},
         {InventoryItem::UpgradeStone, 50, 1}}};
}
