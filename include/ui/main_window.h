#pragma once

#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/glext.h>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "core/io.h"
#include "core/workspace.h"
#include "core/event.h"

#include "ui/timeline_widget.h"
#include "ui/preview_widget.h"
#include "ui/import_widget.h"

namespace ui
{
    class MainWindow
    {
    public:
        struct Style
        {
            ImVec4 normal_bg_color;
            ImVec4 dark_bg_color;
        };

        MainWindow(int init_w, int init_h, Style style);
        bool init(int w, int h);
        void run();

    private:
        GLFWwindow *_window;
        ImGuiContext *_imgui_ctx;
        bool _layout_done{false};
        bool _opengl_init;
        double _frame_delta{1/60.0f};

        core::Event<core::timestamp> _buffer_swapped_event;

        core::io::Directory _import_dir;
        core::Workspace::Properties _props;
        core::Workspace _workspace;

        ui::TimelineWidget::Properties _timeline_props;

        ui::TimelineWidget _timeline_widget;
        ui::ImportWidget _import_widget;
        ui::PreviewWidget _preview_widget;

        void layout_windows();

        friend class TimelineWidget;
        friend class ImportWidget;
        friend class PreviewWidget;
    };
}
