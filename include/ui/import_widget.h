#pragma once

#include "imgui/imgui.h"

#include "core/io.h"
#include "core/workspace.h"
#include "ui/widget_ids.h"
#include "ui/widget.h"

namespace ui
{
    class ImportWidget : public Widget
    {
    public:
        ImportWidget(MainWindow &window);

        void show() override;

    private:
        static constexpr auto *_widget_name = ui::widget_ids::import;
        static constexpr int _win_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        core::io::Directory _current_dir;
    };
}

