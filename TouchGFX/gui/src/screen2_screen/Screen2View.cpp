#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>

/* -----------------------------------------------------------------------
 * Farben
 * --------------------------------------------------------------------- */
static const uint16_t S2_HDR_BG   = touchgfx::Color::getColorFromRGB( 25,  25,  55);
static const uint16_t S2_TEXT     = touchgfx::Color::getColorFromRGB(200, 200, 220);
static const uint16_t S2_LABEL    = touchgfx::Color::getColorFromRGB(140, 180, 220);
static const uint16_t S2_BTN      = touchgfx::Color::getColorFromRGB( 50,  55,  90);
static const uint16_t S2_BTN_ACT  = touchgfx::Color::getColorFromRGB( 70, 130, 200);
static const uint16_t S2_STATUS   = touchgfx::Color::getColorFromRGB( 15,  15,  40);
static const uint16_t S2_IST      = touchgfx::Color::getColorFromRGB(  0, 220, 120);
static const uint16_t S2_SOLL     = touchgfx::Color::getColorFromRGB(220, 160,  30);
static const uint16_t S2_PRESSURE = touchgfx::Color::getColorFromRGB(  0, 220,  80);  // = PressureGraph::colorLine_
static const uint16_t S2_PWM      = touchgfx::Color::getColorFromRGB(255,  60, 220);  // = PressureGraph::colorPwm_

/* -----------------------------------------------------------------------
 * Hilfsfunktionen
 * --------------------------------------------------------------------- */
uint16_t Screen2View::writeStr(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                               const char* str, uint16_t pos)
{
    while (*str && pos < size - 1)
        buf[pos++] = (touchgfx::Unicode::UnicodeChar)(uint8_t)*str++;
    buf[pos] = 0;
    return pos;
}

uint16_t Screen2View::writeInt(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                               int value, uint16_t pos)
{
    if (value < 0) { if (pos < size-1) buf[pos++] = '-'; value = -value; }
    char rev[8]; int rlen = 0;
    do { rev[rlen++] = '0' + value % 10; value /= 10; } while (value);
    for (int k = rlen-1; k >= 0 && pos < size-1; --k)
        buf[pos++] = (touchgfx::Unicode::UnicodeChar)rev[k];
    buf[pos] = 0;
    return pos;
}

uint16_t Screen2View::writeIntPadded(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                                     int value, uint16_t width, uint16_t pos)
{
    if (value < 0) { if (pos < size-1) buf[pos++] = '-'; value = -value; }
    char rev[8]; int rlen = 0;
    do { rev[rlen++] = '0' + value % 10; value /= 10; } while (value);
    for (int pad = rlen; pad < (int)width && pos < size-1; ++pad)
        buf[pos++] = '0';
    for (int k = rlen-1; k >= 0 && pos < size-1; --k)
        buf[pos++] = (touchgfx::Unicode::UnicodeChar)rev[k];
    buf[pos] = 0;
    return pos;
}

touchgfx::TypedText Screen2View::dummyTypedText() const
{
    return touchgfx::TypedText(T___SINGLEUSE_AQEJ);
}

/* -----------------------------------------------------------------------
 * Konstruktor
 * --------------------------------------------------------------------- */
Screen2View::Screen2View() : lastSetpoint_(50.0f), lastPwm_(0.0f) {}

/* -----------------------------------------------------------------------
 * setupScreen
 * --------------------------------------------------------------------- */
