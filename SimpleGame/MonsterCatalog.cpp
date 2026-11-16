#include "stdafx.h"
#include "MonsterCatalog.h"
#include <stdexcept>

namespace
{
    const MonsterDefinition Monsters[] = {
        {0, "멧돼지", MonsterForm::Boar, {.49f, .31f, .19f}, false},
        {1, "숲 늑대", MonsterForm::Wolf, {.48f, .55f, .58f}, false},
        {2, "나무 정령", MonsterForm::Tree, {.31f, .59f, .27f}, false},
        {3, "거대 벌", MonsterForm::Bee, {.95f, .73f, .13f}, false},
        {4, "고대 나무 정령", MonsterForm::Tree, {.19f, .47f, .32f}, true},
        {5, "거대 소라게", MonsterForm::Hermit, {.73f, .47f, .28f}, false},
        {6, "해안 게", MonsterForm::Crab, {.9f, .39f, .2f}, false},
        {7, "해파리 정령", MonsterForm::Jellyfish, {.56f, .73f, .94f}, false},
        {8, "세이렌", MonsterForm::Siren, {.28f, .65f, .73f}, false},
        {9, "진주 수호자", MonsterForm::Pearl, {.88f, .84f, .94f}, true},
        {10, "나무 골렘", MonsterForm::WoodGolem, {.49f, .34f, .19f}, false},
        {11, "밧줄 괴물", MonsterForm::Rope, {.76f, .64f, .38f}, false},
        {12, "도둑 고블린", MonsterForm::Goblin, {.4f, .66f, .25f}, false},
        {13, "갑판 쥐", MonsterForm::Rat, {.43f, .39f, .37f}, false},
        {14, "난파선의 망령", MonsterForm::Ghost, {.57f, .79f, .8f}, true},
        {15, "거대 카멜레온", MonsterForm::Chameleon, {.35f, .75f, .29f}, false},
        {16, "독거미", MonsterForm::Spider, {.4f, .23f, .47f}, false},
        {17, "덩굴 괴물", MonsterForm::Vine, {.21f, .52f, .22f}, false},
        {18, "야생 원숭이", MonsterForm::Monkey, {.63f, .39f, .23f}, false},
        {19, "환영의 카멜레온", MonsterForm::Chameleon, {.7f, .36f, .89f}, true},
        {20, "돌 골렘", MonsterForm::StoneGolem, {.5f, .52f, .56f}, false},
        {21, "수정 박쥐", MonsterForm::Bat, {.62f, .49f, .88f}, false},
        {22, "광석 벌레", MonsterForm::Worm, {.57f, .41f, .25f}, false},
        {23, "동굴 트롤", MonsterForm::Troll, {.42f, .47f, .33f}, false},
        {24, "수정 골렘", MonsterForm::Crystal, {.48f, .8f, .95f}, true},
        {25, "맨드레이크", MonsterForm::Mandrake, {.71f, .57f, .32f}, false},
        {26, "꽃 정령", MonsterForm::Flower, {.97f, .52f, .69f}, false},
        {27, "거대 나비", MonsterForm::Butterfly, {.94f, .61f, .25f}, false},
        {28, "독버섯 괴물", MonsterForm::Mushroom, {.69f, .3f, .65f}, false},
        {29, "꽃의 수호 정령", MonsterForm::Flower, {.9f, .39f, .68f}, true},
        {30, "늪 도마뱀", MonsterForm::Lizard, {.38f, .55f, .25f}, false},
        {31, "물 슬라임", MonsterForm::Slime, {.3f, .69f, .9f}, false},
        {32, "거대 개구리", MonsterForm::Frog, {.46f, .72f, .3f}, false},
        {33, "수초 괴물", MonsterForm::Seaweed, {.2f, .59f, .45f}, false},
        {34, "심연의 수로 수호자", MonsterForm::WaterGuardian, {.24f, .43f, .72f}, true},
        {35, "가시 성게 괴물", MonsterForm::Urchin, {.36f, .23f, .52f}, false},
        {36, "암초 게", MonsterForm::ReefCrab, {.77f, .32f, .4f}, false},
        {37, "바다뱀", MonsterForm::Snake, {.25f, .58f, .67f}, false},
        {38, "해안 하피", MonsterForm::Harpy, {.72f, .64f, .5f}, false},
        {39, "암초의 포식자", MonsterForm::Predator, {.32f, .46f, .57f}, true}};
    static_assert(
        sizeof(Monsters) / sizeof(Monsters[0]) == MonsterCatalog::Count,
        "Monster catalog must contain 40 species.");
}

const MonsterDefinition& MonsterCatalog::Get(unsigned id)
{
    if (id >= Count)
    {
        throw std::out_of_range("Monster species ID");
    }
    return Monsters[id];
}

unsigned MonsterCatalog::ForIsland(IslandShape shape, unsigned slot)
{
    const auto index = static_cast<unsigned>(shape);
    if (index >= 8 || slot >= SlotsPerIsland)
    {
        throw std::out_of_range("Island monster slot");
    }
    return index * SlotsPerIsland + slot;
}
