#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "coords_fwd.h"
#include "item.h"

class Character;
class item_location;
class player_activity;
class vpart_reference;
enum zone_activity_stage : int;

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

bool sorter_out_of_bounds( Character &you );

//returns true if successfully routed to destination
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
