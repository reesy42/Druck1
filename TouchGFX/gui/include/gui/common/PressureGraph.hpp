#ifndef PRESSUREGRAPH_HPP
#define PRESSUREGRAPH_HPP

#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/Color.hpp>

/**
 * Scrollendes Druckdiagramm (Pist vs Zeit).
 * X-Achse: Zeit in ms (scrollt automatisch nach links).
 * Y-Achse: Druck in mmH2O (0 bis 300).
 *
 * Implementiert als custom Widget mit direktem Pixel-Zeichnen in draw().
 * Anzahl der gespeicherten Samples = Breite des Widgets in Pixeln.
 */
class PressureGraph : public touchgfx::Widget
{
public:
    static const float PMIN;      //    0.0 mmH2O Y-Achse Minimum
    static const float PMAX;      //  300.0 mmH2O Y-Achse Maximum
    static const int   MAX_SAMPLES = 430;

    PressureGraph();

    /** Neuen Messwert hinzufuegen und Widget aktualisieren. */
    void addSample(float pressure_mmh2o);

    /** Alle Samples und Zeitbasis zuruecksetzen. */
    void reset();

    virtual void draw(const touchgfx::Rect& invalidatedArea) const;
    virtual touchgfx::Rect getSolidRect() const { return getRect(); }

private:
    float   samples_[MAX_SAMPLES];
    int16_t head_;      // Index des naechsten freien Platzes (Ringpuffer)
    int16_t count_;     // Anzahl gueltige Samples
    uint32_t timeMs_;   // Gesamtzeit seit Reset in ms

    uint16_t colorBg_;
    uint16_t colorGrid_;
    uint16_t colorLine_;
    uint16_t colorSetpoint_;
    float    setpoint_;

    /* Hilfsfunktionen fuer draw() */
    void drawPixelAt(int16_t x, int16_t y, uint16_t color) const;
    void drawVLine(int16_t x, int16_t y0, int16_t y1, uint16_t color) const;

public:
    void setSetpoint(float sp) { setpoint_ = sp; }
    float getSetpoint() const { return setpoint_; }
    uint32_t getTimeMs() const { return timeMs_; }
};

#endif /* PRESSUREGRAPH_HPP */
