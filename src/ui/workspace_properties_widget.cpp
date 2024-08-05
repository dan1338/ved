#include "ui/workspace_properties_widget.h"

#include "logging.h"
#include "ui/main_window.h"

static auto logger = logging::get_logger("WorkspacePropertiesWidget");

namespace ui
{
    WorkspacePropertiesWidget::WorkspacePropertiesWidget(MainWindow &window):
        Widget(window),
        _workspace(window._workspace)
    {
    }

    void WorkspacePropertiesWidget::show()
    {
        if (!_opened)
        {
            _props = _workspace.get_props();

            _opened = true;
            ImGui::OpenPopup(_widget_name);
        }

        ImGui::SetNextWindowSize({500, 600});
        const auto display_size = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos({display_size.x * 0.5f, display_size.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});

        if (ImGui::BeginPopupModal(_widget_name, 0, _win_flags))
        {
            ImGui::InputInt("Video width", &_props.video_width);
            ImGui::InputInt("Video height", &_props.video_height);
            ImGui::InputInt("Frame rate", &_props.frame_rate);

            if (ImGui::Button("Save"))
            {
                _workspace.set_props(_props);

                _opened = false;
                _window._show_workspace_props = false;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::End();
    }
}

