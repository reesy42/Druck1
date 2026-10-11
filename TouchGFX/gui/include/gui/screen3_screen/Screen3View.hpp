#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui/common/FrontendApplication.hpp>
#include <mvp/View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <gui/common/NavArrow.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

/**
 * Screen3 – Druck-Extremwerte (pmax / pmin) mit Reset-Button
 *
 * Bewusst OHNE generierte ViewBase (direkt von touchgfx::View abgeleitet),
 * damit der TouchGFX Designer beim Generieren nichts davon loeschen kann.
 *
 * Layout (480 x 272):
 *   y=  0..29  : Header ("<"-Button + Titel)
 *   y= 36..197 : links pmax / pmin / Ist, rechts ESC: Rot / t / U / I
 *   y=208..253 : Reset-Button
 */
class Screen3View : public touchgfx::View<Screen3Presenter>
{
public:
    Screen3View();
    virtual ~Screen3View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void onPressureUpdated(float pressure_mmh2o, float pmin_mmh2o, float pmax_mmh2o);
    /* ESC-Telemetrie; -1 (tempC: -128) = keine gueltige Telemetrie */
    void onEscUpdated(int32_t rpm, int32_t tempC, int32_t voltage_cV, int32_t current_cA);

    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);

protected:
    FrontendApplication& application()
    {
        return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
    }

private:
    static const int16_t SCREEN_W = 480;
    static const int16_t SCREEN_H = 272;
    static const int16_t HDR_H    = 30;

    enum Row { ROW_PMAX = 0, ROW_PMIN, ROW_IST, ROW_RPM, ROW_TEMP, ROW_VOLT, ROW_CURR, ROW_COUNT };
    static const uint16_t VAL_BUF = 20;

    touchgfx::Box                     background;

    /* Header */
    touchgfx::Box                     hdrBg;
    NavArrow                          prevBtn;
    touchgfx::TextAreaWithOneWildcard titleText;
    touchgfx::Unicode::UnicodeChar    titleBuf[16];

    /* Werte-Zeilen */
    touchgfx::Box                     rowBg[ROW_COUNT];
    touchgfx::TextAreaWithOneWildcard rowLabel[ROW_COUNT];
    touchgfx::Unicode::UnicodeChar    rowLabelBuf[ROW_COUNT][12];
    touchgfx::TextAreaWithOneWildcard rowValue[ROW_COUNT];
    touchgfx::Unicode::UnicodeChar    rowValueBuf[ROW_COUNT][VAL_BUF];

    /* Reset-Button */
    touchgfx::Box                     resetBtn;
    touchgfx::TextAreaWithOneWildcard resetLabel;
    touchgfx::Unicode::UnicodeChar    resetLabelBuf[8];
    bool                              resetHighlighted_;  /* Button nach Druck kurz hervorgehoben */
    int32_t                           lastRpm_;           /* nur bei Aenderung neu zeichnen */
    int32_t                           lastTempC_;
    int32_t                           lastVoltage_cV_;
    int32_t                           lastCurrent_cA_;

    void setValue(int row, float value_mmh2o);
    void setFixed2(int row, int32_t hundredths, const char* unit);   /* "12.34 V", <0 -> "---" */

    touchgfx::TypedText dummyTypedText() const;
    static uint16_t writeStr(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                             const char* str, uint16_t pos = 0);
    static uint16_t writeInt(touchgfx::Unicode::UnicodeChar* buf, uint16_t size,
                             int value, uint16_t pos = 0);
};

#endif // SCREEN3VIEW_HPP