void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();

    /* ---- Header ---- */
    hdrBg.setPosition(0, 0, SCREEN_W, HDR_H);
    hdrBg.setColor(S2_HDR_BG);
    add(hdrBg);

    /* "<"-Button (links im Header) -> Screen1 */
    prevBtn.setPosition(2, 2, 70, HDR_H - 4);
    prevBtn.setDirection(NavArrow::LEFT);
    prevBtn.setBackgroundColor(S2_BTN);
    add(prevBtn);

    /* ">"-Button (rechts im Header) -> Screen3 */
    nextBtn.setPosition(SCREEN_W - 72, 2, 70, HDR_H - 4);
    nextBtn.setDirection(NavArrow::RIGHT);
    nextBtn.setBackgroundColor(S2_BTN);
    add(nextBtn);

    /* Legende – beginnt rechts vom Button: "mmH2O" in Druckfarbe, "PWM%" in PWM-Farbe */
    titleText.setPosition(100, 5, 90, 20);
    titleText.setColor(S2_PRESSURE);
    titleText.setWildcard(titleBuf);
    titleText.setTypedText(dummyTypedText());
    writeStr(titleBuf, 24, "mmH2O");
    add(titleText);

    pwmLegendText.setPosition(200, 5, 70, 20);
    pwmLegendText.setColor(S2_PWM);
    pwmLegendText.setWildcard(pwmLegendBuf);
    pwmLegendText.setTypedText(dummyTypedText());
    writeStr(pwmLegendBuf, 10, "PWM%");
    add(pwmLegendText);

    /* ---- Y-Achsen-Beschriftung ---- */
    /* Druckwerte: 300, 250, 200, 150, 100, 50, 0 mmH2O */
    const int pVals[YLABEL_COUNT] = { 300, 250, 200, 150, 100, 50, 0 };
    for (int i = 0; i < YLABEL_COUNT; ++i){
        /* cy = GRAPH_Y + GRAPH_H - 1 - ((P - PMIN) / (PMAX - PMIN)) * (GRAPH_H - 1) */
        int16_t cy = (int16_t)(GRAPH_Y + GRAPH_H - 1
                     - (int16_t)((pVals[i] / 300.0f) * (GRAPH_H - 1)));
        int16_t ty = (int16_t)(cy - 10);
        if (ty < GRAPH_Y) ty = GRAPH_Y;
        if (ty > SCREEN_H - STATUS_H - 20) ty = SCREEN_H - STATUS_H - 20;

        yLabel[i].setPosition(2, ty, 50, 20);
        yLabel[i].setColor(S2_LABEL);
        yLabel[i].setWildcard(yLabelBuf[i]);
        yLabel[i].setTypedText(dummyTypedText());
        writeInt(yLabelBuf[i], 6, pVals[i]);
        add(yLabel[i]);
    }

    /* ---- Diagramm ---- */
    graph.setPosition(50, GRAPH_Y, SCREEN_W - 50, GRAPH_H);
    add(graph);

    /* ---- Statuszeile ---- */
    statusBg.setPosition(0, SCREEN_H - STATUS_H, SCREEN_W, STATUS_H);
    statusBg.setColor(S2_STATUS);
    add(statusBg);

    statusText.setPosition(4, SCREEN_H - STATUS_H + 2, SCREEN_W - 8, STATUS_H - 4);
    statusText.setColor(S2_IST);
    statusText.setWildcard(statusBuf);
    statusText.setTypedText(dummyTypedText());
    writeStr(statusBuf, 40, "Ist: --- mmH2O  PWM: --- %");
    add(statusText);
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}

/* -----------------------------------------------------------------------
 * onPressureUpdated
 * --------------------------------------------------------------------- */
void Screen2View::onPressureUpdated(float pressure_mmh2o, uint32_t /*time_ms*/)
{
    /* Statuszeile aktualisieren */
    int pi = (int)pressure_mmh2o;
    int pd = (int)((pressure_mmh2o - (float)pi) * 10.0f);
    if (pd < 0) pd = -pd;

    uint16_t pos = writeStr(statusBuf, 40, "Ist:");
    pos = writeIntPadded(statusBuf, 40, pi, 3, pos);
    if (pos < 39) statusBuf[pos++] = '.';
    if (pos < 39) statusBuf[pos++] = '0' + pd;
    pos = writeStr(statusBuf, 40, " mmH2O  PWM:", pos);
    pos = writeIntPadded(statusBuf, 40, (int)lastPwm_, 3, pos);
    pos = writeStr(statusBuf, 40, "%", pos);
    (void)pos;
    statusText.invalidate();

    /* Sample alle 2 Ticks (~33ms) zum Diagramm */
    static uint8_t skip = 0;
    if (++skip >= 2) { skip = 0; graph.addSample(pressure_mmh2o, lastPwm_); }
}

void Screen2View::onSetpointUpdated(float setpoint_mmh2o)
{
    if (setpoint_mmh2o != lastSetpoint_)
    {
        lastSetpoint_ = setpoint_mmh2o;
        graph.setSetpoint(setpoint_mmh2o);
    }
}

/* -----------------------------------------------------------------------
 * handleClickEvent
 * --------------------------------------------------------------------- */
void Screen2View::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    if (evt.getType() != touchgfx::ClickEvent::PRESSED) return;

    /* "<" -> Screen1 */
    if (prevBtn.contains(evt.getX(), evt.getY()))
    {
        prevBtn.setBackgroundColor(S2_BTN_ACT);
        application().gotoScreen1ScreenNoTransition();
        return;
    }

    /* ">" -> Screen3 */
    if (nextBtn.contains(evt.getX(), evt.getY()))
    {
        nextBtn.setBackgroundColor(S2_BTN_ACT);
        application().gotoScreen3ScreenNoTransition();
    }
}
