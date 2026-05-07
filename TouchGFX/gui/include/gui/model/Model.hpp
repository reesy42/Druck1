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
	float getPressure_mbar()   const { return pressureMbar_; }
    float getTemperature_degC() const { return tempDegC_; }
    Time getCurrentTime() const
    {
        return currentTime;
    }

protected:
    ModelListener* modelListener;
    //bool mcuLoadActive;
    Time currentTime;
    //uint8_t selectedMenuIndex;
private:
    float pressureMbar_ ;
    float tempDegC_     ;
    uint32_t tickCounter_;
};

#endif // MODEL_HPP
