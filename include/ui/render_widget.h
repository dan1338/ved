#pragma once

#include "imgui/imgui.h"

#include "core/io.h"
#include "core/workspace.h"
#include "ui/widget_ids.h"
#include "ui/widget.h"

namespace ui
{
    class MainWindow;

    class RenderWidget : public Widget
    {
    public:
        RenderWidget(MainWindow &window);

        void show() override;

    private:
        static constexpr auto *_widget_name = ui::widget_ids::render;
        static constexpr int _win_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        core::Workspace &_workspace;
        bool _opened{false};
    };
}

