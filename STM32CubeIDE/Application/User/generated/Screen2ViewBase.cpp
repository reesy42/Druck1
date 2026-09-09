#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <touchgfx/Color.hpp>

Screen2ViewBase::Screen2ViewBase()
{
    __background.setPosition(0, 0, 480, 272);
    __background.setColor(touchgfx::Color::getColorFromRGB(10, 10, 30));
    add(__background);
}

Screen2ViewBase::~Screen2ViewBase() {}

void Screen2ViewBase::setupScreen() {}
