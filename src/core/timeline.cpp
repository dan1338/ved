#include "core/timeline.h"

namespace core
{
    std::pair<core::timestamp, core::timestamp> Timeline::Track::bounds() const
    {
        core::timestamp low{0}, high{0};

        for (const auto &clip : clips)
        {
            low = std::min(low, clip.position);
            high = std::max(high, clip.end_position());
        }

        return {low, high};
    }

    // TODO: replace with sorted container
    std::optional<size_t> Timeline::Track::clip_at(core::timestamp position)
    {
        for (size_t i = 0; i < clips.size(); i++)
        {
            const auto &clip = clips[i];

            if (position >= clip.position && position <= (clip.position + clip.duration))
            {
                return i;
            }
        }

        return {};
    }

    Timeline::Clip &Timeline::Track::add_clip(core::MediaFile file, core::timestamp position)
    {
        Clip clip{timeline->_clip_id_counter++, id, position, 0s, file.duration, file};

        if (file.type == MediaFile::STATIC_IMAGE)
        {
            clip.duration = 1s;
        }

        clip.transforms.emplace(ClipTransform{});

        clips.push_back(clip);
        timeline->clip_added_event.notify(clip);

        return clips.back();
    }

    void Timeline::Track::move_clip(Clip &clip, core::timestamp new_position)
    {
        clip.position = core::align_timestamp(new_position, timeline->_props.frame_dt());
        timeline->clip_moved_event.notify(clip);
    }

    void Timeline::Track::split_clip(Clip &clip, core::timestamp split_position)
    {
        const auto lhs_duration = split_position - clip.position;
        const auto rhs_duration = clip.duration - lhs_duration;

        auto &new_clip = add_clip(clip.file, split_position);
        new_clip.duration = rhs_duration;

        clip.duration = lhs_duration;

        timeline->clip_resized_event.notify(clip);
    }

    void Timeline::Track::translate_clip(Clip &clip, float dx, float dy)
    {
        auto &xform = const_cast<ClipTransform&>(*clip.transforms.begin());
        xform.translate_x += dx;
        xform.translate_y += dy;

        timeline->clip_transformed_event.notify(clip);
    }

    void Timeline::Track::scale_clip(Clip &clip, float dx, float dy)
    {
        auto &xform = const_cast<ClipTransform&>(*clip.transforms.begin());
        xform.scale_x += dx;
        xform.scale_y += dy;

        timeline->clip_transformed_event.notify(clip);
    }

    void Timeline::Track::rotate_clip(Clip &clip, float dr)
    {
        auto &xform = const_cast<ClipTransform&>(*clip.transforms.begin());
        xform.rotation += dr;

        timeline->clip_transformed_event.notify(clip);
    }

    Timeline::Timeline(WorkspaceProperties &props):
        _props(props)
    {
        // Setup aggregate event notifications
        clip_added_event.add_callback([this](auto &clip){
            track_modified_event.notify(clip.track_id);
        });

        clip_moved_event.add_callback([this](auto &clip){
            track_modified_event.notify(clip.track_id);
        });

        clip_resized_event.add_callback([this](auto &clip){
            track_modified_event.notify(clip.track_id);
        });

        clip_transformed_event.add_callback([this](auto &clip){
            track_modified_event.notify(clip.track_id);
        });
    }
}

