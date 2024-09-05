#include "ui/render_widget.h"

#include "logging.h"
#include "misc/cpp/imgui_stdlib.h"
#include "ui/main_window.h"
#include "core/application.h"

static auto logger = logging::get_logger("RenderWidget");

namespace ui
{
    RenderWidget::RenderWidget(MainWindow &window):
        Widget(window),
        _workspace(core::app->get_workspace())
    {
    }

    void RenderWidget::show()
    {
        if (!_opened)
        {
            _opened = true;
            ImGui::OpenPopup(_widget_name);
        }

        ImGui::SetNextWindowSize({500, 600});
        const auto display_size = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos({display_size.x * 0.5f, display_size.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});

        if (ImGui::BeginPopupModal(_widget_name, 0, _win_flags))
        {
            std::string output_path = "/home/dan/Downloads";

            ImGui::InputText("Output path", &output_path);

            if (ImGui::Button("Start render"))
            {
                _opened = false;
                _window._show_render_widget = false;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::End();
    }
}

