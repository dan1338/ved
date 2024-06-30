#include "ui/main_window.h"

#include <stdexcept>
#include <unistd.h>

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "fmt/base.h"
#include "fmt/format.h"

#include "logging.h"
#include "shaders.h"

namespace ui
{
    static void set_imgui_style(MainWindow::Style style)
    {
        auto &imgui_style = ImGui::GetStyle();

        imgui_style.Colors[ImGuiCol_Text] = {0.9, 0.9, 0.9, 0.9};
        imgui_style.Colors[ImGuiCol_TextDisabled] = {0.6, 0.6, 0.6, 1.0};
        imgui_style.Colors[ImGuiCol_WindowBg] = style.normal_bg_color;
        imgui_style.Colors[ImGuiCol_TitleBgActive] = style.dark_bg_color;

        auto &io = ImGui::GetIO();

        io.Fonts->AddFontFromFileTTF("./fonts/Roboto-Regular.ttf", 18);
    }

    bool MainWindow::init(int w, int h)
    {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("glfwInit");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

        _window = glfwCreateWindow(w, h, "ved", nullptr, nullptr);

        glfwMakeContextCurrent(_window);
        glfwSwapInterval(1);

        _imgui_ctx = ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(_window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("glewInit");
        }

        return true;
    }

    MainWindow::MainWindow(int init_w, int init_h, Style style):
        _opengl_init(init(init_w, init_h)),
        _import_dir({getcwd(0, 0)}),
        _props({1280, 720, 30}),
        _workspace(_props),
        _timeline_widget(_workspace, _timeline_props),
        _preview_widget(_workspace),
        _import_widget(_workspace, _import_dir)
    {

        set_imgui_style(style);

        _timeline_props.track_height = 80.0f;
        _timeline_props.visible_timespan = 45s;
        _timeline_props.time_offset = 0s;

        const auto bg_color = style.normal_bg_color;
        glClearColor(bg_color.x, bg_color.y, bg_color.y, 1.0);
    }

    void MainWindow::layout_windows()
    {
        auto id = ImGui::GetID("MainWindow");

        const auto vp = ImGui::GetMainViewport();

        ImGui::DockBuilderRemoveNode(id);
        ImGui::DockBuilderAddNode(id);
        ImGui::DockBuilderSetNodePos(id, vp->WorkPos);
        ImGui::DockBuilderSetNodeSize(id, vp->WorkSize);

        auto import_id = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 0.22f, nullptr, &id);
        auto timeline_id = ImGui::DockBuilderSplitNode(id, ImGuiDir_Down, 0.40f, nullptr, &id);

        ImGui::DockBuilderDockWindow("Import files", import_id);
        ImGui::DockBuilderDockWindow("Timeline", timeline_id);
        ImGui::DockBuilderDockWindow("Preview", id);
        ImGui::DockBuilderFinish(id);

        _layout_done = true;
    }

    void MainWindow::run()
    {
        while (!glfwWindowShouldClose(_window)) {
            glfwPollEvents();

            if (glfwGetKey(_window, GLFW_KEY_ESCAPE)) {
                break;
            }

            if (glfwGetKey(_window, GLFW_KEY_LEFT)) {
                _workspace.decrement_cursor();
            }

            if (glfwGetKey(_window, GLFW_KEY_RIGHT)) {
                _workspace.increment_cursor();
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();

            ImGui::NewFrame();

            if (!_layout_done)
                layout_windows();

            double current_time = glfwGetTime();

            _timeline_widget.show(current_time);
            _import_widget.show();
            _preview_widget.show();

            ImGui::Render();

            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(_window);

            _workspace.clean_cursor();
        }
    }
}
