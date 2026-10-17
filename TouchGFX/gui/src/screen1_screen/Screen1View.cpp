#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <main.h>
#include <flash_params.h>

/* -----------------------------------------------------------------------
 * Farben
 * --------------------------------------------------------------------- */
static const uint16_t COL_BG       = touchgfx::Color::getColorFromRGB( 20,  20,  35);
static const uint16_t COL_PANEL    = touchgfx::Color::getColorFromRGB( 30,  30,  50);
static const uint16_t COL_SELECTED = touchgfx::Color::getColorFromRGB(  0, 100, 180);
static const uint16_t COL_TEXT     = touchgfx::Color::getColorFromRGB(220, 220, 220);
static const uint16_t COL_LABEL    = touchgfx::Color::getColorFromRGB(140, 180, 220);
static const uint16_t COL_BTN      = touchgfx::Color::getColorFromRGB( 45,  50,  75);
static const uint16_t COL_BTN_ACT  = touchgfx::Color::getColorFromRGB( 70, 130, 200);
static const uint16_t COL_IST      = touchgfx::Color::getColorFromRGB(  0, 220, 120);
static const uint16_t COL_SOLL     = touchgfx::Color::getColorFromRGB(220, 160,  30);
static const uint16_t COL_RUN      = touchgfx::Color::getColorFromRGB( 20, 140,  60);
static const uint16_t COL_STOPPED  = touchgfx::Color::getColorFromRGB(150,  30,  30);

/* -----------------------------------------------------------------------
 * writeStr: ASCII-String zeichenweise als UnicodeChar schreiben
 * --------------------------------------------------------------------- */
uint16_t Screen1View::writeStr(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                               const char* str, uint16_t pos)
{
    while (*str && pos < size - 1)
        buf[pos++] = (touchgfx::Unicode::UnicodeChar)(uint8_t)*str++;
    buf[pos] = 0;
    return pos;
}

/* buildFixed: schreibt "label:vi.vd" mit 'decimals' Nachkommastellen */
uint16_t Screen1View::buildFixed(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                                  const char* label, int vi, int vd, int decimals)
{
    /* Vorzeichenbehandlung */
    uint16_t pos = writeStr(buf, size, label);
    pos = writeStr(buf, size, ":", pos);

    if (vi < 0) { buf[pos++] = '-'; vi = -vi; }

    /* Ganzzahlanteil */
    char rev[8]; int rlen = 0;
    int tmp = vi;
    do { rev[rlen++] = '0' + tmp % 10; tmp /= 10; } while (tmp);
    for (int k = rlen - 1; k >= 0 && pos < size - 1; --k)
        buf[pos++] = (touchgfx::Unicode::UnicodeChar)rev[k];

    if (decimals > 0 && pos < size - 1)
    {
        buf[pos++] = '.';
        /* Dezimalstellen */
        int div = 1;
        for (int d = 1; d < decimals; ++d) div *= 10;
        for (int d = 0; d < decimals && pos < size - 1; ++d)
        {
            buf[pos++] = (touchgfx::Unicode::UnicodeChar)('0' + (vd / div) % 10);
            div /= 10;
        }
    }
    buf[pos] = 0;
    return pos;
}

/* -----------------------------------------------------------------------
 * Konstruktor
 * --------------------------------------------------------------------- */
Screen1View::Screen1View()
    : activeParam(-1), inputLen(0), fanRunning(g_fan_run != 0U), infoTicks(0)
{
    paramValues[PARAM_KP]    = 2.0f;
    paramValues[PARAM_KI]    = 0.5f;
    paramValues[PARAM_KD]    = 0.1f;
    paramValues[PARAM_PSOLL] = 50.0f;
    inputStr[0] = '\0';
}

touchgfx::TypedText Screen1View::dummyTypedText() const
{
    return touchgfx::TypedText(T___SINGLEUSE_AQEJ);
}

const char* Screen1View::paramLabel(int id)
{
    static const char* names[] = { "KP", "KI", "KD", "Soll" };
    return (id >= 0 && id < PARAM_COUNT) ? names[id] : "";
}

/* -----------------------------------------------------------------------
 * setupScreen
 * --------------------------------------------------------------------- */
