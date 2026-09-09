#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

Screen2Presenter::Screen2Presenter(Screen2View& v) : view(v) {}

void Screen2Presenter::activate()   {}
void Screen2Presenter::deactivate() {}

void Screen2Presenter::pressureUpdated(float pressure_mmh2o, uint32_t time_ms)
{
    view.onPressureUpdated(pressure_mmh2o, time_ms);
}
