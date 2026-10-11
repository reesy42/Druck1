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

    /* Druck-Extremwerte seit Start bzw. letztem Reset (laufen screen-unabhaengig mit) */
    float getPressureMax() const { return pressureMax_; }
    float getPressureMin() const { return pressureMin_; }
    void  resetPressureMinMax()  { pressureMinMaxValid_ = false; }

    /* Aus der ESC-Telemetrie (KISS, esc_telem.c): Drehzahl in U/min (-1 = ungueltig),
     * ESC-Temperatur in Grad C (-128 = ungueltig) */
    int32_t getFanRpm() const { return fanRpm_; }
    int32_t getEscTempC() const { return escTempC_; }

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
    float pressureMax_;
    float pressureMin_;
    bool  pressureMinMaxValid_;   /* false -> beim naechsten tick() mit aktuellem Druck neu starten */
    int32_t fanRpm_;
    int32_t escTempC_;
};

#endif // MODEL_HPP
