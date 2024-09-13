#include "ui/helpers.h"
#include "core/application.h"
#include <GLFW/glfw3.h>

namespace ui
{
    core::timestamp now()
    {
        return core::timestamp_from_double(glfwGetTime());
    }

    void handle_playback_control(logging::Logger *logger)
    {
        auto &workspace = core::app->get_workspace();

        // Toggle playback
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            LOG_DEBUG(logger, "Space pressed");

            if (workspace.is_preview_active())
                workspace.stop_preview();
            else
                workspace.start_preview();
        }

        // Seek backwards
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            LOG_DEBUG(logger, "Left pressed");

            workspace.decrement_cursor();
        }

        // Seek forward
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            LOG_DEBUG(logger, "Right pressed");

            workspace.increment_cursor();
        }
    }
}
