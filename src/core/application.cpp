#include "core/application.h"
#include <cstdlib>
#include <cstdio>

namespace core
{
    const auto logger = logging::get_logger("Application");

    std::unique_ptr<Application> app;

    static std::string getcwd_string()
    {
        static char buf[256];

        return getcwd(buf, sizeof buf);
    }

    Application::Application(fs::path working_dir):
        _working_dir(working_dir.empty()? fs::path{getcwd_string()} : std::move(working_dir))
    {
        _codecs.push_back(&codec::avc);

        init_opengl();
    }

    void Application::run()
    {
        core::WorkspaceProperties workspace_props{1920, 1080, 30};
        _workspace = std::make_unique<core::Workspace>(workspace_props);

        create_main_window();

        _main_window->run();
    }

    void Application::init_opengl()
    {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("glfwInit");
        }
    }

    void Application::create_main_window()
    {
        int width, height;
        const int opengl_ver[2] = {3, 3};

        GLFWmonitor *monitor = glfwGetPrimaryMonitor();

        if (!monitor) {
            throw std::runtime_error("No primary monitor found, glfwGetPrimaryMonitor -> null");
        }

        if (const char *winsize = std::getenv("VED_WINSIZE"))
        {
            if (std::sscanf(winsize, "%ux%u", &width, &height) != 2)
            {
                throw std::exception();
            }
        }
        else
        {
            int xmon, ymon;
            glfwGetMonitorWorkarea(monitor, &xmon, &ymon, &width, &height);

            LOG_DEBUG(logger, "glfwGetMonitorWorkarea, workarea = ({}, {}, {}, {})", xmon, ymon, width, height);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, opengl_ver[0]);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, opengl_ver[1]);

        auto *window = glfwCreateWindow(width, height, _name, nullptr, nullptr);

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("glewInit");
        }

        // TODO: move somewhere else
        ui::MainWindow::Style style;
        style.normal_bg_color = {0.18431373, 0.19215686, 0.21176471, 1.0};
        style.dark_bg_color = {0.14431373, 0.15215686, 0.18176471, 1.0};

        _main_window = std::make_unique<ui::MainWindow>(window, style);
    }
}
