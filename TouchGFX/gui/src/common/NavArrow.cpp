#include <gui/common/NavArrow.hpp>

NavArrow::NavArrow()
    : dir_(RIGHT),
      colorBg_(touchgfx::Color::getColorFromRGB(50, 55, 90)),
      colorArrow_(touchgfx::Color::getColorFromRGB(200, 220, 255))
{
}

void NavArrow::fillLocal(touchgfx::Rect r, uint16_t color, const touchgfx::Rect& invalidatedArea) const
{
    r &= invalidatedArea;
    if (r.isEmpty()) return;
    translateRectToAbsolute(r);
    touchgfx::HAL::lcd().fillRect(r, color, 255);
}

void NavArrow::draw(const touchgfx::Rect& invalidatedArea) const
{
    const int16_t w = getWidth();
    const int16_t h = getHeight();

    /* --- Hintergrund --- */
    fillLocal(touchgfx::Rect(0, 0, w, h), colorBg_, invalidatedArea);

    /* --- Chevron mit 45°-Schenkeln, zeilenweise gezeichnet --- */
    const int16_t size  = (int16_t)(((w < h) ? w : h) * 7 / 10);  // Pfeilhoehe
    const int16_t half  = size / 2;                                  // = Pfeilbreite
    const int16_t thick = (size / 4 < 3) ? 3 : size / 4;             // Strichstaerke (horizontal)
    const int16_t cx    = w / 2;
    const int16_t cy    = h / 2;

    for (int16_t dy = -half; dy <= half; ++dy)
    {
        const int16_t ady = (dy < 0) ? -dy : dy;
        int16_t x;
        if (dir_ == RIGHT)
            x = (int16_t)(cx + half / 2 - ady - thick);   // Spitze rechts
        else
            x = (int16_t)(cx - half / 2 + ady);           // Spitze links
        fillLocal(touchgfx::Rect(x, (int16_t)(cy + dy), thick, 1), colorArrow_, invalidatedArea);
    }
}
