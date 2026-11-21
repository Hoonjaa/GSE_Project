#include "stdafx.h"
#include "UiLayout.h"

using namespace UiTheme;

UiRect UiCanvas::Center(float width, float height) const
{
    return {(w - width) * .5f, (h - height) * .5f, width, height};
}

void UiCanvas::Fill(UiRect box, Color color) const
{
    if (draw && box.w > 0 && box.h > 0)
    {
        draw->Panel(box.x * scale, box.y * scale, box.w * scale, box.h * scale, color);
    }
}

void UiCanvas::Text(
    float x,
    float y,
    const std::string& text,
    float size,
    Color color,
    float width,
    float height) const
{
    if (draw)
    {
        draw->Text(x * scale, y * scale, text, color, size * scale, width * scale, height * scale);
    }
}

void UiCanvas::Hit(UiRect box, UiAction command, unsigned arg)
{
    if (box.Contains(m_x, m_y))
    {
        consumed = true;
        if (command != UiAction::None)
        {
            action = command;
            value = arg;
        }
    }
}

void UiCanvas::Frame(UiRect box, float corner)
{
    Hit(box);
    if (!draw)
    {
        return;
    }
    if (!draw->HasArt(UiArt::Panel))
    {
        Fill(box, Gold);
        Fill({box.x + 2, box.y + 2, box.w - 4, box.h - 4}, Teal);
        return;
    }

    // Source dimensions and slice margins come from Assets/UI/teal-brass/skin.json.
    const float x[]{box.x, box.x + corner, box.x + box.w - corner, box.x + box.w};
    const float y[]{box.y, box.y + corner, box.y + box.h - corner, box.y + box.h};
    const float u[]{0, 58.f / 746, 1 - 58.f / 746, 1};
    const float v[]{0, 58.f / 271, 1 - 58.f / 271, 1};
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            draw->Sprite(
                UiArt::Panel,
                x[col] * scale,
                y[row] * scale,
                (x[col + 1] - x[col]) * scale,
                (y[row + 1] - y[row]) * scale,
                {u[col], v[row]},
                {u[col + 1], v[row + 1]});
        }
    }
}

void UiCanvas::Compass(float x, float y, float size) const
{
    if (!draw)
    {
        return;
    }
    if (draw->HasArt(UiArt::Compass))
    {
        draw->Sprite(UiArt::Compass, x * scale, y * scale, size * scale, size * scale);
    }
    else
    {
        Text(x + size * .25f, y + size * .25f, "✦", 2, Gold);
    }
}

void UiCanvas::Icon(const Mesh& mesh, float x, float y, float size) const
{
    if (draw)
    {
        draw->Icon(mesh, x * scale, y * scale, size * scale, size * scale);
    }
}

void UiCanvas::Bar(UiRect box, float fraction, Color color) const
{
    Fill(box, Track);
    box.w *= (std::max)(0.f, (std::min)(1.f, fraction));
    Fill(box, color);
}

void UiCanvas::Card(UiRect box, bool selected) const
{
    Fill(box, selected ? Gold : Line);
    Fill({box.x + 1, box.y + 1, box.w - 2, box.h - 2}, selected ? Selected : Deep);
}

void UiCanvas::Header(UiRect box, const std::string& title, const std::string& subtitle)
{
    Frame(box, 19);
    Text(box.x + 28, box.y + 20, title, 3.05f, Ivory, box.w - 110, 36);
    Text(box.x + 28, box.y + 62, subtitle, 1.55f, Muted, box.w - 110, 21);
    Fill({box.x + 28, box.y + 94, box.w - 56, 1}, Line);
    Button({box.x + box.w - 70, box.y + 20, 42, 42}, "×", UiAction::Close);
}

void UiCanvas::Footer(UiRect box)
{
    Fill({box.x + 28, box.y + box.h - 76, box.w - 56, 1}, Line);
    Button({box.x + 28, box.y + box.h - 62, 104, 42}, "Esc  닫기", UiAction::Close);
}

void UiCanvas::Button(
    UiRect box,
    const std::string& label,
    UiAction command,
    unsigned arg,
    bool primary,
    bool enabled)
{
    Hit(box, enabled ? command : UiAction::None, arg);
    if (primary && enabled)
    {
        Fill(box, Gold);
        Fill({box.x + 1, box.y + 1, box.w - 2, box.h * .48f}, Ivory);
    }
    else
    {
        Card(box);
    }
    Text(
        box.x + 10,
        box.y + (box.h - 17) * .5f,
        label,
        1.65f,
        primary && enabled ? Deep : (enabled ? Ivory : Muted),
        box.w - 18,
        20);
}
