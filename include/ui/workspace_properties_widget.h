#pragma once

#include "core/workspace_properties.h"
#include "imgui/imgui.h"

#include "core/io.h"
#include "core/workspace.h"
#include "ui/widget_ids.h"
#include "ui/widget.h"

namespace ui
{
    class WorkspacePropertiesWidget : public Widget
    {
    public:
        WorkspacePropertiesWidget(MainWindow &window);

        void show() override;

    private:
        static constexpr auto *_widget_name = ui::widget_ids::workspace_properties;
        static constexpr int _win_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        core::WorkspaceProperties _props;
        bool _opened{false};
    };
}