void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    /* Gespeicherte Werte aus dem Model laden – überschreibt Konstruktor-Defaults */
    presenter->getPidParams(paramValues[PARAM_KP], paramValues[PARAM_KI],
                            paramValues[PARAM_KD], paramValues[PARAM_PSOLL]);

    DruckLabel.setPosition(0, 0, 0, 0);   // ViewBase-Label verstecken

    /* Hintergrund rechtes Panel ZUERST hinzufuegen – muss hinter allem liegen */
    rightBg.setPosition(RPAD_X, 0, RPAD_W, SCREEN_H);
    rightBg.setColor(touchgfx::Color::getColorFromRGB(10, 10, 30));
    add(rightBg);

    buildParamPanel();
    buildNumpad();
    buildIstSollLines();
    buildNextButton();
    buildSaveButton();
    buildStartStopButton();

    presenter->setPidParams(paramValues[PARAM_KP], paramValues[PARAM_KI],
                            paramValues[PARAM_KD], paramValues[PARAM_PSOLL]);
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

/* -----------------------------------------------------------------------
 * buildParamPanel  (linkes Panel oben)
 * --------------------------------------------------------------------- */
void Screen1View::buildParamPanel()
{
    for (int i = 0; i < PARAM_COUNT; ++i)
    {
        int16_t y = (int16_t)(i * PARAM_H);

        paramBg[i].setPosition(0, y, PANEL_W, PARAM_H - 1);
        paramBg[i].setColor(COL_PANEL);
        add(paramBg[i]);

        paramHighlight[i].setPosition(0, y, 3, PARAM_H - 1);
        paramHighlight[i].setColor(COL_LABEL);
        add(paramHighlight[i]);

        valueText[i].setPosition(5, y + 4, PANEL_W - 8, 22);
        valueText[i].setColor(COL_TEXT);
        valueText[i].setWildcard(valueBuf[i]);
        valueText[i].setTypedText(dummyTypedText());
        refreshValueDisplay(i);
        add(valueText[i]);
    }
}

/* -----------------------------------------------------------------------
 * buildNumpad  (rechtes Panel)
 * --------------------------------------------------------------------- */
void Screen1View::buildNumpad()
{
    const char  chars[]  = { '1','2','3','4','5','6','7','8','9','.','0','B','C','E' };
    const char* labels[] = { "1","2","3","4","5","6","7","8","9",".",
                              "0","BS","CL","OK" };

    for (int i = 0; i < NUM_BTNS; ++i)
    {
        btnChar[i] = chars[i];

        int row = i / 3;
        int col = i % 3;

        int16_t bx, by, bw;

        if (i < 12)
        {
            bx = (int16_t)(RPAD_X + BTN_PADX + col * (BTN_W + BTN_GAP));
            by = (int16_t)(BTN_PADY + row * (BTN_H + BTN_GAP));
            bw = BTN_W;
        }
        else if (i == 12) /* CL */
        {
            bx = (int16_t)(RPAD_X + BTN_PADX);
            by = (int16_t)(BTN_PADY + 4 * (BTN_H + BTN_GAP));
            bw = BTN_W;
        }
        else /* OK */
        {
            bx = (int16_t)(RPAD_X + BTN_PADX + BTN_W + BTN_GAP);
            by = (int16_t)(BTN_PADY + 4 * (BTN_H + BTN_GAP));
            bw = (int16_t)(2 * BTN_W + BTN_GAP);  // doppelt breit
        }

        btnBox[i].setPosition(bx, by, bw, BTN_H);
        btnBox[i].setColor(COL_BTN);
        add(btnBox[i]);

        btnLabel[i].setPosition(bx + 3, by + 2, bw - 6, BTN_H - 4);
        btnLabel[i].setColor(COL_TEXT);
        btnLabel[i].setWildcard(btnLabelBuf[i]);
        btnLabel[i].setTypedText(dummyTypedText());
        writeStr(btnLabelBuf[i], 4, labels[i]);
        add(btnLabel[i]);
    }
}

/* -----------------------------------------------------------------------
 * buildIstSollLines  (linkes Panel unten)
 * --------------------------------------------------------------------- */
