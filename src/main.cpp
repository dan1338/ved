#include "core/application.h"
#include "core/time.h"
#include "logging.h"

int main(int argc, char **argv)
{
    logging::init();

    if (argc == 2)
    {
        core::app = std::make_unique<core::Application>(argv[argc - 1]);
    }
    else
    {
        core::app = std::make_unique<core::Application>();
    }

    core::app->run();

    return 0;
}

