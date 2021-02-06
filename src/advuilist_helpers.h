#ifndef CATA_SRC_ADVUILIST_HELPERS_H
#define CATA_SRC_ADVUILIST_HELPERS_H

#include <functional> // for function
#include <string>     // for string, allocator
#include <utility>    // for pair
#include <vector>     // for vector

#include "advuilist.h"         // for advuilist
#include "advuilist_sourced.h" // for advuilist_sourced
#include "item_location.h"     // for item_location
#include "transaction_ui.h"    // for transaction_ui
#include "vpart_position.h"    // for vpart_reference
#include "units_fwd.h"         // for mass, volume

class Character;
class item;
class map_cursor;
class vehicle_cursor;
class map_stack;
class vehicle_stack;
struct tripoint;
namespace catacurses
{
class window;
} // namespace catacurses

namespace advuilist_helpers
{
/// entry type for advuilist based on item_location
struct iloc_entry {
    // entries are stacks of items
    using stack_t = std::vector<item_location>;
    stack_t stack;
};

using iloc_stack_t = std::vector<iloc_entry>;
using aim_container_t = std::vector<iloc_entry>;
using aim_advuilist_t = advuilist<aim_container_t, iloc_entry>;
using aim_advuilist_sourced_t = advuilist_sourced<aim_container_t, iloc_entry>;
using aim_transaction_ui_t = transaction_ui<aim_container_t, iloc_entry>;
using aim_stats_t = std::pair<units::mass, units::volume>;

using filoc_t = std::function<item_location( item * )>;
template <class Iterable>
iloc_stack_t get_stacks( Iterable items, filoc_t const &iloc_helper );
iloc_stack_t get_stacks( std::list<item *> const &items, filoc_t const &iloc_helper );

aim_advuilist_t::count_t iloc_entry_counter( iloc_entry const &it );
std::string iloc_entry_count( iloc_entry const &it, int width );
std::string iloc_entry_weight( iloc_entry const &it, int width );
std::string iloc_entry_volume( iloc_entry const &it, int width );
std::string iloc_entry_name( iloc_entry const &it, int width );

bool iloc_entry_count_sorter( iloc_entry const &l, iloc_entry const &r );
bool iloc_entry_weight_sorter( iloc_entry const &l, iloc_entry const &r );
bool iloc_entry_volume_sorter( iloc_entry const &l, iloc_entry const &r );
bool iloc_entry_damage_sorter( iloc_entry const &l, iloc_entry const &r );
bool iloc_entry_spoilage_sorter( iloc_entry const &l, iloc_entry const &r );
bool iloc_entry_price_sorter( iloc_entry const &l, iloc_entry const &r );
bool iloc_entry_name_sorter( iloc_entry const &l, iloc_entry const &r );

bool iloc_entry_gsort( iloc_entry const &l, iloc_entry const &r );
std::string iloc_entry_glabel( iloc_entry const &it );

bool iloc_entry_filter( iloc_entry const &it, std::string const &filter );

void iloc_entry_stats( aim_stats_t *stats, bool first, advuilist_helpers::iloc_entry const &it );
void iloc_entry_examine( catacurses::window *w, iloc_entry const &it );

cata::optional<vpart_reference> veh_cargo_at( tripoint const &loc );
aim_container_t source_ground_all( Character *guy, int radius );
aim_container_t source_ground( tripoint const &loc );
aim_container_t source_vehicle( tripoint const &loc );
aim_container_t source_char_inv( Character *guy );
aim_container_t source_char_worn( Character *guy );

bool source_vehicle_avail( tripoint const &loc );

// for map::i_at()
extern template iloc_stack_t get_stacks<>( map_stack items, filoc_t const &iloc_helper );
// for vehicle::get_items()
extern template iloc_stack_t get_stacks<>( vehicle_stack items, filoc_t const &iloc_helper );

} // namespace advuilist_helpers

#endif // CATA_SRC_ADVUILIST_HELPERS_H
