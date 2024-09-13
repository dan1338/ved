#include "ui/main_window.h"
#include "ui/helpers.h"
#include "core/time.h"
#include "core/application.h"

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

static auto logger = logging::get_logger("MainWindow");

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

    MainWindow::MainWindow(GLFWwindow *window, Style style):
        _window(window),
        _timeline_widget(*this, _timeline_props),
        _import_widget(*this),
        _preview_widget(*this),
        _workspace_props_widget(*this),
        _render_widget(*this)
    {
        _imgui_ctx = ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(_window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        set_imgui_style(style);

        _timeline_props.track_height = 65.0f;
        _timeline_props.visible_timespan = 45s;
        _timeline_props.time_offset = 0s;

        const auto bg_color = style.normal_bg_color;
        glClearColor(bg_color.x, bg_color.y, bg_color.y, 1.0);
    }

    void MainWindow::layout_windows()
    {
        LOG_DEBUG(logger, "Layout windows");

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
        auto &workspace = core::app->get_workspace();

        while (!glfwWindowShouldClose(_window)) {
            const auto frame_start_time = now();
            _frame_sync_time = 0s;

            glfwPollEvents();

            if (glfwGetKey(_window, GLFW_KEY_ESCAPE)) {
                break;
            }

            LOG_TRACE_L3(logger, "Begin ui frame");

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();

            ImGui::NewFrame();

            if (!_layout_done)
                layout_windows();

            _timeline_widget.show();
            _import_widget.show();
            _preview_widget.show();

            if (_show_workspace_props)
            {
                _workspace_props_widget.show();
            }

            if (_show_render_widget)
            {
                _render_widget.show();
            }

            ImGui::EndPopup();

            ImGui::Render();

            LOG_TRACE_L3(logger, "Draw ui frame");

            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            LOG_TRACE_L3(logger, "End ui frame");

            // Frame sync time is set when a new frame will be shown to the user after this buffer swap
            // We should strive to time it accurately to ensure proper display duration of the previous frame
            if (_frame_sync_time != 0s)
            {
                const auto remaining_time = (_frame_sync_time - now());

                if (remaining_time > 0s)
                {
                    LOG_DEBUG(logger, "Have {}ms remaining frame time, sleeping", remaining_time / 1ms);
                    std::this_thread::sleep_for(remaining_time);
                }
                else if (remaining_time < 0s)
                {
                    LOG_WARNING(logger, "Exceeded frame sync time, remaining time {}ms", remaining_time / 1ms);
                }
            }

            glfwSwapBuffers(_window);
            LOG_TRACE_L2(logger, "Screen buffers swapped");

            const auto tp_now = now();
            _buffer_swapped_event.notify(tp_now);
            _frame_delta = tp_now - frame_start_time;
        }
    }
}
