#include "ui/helpers.h"
#include <GLFW/glfw3.h>

namespace ui
{
    core::timestamp now()
    {
        return core::timestamp_from_double(glfwGetTime());
    }
}
