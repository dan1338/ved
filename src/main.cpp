#include "core/time.h"
#include "ui/main_window.h"
#include "logging.h"

int main()
{
    logging::init();

    ui::MainWindow::Style style;
    style.normal_bg_color = {0.18431373, 0.19215686, 0.21176471, 1.0};
    style.dark_bg_color = {0.14431373, 0.15215686, 0.18176471, 1.0};

    ui::MainWindow window(1920, 1440, style);

    window.run();

    return 0;
}

