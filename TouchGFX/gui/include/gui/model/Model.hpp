#ifndef MODEL_HPP
#define MODEL_HPP
#include <gui/model/Time.hpp>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    /* PID-Parameter an Hardware weitergeben und persistent speichern */
    void setPidParams(float kp, float ki, float kd, float setpoint);

    float getPressureMmh2o()    const { return pressureMmh2o_; }
    float getTemperature_degC() const { return tempDegC_; }
    float getPidOutput()        const { return pidOutput_; }
    Time  getCurrentTime()      const { return currentTime; }

    float getPidKp()        const { return pidKp_; }
    float getPidKi()        const { return pidKi_; }
    float getPidKd()        const { return pidKd_; }
    float getPidSetpoint()  const { return pidSetpoint_; }

protected:
    ModelListener* modelListener;
    Time currentTime;

private:
    float pressureMmh2o_;
    float tempDegC_;
    float pidOutput_;
    float pidKp_;
    float pidKi_;
    float pidKd_;
    float pidSetpoint_;
};

#endif // MODEL_HPP
