#include <gui/common/PressureGraph.hpp>
#include <touchgfx/hal/HAL.hpp>

/* Abtastintervall 40ms entspricht ~25 Samples/Sekunde.
 * Bei 300 Samples sieht man die letzten 12 Sekunden. */
static const uint32_t SAMPLE_INTERVAL_MS = 40u;

const float PressureGraph::PMIN =    0.0f;
const float PressureGraph::PMAX =  300.0f;
const float PressureGraph::PWMIN =   0.0f;
const float PressureGraph::PWMAX = 100.0f;

PressureGraph::PressureGraph()
    : head_(0), count_(0), timeMs_(0),
      colorBg_(touchgfx::Color::getColorFromRGB(10, 10, 40)),
      colorGrid_(touchgfx::Color::getColorFromRGB(40, 40, 80)),
      colorLine_(touchgfx::Color::getColorFromRGB(0, 220, 80)),
      colorPwm_(touchgfx::Color::getColorFromRGB(255, 60, 220)),
      colorSetpoint_(touchgfx::Color::getColorFromRGB(220, 80, 0)),
      setpoint_(50.0f)
{
    for (int i = 0; i < MAX_SAMPLES; ++i)
    {
        samples_[i]    = 0.0f;
        pwmSamples_[i] = 0.0f;
    }
}

void PressureGraph::addSample(float pressure_mmh2o, float pwm_percent)
{
    samples_[head_]    = pressure_mmh2o;
    pwmSamples_[head_] = pwm_percent;
    head_ = (head_ + 1) % MAX_SAMPLES;
    if (count_ < MAX_SAMPLES)
        ++count_;
    timeMs_ += SAMPLE_INTERVAL_MS;
    invalidate();
}

void PressureGraph::reset()
{
    head_   = 0;
    count_  = 0;
    timeMs_ = 0;
    for (int i = 0; i < MAX_SAMPLES; ++i)
    {
        samples_[i]    = 0.0f;
        pwmSamples_[i] = 0.0f;
    }
    invalidate();
}

/* Zeichnet einen einzelnen Pixel im absoluten Displaykoordinatensystem. */
void PressureGraph::drawPixelAt(int16_t x, int16_t y, uint16_t color) const
{
    touchgfx::Rect r(x, y, 1, 1);
    touchgfx::HAL::lcd().fillRect(r, color, 255);
}

/* Zeichnet eine vertikale Linie von y0 bis y1. */
void PressureGraph::drawVLine(int16_t x, int16_t y0, int16_t y1, uint16_t color) const
{
    if (y0 > y1) { int16_t tmp = y0; y0 = y1; y1 = tmp; }
    for (int16_t y = y0; y <= y1; ++y)
        drawPixelAt(x, y, color);
}

void PressureGraph::draw(const touchgfx::Rect& /*invalidatedArea*/) const
{
    const int16_t W = getWidth();
    const int16_t H = getHeight();
    const int16_t ax = getX();   // absolute X-Position
    const int16_t ay = getY();   // absolute Y-Position

    /* --- Hintergrund --- */
    {
        touchgfx::Rect bg(ax, ay, W, H);
        touchgfx::HAL::lcd().fillRect(bg, colorBg_, 255);
    }

    /* --- Gitternetzlinien bei 50, 100, 150, 200, 250 mmH2O --- */
    {
        const float gridVals[] = { 50.0f, 100.0f, 150.0f, 200.0f, 250.0f };
        const float range = PMAX - PMIN;
        for (int g = 0; g < 5; ++g)
        {
            int16_t gy = ay + H - 1 - (int16_t)(((gridVals[g] - PMIN) / range) * (H - 1));
            touchgfx::Rect gridLine(ax, gy, W, 1);
            touchgfx::HAL::lcd().fillRect(gridLine, colorGrid_, 255);
        }
    }

    /* --- Sollwert-Linie (orange, gestrichelt) --- */
    {
        int16_t sy = ay + H - 1 - (int16_t)(((setpoint_ - PMIN) / (PMAX - PMIN)) * (H - 1));
        if (sy >= ay && sy < ay + H)
        {
            for (int16_t x = ax; x < ax + W; x += 6)
            {
                touchgfx::Rect dash(x, sy, 4, 1);
                touchgfx::HAL::lcd().fillRect(dash, colorSetpoint_, 255);
            }
        }
    }

    /* --- Messwert-Kurve --- */
    if (count_ < 2) return;

    /* Neueste Samples stehen am Ende des Ringpuffers.
     * Wir zeichnen rechts die juengsten Samples. */
    int16_t numDraw = (count_ < W) ? count_ : W;

    /* Index des aeltesten anzuzeigenden Samples */
    int   startIdx = (head_ - numDraw + MAX_SAMPLES) % MAX_SAMPLES;
    float prevP    = samples_[startIdx];
    int16_t prevY  = ay + H - 1 - (int16_t)(((prevP - PMIN) / (PMAX - PMIN)) * (H - 1));
    if (prevY < ay)       prevY = ay;
    if (prevY >= ay + H)  prevY = ay + H - 1;

    int16_t xBase = ax + (W - numDraw);  // linker Startpixel der Kurve

    /* --- PWM-Kurve (0-100%, auf volle Grafikhoehe skaliert) --- */
    float   prevPwm  = pwmSamples_[startIdx];
    int16_t prevPwmY = ay + H - 1 - (int16_t)(((prevPwm - PWMIN) / (PWMAX - PWMIN)) * (H - 1));
    if (prevPwmY < ay)      prevPwmY = ay;
    if (prevPwmY >= ay + H) prevPwmY = ay + H - 1;

    for (int16_t i = 1; i < numDraw; ++i)
    {
        int   idx    = (startIdx + i) % MAX_SAMPLES;
        float pwm    = pwmSamples_[idx];
        int16_t cy   = ay + H - 1 - (int16_t)(((pwm - PWMIN) / (PWMAX - PWMIN)) * (H - 1));
        if (cy < ay)      cy = ay;
        if (cy >= ay + H) cy = ay + H - 1;

        int16_t cx = xBase + i;
        drawVLine(cx, prevPwmY, cy, colorPwm_);
        prevPwmY = cy;
    }

    /* --- Messwert-Kurve (Ist-Druck), zuletzt gezeichnet damit sie oben liegt --- */
    for (int16_t i = 1; i < numDraw; ++i)
    {
        int   idx  = (startIdx + i) % MAX_SAMPLES;
        float p    = samples_[idx];
        int16_t cy = ay + H - 1 - (int16_t)(((p - PMIN) / (PMAX - PMIN)) * (H - 1));
        if (cy < ay)      cy = ay;
        if (cy >= ay + H) cy = ay + H - 1;

        int16_t cx = xBase + i;
        drawVLine(cx, prevY, cy, colorLine_);
        prevY = cy;
    }
}