void Screen1View::buildIstSollLines()
{
    /* Ist/Soll links im Panel – Breite auf PANEL_W begrenzt, damit
       der Diagramm-Button rechts davon ohne Ueberlappung platziert werden kann */
    istLine.setPosition(0, 198, PANEL_W+70, 24);
    istLine.setColor(COL_IST);
    istLine.setWildcard(istLineBuf);
    istLine.setTypedText(dummyTypedText());
    writeStr(istLineBuf, 20, "Ist:--- mmH2O");
    add(istLine);

    sollLine.setPosition(0, 226, PANEL_W+20, 24);
    sollLine.setColor(COL_SOLL);
    sollLine.setWildcard(sollLineBuf);
    sollLine.setTypedText(dummyTypedText());
    updateSollLine();
    add(sollLine);
}

/* -----------------------------------------------------------------------
 * updateSollLine
 * --------------------------------------------------------------------- */
void Screen1View::updateSollLine()
{
    float v  = paramValues[PARAM_PSOLL];
    int   vi = (int)v;
    int   vd = (int)((v - (float)vi) * 10.0f + 0.5f);
    if (vd < 0) vd = -vd;

    uint16_t pos = buildFixed(sollLineBuf, 20, "Soll", vi, vd, 1);
    writeStr(sollLineBuf, 20, " mmH2O", pos);
    sollLine.invalidate();
}

/* -----------------------------------------------------------------------
 * refreshValueDisplay
 * --------------------------------------------------------------------- */
void Screen1View::refreshValueDisplay(int id)
{
    float v  = paramValues[id];
    int   vi = (int)v;
    int   vd = (int)((v - (float)vi) * 100.0f + 0.5f);
    if (vd < 0) vd = -vd;

    buildFixed(valueBuf[id], VALUE_BUF, paramLabel(id), vi, vd, 2);
    valueText[id].invalidate();
}

/* -----------------------------------------------------------------------
 * selectParam
 * --------------------------------------------------------------------- */
void Screen1View::selectParam(int id)
{
    if (activeParam >= 0){
        paramBg[activeParam].setColor(COL_PANEL);
        paramBg[activeParam].invalidate();
        paramHighlight[activeParam].setColor(COL_LABEL);
        paramHighlight[activeParam].invalidate();
        refreshValueDisplay(activeParam);
    }
    activeParam = id;
    inputStr[0] = '\0';
    inputLen    = 0;
    if (id >= 0){
        paramBg[id].setColor(COL_SELECTED);
        paramBg[id].invalidate();
        paramHighlight[id].setColor(touchgfx::Color::getColorFromRGB(255, 200, 0));
        paramHighlight[id].invalidate();
        uint16_t pos = writeStr(valueBuf[id], VALUE_BUF, paramLabel(id));
        writeStr(valueBuf[id], VALUE_BUF, ":_", pos);
        valueText[id].invalidate();
    }
}

/* -----------------------------------------------------------------------
 * processKey
 * --------------------------------------------------------------------- */
void Screen1View::processKey(char c)
{
    if (activeParam < 0) return;

    auto refresh = [this]() {
        uint16_t pos = writeStr(valueBuf[activeParam], VALUE_BUF, paramLabel(activeParam));
        pos = writeStr(valueBuf[activeParam], VALUE_BUF, ":", pos);
        if (inputLen > 0)
            pos = writeStr(valueBuf[activeParam], VALUE_BUF, inputStr, pos);
        writeStr(valueBuf[activeParam], VALUE_BUF, "_", pos);
        valueText[activeParam].invalidate();
    };

    if (c == 'C'){     //clear;
        inputStr[0] = '\0';
        inputLen    = 0;
        refresh();
    }else if (c == 'B'){     //Backspace
        if (inputLen > 0){
            inputStr[--inputLen] = '\0';
            refresh();
        }
    }else if (c == 'E'){   //enter
        applyInput();
        g_pid_enabled=1;   // neue Parameter können eingelesen werden
    }
    else
    {
        if (inputLen < (int)(sizeof(inputStr) - 1))
        {
            if (c == '.')
            {
                for (int i = 0; i < inputLen; ++i)
                    if (inputStr[i] == '.') return;
            }
            inputStr[inputLen++] = c;
            inputStr[inputLen]   = '\0';
            refresh();
        }
    }
}

/* -----------------------------------------------------------------------
 * applyInput
 * --------------------------------------------------------------------- */
