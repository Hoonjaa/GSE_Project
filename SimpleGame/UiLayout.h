#pragma once
#include "Renderer.h"
#include <algorithm>

struct UiRect
{
    float x, y, w, h;

    bool Contains(int X, int Y) const
    {
        return X >= x && Y >= y && X < x + w && Y < y + h;
    }

    void Draw(Renderer& r, Color color) const
    {
        r.Panel(x, y, w, h, color);
    }
};

struct HudLayout
{
    UiRect title, stats, postStats, chart, testChart, island, economy, marine, footer, message, combatNotice;

    HudLayout(int width, int height)
        : title{20, 20, 330, 88}, stats{20, 118, 420, 135}, postStats{20, 118, 420, 107},
          chart{float(width - 192), 20, 172, 200},
          testChart{(std::max)(20.f, float(width - 370)), 20, 350, 185}, island{20, 265, 430, 86},
          economy{20, 360, 430, 84},
          marine{(std::max)(20.f, float(width - 450)), 235, (std::min)(430.f, float(width - 40)), 185},
          footer{20, float(height - 122), float(width - 40), 102},
          message{20, float(height - 185), (std::min)(float(width - 40), 700.f), 55},
          combatNotice{20, float(height - 246), 680, 53}
    {
    }
};
