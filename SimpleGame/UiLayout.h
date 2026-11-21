#pragma once
#include "Renderer.h"
#include <algorithm>
#include <cmath>

struct UiRect
{
    float x, y, w, h;

    bool Contains(float X, float Y) const
    {
        return X >= x && Y >= y && X < x + w && Y < y + h;
    }
};

enum class UiAction
{
    None,
    Key,
    Close,
    Bag,
    Gear,
    Dismiss,
    Market,
    Cargo,
    Quest,
    Island,
    TestIsland,
    TestPanel,
    TestOverview,
    TestTravel,
    Grid,
    Help,
    Quit
};

namespace UiTheme
{
    constexpr Color Deep{.035f, .169f, .204f}, Teal{.063f, .255f, .294f};
    constexpr Color Ivory{1, .961f, .863f}, Muted{.749f, .835f, .812f};
    constexpr Color Gold{.937f, .808f, .541f}, Safe{.545f, .827f, .741f};
    constexpr Color Warning{.933f, .576f, .482f}, Line{.28f, .38f, .34f};
    constexpr Color Selected{.16f, .32f, .30f}, Track{.025f, .105f, .139f};
}

inline float UiScale(int width, int height)
{
    return (std::min)(1.f, (std::min)((std::max)(1, width) / 1024.f, (std::max)(1, height) / 700.f));
}

struct HudLayout
{
    UiRect stats, economy, chart, side, activity, message, menu;

    HudLayout(float width, float height)
        : stats{24, 24, 342, 111}, economy{24, stats.y + stats.h + 12, 132, 40},
          chart{width - 210, 24, 186, 194}, side{width - 252, 242, 228, 96},
          activity{width * .5f - 224, height - 204, 448, 108}, message{24, height - 156, 370, 74},
          menu{width - 128, height - 64, 104, 42}
    {
    }
};

// One layout is evaluated both for drawing and for pointer hits.
class UiCanvas
{
public:
    UiCanvas(int width, int height, Renderer* renderer, int pointerX = -1, int pointerY = -1)
        : scale(UiScale(width, height)), w(width / scale), h(height / scale), draw(renderer),
          m_x(pointerX / scale), m_y(pointerY / scale)
    {
    }

    UiRect Center(float width, float height) const;
    void Fill(UiRect box, Color color) const;
    void Text(
        float x,
        float y,
        const std::string& text,
        float size,
        Color color = UiTheme::Ivory,
        float width = 0,
        float height = 0) const;
    void Frame(UiRect box, float corner = 14);
    void Compass(float x, float y, float size) const;
    void Icon(const Mesh& mesh, float x, float y, float size) const;
    void Bar(UiRect box, float fraction, Color color) const;
    void Card(UiRect box, bool selected = false) const;
    void Header(UiRect box, const std::string& title, const std::string& subtitle);
    void Footer(UiRect box);
    void Button(
        UiRect box,
        const std::string& label,
        UiAction action,
        unsigned value = 0,
        bool primary = false,
        bool enabled = true);
    void Hit(UiRect box, UiAction action = UiAction::None, unsigned value = 0);

    const float scale, w, h;
    Renderer* const draw;
    bool consumed = false;
    UiAction action = UiAction::None;
    unsigned value = 0;

private:
    const float m_x, m_y;
};
