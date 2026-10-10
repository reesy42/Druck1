#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>

/* -----------------------------------------------------------------------
 * Farben
 * --------------------------------------------------------------------- */
static const uint16_t S3_BG        = touchgfx::Color::getColorFromRGB( 10,  10,  30);
static const uint16_t S3_HDR_BG    = touchgfx::Color::getColorFromRGB( 25,  25,  55);
static const uint16_t S3_TEXT      = touchgfx::Color::getColorFromRGB(200, 200, 220);
static const uint16_t S3_ROW_BG    = touchgfx::Color::getColorFromRGB( 30,  30,  50);
static const uint16_t S3_BTN       = touchgfx::Color::getColorFromRGB( 50,  55,  90);
static const uint16_t S3_BTN_ACT   = touchgfx::Color::getColorFromRGB( 70, 130, 200);
static const uint16_t S3_PMAX      = touchgfx::Color::getColorFromRGB(255,  90,  90);
static const uint16_t S3_PMIN      = touchgfx::Color::getColorFromRGB( 90, 170, 255);
static const uint16_t S3_IST       = touchgfx::Color::getColorFromRGB(  0, 220, 120);
static const uint16_t S3_RESET     = touchgfx::Color::getColorFromRGB(120,  40,  20);
static const uint16_t S3_RESET_ACT = touchgfx::Color::getColorFromRGB(200,  80,  40);

/* -----------------------------------------------------------------------
 * Hilfsfunktionen
 * --------------------------------------------------------------------- */
uint16_t Screen3View::writeStr(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                               const char* str, uint16_t pos)
{
    while (*str && pos < size - 1)
        buf[pos++] = (touchgfx::Unicode::UnicodeChar)(uint8_t)*str++;
    buf[pos] = 0;
    return pos;
}

uint16_t Screen3View::writeInt(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
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

touchgfx::TypedText Screen3View::dummyTypedText() const
{
    return touchgfx::TypedText(T___SINGLEUSE_AQEJ);
}

/* Schreibt "123.4 mmH2O" (mit Vorzeichen) in die Werte-Spalte von 'row' */
void Screen3View::setValue(int row, float value_mmh2o)
{
    uint16_t pos = 0;
    if (value_mmh2o < 0.0f) { pos = writeStr(rowValueBuf[row], VAL_BUF, "-"); value_mmh2o = -value_mmh2o; }
    int tenths = (int)(value_mmh2o * 10.0f + 0.5f);
    pos = writeInt(rowValueBuf[row], VAL_BUF, tenths / 10, pos);
    if (pos < VAL_BUF - 1) rowValueBuf[row][pos++] = '.';
    if (pos < VAL_BUF - 1) rowValueBuf[row][pos++] = '0' + tenths % 10;
    writeStr(rowValueBuf[row], VAL_BUF, " mmH2O", pos);
    rowValue[row].invalidate();
}

/* -----------------------------------------------------------------------
 * Konstruktor
 * --------------------------------------------------------------------- */
Screen3View::Screen3View() : resetHighlighted_(false) {}

/* -----------------------------------------------------------------------
 * setupScreen
 * --------------------------------------------------------------------- */
void Screen3View::setupScreen()
{
    touchgfx::View<Screen3Presenter>::setupScreen();

    background.setPosition(0, 0, SCREEN_W, SCREEN_H);
    background.setColor(S3_BG);
    add(background);

    /* ---- Header ---- */
    hdrBg.setPosition(0, 0, SCREEN_W, HDR_H);
    hdrBg.setColor(S3_HDR_BG);
    add(hdrBg);

    /* "<"-Button -> Screen2 (gleiche Position wie auf Screen2) */
    prevBtn.setPosition(2, 2, 70, HDR_H - 4);
    prevBtn.setDirection(NavArrow::LEFT);
    prevBtn.setBackgroundColor(S3_BTN);
    add(prevBtn);

    titleText.setPosition(100, 5, 200, 20);
    titleText.setColor(S3_TEXT);
    titleText.setWildcard(titleBuf);
    titleText.setTypedText(dummyTypedText());
    writeStr(titleBuf, 16, "Extremwerte");
    add(titleText);

    /* ---- Werte-Zeilen: pmax, pmin, Ist ---- */
    const char*    labels[ROW_COUNT] = { "pmax:", "pmin:", "Ist:" };
    const uint16_t colors[ROW_COUNT] = { S3_PMAX, S3_PMIN, S3_IST };
    for (int i = 0; i < ROW_COUNT; ++i)
    {
        int16_t y = (int16_t)(45 + i * 45);

        rowBg[i].setPosition(60, y, SCREEN_W - 120, 38);
        rowBg[i].setColor(S3_ROW_BG);
        add(rowBg[i]);

        rowLabel[i].setPosition(80, y + 8, 90, 24);
        rowLabel[i].setColor(colors[i]);
        rowLabel[i].setWildcard(rowLabelBuf[i]);
        rowLabel[i].setTypedText(dummyTypedText());
        writeStr(rowLabelBuf[i], 8, labels[i]);
        add(rowLabel[i]);

        rowValue[i].setPosition(180, y + 8, 220, 24);
        rowValue[i].setColor(colors[i]);
        rowValue[i].setWildcard(rowValueBuf[i]);
        rowValue[i].setTypedText(dummyTypedText());
        writeStr(rowValueBuf[i], VAL_BUF, "--- mmH2O");
        add(rowValue[i]);
    }

    /* ---- Reset-Button ---- */
    resetBtn.setPosition(160, 200, 160, 50);
    resetBtn.setColor(S3_RESET);
    add(resetBtn);

    resetLabel.setPosition(160 + 45, 200 + 14, 110, 24);
    resetLabel.setColor(touchgfx::Color::getColorFromRGB(240, 240, 240));
    resetLabel.setWildcard(resetLabelBuf);
    resetLabel.setTypedText(dummyTypedText());
    writeStr(resetLabelBuf, 8, "Reset");
    add(resetLabel);
}

void Screen3View::tearDownScreen()
{
    touchgfx::View<Screen3Presenter>::tearDownScreen();
}

/* -----------------------------------------------------------------------
 * onPressureUpdated
 * --------------------------------------------------------------------- */
void Screen3View::onPressureUpdated(float pressure_mmh2o, float pmin_mmh2o, float pmax_mmh2o)
{
    setValue(0, pmax_mmh2o);
    setValue(1, pmin_mmh2o);
    setValue(2, pressure_mmh2o);

    /* Reset-Button nach Druck wieder in Normalfarbe */
    if (resetHighlighted_)
    {
        resetHighlighted_ = false;
        resetBtn.setColor(S3_RESET);
        resetBtn.invalidate();
    }
}

/* -----------------------------------------------------------------------
 * handleClickEvent
 * --------------------------------------------------------------------- */
void Screen3View::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    if (evt.getType() != touchgfx::ClickEvent::PRESSED) return;

    const int16_t x = evt.getX();
    const int16_t y = evt.getY();

    /* "<" -> Screen2 */
    if (prevBtn.contains(x, y))
    {
        prevBtn.setBackgroundColor(S3_BTN_ACT);
        application().gotoScreen2ScreenNoTransition();
        return;
    }

    /* Reset pmax/pmin */
    touchgfx::Rect r = resetBtn.getRect();
    if (x >= r.x && x < r.x + r.width &&
        y >= r.y && y < r.y + r.height)
    {
        resetBtn.setColor(S3_RESET_ACT);
        resetBtn.invalidate();
        resetHighlighted_ = true;
        presenter->resetMinMax();
    }
}