void Screen1View::applyInput()
{
    if (activeParam < 0 || inputLen == 0) return;

    float val = 0.0f; bool afterDot = false; float frac = 0.1f;
    for (int i = 0; i < inputLen; ++i)
    {
        char c = inputStr[i];
        if (c == '.')            { afterDot = true; continue; }
        if (c < '0' || c > '9') continue;
        if (!afterDot) val = val * 10.0f + (float)(c - '0');
        else           { val += (float)(c - '0') * frac; frac *= 0.1f; }
    }

    int saved = activeParam;
    paramValues[saved] = val;

    selectParam(-1);
    for (int i = 0; i < PARAM_COUNT; ++i)
        refreshValueDisplay(i);

    if (saved == PARAM_PSOLL)
        updateSollLine();

    /* Bei gestopptem Luefter sendet der PID-Task DShot 0 (g_fan_run = 0);
       der Sollwert wird trotzdem uebernommen und gilt ab dem naechsten Start. */
    presenter->setPidParams(paramValues[PARAM_KP], paramValues[PARAM_KI],
                            paramValues[PARAM_KD], paramValues[PARAM_PSOLL]);
}

/* -----------------------------------------------------------------------
 * handleClickEvent
 * --------------------------------------------------------------------- */
void Screen1View::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    if (evt.getType() != touchgfx::ClickEvent::PRESSED) return;

    int16_t x = evt.getX();
    int16_t y = evt.getY();

    /* Parametertzeilen (linkes Panel, obere 150px) */
    if (x < PANEL_W && y < PARAM_COUNT * PARAM_H)
    {
        int id = y / PARAM_H;
        if (id >= 0 && id < PARAM_COUNT)
            selectParam(id);
        return;
    }

    /* Speichern-Button */
    {
        touchgfx::Rect r = saveBtn.getRect();
        if (x >= r.x && x < r.x + r.width &&
            y >= r.y && y < r.y + r.height)
        {
            int res = FlashParams_Save(
                paramValues[PARAM_KP],
                paramValues[PARAM_KI],
                paramValues[PARAM_KD],
                paramValues[PARAM_PSOLL]);
            showInfo(res == 0 ? "Gespeichert!" : "Fehler beim Speichern!");
            return;
        }
    }

    /* ">"-Button -> Screen2 */
    if (nextBtn.contains(x, y))
    {
        nextBtn.setBackgroundColor(COL_BTN_ACT);
        application().gotoScreen2ScreenNoTransition();
        return;
    }

    /* Start/Stop-Button */
    {
        touchgfx::Rect r = startStopBtn.getRect();
        if (x >= r.x && x < r.x + r.width &&
            y >= r.y && y < r.y + r.height)
        {
            fanRunning = !fanRunning;

            /* Stop = DShot MOTOR_STOP (Wert 0) statt nur Sollwert 0, siehe PID-Task in main.c */
            g_fan_run = fanRunning ? 1U : 0U;

            updateStartStopButton();
            showInfo(fanRunning ? "Gestartet" : "Gestoppt");
            return;
        }
    }

    /* Nummernblock-Tasten */
    for (int i = 0; i < NUM_BTNS; ++i)
    {
        touchgfx::Rect r = btnBox[i].getRect();
        if (x >= r.x && x < r.x + r.width &&
            y >= r.y && y < r.y + r.height)
        {
            btnBox[i].setColor(COL_BTN_ACT);
            btnBox[i].invalidate();
            processKey(btnChar[i]);
            btnBox[i].setColor(COL_BTN);
            btnBox[i].invalidate();
            return;
        }
    }
}

/* -----------------------------------------------------------------------
 * buildNextButton  ">" -> Screen2 (rechtes Panel, ganz unten)
 * --------------------------------------------------------------------- */
void Screen1View::buildNextButton()
{
    /* Rechts neben dem Speichern-Button (der bei x=230, w=105, also endet bei x=335) */
    const int16_t bx = 340;
    const int16_t by = 206;
    const int16_t bw = SCREEN_W - bx - 5;
    const int16_t bh = 46;

    nextBtn.setPosition(bx, by, bw, bh);
    nextBtn.setDirection(NavArrow::RIGHT);
    nextBtn.setBackgroundColor(touchgfx::Color::getColorFromRGB(40, 80, 140));
    add(nextBtn);
}

/* -----------------------------------------------------------------------
 * buildStartStopButton  (linkes Panel, Luecke unter der Parameterliste)
 * --------------------------------------------------------------------- */
