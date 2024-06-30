#include "ui/import_widget.h"

namespace ui
{
    void ImportWidget::show()
    {
        static constexpr int node_flags =
            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
            ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_SpanFullWidth;

        if (ImGui::Begin(_widget_name, 0, _win_flags))
        {
            auto entries = _import_dir.iter();

            if (ImGui::TreeNodeEx("..", node_flags) && ImGui::IsItemClicked()) {    
                _import_dir = _import_dir.back();
            }

            for (const auto &ent : entries) {
                if (const auto *dir = std::get_if<core::io::Directory>(&ent))
                {
                    if (ImGui::TreeNodeEx(dir->path.filename().c_str(), node_flags) && ImGui::IsItemClicked()) {
                        _import_dir = *dir;
                    }
                }
                else if (const auto *file = std::get_if<core::io::File>(&ent))
                {
                    if (ImGui::TreeNodeEx(file->path.filename().c_str(), node_flags) && ImGui::IsItemClicked()) {
                        _workspace.get_active_track().add_clip(file->open());
                    }
                }
            }
        }

        ImGui::End();
    }
}

