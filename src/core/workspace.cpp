#include "core/workspace.h"

namespace core
{
    Workspace::Workspace(WorkspaceProperties props):
        _logger(logging::get_logger("Workspace")),
        _props(std::move(props)),
        _timeline(_props)
    {
        _timeline.track_modified_event.add_callback([this](auto track_id) {
            _force_preview_refresh = true;
        });

        _timeline.track_removed_event.add_callback([this](auto removed_id) {
            if (_active_track_id == removed_id)
            {
                _timeline.foreach_track([this, removed_id](auto &track){
                    // Find the next track with smaller id
                    _active_track_id = track.id;

                    // Stop if we're past the removed track
                    if (_active_track_id > removed_id)
                        return false;

                    return true;
                });

                _force_preview_refresh = true;
            }
        });

        _timeline.duration_changed_event.add_callback([this](auto ts) {
            if (_cursor > ts)
                set_cursor(_timeline.get_duration());
        });

        const auto &default_track = _timeline.add_track(); // Default track
        _active_track_id = default_track.id;
    }
}