void Screen1View::buildStartStopButton()
{
    const int16_t bx = 15;
    const int16_t by = 134;
    const int16_t bw = PANEL_W - 30;
    const int16_t bh = 44;

    startStopBtn.setPosition(bx, by, bw, bh);
    add(startStopBtn);

    /* Label etwas hoeher als die Zeilenhoehe des Fonts, damit Unterlaengen
       (z.B. das "p" in "Stop") nicht am unteren Rand abgeschnitten werden. */
    const int16_t labelH = 26;
    startStopBtnLabel.setPosition(bx + 2, by + (bh - labelH) / 2, bw - 4, labelH);
    startStopBtnLabel.setWildcard(startStopBtnBuf);
    startStopBtnLabel.setTypedText(dummyTypedText());
    add(startStopBtnLabel);

    updateStartStopButton();
}

/* -----------------------------------------------------------------------
 * updateStartStopButton  — Farbe/Text passend zu fanRunning aktualisieren
 * --------------------------------------------------------------------- */
void Screen1View::updateStartStopButton()
{
    startStopBtn.setColor(fanRunning ? COL_RUN : COL_STOPPED);
    startStopBtn.invalidate();

    startStopBtnLabel.setColor(touchgfx::Color::getColorFromRGB(240, 240, 240));
    writeStr(startStopBtnBuf, 8, fanRunning ? "Stop" : "Start");
    startStopBtnLabel.invalidate();
}

/* -----------------------------------------------------------------------
 * buildSaveButton  (links neben Graph-Button)
 * --------------------------------------------------------------------- */
void Screen1View::buildSaveButton()
{
    const int16_t bx = 230;   /* rechts von der Ist-Anzeige (endet bei x=230) */
    const int16_t by = 206;
    const int16_t bw = 105;
    const int16_t bh = 46;

    saveBtn.setPosition(bx, by, bw, bh);
    saveBtn.setColor(touchgfx::Color::getColorFromRGB(10, 80, 30));
    add(saveBtn);

    saveBtnLabel.setPosition(bx + 2, by + (bh - 20) / 2, bw - 4, 20);
    saveBtnLabel.setColor(touchgfx::Color::getColorFromRGB(100, 255, 140));
    saveBtnLabel.setWildcard(saveBtnBuf);
    saveBtnLabel.setTypedText(dummyTypedText());
    writeStr(saveBtnBuf, 12, "Speichern");
    add(saveBtnLabel);

    /* Info-Zeile am unteren Rand */
    infoBar.setPosition(0, 252, SCREEN_W, 20);
    infoBar.setColor(touchgfx::Color::getColorFromRGB(10, 10, 10));
    add(infoBar);

    infoText.setPosition(4, 253, SCREEN_W - 8, 18);
    infoText.setColor(touchgfx::Color::getColorFromRGB(100, 255, 140));
    infoText.setWildcard(infoBuf);
    infoText.setTypedText(dummyTypedText());
    infoBuf[0] = 0;
    add(infoText);
}

/* -----------------------------------------------------------------------
 * showInfo  — zeigt Meldung in der Info-Zeile fuer ~3s
 * --------------------------------------------------------------------- */
void Screen1View::showInfo(const char* msg)
{
    writeStr(infoBuf, 32, msg);
    infoTicks = 180;  /* ~3s bei ~60 ticks/s */
    infoText.invalidate();
    infoBar.invalidate();
}

/* -----------------------------------------------------------------------
 * onPressureUpdated
 * --------------------------------------------------------------------- */
void Screen1View::onPressureUpdated(float pressure_mmh2o, uint32_t /*time_ms*/)
{
    int pi = (int)pressure_mmh2o;
    int pd = (int)((pressure_mmh2o - (float)pi) * 10.0f);
    if (pd < 0) pd = -pd;

    uint16_t pos = buildFixed(istLineBuf, 20, "Ist", pi, pd, 1);
    writeStr(istLineBuf, 20, " mmH2O", pos);
    istLine.invalidate();

    /* Info-Zeile nach Ablauf ausblenden */
    if (infoTicks > 0 && --infoTicks == 0)
    {
        infoBuf[0] = 0;
        infoText.invalidate();
        infoBar.invalidate();
    }
}
