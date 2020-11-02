#include "advuilist_helpers.h"

#include <algorithm>     // for __niter_base, max, copy
#include <functional>    // for function
#include <iterator>      // for move_iterator, operator!=, __nite...
#include <list>          // for list
#include <memory>        // for _Destroy, allocator_traits<>::val...
#include <set>           // for set
#include <string>        // for allocator, basic_string, string
#include <unordered_map> // for unordered_map, unordered_map<>::i...
#include <utility>       // for move, get, pair
#include <vector>        // for vector

#include "avatar.h"           // for avatar, get_avatar
#include "character.h"        // for get_player_character, Character
#include "color.h"            // for get_all_colors, color_manager
#include "enums.h"            // for object_type, object_type::VEHICLE
#include "input.h"            // for input_event
#include "item.h"             // for item, iteminfo
#include "item_category.h"    // for item_category
#include "item_contents.h"    // for item_contents
#include "item_location.h"    // for item_location
#include "item_search.h"      // for item_filter_from_string
#include "map.h"              // for get_map, map, map_stack
#include "map_selector.h"     // for map_cursor, map_selector
#include "optional.h"         // for optional
#include "output.h"           // for format_volume, draw_item_info
#include "point.h"            // for tripoint
#include "string_formatter.h" // for string_format
#include "translations.h"     // for _, localized_comparator, localize...
#include "type_id.h"          // for hash, itype_id
#include "units.h"            // for quantity, operator""_kilogram
#include "units_utility.h"    // for convert_weight, volume_units_abbr
#include "vehicle.h"          // for vehicle
#include "vehicle_selector.h" // for vehicle_cursor
#include "vpart_position.h"   // for vpart_reference, optional_vpart_p...

namespace catacurses
{
class window;
} // namespace catacurses

namespace
{
// FIXME: this string is duplicated from draw_item_filter_rules() because that function doesn't fit
// anywhere in the current implementation of advuilist
std::string const desc = string_format(
    "%s\n\n%s\n %s\n\n%s\n %s\n\n%s\n %s", _( "Type part of an item's name to filter it." ),
    _( "Separate multiple items with [<color_yellow>,</color>]." ),
    _( "Example: back,flash,aid, ,band" ),
    _( "To exclude items, place [<color_yellow>-</color>] in front." ),
    _( "Example: -pipe,-chunk,-steel" ),
    _( "Search [<color_yellow>c</color>]ategory, [<color_yellow>m</color>]aterial, "
       "[<color_yellow>q</color>]uality, [<color_yellow>n</color>]otes or "
       "[<color_yellow>d</color>]isassembled components." ),
    _( "Examples: c:food,m:iron,q:hammering,n:toolshelf,d:pipe" ) );

units::mass _iloc_entry_weight( advuilist_helpers::iloc_entry const &it )
{
    units::mass ret = 0_kilogram;
    for( auto const &v : it.stack ) {
        ret += v->weight();
    }
    return ret;
}

units::volume _iloc_entry_volume( advuilist_helpers::iloc_entry const &it )
{
    units::volume ret = 0_liter;
    for( auto const &v : it.stack ) {
        ret += v->volume();
    }
    return ret;
}

using stack_cache_t = std::unordered_map<itype_id, std::set<int>>;
void _get_stacks( item *elem, advuilist_helpers::iloc_stack_t *stacks, stack_cache_t *cache,
                  advuilist_helpers::filoc_t const &iloc_helper )
{
    const auto id = elem->typeId();
    auto iter = cache->find( id );
    bool got_stacked = false;
    if( iter != cache->end() ) {
        for( auto const &idx : iter->second ) {
            got_stacked = ( *stacks )[idx].stack.front()->display_stacked_with( *elem );
            if( got_stacked ) {
                ( *stacks )[idx].stack.emplace_back( iloc_helper( elem ) );
                break;
            }
        }
    }
    if( !got_stacked ) {
        ( *cache )[id].insert( stacks->size() );
        stacks->emplace_back( advuilist_helpers::iloc_entry{ { iloc_helper( elem ) } } );
    }
}

advuilist_helpers::aim_container_t source_ground_player_all()
{
    return advuilist_helpers::source_ground_all( &get_player_character(), 1 );
}

advuilist_helpers::aim_container_t source_player_ground( tripoint const &offset )
{
    Character &u = get_player_character();
    return advuilist_helpers::source_ground( u.pos() + offset );
}

bool source_player_ground_avail( tripoint const &offset )
{
    Character &u = get_player_character();
    return get_map().can_put_items_ter_furn( u.pos() + offset );
}

bool source_player_dragged_avail()
{
    avatar &u = get_avatar();
    if( u.get_grab_type() == object_type::VEHICLE ) {
        tripoint const pos = u.pos() + u.grab_point;
        cata::optional<vpart_reference> vp =
            get_map().veh_at( pos ).part_with_feature( "CARGO", false );
        return vp.has_value();
    }

    return false;
}

advuilist_helpers::aim_container_t source_player_dragged()
{
    avatar &u = get_avatar();
    return advuilist_helpers::source_vehicle( u.pos() + u.grab_point );
}

advuilist_helpers::aim_container_t source_player_inv()
{
    return advuilist_helpers::source_char_inv( &get_player_character() );
}

advuilist_helpers::aim_container_t source_player_worn()
{
    return advuilist_helpers::source_char_worn( &get_player_character() );
}

constexpr tripoint const off_NW = { -1, -1, 0 };
constexpr tripoint const off_N = { 0, -1, 0 };
constexpr tripoint const off_NE = { 1, -1, 0 };
constexpr tripoint const off_W = { -1, 0, 0 };
constexpr tripoint const off_C = { 0, 0, 0 };
constexpr tripoint const off_E = { 1, 0, 0 };
constexpr tripoint const off_SW = { -1, 1, 0 };
constexpr tripoint const off_S = { 0, 1, 0 };
constexpr tripoint const off_SE = { 1, 1, 0 };

} // namespace

