#include "ui/workspace_properties_widget.h"

#include "logging.h"
#include "ui/main_window.h"
#include "core/application.h"

static auto logger = logging::get_logger("WorkspacePropertiesWidget");

namespace ui
{
    WorkspacePropertiesWidget::WorkspacePropertiesWidget(MainWindow &window):
        Widget(window)
    {
    }

    void WorkspacePropertiesWidget::show()
    {
        auto &workspace = core::app->get_workspace();

        if (!_opened)
        {
            _props = workspace.get_props();

            _opened = true;
            ImGui::OpenPopup(_widget_name);
        }

        ImGui::SetNextWindowSize({500, 600});
        const auto display_size = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos({display_size.x * 0.5f, display_size.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});

        if (ImGui::BeginPopupModal(_widget_name, 0, _win_flags))
        {
            ImGui::InputInt("Video width", &_props.video.width);
            ImGui::InputInt("Video height", &_props.video.height);
            ImGui::InputInt("Frame rate", &_props.video.fps);

            ImGui::Separator();
            ImGui::Columns(2);

            if (ImGui::Button("Save", {-1, 0}))
            {
                workspace.set_props(_props);
                close();
            }

            ImGui::NextColumn();

            if (ImGui::Button("Close", {-1, 0}))
            {
                close();
            }
        }

        ImGui::End();
    }

    void WorkspacePropertiesWidget::close()
    {
        _opened = false;
        _window._show_workspace_props = false;

        ImGui::CloseCurrentPopup();
    }
}

