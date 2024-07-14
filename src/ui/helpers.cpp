#include "ui/helpers.h"
#include "imgui/imgui.h"

namespace ui
{
    core::timestamp now()
    {
        return core::timestamp_from_double(ImGui::GetTime());
    }
}
