#pragma once
#ifndef CATA_SRC_DEBUG_CONSOLE_SNAP_H
#define CATA_SRC_DEBUG_CONSOLE_SNAP_H

#include <functional>
#include <string>

#include "coordinates.h"
#include "type_id.h"

class character_id;

namespace debug_menu::snap
{

// Snapshot factories shared by inline +M buttons and the monitor target
// table. Each function returns a std::function<std::string()> closure that
// produces the snapshot string when called. Closures may run many times
// across game ticks; they must not capture stale references.

// Zero-arg snapshots.
std::function<std::string()> avatar_vitals();
std::function<std::string()> avatar_pos();
std::function<std::string()> avatar_metabolism();
std::function<std::string()> avatar_gi();
std::function<std::string()> avatar_stamina_focus_mana_power();
std::function<std::string()> avatar_morale();
std::function<std::string()> avatar_weight_volume();
std::function<std::string()> avatar_wielded();
std::function<std::string()> avatar_activity();
std::function<std::string()> avatar_hp_per_bp();
std::function<std::string()> world_creature_count();
std::function<std::string()> world_npc_count();
std::function<std::string()> world_timed_event_count();
std::function<std::string()> world_item_wakeup_count();
std::function<std::string()> world_creature_counts_in_bubble();
std::function<std::string()> world_active_mission_count();
std::function<std::string()> world_active_eoc_depth();
std::function<std::string()> world_active_items();
std::function<std::string()> world_weather();
std::function<std::string()> world_calendar();
std::function<std::string()> vehicle_under_avatar();
std::function<std::string()> world_tile_under_feet();
std::function<std::string()> world_items_under_feet();
std::function<std::string()> imgui_fps();

// Parameterized snapshots. Each captures its argument by value into the
// closure so the snap survives mutation of any caller-side temporaries.
std::function<std::string()> skill( skill_id sid );
std::function<std::string()> proficiency_progress( proficiency_id pid );
// Character-targeted variants: re-resolve the creature by id each tick, so a
// monitor can track any avatar or NPC, not only the player.
std::function<std::string()> char_vitals( character_id id );
std::function<std::string()> char_metabolism( character_id id );
std::function<std::string()> char_gi( character_id id );
std::function<std::string()> char_skill( character_id id, skill_id sid );
std::function<std::string()> char_proficiency( character_id id, proficiency_id pid );
std::function<std::string()> item_charges( itype_id iid );
std::function<std::string()> item_count( itype_id iid );
std::function<std::string()> effect_intensity( efftype_id eid );
std::function<std::string()> bionic_state( bionic_id bid );
std::function<std::string()> tile_at_offset( tripoint_rel_ms off );
std::function<std::string()> field_at_offset( tripoint_rel_ms off );
std::function<std::string()> light_at_offset( tripoint_rel_ms off );
std::function<std::string()> tile_at_abs( tripoint_bub_ms p );
std::function<std::string()> field_at_abs( tripoint_bub_ms p );
std::function<std::string()> light_at_abs( tripoint_bub_ms p );
std::function<std::string()> creature_named( std::string name );
std::function<std::string()> npc_distance( std::string name );
std::function<std::string()> faction_stats( faction_id fid );
std::function<std::string()> event_count_this_turn( int event_type_idx,
        std::string ev_name );
std::function<std::string()> global_var( std::string vname );
// May return an empty std::function when expr fails to parse.
std::function<std::string()> math_expr( std::string expr );
std::function<std::string()> eoc_state( effect_on_condition_id eid );
// Used by Nearest-creature monitor: returns nullable closure that captures
// the name of the nearest non-avatar creature at registration time and
// then re-resolves by name every snapshot. Returns empty std::function
// when no candidate exists.
std::function<std::string()> nearest_creature();

} // namespace debug_menu::snap

#endif // CATA_SRC_DEBUG_CONSOLE_SNAP_H
