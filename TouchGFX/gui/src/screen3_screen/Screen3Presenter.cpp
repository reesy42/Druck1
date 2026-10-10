#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>

Screen3Presenter::Screen3Presenter(Screen3View& v) : view(v) {}

void Screen3Presenter::activate()   {}
void Screen3Presenter::deactivate() {}

void Screen3Presenter::pressureUpdated(float pressure_mmh2o, uint32_t /*time_ms*/)
{
    view.onPressureUpdated(pressure_mmh2o, model->getPressureMin(), model->getPressureMax());
}

void Screen3Presenter::resetMinMax()
{
    model->resetPressureMinMax();
}
