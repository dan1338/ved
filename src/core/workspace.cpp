#include "core/workspace.h"

namespace core
{
    Workspace::Workspace(Properties &props):
        _props(props),
        _frame_dt(core::timestamp(1s).count() / props.frame_rate)
    {
        _timeline.add_track(); // Default track
    }

    void Workspace::remove_track(uint32_t idx)
    {
        if (_timeline.tracks.size() > 1) {
            _timeline.tracks.erase(_timeline.tracks.begin() + idx);
            _active_track_idx = std::min((size_t)_active_track_idx, _timeline.tracks.size());
        }
    }
}

