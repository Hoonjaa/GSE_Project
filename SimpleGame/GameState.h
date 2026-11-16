#pragma once
#include "PlayerEconomy.h"
#include "NpcState.h"

struct GameState
{
    PlayerEconomy economy;
    NpcState npcs;
};
