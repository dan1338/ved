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
        const auto &props = _workspace.get_props();

        _settings.output_path = core::app->get_working_dir() / "out.mp4";
        _settings.video.fps = props.frame_rate;
        _settings.video.width = props.video_width;
        _settings.video.height = props.video_height;
        _settings.video.crf = 24;
        _settings.video.bitrate = 4000;
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
            ImGui::InputText("Output path", &_settings.output_path);
            ImGui::InputInt("Crf", &_settings.video.crf);
            ImGui::InputInt("Bitrate (kbps)", &_settings.video.bitrate);

            if (ImGui::Button("Start render"))
            {
                _opened = false;
                _window._show_render_widget = false;

                _workspace.start_render_session(_settings);

                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::End();
    }
}

