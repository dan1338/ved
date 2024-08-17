#include "core/render_session.h"

namespace core
{
    RenderSession::RenderSession(core::Timeline &timeline, WorkspaceProperties props):
        _composer(timeline, props)
    {
    }
}
