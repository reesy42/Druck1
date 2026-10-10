#ifndef SCREEN3PRESENTER_HPP
#define SCREEN3PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen3View;

class Screen3Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen3Presenter(Screen3View& v);
    virtual void activate();
    virtual void deactivate();
    virtual void pressureUpdated(float pressure_mmh2o, uint32_t time_ms);

    /* pmax/pmin im Model auf den aktuellen Druck zuruecksetzen */
    void resetMinMax();

    virtual ~Screen3Presenter() {}

private:
    Screen3Presenter();
    Screen3View& view;
};

#endif // SCREEN3PRESENTER_HPP
