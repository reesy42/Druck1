#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

/**
 * Odem – PID-Druckregler UI
 *
 * Linkes Panel  (0..PANEL_W-1):
 *   4 Parameterzeilen: KP, KI, KD, PSoll
 *   Ist/Soll-Anzeige unten links
 *
 * Rechtes Panel (PANEL_W+2..479):
 *   Groesseres Nummernblock
 */
class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void onPressureUpdated(float pressure_mmh2o, uint32_t time_ms);

    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);

private:
    /* ---- Layout ---- */
    static const int16_t PANEL_W  = 160;
    static const int16_t SCREEN_H = 272;
    static const int16_t SCREEN_W = 480;
    static const int16_t RPAD_X   = PANEL_W + 2;          // 162
    static const int16_t RPAD_W   = SCREEN_W - RPAD_X;    // 318

    static const int16_t PARAM_H  = 30;                   // 5 Zeilen * 30 = 150px

    /* Nummernblock-Dimensionen (im rechten Panel) */
    static const int16_t BTN_W    = 90;
    static const int16_t BTN_H    = 34;
    static const int16_t BTN_GAP  = 3;
    static const int16_t BTN_PADX = 9;                    // linker Abstand im rechten Panel
    static const int16_t BTN_PADY = 6;                    // oberer Abstand

    /* ---- Parameter ---- */
    enum ParamId { PARAM_KP=0, PARAM_KI, PARAM_KD, PARAM_PSOLL, PARAM_COUNT };

    touchgfx::Box                     paramBg[PARAM_COUNT];
    touchgfx::Box                     paramHighlight[PARAM_COUNT];
    touchgfx::TextAreaWithOneWildcard valueText[PARAM_COUNT];

    static const uint16_t VALUE_BUF = 16;
    touchgfx::Unicode::UnicodeChar    valueBuf[PARAM_COUNT][VALUE_BUF];

    float paramValues[PARAM_COUNT];
    int   activeParam;
    char  inputStr[12];
    int   inputLen;

    /* ---- Nummernblock ---- */
    static const int NUM_BTNS = 14;
    touchgfx::Box                     btnBox[NUM_BTNS];
    touchgfx::TextAreaWithOneWildcard btnLabel[NUM_BTNS];
    touchgfx::Unicode::UnicodeChar    btnLabelBuf[NUM_BTNS][4];
    char btnChar[NUM_BTNS];

    /* ---- Rechter Panel-Hintergrund ---- */
    touchgfx::Box                     rightBg;

    /* ---- Navigations-Button -> Screen2 ---- */
    touchgfx::Box                     graphBtn;
    touchgfx::TextAreaWithOneWildcard graphBtnLabel;
    touchgfx::Unicode::UnicodeChar    graphBtnBuf[16];

    /* ---- Speichern-Button ---- */
    touchgfx::Box                     saveBtn;
    touchgfx::TextAreaWithOneWildcard saveBtnLabel;
    touchgfx::Unicode::UnicodeChar    saveBtnBuf[12];

    /* ---- Info-Zeile am unteren Rand (z.B. "Gespeichert!") ---- */
    touchgfx::Box                     infoBar;
    touchgfx::TextAreaWithOneWildcard infoText;
    touchgfx::Unicode::UnicodeChar    infoBuf[32];
    int                               infoTicks;  /* >0 = sichtbar, wird heruntergezaehlt */

    /* ---- Istwert / Sollwert (unten links) ---- */
    touchgfx::TextAreaWithOneWildcard istLine;
    touchgfx::Unicode::UnicodeChar    istLineBuf[20];
    touchgfx::TextAreaWithOneWildcard sollLine;
    touchgfx::Unicode::UnicodeChar    sollLineBuf[20];

    /* ---- Hilfsfunktionen ---- */
    void buildParamPanel();
    void buildNumpad();
    void buildIstSollLines();
    void buildGraphButton();
    void buildSaveButton();
    void showInfo(const char* msg);
    void updateSollLine();
    void refreshValueDisplay(int id);
    void processKey(char c);
    void applyInput();
    void selectParam(int id);

    touchgfx::TypedText dummyTypedText() const;
    static const char*  paramLabel(int id);
    static uint16_t     writeStr(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                                 const char* str, uint16_t pos = 0);
    /* Baut "LABEL:vi.vd" in buf; gibt neue Laenge zurueck */
    static uint16_t     buildFixed(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                                   const char* label, int vi, int vd, int decimals);
};

#endif // SCREEN1VIEW_HPP
