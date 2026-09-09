#ifndef SCREEN1PRESENTER_HPP
#define SCREEN1PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen1View;

class Screen1Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen1Presenter(Screen1View& v);

    virtual void activate();
    virtual void deactivate();

    /* PID-Parameter von der View entgegennehmen und an Model weiterleiten */
    void setPidParams(float kp, float ki, float kd, float setpoint);

    /* Gespeicherte PID-Parameter aus dem Model lesen */
    void getPidParams(float& kp, float& ki, float& kd, float& setpoint);

    /* Callback: neuer Druckwert vom Model */
    virtual void pressureUpdated(float pressure_mmh2o, uint32_t time_ms);

    virtual ~Screen1Presenter() {}

private:
    Screen1Presenter();
    Screen1View& view;
};

#endif // SCREEN1PRESENTER_HPP
