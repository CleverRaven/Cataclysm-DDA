#pragma once
#ifndef CATA_SRC_CATA3D_BRIDGE_H
#define CATA_SRC_CATA3D_BRIDGE_H

#include <optional>
#include <string>

#include "action.h"\n#include "coordinates.h"

// Lightweight localhost bridge used by the external Cata3D first-person client.
// The bridge never owns game state: CDDA remains the source of truth.
namespace cata3d_bridge {

struct bridge_command {
    action_id action = ACTION_NULL;
    std::optional<tripoint_rel_ms> target_offset;
};

// Poll a pending command from the Cata3D client. Non-blocking.
// Returns the action plus an optional target relative to the player.
std::optional<bridge_command> poll_command();

// Compatibility helper for callers that only need the action.
std::optional<action_id> poll_action();

// Publish the current CDDA world snapshot to the Cata3D client.
// Safe to call every turn; does nothing when the bridge is inactive/unavailable.
void publish_state();

// Tear down sockets on normal process exit.
void shutdown();

} // namespace cata3d_bridge

#endif // CATA_SRC_CATA3D_BRIDGE_H
