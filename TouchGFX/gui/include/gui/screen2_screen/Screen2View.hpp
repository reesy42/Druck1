#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/common/PressureGraph.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

/**
 * Screen2 – Druckverlauf-Diagramm
 *
 * Layout (480 x 272):
 *   y=  0..29  : Header (Titel + Zurück-Button)
 *   y= 30..247 : PressureGraph (480 x 218)
 *   y=248..271 : Statuszeile (Ist/Soll)
 */
class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void onPressureUpdated(float pressure_mmh2o, uint32_t time_ms);

    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);

private:
    static const int16_t SCREEN_W = 480;
    static const int16_t SCREEN_H = 272;
    static const int16_t HDR_H    = 30;
    static const int16_t STATUS_H = 24;
    static const int16_t GRAPH_Y  = HDR_H;
    static const int16_t GRAPH_H  = SCREEN_H - HDR_H - STATUS_H;   // 218

    /* Header */
    touchgfx::Box                     hdrBg;
    touchgfx::TextAreaWithOneWildcard titleText;
    touchgfx::Unicode::UnicodeChar    titleBuf[24];

    /* Zurück-Button */
    touchgfx::Box                     backBtn;
    touchgfx::TextAreaWithOneWildcard backLabel;
    touchgfx::Unicode::UnicodeChar    backLabelBuf[10];

    /* Diagramm */
    PressureGraph graph;

    /* Statuszeile */
    touchgfx::Box                     statusBg;
    touchgfx::TextAreaWithOneWildcard statusText;
    touchgfx::Unicode::UnicodeChar    statusBuf[32];

    /* Y-Achsen-Beschriftung (0, 50, 100, 150, 200, 250, 300 mmH2O) */
    static const int YLABEL_COUNT = 7;
    touchgfx::TextAreaWithOneWildcard yLabel[YLABEL_COUNT];
    touchgfx::Unicode::UnicodeChar    yLabelBuf[YLABEL_COUNT][6];

    float lastSetpoint_;

    touchgfx::TypedText dummyTypedText() const;
    static uint16_t writeStr(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                             const char* str, uint16_t pos = 0);
    /* Konvertiert int zu ASCII in buf, gibt neue pos zurueck */
    static uint16_t writeInt(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                             int value, uint16_t pos = 0);
};

#endif // SCREEN2VIEW_HPP
