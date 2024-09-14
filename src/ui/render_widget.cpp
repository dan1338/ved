#include "ui/render_widget.h"

#include <optional>

#include "core/application.h"
#include "fmt/format.h"
#include "logging.h"
#include "misc/cpp/imgui_stdlib.h"
#include "ui/main_window.h"

static auto logger = logging::get_logger("RenderWidget");

namespace ui
{
    RenderWidget::RenderWidget(MainWindow &window):
        Widget(window),
        _workspace(core::app->get_workspace())
    {
        const auto &props = _workspace.get_props();

        _settings.output_path = core::app->get_working_dir() / "out.mp4";
        _settings.video.codec = nullptr;
        _settings.video.codec_params.clear();
        _settings.video.width = props.video.width;
        _settings.video.height = props.video.height;
        _settings.video.fps = props.video.fps;
        _settings.video.crf = 24;
        _settings.video.bitrate = 4000;
    }

    template<typename Items, typename FGetName>
    static void input_list(const std::string &label, Items items, FGetName getname, typename Items::value_type *item)
    {
        if (ImGui::BeginCombo(label.c_str(), (*item)? getname(*item) : nullptr))
        {
            for (size_t i = 0; i < items.size(); i++)
            {
                if (ImGui::Selectable(getname(items[i])))
                {
                    *item = items[i];
                }
            }

            ImGui::EndCombo();
        }
    }

    template<typename Items, typename FGetName>
    static void input_list(const std::string &label, Items items, FGetName getname, std::optional<typename Items::value_type> &item)
    {
        if (ImGui::BeginCombo(label.c_str(), item.has_value()? getname(*item) : nullptr))
        {
            for (size_t i = 0; i < items.size(); i++)
            {
                if (ImGui::Selectable(getname(items[i])))
                {
                    item.emplace(items[i]);
                }
            }

            ImGui::EndCombo();
        }
    }

    void RenderWidget::show()
    {
        if (!_opened)
        {
            _opened = true;
            ImGui::OpenPopup(_widget_name);

            const auto &props = _workspace.get_props();
            _settings.video.width = props.video.width;
            _settings.video.height = props.video.height;
            _settings.video.fps = props.video.fps;
        }

        ImGui::SetNextWindowSize({500, 600});
        const auto display_size = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos({display_size.x * 0.5f, display_size.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});

        if (ImGui::BeginPopupModal(_widget_name, 0, _win_flags))
        {
            ImGui::InputText("Output path", &_settings.output_path);
            ImGui::InputInt("Crf", &_settings.video.crf);
            ImGui::InputInt("Bitrate (kbps)", &_settings.video.bitrate, 100);

            const auto codecs = core::app->get_available_codecs();
            input_list("Codec", codecs, [](auto codec){ return codec->name.c_str(); }, &_settings.video.codec);

            if (_settings.video.codec)
            {
                ImGui::SeparatorText(fmt::format("{} params", _settings.video.codec->name).c_str());
                auto &params = _settings.video.codec_params;

                for (const auto &[name, values] : _settings.video.codec->params)
                {
                    std::optional<std::string> value{};

                    if (const auto &it = params.find(name); it != params.end())
                        value.emplace(it->second);

                    input_list(name, values, [](const auto &value){ return value.c_str(); }, value);

                    if (value.has_value())
                    {
                        params[name] = *value;

                        const std::string label{fmt::format("X##{}", name)};
                        constexpr auto btn_width = 30;
                        ImGui::SameLine(ImGui::GetWindowWidth() - btn_width - 10);

                        if (ImGui::Button(label.c_str(), {btn_width, 0}))
                        {
                            params.erase(name);
                        }
                    }
                }
            }

            ImGui::Separator();
            ImGui::Columns(2);

            if (ImGui::Button("Start render", {-1, 0}))
            {
                _workspace.start_render_session(_settings);
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

    void RenderWidget::close()
    {
        _opened = false;
        _window._show_render_widget = false;

        ImGui::CloseCurrentPopup();
    }
}

