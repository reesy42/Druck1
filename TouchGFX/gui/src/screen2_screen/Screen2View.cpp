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

touchgfx::TypedText Screen2View::dummyTypedText() const
{
    return touchgfx::TypedText(T___SINGLEUSE_AQEJ);
}

/* -----------------------------------------------------------------------
 * Konstruktor
 * --------------------------------------------------------------------- */
Screen2View::Screen2View() : lastSetpoint_(50.0f) {}

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

    /* Zurueck-Button (links im Header) – breiter fuer vollstaendigen Text */
    backBtn.setPosition(2, 4, 92, 22);
    backBtn.setColor(S2_BTN);
    add(backBtn);

    backLabel.setPosition(4, 5, 88, 20);
    backLabel.setColor(S2_TEXT);
    backLabel.setWildcard(backLabelBuf);
    backLabel.setTypedText(dummyTypedText());
    writeStr(backLabelBuf, 10, "Zurueck");
    add(backLabel);

    /* Titel – beginnt rechts vom Button, kein Sonderzeichen */
    titleText.setPosition(98, 5, 300, 20);
    titleText.setColor(S2_LABEL);
    titleText.setWildcard(titleBuf);
    titleText.setTypedText(dummyTypedText());
    writeStr(titleBuf, 24, "Druckverlauf mmH2O");
    add(titleText);

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
    writeStr(statusBuf, 32, "Ist: --- mmH2O");
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

    uint16_t pos = writeStr(statusBuf, 32, "Ist:");
    pos = writeInt(statusBuf, 32, pi, pos);
    if (pos < 31) statusBuf[pos++] = '.';
    if (pos < 31) statusBuf[pos++] = '0' + pd;
    pos = writeStr(statusBuf, 32, " mmH2O", pos);
    (void)pos;
    statusText.invalidate();

    /* Sample alle 2 Ticks (~33ms) zum Diagramm */
    static uint8_t skip = 0;
    if (++skip >= 2) { skip = 0; graph.addSample(pressure_mmh2o); }
}

/* -----------------------------------------------------------------------
 * handleClickEvent
 * --------------------------------------------------------------------- */
void Screen2View::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    if (evt.getType() != touchgfx::ClickEvent::PRESSED) return;

    /* Klick auf Zurück-Button */
    touchgfx::Rect r = backBtn.getRect();
    if (evt.getX() >= r.x && evt.getX() < r.x + r.width &&
        evt.getY() >= r.y && evt.getY() < r.y + r.height)
    {
        backBtn.setColor(S2_BTN_ACT);
        backBtn.invalidate();
        application().gotoScreen1ScreenNoTransition();
    }
}
