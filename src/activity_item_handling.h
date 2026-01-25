#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "activity_handlers.h"
#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_scale_constants.h"
#include "requirements.h"
#include "type_id.h"

class item_location;
class player_activity;
class vehicle;
class vpart_reference;
class zone_data;
class zone_manager;
enum zone_activity_stage : int;

struct requirement_failure_reasons {
    void convert_requirement_check_result( requirement_check_result req_res );
    bool check_skip_location() const;

    bool no_path = false;
    bool skip_location_no_zone = false;
    bool skip_location_blocking = false;
    bool skip_location_no_skill = false;
    bool skip_location_unknown_activity = false;
    bool skip_location_no_location = false;
    bool skip_location_no_match = false;
    bool skip_location = false;
    bool no_craft_disassembly_location_route_found = false;
};

namespace zone_sorting
{
// the boolean in this pair being true indicates the item is from a vehicle storage space
using zone_items = std::vector<std::pair<item *, bool>>;

struct unload_sort_options {
    bool unload_mods = false;
    bool unload_molle = false;
    bool unload_always = false;
    bool ignore_favorite = false;
    bool unload_all = false;
    bool unload_corpses = false;
    bool unload_sparse_only = false;
    int unload_sparse_threshold = 0;
};

template <typename T>
bool sorter_out_of_bounds( Character &you, const T &act )
{
    const map &here = get_map();
    if( !here.inbounds( you.pos_bub() ) ) {
        // p is implicitly an NPC that has been moved off the map, so reset the activity
        // and unload them
        you.cancel_activity();
        you.assign_activity( act );
        you.set_moves( 0 );
        g->reload_npcs();
        return true;
    }
    return false;
}

//returns true if there's an existing route to destination
bool route_to_destination( Character &you, player_activity &act,
                           const tripoint_bub_ms &dest, zone_activity_stage &stage );
/**
* Returns true if the given item should be skipped while sorting
*/
bool sort_skip_item( Character &you, const item *it,
                     const std::vector<item_location> &other_activity_items,
                     bool ignore_favorite, const tripoint_abs_ms &src );

/**
* Sets sorting options given nearby UNLOAD_ALL zones
* @param use_zone_type - check for zone types other than UNLOAD_ALL
* @param unloading - whether to set unload_always
*/
unload_sort_options set_unload_options( Character &you, const tripoint_abs_ms &src,
                                        bool use_zone_type );

// return items at the given location from vehicle cargo and ground
// TODO: build into map class
zone_items populate_items( const tripoint_bub_ms &src_bub );
// returns whether to ignore the zones at `src`
// if one zone is ignored, all will be
bool ignore_contents( Character &you, const tripoint_abs_ms &src );

// skip tiles in IGNORE zone or with ignore option,
// tiles on fire (to prevent taking out wood off the lit brazier)
// and inaccessible furniture, like filled charcoal kiln
bool ignore_zone_position( Character &you, const tripoint_abs_ms &src, bool ignore_contents );

// of `items` at `src`, do any need to be sorted?
bool has_items_to_sort( Character &you, const tripoint_abs_ms &src,
                        zone_sorting::unload_sort_options zone_unload_options,
                        const std::vector<item_location> &other_activity_items,
                        const zone_sorting::zone_items &items );
bool can_unload( item *it );
void add_item( const std::optional<vpart_reference> &vp, const tripoint_bub_ms &src_bub,
               const item &it );
void remove_item( const std::optional<vpart_reference> &vp, const tripoint_bub_ms &src_bub,
                  item *it );
std::optional<bool> unload_item( Character &you, const tripoint_abs_ms &src,
                                 zone_sorting::unload_sort_options zone_unload_options,
                                 const std::optional<vpart_reference> &vpr_src,
                                 item *it, const std::unordered_set<tripoint_abs_ms> &dest_set,
                                 int &num_processed );
void move_item( Character &you, const std::optional<vpart_reference> &vpr_src,
                const tripoint_bub_ms &src_bub, const std::unordered_set<tripoint_abs_ms> &dest_set,
                item &it, int &num_processed );
} //namespace zone_sorting


