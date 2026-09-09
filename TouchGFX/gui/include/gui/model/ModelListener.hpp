#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }
    virtual void timeUpdated(Time time) {}
    virtual void pressureUpdated(float pressure_mmh2o, uint32_t time_ms) {}
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
