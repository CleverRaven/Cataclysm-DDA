#pragma once
#ifndef FACTION_CAMP_H
#define FACTION_CAMP_H

#include <string>
#include <vector>

namespace catacurses
{
class window;
} // namespace catacurses
class martialart;
class JsonObject;
class mission;
class time_point;
class npc;
class item;
struct point;
struct tripoint;
struct comp_rank;
struct mission_entry;
class player;
class npc_template;
template<typename T>
class string_id;

enum camp_tab_mode {
    TAB_MAIN,
    TAB_N,
    TAB_NE,
    TAB_E,
    TAB_SE,
    TAB_S,
    TAB_SW,
    TAB_W,
    TAB_NW
};

enum class farm_ops {
    plow = 1,
    plant = 2,
    harvest = 4
};
inline bool operator&( const farm_ops &rhs, const farm_ops &lhs )
{
    return static_cast<int>( rhs ) & static_cast<int>( lhs );
}

namespace talk_function
{
///Changes an NPC follower to a camp manager
void become_overseer( npc & );
///Changes an NPC follower to a camp manager, displays camp warnings, and sets the current OM tile to a camp survey
void start_camp( npc & );
///Changes an NPC follower to a camp manager of an existing camp.
void recover_camp( npc & );
///Changes an NPC camp manager to a follower
void remove_overseer( npc & );

void draw_camp_tabs( const catacurses::window &win, camp_tab_mode cur_tab,
                     std::vector<std::vector<mission_entry>> &entries );
std::string name_mission_tabs( npc &p, const std::string &id, const std::string &cur_title,
                               camp_tab_mode cur_tab );

/// Returns the OM tiles surrounding the camp, @ref purge removes all tiles that aren't expansions
std::vector<std::pair<std::string, tripoint>> om_building_region( npc &p, int range,
        bool purge = false );
/// Converts the camp and expansion points into direction strings, "[NW]"
std::string om_simple_dir( const tripoint &omt_pos, const tripoint &omt_tar );
/// Converts a direction into a point offset
point om_dir_to_offset( const std::string &dir );
}
#endif
