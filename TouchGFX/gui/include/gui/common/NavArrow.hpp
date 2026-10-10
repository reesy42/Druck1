#ifndef NAVARROW_HPP
#define NAVARROW_HPP

#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/Color.hpp>

/**
 * Navigations-Button mit gezeichnetem Pfeil ("<" bzw. ">").
 *
 * Der Pfeil wird direkt in draw() gezeichnet (wie PressureGraph), weil der
 * Font verdana_20 keine Glyphen fuer '<' und '>' enthaelt und eine
 * Font-Neugenerierung den TouchGFX Designer erfordern wuerde.
 */
class NavArrow : public touchgfx::Widget
{
public:
    enum Direction { LEFT, RIGHT };

    NavArrow();

    void setDirection(Direction d)            { dir_ = d; invalidate(); }
    void setBackgroundColor(uint16_t color)   { colorBg_ = color; invalidate(); }
    void setArrowColor(uint16_t color)        { colorArrow_ = color; invalidate(); }

    /** true, wenn (x,y) in Elternkoordinaten innerhalb des Buttons liegt */
    bool contains(int16_t x, int16_t y) const
    {
        touchgfx::Rect r = getRect();
        return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
    }

    virtual void draw(const touchgfx::Rect& invalidatedArea) const;
    virtual touchgfx::Rect getSolidRect() const { return touchgfx::Rect(0, 0, getWidth(), getHeight()); }

private:
    Direction dir_;
    uint16_t  colorBg_;
    uint16_t  colorArrow_;

    /* Fuellt ein Rechteck in Widget-Koordinaten, beschnitten auf invalidatedArea */
    void fillLocal(touchgfx::Rect r, uint16_t color, const touchgfx::Rect& invalidatedArea) const;
};

#endif /* NAVARROW_HPP */
