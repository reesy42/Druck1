#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>
#include <touchgfx/transitions/NoTransition.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() {}

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

    /* Navigation zu Screen2 – Implementierung in FrontendApplication.cpp */
    void gotoScreen2ScreenNoTransition();

    /* Navigation zu Screen3 (pmax/pmin) – rein user-seitig, kein Designer-Screen */
    void gotoScreen3ScreenNoTransition();

private:
    void gotoScreen2Impl();
    touchgfx::Callback<FrontendApplication> screen2TransCallback;
    void gotoScreen3Impl();
    touchgfx::Callback<FrontendApplication> screen3TransCallback;
};

#endif // FRONTENDAPPLICATION_HPP
