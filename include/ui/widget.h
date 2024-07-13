#pragma once

namespace ui
{
    class MainWindow;

    class Widget
    {
    public:
        Widget(MainWindow &window):
            _window(window)
        {
        }

        virtual void show() = 0;

    protected:
        MainWindow &_window;
    };
}
