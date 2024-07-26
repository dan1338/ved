#include "core/workspace.h"

namespace core
{
    Workspace::Workspace(WorkspaceProperties props):
        _logger(logging::get_logger("Workspace")),
        _props(std::move(props)),
        _timeline(_props)
    {
        _timeline.clip_added_event.add_callback([this](auto &clip) {
            _force_preview_refresh = true;
        });

        _timeline.clip_moved_event.add_callback([this](auto &clip) {
            _force_preview_refresh = true;
        });

        _timeline.clip_transformed_event.add_callback([this](auto &clip) {
            _force_preview_refresh = true;
        });

        _timeline.track_removed_event.add_callback([this](auto track_idx) {
            if (_active_track_idx > track_idx || _active_track_idx >= _timeline.get_tracks().size())
                --_active_track_idx;
        });

        _timeline.add_track(); // Default track
    }
}

