#include "ui/import_widget.h"

#include "ui/main_window.h"
#include "core/application.h"

namespace ui
{
    ImportWidget::ImportWidget(MainWindow &window):
        Widget(window),
        _current_dir({core::app->get_working_dir()})
    {
    }

    void ImportWidget::show()
    {
        static constexpr int node_flags =
            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
            ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_SpanFullWidth;

        if (ImGui::Begin(_widget_name, 0, _win_flags))
        {
            auto entries = _current_dir.iter();

            if (ImGui::TreeNodeEx("..", node_flags) && ImGui::IsItemClicked()) {    
                _current_dir = _current_dir.back();
            }

            for (const auto &ent : entries) {
                if (const auto *dir = std::get_if<core::io::Directory>(&ent))
                {
                    if (ImGui::TreeNodeEx(dir->path.filename().c_str(), node_flags) && ImGui::IsItemClicked()) {
                        _current_dir = *dir;
                    }
                }
                else if (const auto *file = std::get_if<core::io::File>(&ent))
                {
                    if (ImGui::TreeNodeEx(file->path.filename().c_str(), node_flags) && ImGui::IsItemClicked()) {
                        auto &workspace = core::app->get_workspace();
                        workspace.add_clip(file->open());
                    }
                }
            }
        }

        ImGui::End();
    }
}

