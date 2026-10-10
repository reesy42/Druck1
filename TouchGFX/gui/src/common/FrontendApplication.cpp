#include <gui/common/FrontendApplication.hpp>
/* Screen2/3-Typen hier einbinden – NICHT im Header (vermeidet zirkulaere Abhaengigkeit) */
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <gui/common/FrontendHeap.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{
}

void FrontendApplication::gotoScreen2ScreenNoTransition()
{
    screen2TransCallback = touchgfx::Callback<FrontendApplication>(
        this, &FrontendApplication::gotoScreen2Impl);
    pendingScreenTransitionCallback = &screen2TransCallback;
}

void FrontendApplication::gotoScreen2Impl()
{
    touchgfx::makeTransition<Screen2View, Screen2Presenter,
                              touchgfx::NoTransition, Model>(
        &currentScreen, &currentPresenter,
        frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoScreen3ScreenNoTransition()
{
    screen3TransCallback = touchgfx::Callback<FrontendApplication>(
        this, &FrontendApplication::gotoScreen3Impl);
    pendingScreenTransitionCallback = &screen3TransCallback;
}

void FrontendApplication::gotoScreen3Impl()
{
    touchgfx::makeTransition<Screen3View, Screen3Presenter,
                              touchgfx::NoTransition, Model>(
        &currentScreen, &currentPresenter,
        frontendHeap, &currentTransition, &model);
}