namespace advuilist_helpers
{

item_location iloc_map_cursor( map_cursor const &cursor, item *it )
{
    return item_location( cursor, it );
}

item_location iloc_tripoint( tripoint const &loc, item *it )
{
    return iloc_map_cursor( map_cursor( loc ), it );
}

item_location iloc_character( Character *guy, item *it )
{
    return item_location( *guy, it );
}

item_location iloc_vehicle( vehicle_cursor const &cursor, item *it )
{
    return item_location( cursor, it );
}

template <class Iterable>
iloc_stack_t get_stacks( Iterable items, filoc_t const &iloc_helper )
{
    iloc_stack_t stacks;
    stack_cache_t cache;
    for( item &elem : items ) {
        _get_stacks( &elem, &stacks, &cache, iloc_helper );
    }

    return stacks;
}

// all_items_top() returns an Iterable of element pointers unlike map::i_at() and friends (which
// return an Iterable of elements) so we need this specialization and minor code duplication.
// where is c++17 when you need it?
template <>
iloc_stack_t get_stacks<std::list<item *>>( std::list<item *> items, filoc_t const &iloc_helper )
{
    iloc_stack_t stacks;
    stack_cache_t cache;
    for( item *elem : items ) {
        _get_stacks( elem, &stacks, &cache, iloc_helper );
    }

    return stacks;
}

std::size_t iloc_entry_counter( iloc_entry const &it )
{
    return it.stack[0]->count_by_charges() ? it.stack[0]->charges : it.stack.size();
}

std::string iloc_entry_count( iloc_entry const &it )
{
    return std::to_string( iloc_entry_counter( it ) );
}

std::string iloc_entry_weight( iloc_entry const &it )
{
    return string_format( "%3.2f", convert_weight( _iloc_entry_weight( it ) ) );
}

std::string iloc_entry_volume( iloc_entry const &it )
{
    return format_volume( _iloc_entry_volume( it ) );
}

std::string iloc_entry_name( iloc_entry const &it )
{
    item const &i = *it.stack[0];
    return string_format( "<color_%s>%s</color>",
                          get_all_colors().get_name( i.color_in_inventory() ), i.tname() );
}

bool iloc_entry_count_sorter( iloc_entry const &l, iloc_entry const &r )
{
    return iloc_entry_counter( l ) > iloc_entry_counter( r );
}

bool iloc_entry_weight_sorter( iloc_entry const &l, iloc_entry const &r )
{
    return _iloc_entry_weight( l ) > _iloc_entry_weight( r );
}

bool iloc_entry_volume_sorter( iloc_entry const &l, iloc_entry const &r )
{
    return _iloc_entry_volume( l ) > _iloc_entry_volume( r );
}

bool iloc_entry_name_sorter( iloc_entry const &l, iloc_entry const &r )
{
    return localized_compare( l.stack[0]->tname(), r.stack[0]->tname() );
}

std::size_t iloc_entry_gid( iloc_entry const &it )
{
    return it.stack[0]->get_category_shallow().sort_rank();
}

std::string iloc_entry_glabel( iloc_entry const &it )
{
    return it.stack[0]->get_category_shallow().name();
}

bool iloc_entry_filter( iloc_entry const &it, std::string const &filter )
{
    // FIXME: salvage filter caching from old AIM code
    auto const filterf = item_filter_from_string( filter );
    return filterf( *it.stack[0] );
}

void iloc_entry_stats( aim_stats_t *stats, bool first, advuilist_helpers::iloc_entry const &it )
{
    if( first ) {
        stats->first = 0_kilogram;
        stats->second = 0_liter;
    }
    for( auto const &v : it.stack ) {
        stats->first += v->weight();
        stats->second += v->volume();
    }
}

void iloc_entry_stats_printer( aim_stats_t *stats, catacurses::window *w )
{
    right_print( *w, 1, 2, c_white,
                 string_format( "%3.1f %s  %s %s", convert_weight( stats->first ), weight_units(),
                                format_volume( stats->second ), volume_units_abbr() ) );
}

void iloc_entry_examine( catacurses::window *w, iloc_entry const &it )
{
    // FIXME: apparently inventory examine needs special handling
    item const &_it = *it.stack.front();
    std::vector<iteminfo> vThisItem;
    std::vector<iteminfo> vDummy;
    _it.info( true, vThisItem );

    item_info_data data( _it.tname(), _it.type_name(), vThisItem, vDummy );
    data.handle_scrolling = true;

    draw_item_info( *w, data ).get_first_input();
}

aim_container_t source_ground_all( Character *guy, int radius )
{
    aim_container_t itemlist;
    for( map_cursor &cursor : map_selector( guy->pos(), radius ) ) {
        aim_container_t const &stacks =
            get_stacks<>( get_map().i_at( tripoint( cursor ) ),
                          [&]( item *it ) { return iloc_map_cursor( cursor, it ); } );
        itemlist.insert( itemlist.end(), std::make_move_iterator( stacks.begin() ),
                         std::make_move_iterator( stacks.end() ) );
    }
    return itemlist;
}

aim_container_t source_ground( tripoint const &loc )
{
    return get_stacks<>( get_map().i_at( loc ),
                         [&]( item *it ) { return iloc_tripoint( loc, it ); } );
}

aim_container_t source_vehicle( tripoint const &loc )
{
    cata::optional<vpart_reference> vp =
        get_map().veh_at( loc ).part_with_feature( "CARGO", false );

    return get_stacks( vp->vehicle().get_items( vp->part_index() ), [&]( item *it ) {
        return iloc_vehicle( vehicle_cursor( vp->vehicle(), vp->part_index() ), it );
    } );
}

aim_container_t source_char_inv( Character *guy )
{
    aim_container_t ret;
    for( item &worn_item : guy->worn ) {
        aim_container_t temp =
            get_stacks<>( worn_item.contents.all_standard_items_top(),
                          [guy]( item *it ) { return iloc_character( guy, it ); } );
        ret.insert( ret.end(), std::make_move_iterator( temp.begin() ),
                    std::make_move_iterator( temp.end() ) );
    }

    return ret;
}

aim_container_t source_char_worn( Character *guy )
{
    aim_container_t ret;
    for( item &worn_item : guy->worn ) {
        ret.emplace_back( iloc_entry{ { item_location( *guy, &worn_item ) } } );
    }

    return ret;
}

void setup_for_aim( aim_advuilist_t *myadvuilist, aim_stats_t *stats )
{
    using col_t = typename aim_advuilist_t::col_t;
    using sorter_t = typename aim_advuilist_t::sorter_t;
    using grouper_t = typename aim_advuilist_t::grouper_t;
    using filter_t = typename aim_advuilist_t::filter_t;

    myadvuilist->setColumns( std::vector<col_t>{ { "Name", iloc_entry_name, 8.F },
                                                 { "count", iloc_entry_count, 1.F },
                                                 { "weight", iloc_entry_weight, 1.F },
                                                 { "vol", iloc_entry_volume, 1.F } } );
    myadvuilist->setcountingf( iloc_entry_counter );
    // replace lexicographic sorters with numeric ones (where is std::variant when you need it?)
    myadvuilist->addSorter( sorter_t{ "count", iloc_entry_count_sorter } );
    myadvuilist->addSorter( sorter_t{ "weight", iloc_entry_weight_sorter } );
    myadvuilist->addSorter( sorter_t{ "vol", iloc_entry_volume_sorter } );
    // we need to replace name sorter too due to color tags
    myadvuilist->addSorter( sorter_t{ "Name", iloc_entry_name_sorter } );
    // FIXME: this might be better in the ctxt handler of the top transaction_ui so we can show the
    // info on the opposite pane
    catacurses::window *w = myadvuilist->get_window();
    myadvuilist->setexaminef( [w]( iloc_entry const &it ) { iloc_entry_examine( w, it ); } );
    myadvuilist->addGrouper( grouper_t{ "category", iloc_entry_gid, iloc_entry_glabel } );
    myadvuilist->setfilterf( filter_t{ desc, iloc_entry_filter } );
    myadvuilist->on_rebuild(
        [stats]( bool first, iloc_entry const &it ) { iloc_entry_stats( stats, first, it ); } );
    myadvuilist->on_redraw(
        [stats]( catacurses::window *w ) { iloc_entry_stats_printer( stats, w ); } );
}

void add_aim_sources( aim_advuilist_sourced_t *myadvuilist )
{
    using fsource_t = typename aim_advuilist_sourced_t::fsource_t;
    using fsourceb_t = typename aim_advuilist_sourced_t::fsourceb_t;

    fsource_t source_dummy = []() { return aim_container_t(); };
    fsourceb_t _always = []() { return true; };
    fsourceb_t _never = []() { return false; };
    myadvuilist->addSource( { _( SOURCE_NW ),
                              SOURCE_NW_i,
                              { 3, 0 },
                              []() { return source_player_ground( off_NW ); },
                              []() { return source_player_ground_avail( off_NW ); } } );
    myadvuilist->addSource( { _( SOURCE_N ),
                              SOURCE_N_i,
                              { 4, 0 },
                              []() { return source_player_ground( off_N ); },
                              []() { return source_player_ground_avail( off_N ); } } );
    myadvuilist->addSource( { _( SOURCE_NE ),
                              SOURCE_NE_i,
                              { 5, 0 },
                              []() { return source_player_ground( off_NE ); },
                              []() { return source_player_ground_avail( off_NE ); } } );
    myadvuilist->addSource( { _( SOURCE_W ),
                              SOURCE_W_i,
                              { 3, 1 },
                              []() { return source_player_ground( off_W ); },
                              []() { return source_player_ground_avail( off_W ); } } );
    myadvuilist->addSource( { _( SOURCE_CENTER ),
                              SOURCE_CENTER_i,
                              { 4, 1 },
                              []() { return source_player_ground( off_C ); },
                              []() { return source_player_ground_avail( off_C ); } } );
    myadvuilist->addSource( { _( SOURCE_E ),
                              SOURCE_E_i,
                              { 5, 1 },
                              []() { return source_player_ground( off_E ); },
                              []() { return source_player_ground_avail( off_E ); } } );
    myadvuilist->addSource( { _( SOURCE_SW ),
                              SOURCE_SW_i,
                              { 3, 2 },
                              []() { return source_player_ground( off_SW ); },
                              []() { return source_player_ground_avail( off_SW ); } } );
    myadvuilist->addSource( { _( SOURCE_S ),
                              SOURCE_S_i,
                              { 4, 2 },
                              []() { return source_player_ground( off_S ); },
                              []() { return source_player_ground_avail( off_S ); } } );
    myadvuilist->addSource( { _( SOURCE_SE ),
                              SOURCE_SE_i,
                              { 5, 2 },
                              []() { return source_player_ground( off_SE ); },
                              []() { return source_player_ground_avail( off_SE ); } } );
    myadvuilist->addSource( { _( SOURCE_CONT ), SOURCE_CONT_i, { 0, 0 }, source_dummy, _never } );
    myadvuilist->addSource( { _( SOURCE_DRAGGED ),
                              SOURCE_DRAGGED_i,
                              { 1, 0 },
                              source_player_dragged,
                              source_player_dragged_avail } );
    myadvuilist->addSource(
        { _( SOURCE_INV ), SOURCE_INV_i, { 1, 1 }, source_player_inv, _always } );
    myadvuilist->addSource(
        { _( SOURCE_ALL ), SOURCE_ALL_i, { 0, 2 }, source_ground_player_all, _always } );
    myadvuilist->addSource(
        { _( SOURCE_WORN ), SOURCE_WORN_i, { 1, 2 }, source_player_worn, _always } );
}

template iloc_stack_t get_stacks<>( map_stack items, filoc_t const &iloc_helper );
template iloc_stack_t get_stacks<>( vehicle_stack items, filoc_t const &iloc_helper );

} // namespace advuilist_helpers