namespace multi_activity_actor
{

//TODO: move to JSON flag?
bool can_do_in_dark( const activity_id &act_id );
bool activity_must_be_in_zone( activity_id act_id, const tripoint_bub_ms &src_loc );
// whether the given activity reason needs to check activity requirements
bool activity_reason_continue( do_activity_reason reason );
// the activity reason has no activity requirements
bool activity_reason_quit( do_activity_reason reason );
// the activity (reason) requires picking up tools
bool activity_reason_picks_up_tools( do_activity_reason reason );
// assigns fetch activity to find requirements
requirement_check_result fetch_requirements( Character &you, requirement_id what_we_need,
        const activity_id &act_id,
        activity_reason_info &act_info, const tripoint_abs_ms &src, const tripoint_bub_ms &src_loc,
        const std::unordered_set<tripoint_abs_ms> &src_set );
// converts invalid do_activity_reason into failed requirement_check_result
requirement_check_result requirement_fail( Character &you, const do_activity_reason &reason,
        const activity_id &act_id, const zone_data *zone );
// hashes and caches custom requirement data if new
requirement_id hash_and_cache_requirement_data( const requirement_data &reqs_data );
// creates a custom requirement
requirement_id synthesize_requirements(
    const requirement_data::alter_item_comp_vector &requirement_comp_vector,
    const requirement_data::alter_quali_req_vector &quality_comp_vector,
    const requirement_data::alter_tool_comp_vector &tool_comp_vector );
// eliminates satisfied requirements from tools/qualities, keeps items
requirement_id remove_met_requirements( requirement_id base_req_id, Character &you );

/* begin TODO: move to activity actor */

std::optional<requirement_id> construction_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms & );
std::optional<requirement_id> vehicle_work_requirements( Character &you,
        activity_reason_info &act_info, const tripoint_bub_ms &src_loc );
std::optional<requirement_id> mining_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms & );
std::optional<requirement_id> farm_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data *zone );
std::optional<requirement_id> chop_planks_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms & );
std::optional<requirement_id> chop_trees_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms & );
std::optional<requirement_id> butcher_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms & );
std::optional<requirement_id> fish_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms & );
std::optional<requirement_id> craft_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms & );
std::optional<requirement_id> disassemble_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms & );

//general function for non-specific multi activities
std::unordered_set<tripoint_abs_ms> generic_locations( Character &you, const activity_id &act_id );
std::unordered_set<tripoint_abs_ms> construction_locations( Character &you,
        const activity_id &act_id );
std::unordered_set<tripoint_abs_ms> tidy_up_locations( Character &you, const activity_id & );
std::unordered_set<tripoint_abs_ms> read_locations( Character &you, const activity_id &act_id );
std::unordered_set<tripoint_abs_ms> study_locations( Character &you, const activity_id &act_id );
std::unordered_set<tripoint_abs_ms> craft_locations( Character &you, const activity_id &act_id );
std::unordered_set<tripoint_abs_ms> fetch_locations( Character &you, const activity_id & );
std::unordered_set<tripoint_abs_ms> fish_locations( Character &you, const activity_id &act_id );
std::unordered_set<tripoint_abs_ms> mop_locations( Character &you, const activity_id &act_id );
void prune_same_tile_locations( Character &you, std::unordered_set<tripoint_abs_ms> &src_set );
void prune_dark_locations( Character &you, std::unordered_set<tripoint_abs_ms> &src_set,
                           const activity_id &act_id );
void prune_dangerous_field_locations( std::unordered_set<tripoint_abs_ms> &src_set );
std::unordered_set<tripoint_abs_ms> no_same_tile_locations( Character &you,
        const activity_id &act_id );

//shared helper function for deconstruction/repair
activity_reason_info vehicle_work_can_do( const activity_id &, Character &you,
        const tripoint_bub_ms &src_loc, std::vector<int> &already_working_indexes,
        vehicle *veh );
activity_reason_info vehicle_deconstruction_can_do( const activity_id &act, Character &you,
        const tripoint_bub_ms &src_loc );
activity_reason_info vehicle_repair_can_do( const activity_id &act, Character &you,
        const tripoint_bub_ms &src_loc );
activity_reason_info mine_can_do( const activity_id &, Character &you,
                                  const tripoint_bub_ms &src_loc );
activity_reason_info mop_can_do( const activity_id &, Character &you,
                                 const tripoint_bub_ms &src_loc );
