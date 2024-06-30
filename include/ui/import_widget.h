#pragma once

#include "imgui/imgui.h"

#include "core/io.h"
#include "core/workspace.h"
#include "ui/widget_ids.h"

namespace ui
{
    class ImportWidget
    {
    public:
        ImportWidget(core::Workspace &workspace, core::io::Directory &import_dir):
            _workspace(workspace),
            _import_dir(import_dir)
        {
        }

        void show();

    private:
        static constexpr auto *_widget_name = ui::widget_ids::import;
        static constexpr int _win_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        core::Workspace &_workspace;
        core::io::Directory &_import_dir;
    };
}

