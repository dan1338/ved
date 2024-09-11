#pragma once

#include <memory>
#include <filesystem>

#include "ui/main_window.h"
#include "core/event.h"
#include "codec/codec.h"

namespace core
{
    namespace fs = std::filesystem;

    struct GLFWwindow;

    class Application
    {
    public:
        Application(fs::path working_dir = "");

        void run();

        ui::MainWindow &get_main_window()
        {
            return *_main_window;
        }

        core::Workspace &get_workspace()
        {
            return *_workspace;
        }

        fs::path get_working_dir() const
        {
            return _working_dir;
        }

        const std::vector<codec::Codec*> &get_available_codecs() const
        {
            return _codecs;
        }

    private:
        static constexpr auto _name{"ved"};

        fs::path _working_dir;

        std::unique_ptr<ui::MainWindow> _main_window;
        std::unique_ptr<core::Workspace> _workspace;
        std::vector<codec::Codec*> _codecs;

        void init_opengl();
        void create_main_window();
    };

    extern std::unique_ptr<Application> app;
}