activity_reason_info fish_can_do( const activity_id &act, Character &you,
                                  const tripoint_bub_ms &src_loc );
activity_reason_info chop_trees_can_do( const activity_id &act, Character &you,
                                        const tripoint_bub_ms &src_loc );
activity_reason_info butcher_can_do( const activity_id &, Character &you,
                                     const tripoint_bub_ms &src_loc );
activity_reason_info read_can_do( const activity_id &, Character &you,
                                  const tripoint_bub_ms & );
activity_reason_info study_can_do( const activity_id &, Character &you,
                                   const tripoint_bub_ms &src_loc );
activity_reason_info chop_planks_can_do( const activity_id &, Character &you,
        const tripoint_bub_ms &src_loc );
activity_reason_info tidy_up_can_do( const activity_id &, Character &you,
                                     const tripoint_bub_ms &src_loc, int distance = MAX_VIEW_DISTANCE );
activity_reason_info construction_can_do( const activity_id &, Character &you,
        const tripoint_bub_ms &src_loc );
activity_reason_info farm_can_do( const activity_id &, Character &you,
                                  const tripoint_bub_ms &src_loc );
activity_reason_info fetch_can_do( const activity_id &, Character &,
                                   const tripoint_bub_ms & );
activity_reason_info craft_can_do( const activity_id &, Character &you,
                                   const tripoint_bub_ms &src_loc );
activity_reason_info disassemble_can_do( const activity_id &, Character &you,
        const tripoint_bub_ms &src_loc );

bool mine_do( Character &you, const activity_reason_info &act_info,
              const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool mop_do( Character &you, const activity_reason_info &act_info,
             const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool fish_do( Character &you, const activity_reason_info &act_info,
              const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool chop_trees_do( Character &you, const activity_reason_info &act_info,
                    const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool butcher_do( Character &you, const activity_reason_info &act_info,
                 const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool read_do( Character &you, const activity_reason_info &act_info,
              const tripoint_abs_ms &, const tripoint_bub_ms & );
bool study_do( Character &you, const activity_reason_info &act_info,
               const tripoint_abs_ms &src, const tripoint_bub_ms & );
bool chop_planks_do( Character &you, const activity_reason_info &act_info,
                     const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool tidy_up_do( Character &you, const activity_reason_info &act_info,
                 const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool construction_do( Character &you, const activity_reason_info &act_info,
                      const tripoint_abs_ms &src, const tripoint_bub_ms &src_loc );
bool farm_do( Character &you, const activity_reason_info &act_info,
              const tripoint_abs_ms &src, const tripoint_bub_ms &src_loc );
bool fetch_do( Character &you, const activity_reason_info &act_info,
               const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool craft_do( Character &you, const activity_reason_info &act_info,
               const tripoint_abs_ms &, const tripoint_bub_ms & );
bool disassemble_do( Character &you, const activity_reason_info &act_info,
                     const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool vehicle_deconstruction_do( Character &you, const activity_reason_info &act_info,
                                const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );
bool vehicle_repair_do( Character &you, const activity_reason_info &act_info,
                        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc );

/* end TODO: move to activity actor */

void revert_npc_post_activity( Character &you, activity_id act_id, bool no_locations );
bool out_of_moves( Character &you, activity_id act_id );
std::optional<bool> route( Character &you, player_activity &act, const tripoint_bub_ms &src_bub,
                           requirement_failure_reasons &fail_reason, bool check_only );
void activity_failure_message( Character &you, activity_id new_activity,
                               const requirement_failure_reasons &fail_reason, bool no_locations );

zone_type_id get_zone_for_act( const tripoint_bub_ms &src_loc, const zone_manager &mgr,
                               const activity_id &act_id, const faction_id &fac_id );
void add_basecamp_storage_to_loot_zone_list(
    zone_manager &mgr, const tripoint_bub_ms &src_loc, Character &you,
    std::vector<tripoint_bub_ms> &loot_zone_spots, std::vector<tripoint_bub_ms> &combined_spots );
bool are_requirements_nearby(
    const std::vector<tripoint_bub_ms> &loot_spots, const requirement_id &needed_things,
    Character &you, const activity_id &activity_to_restore, bool in_loot_zones,
    const tripoint_bub_ms &src_loc );

} //namespace multi_activity_actor
