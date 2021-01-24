#include "advanced_inv.h"

#include <algorithm> // for max
#include <array>     // for array
#include <memory>    // for unique_ptr
#include <string>    // for operator==, basic_string, string
#include <tuple>     // for tie
#include <utility>   // for move
#include <vector>    // for vector

#include "activity_actor_definitions.h"  // for drop_or_stash_item_info, dro...
#include "advuilist.h"                   // for advuilist<>::groupcont_t
#include "advuilist_helpers.h"           // for aim_advuilist_sourced_t, aim...
#include "advuilist_sourced.h"           // for advuilist_sourced
#include "auto_pickup.h"                 // for get_auto_pickup, player_sett...
#include "avatar.h"                      // for get_avatar, avatar
#include "game.h"                        // for game, g, game::LEFT_OF_INFO
#include "map.h"                         // for get_map, map
#include "options.h"                     // for get_option
#include "output.h"                      // for format_volume, right_print
#include "panels.h"                      // for panel_manager
#include "point.h"                       // for tripoint, point, tripoint_zero
#include "activity_type.h"               // for activity_id
#include "advuilist_const.h"             // for ACTION_CYCLE_SOURCES, ACTION...
#include "color.h"                       // for colorize, color_manager, c_l...
#include "enums.h"                       // for object_type, object_type::VE...
#include "input.h"                       // for input_context
#include "item.h"                        // for item
#include "item_location.h"               // for item_location
#include "optional.h"                    // for optional, nullopt
#include "player_activity.h"             // for player_activity
#include "string_formatter.h"            // for string_format
#include "translations.h"                // for _
#include "ui.h"                          // for uilist, MENU_AUTOASSIGN
#include "ui_manager.h"                  // for ui_adaptor
#include "vpart_position.h"              // for vpart_reference
#include "transaction_ui.h"              // for transaction_ui, transaction_...
#include "uistate.h"                     // for uistate, uistatedata
#include "units.h"                       // for volume, operator""_liter, mass
#include "units_utility.h"               // for convert_weight, volume_units...
#include "veh_type.h"                    // for vpart_info
#include "vehicle.h"                     // for vehicle


static const activity_id ACT_WEAR( "ACT_WEAR" );
static const activity_id ACT_ADV_INVENTORY( "ACT_ADV_INVENTORY" );

namespace
{
using namespace advuilist_helpers;

constexpr std::size_t const aim_nsources = 18;
using pane_mutex_t = std::array<bool, aim_nsources * 2>;
constexpr point const aimlayout{ 6, 3 };

constexpr char const SOURCE_DRAGGED_i = 'D';
constexpr char const SOURCE_VEHICLE_i = 'V';
constexpr char const *ACTION_EXAMINE = "EXAMINE";
constexpr char const *ACTION_ITEMS_DEFAULT = "ITEMS_DEFAULT";
constexpr char const *ACTION_SAVE_DEFAULT = "SAVE_DEFAULT";
constexpr char const *TOGGLE_AUTO_PICKUP = "TOGGLE_AUTO_PICKUP";
constexpr char const *TOGGLE_FAVORITE = "TOGGLE_FAVORITE";

#ifdef __clang__
#pragma clang diagnostic push
// travis' old clang wants a change that breaks compilation with newer versions
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

// Cataclysm: Hacky Stuff Ahead
// this is actually an attempt to make the code more readable and reduce duplication
// is_ground_source, label, direction abbreviation, icon, offset
using _sourcetuple =
    std::tuple<bool, char const *, char const *, aim_advuilist_sourced_t::icon_t, tripoint>;
constexpr std::size_t const _tuple_label_idx = 1;
constexpr std::size_t const _tuple_abrev_idx = 2;
using _sourcearray = std::array<_sourcetuple, aim_nsources>;
constexpr char const *_error = "error";
constexpr _sourcearray const aimsources = {
    _sourcetuple{ false, "Container", _error, 'C', tripoint_zero },
    _sourcetuple{ false, "Grabbed Vehicle", _error, SOURCE_DRAGGED_i, tripoint_zero },
    _sourcetuple{ false, _error, _error, 0, tripoint_zero },
    _sourcetuple{ true, "North West", "NW", '7', tripoint_north_west },
    _sourcetuple{ true, "North", "N", '8', tripoint_north },
    _sourcetuple{ true, "North East", "NE", '9', tripoint_north_east },
    _sourcetuple{ false, _error, _error, 0, tripoint_zero },
    _sourcetuple{ false, "Inventory", _error, 'I', tripoint_zero },
    _sourcetuple{ false, _error, _error, 0, tripoint_zero },
    _sourcetuple{ true, "West", "W", '4', tripoint_west },
    _sourcetuple{ true, "Directly below you", "DN", '5', tripoint_zero },
    _sourcetuple{ true, "East", "E", '6', tripoint_east },
    _sourcetuple{ false, "Surrounding area", _error, 'A', tripoint_zero },
    _sourcetuple{ false, "Worn Items", _error, 'W', tripoint_zero },
    _sourcetuple{ false, _error, _error, 0, tripoint_zero },
    _sourcetuple{ true, "South West", "SW", '1', tripoint_south_west },
    _sourcetuple{ true, "South", "S", '2', tripoint_south },
    _sourcetuple{ true, "South East", "SE", '3', tripoint_south_east },
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

constexpr _sourcearray::size_type const CONT_IDX = 0;
constexpr _sourcearray::size_type const DRAGGED_IDX = 1;
constexpr _sourcearray::size_type const INV_IDX = 7;
constexpr _sourcearray::size_type const ALL_IDX = 12;
constexpr _sourcearray::size_type const WORN_IDX = 13;
// this could be a constexpr too if we didn't have to use old compilers
tripoint slotidx_to_offset( aim_advuilist_sourced_t::slotidx_t idx )
{
    if( idx == DRAGGED_IDX ) {
        return get_avatar().grab_point;
    }

    return std::get<tripoint>( aimsources[idx] );
}

// this could be constexpr in C++20
_sourcearray::size_type offset_to_slotidx( tripoint const &off )
{
    _sourcearray::const_iterator const it =
    std::find_if( aimsources.begin(), aimsources.end(), [&]( _sourcetuple const & v ) {
        return std::get<bool>( v ) and std::get<tripoint>( v ) == off;
    } );
    return std::distance( aimsources.begin(), it );
}

pane_mutex_t::size_type idxtovehidx( pane_mutex_t::size_type idx )
{
    return idx + aim_nsources;
}

constexpr bool is_vehicle( aim_advuilist_sourced_t::icon_t icon )
{
    return icon == SOURCE_DRAGGED_i or icon == SOURCE_VEHICLE_i;
}

bool source_player_dragged_avail()
{
    avatar &u = get_avatar();
    if( u.get_grab_type() == object_type::VEHICLE ) {
        return source_vehicle_avail( u.pos() + u.grab_point );
    }

    return false;
}

std::string aim_sourcelabel( _sourcearray::size_type idx, bool veh = false )
{
    _sourcetuple const &src = aimsources[idx];
    std::string const &srcname = std::get<_tuple_label_idx>( src );

    tripoint const pos = get_avatar().pos() + slotidx_to_offset( idx );
    std::string prefix = srcname;
    std::string label;

    if( ( veh and source_vehicle_avail( pos ) ) or
        ( idx == DRAGGED_IDX and source_player_dragged_avail() ) ) {
        cata::optional<vpart_reference> vp = veh_cargo_at( pos );
        prefix = vp->vehicle().name;
        label = vp->get_label().value_or( vp->info().name() );
    } else {
        if( std::get<bool>( src ) ) {
            label = get_map().name( pos );
        }
    }

    return string_format( "%s\n%s", colorize( prefix, c_green ), colorize( label, c_light_blue ) );
}
// end hacky stuff

aim_container_t source_player_ground( tripoint const &offset )
{
    return source_ground( get_avatar().pos() + offset );
}

bool source_player_ground_avail( tripoint const &offset )
{
    return get_map().can_put_items_ter_furn( get_avatar().pos() + offset );
}

aim_container_t source_player_vehicle( tripoint const &offset )
{
    return source_vehicle( get_avatar().pos() + offset );
}

bool source_player_vehicle_avail( tripoint const &offset )
{
    return source_vehicle_avail( get_avatar().pos() + offset );
}

aim_container_t source_player_dragged()
{
    avatar &u = get_avatar();
    return source_vehicle( u.pos() + u.grab_point );
}

aim_container_t source_player_inv()
{
    return source_char_inv( &get_avatar() );
}

aim_container_t source_player_worn()
{
    return source_char_worn( &get_avatar() );
}

aim_container_t source_player_all( aim_advuilist_sourced_t *ui, pane_mutex_t *mutex )
{
    // due to operations order in advuilist_sourced, we need to reset the (previous) location mutex
    // this fixes a bug when switching from a ground or vehicle source to All in the same pane
    using slotidx_t = aim_advuilist_sourced_t::slotidx_t;
    using icon_t = aim_advuilist_sourced_t::icon_t;
    slotidx_t slotidx = 0;
    icon_t icon = 0;
    std::tie( slotidx, icon ) = ui->getSource();
    slotidx = is_vehicle( icon ) ? idxtovehidx( slotidx ) : slotidx;
    ( *mutex )[slotidx] = false;

    aim_container_t itemlist;
    pane_mutex_t::size_type idx = 0;
    for( auto const &v : aimsources ) {
        // only consider entries with is_ground_source = true and not mutex'ed
        if( std::get<bool>( v ) ) {
            tripoint const off = std::get<tripoint>( v );
            if( source_player_ground_avail( off ) and !mutex->at( idx ) ) {
                aim_container_t const &stacks = source_player_ground( off );
                itemlist.insert( itemlist.end(), std::make_move_iterator( stacks.begin() ),
                                 std::make_move_iterator( stacks.end() ) );
            }
            if( source_player_vehicle_avail( off ) and !mutex->at( idxtovehidx( idx ) ) ) {
                aim_container_t const &stacks = source_player_vehicle( off );
                itemlist.insert( itemlist.end(), std::make_move_iterator( stacks.begin() ),
                                 std::make_move_iterator( stacks.end() ) );
            }
        }
        idx++;
    }

    return itemlist;
}

void set_mutex( pane_mutex_t *mutex, aim_advuilist_sourced_t::slotidx_t slot,
                aim_advuilist_sourced_t::icon_t icon, bool val )
{
    mutex->at( icon == SOURCE_VEHICLE_i ? idxtovehidx( slot ) : slot ) = val;
    // dragged vehicle needs more checks...
    if( source_player_dragged_avail() ) {
        tripoint const off = get_avatar().grab_point;
        if( slot == DRAGGED_IDX ) {
            pane_mutex_t::size_type const idx = offset_to_slotidx( off );
            mutex->at( idxtovehidx( idx ) ) = val;
        }
        if( ( off == slotidx_to_offset( slot ) and is_vehicle( icon ) ) ) {
            mutex->at( DRAGGED_IDX ) = val;
        }
    }
}

void reset_mutex( pane_mutex_t *mutex )
{
    mutex->fill( false );
}

void reset_mutex( aim_transaction_ui_t *ui, pane_mutex_t *mutex )
{
    using slotidx_t = aim_advuilist_sourced_t::slotidx_t;
    using icon_t = aim_advuilist_sourced_t::icon_t;
    slotidx_t lsrc = 0;
    slotidx_t rsrc = 0;
    icon_t licon = 0;
    icon_t ricon = 0;

    std::tie( lsrc, licon ) = ui->left()->getSource();
    std::tie( rsrc, ricon ) = ui->right()->getSource();
    reset_mutex( mutex );
    set_mutex( mutex, lsrc, licon, true );
    set_mutex( mutex, rsrc, ricon, true );
}

std::string iloc_entry_src( iloc_entry const &it, int /* width */ )
{
    tripoint const off = it.stack.front().position() - get_avatar().pos();
    _sourcearray::size_type idx = offset_to_slotidx( off );
    return std::get<_tuple_abrev_idx>( aimsources[idx] );
}

std::pair<point, point> aim_size( bool full_screen )
{
    int const min_w_width = FULL_SCREEN_WIDTH;
    int const max_w_width = full_screen ? TERMX : std::max( 120,
                            TERMX - 2 * ( panel_manager::get_manager().get_width_right() +
                                          panel_manager::get_manager().get_width_left() ) );

    int const width = TERMX < min_w_width
                      ? min_w_width
                      : TERMX > max_w_width ? max_w_width : TERMX;
    int const originx = TERMX > width ? ( TERMX - width ) / 2 : 0;

    return { point{width, TERMY}, point{originx, 0} };
}

void aim_inv_idv_stats( aim_advuilist_sourced_t *ui )
{
    using select_t = aim_transaction_ui_t::select_t;
    select_t const peek = ui->peek();
    catacurses::window &w = *ui->get_window();
    avatar &u = get_avatar();

    if( !peek.empty() ) {
        iloc_entry &entry = *std::get<aim_advuilist_t::ptr_t>( peek.front() );
        double const peek_len = convert_length_cm_in( entry.stack[0]->length() );
        double const indiv_len_cap = convert_length_cm_in( u.max_single_item_length() );
        std::string const peek_len_str = colorize(
                                             string_format( "%.1f", peek_len ), peek_len > indiv_len_cap ? c_red : c_light_green );
        std::string const indiv_len_cap_str = string_format( "%.1f", indiv_len_cap );
        bool const metric = get_option<std::string>( "DISTANCE_UNITS" ) == "metric";
        std::string const len_unit = metric ? "cm" : "in";

        units::volume const indiv_vol_cap = u.max_single_item_volume();
        units::volume const peek_vol = entry.stack[0]->volume();
        std::string const indiv_vol_cap_str = format_volume( indiv_vol_cap );
        std::string const peek_vol_str =
            colorize( format_volume( peek_vol ), peek_vol > indiv_vol_cap ? c_red : c_light_green );

        right_print( w, 2, 2, c_white,
                     string_format( _( "INDV %s/%s %s  %s/%s %s" ), peek_len_str, indiv_len_cap_str,
                                    len_unit, peek_vol_str, indiv_vol_cap_str,
                                    volume_units_abbr() ) );
    }
}

void aim_inv_stats( aim_advuilist_sourced_t *ui )
{
    catacurses::window &w = *ui->get_window();
    avatar &u = get_avatar();
    double const weight = convert_weight( u.weight_carried() );
    double const weight_cap = convert_weight( u.weight_capacity() );
    std::string const weight_str =
        colorize( string_format( "%.1f", weight ), weight >= weight_cap ? c_red : c_light_green );
    std::string const volume_cap = format_volume( u.volume_capacity() );
    std::string const volume = format_volume( u.volume_carried() );

    right_print( w, 1, 2, c_white,
                 string_format( "%s/%.1f %s  %s/%s %s", weight_str, weight_cap, weight_units(),
                                volume, volume_cap, volume_units_abbr() ) );
}

void aim_ground_veh_stats( aim_advuilist_sourced_t *ui, aim_stats_t *stats )
{
    using namespace advuilist_helpers;
    using slotidx_t = aim_advuilist_sourced_t::slotidx_t;
    using icon_t = aim_advuilist_sourced_t::icon_t;
    slotidx_t src = 0;
    icon_t srci = 0;
    std::tie( src, srci ) = ui->getSource();
    catacurses::window &w = *ui->get_window();
    avatar &u = get_avatar();
    units::volume vol_cap = 0_liter;
    tripoint const off = slotidx_to_offset( src );
    tripoint const loc = u.pos() + off;

    if( srci == SOURCE_VEHICLE_i or src == DRAGGED_IDX ) {
        cata::optional<vpart_reference> vp = veh_cargo_at( loc );
        vol_cap = vp ? vp->vehicle().max_volume( vp->part_index() ) : 0_liter;
    } else {
        vol_cap = get_map().max_volume( loc );
    }

    double const weight = convert_weight( stats->first );
    std::string const volume = format_volume( stats->second );
    std::string const volume_cap = format_volume( vol_cap );

    right_print( w, 1, 2, c_white,
                 string_format( "%3.1f %s  %s/%s %s", weight, weight_units(), volume,
                                volume_cap, volume_units_abbr() ) );
}

void aim_default_columns( aim_advuilist_t *myadvuilist )
{
    using col_t = typename aim_advuilist_t::col_t;
    myadvuilist->setColumns( std::vector<col_t> { col_t{ "Name", iloc_entry_name, 16 },
                             col_t{ "amt", iloc_entry_count, 2 },
                             col_t{ "weight", iloc_entry_weight, 3 },
                             col_t{ "vol", iloc_entry_volume, 3 }
                                                },
                             false );
}

void aim_all_columns( aim_advuilist_t *myadvuilist )
{
    using col_t = typename aim_advuilist_t::col_t;
    myadvuilist->setColumns( std::vector<col_t> { col_t{ "Name", iloc_entry_name, 16 },
                             col_t{ "src", iloc_entry_src, 2 },
                             col_t{ "amt", iloc_entry_count, 2 },
                             col_t{ "weight", iloc_entry_weight, 3 },
                             col_t{ "vol", iloc_entry_volume, 3 }
                                                },
                             false );
}

void aim_stats_printer( aim_advuilist_t *ui, aim_stats_t *stats )
{
    aim_advuilist_sourced_t *_ui = reinterpret_cast<aim_advuilist_sourced_t *>( ui );
    using slotidx_t = aim_advuilist_sourced_t::slotidx_t;
    slotidx_t src = std::get<slotidx_t>( _ui->getSource() );

    if( src == INV_IDX or src == WORN_IDX ) {
        aim_inv_stats( _ui );
    } else {
        aim_ground_veh_stats( _ui, stats );
        aim_inv_idv_stats( _ui );
    }
}

void player_take_off( aim_transaction_ui_t::select_t const &sel )
{
    avatar &u = get_avatar();
    for( auto const &it : sel ) {
        u.takeoff( *it.second->stack[0] );
    }
}

// FIXME: can I dedup this with get_selection_amount() ?
void player_drop( aim_transaction_ui_t::select_t const &sel, tripoint const pos, bool to_vehicle )
{
    avatar &u = get_avatar();
    std::vector<drop_or_stash_item_info> to_drop;
    for( auto const &it : sel ) {
        if( sel.size() > 1 and it.second->stack.front()->is_favorite ) {
            continue;
        }
        aim_advuilist_t::count_t count = it.first;
        if( it.second->stack.front()->count_by_charges() ) {
            to_drop.emplace_back( it.second->stack.front(), count );
        } else {
            for( auto v = it.second->stack.begin(); v != it.second->stack.begin() + count; ++v ) {
                to_drop.emplace_back( *v, count );
            }
        }
    }
    u.assign_activity( player_activity( drop_activity_actor( to_drop, pos, !to_vehicle ) ) );
}

void get_selection_amount( aim_transaction_ui_t::select_t const &sel,
                           std::vector<item_location> *targets, std::vector<int> *quantities,
                           bool ignorefav = false )
{
    bool const _ifav = sel.size() > 1 and ignorefav;
    for( auto const &it : sel ) {
        if( _ifav and it.second->stack.front()->is_favorite ) {
            continue;
        }
        if( it.second->stack.front()->count_by_charges() ) {
            targets->emplace_back( *it.second->stack.begin() );
            quantities->emplace_back( it.first );
        } else {
            targets->insert( targets->end(), it.second->stack.begin(),
                             it.second->stack.begin() + it.first );
            quantities->insert( quantities->end(), it.first, 0 );
        }
    }
}

void player_wear( aim_transaction_ui_t::select_t const &sel )
{
    avatar &u = get_avatar();
    u.assign_activity( ACT_WEAR );
    get_selection_amount( sel, &u.activity.targets, &u.activity.values );
}

void player_pick_up( aim_transaction_ui_t::select_t const &sel, bool from_vehicle )
{
    avatar &u = get_avatar();

    std::vector<item_location> targets;
    std::vector<int> quantities;
    get_selection_amount( sel, &targets, &quantities );

    u.assign_activity( player_activity( pickup_activity_actor(
                                            targets, quantities,
                                            from_vehicle ? cata::nullopt : cata::optional<tripoint>( u.pos() ) ) ) );
}

void player_move_items( aim_transaction_ui_t::select_t const &sel, tripoint const pos,
                        bool to_vehicle )
{
    avatar &u = get_avatar();
    std::vector<item_location> targets;
    std::vector<int> quantities;
    get_selection_amount( sel, &targets, &quantities, true );

    u.assign_activity(
        player_activity( move_items_activity_actor( targets, quantities, to_vehicle, pos ) ) );
}

void change_columns( aim_advuilist_sourced_t *ui )
{
    using slotidx_t = aim_advuilist_sourced_t::slotidx_t;
    if( std::get<slotidx_t>( ui->getSource() ) == ALL_IDX ) {
        aim_all_columns( ui ) ;
    } else {
        aim_default_columns( ui );
    }
}

int query_destination()
{
    uilist menu;
    menu.text = _( "Select destination" );
    int idx = 0;
    for( _sourcetuple const &v : aimsources ) {
        tripoint const off = std::get<tripoint>( v );
        if( idx != ALL_IDX and std::get<bool>( v ) ) {
            bool const valid = source_player_ground_avail( off );
            menu.addentry( idx, valid, MENU_AUTOASSIGN, std::get<_tuple_label_idx>( v ) );
        }
        idx++;
    }
    menu.query();
    return menu.ret;
}

void swap_panes_maybe( aim_transaction_ui_t *ui, std::string const &action, pane_mutex_t *mutex )
{
    if( !ui->curpane()->setSourceSuccess() ) {
        using namespace advuilist_literals;
        using slotidx_t = aim_advuilist_sourced_t::slotidx_t;
        using icon_t = aim_advuilist_sourced_t::icon_t;

        slotidx_t cslot = 0;
        slotidx_t oslot = 0;
        icon_t cicon = 0;
        icon_t oicon = 0;
        std::tie( cslot, cicon ) = ui->curpane()->getSource();
        std::tie( oslot, oicon ) = ui->otherpane()->getSource();
        // requested slot
        slotidx_t const rslot =
            action == ACTION_CYCLE_SOURCES
            ? cslot
            : std::stoul( action.substr( ACTION_SOURCE_PRFX_len, action.size() ) );
        // swap panes if the requested source is already selected in the other pane
        // also swap panes if the current source is re-selected since people have grown accustomed
        // to this behaviour (see discussion in #45900)
        if( rslot == oslot or rslot == cslot ) {
            set_mutex( mutex, cslot, cicon, false );
            ui->otherpane()->setSource( cslot, cicon );
            set_mutex( mutex, cslot, cicon, true );
            set_mutex( mutex, oslot, oicon, false );
            ui->curpane()->setSource( oslot, oicon );
            set_mutex( mutex, oslot, oicon, true );
            change_columns( ui->otherpane() );
            ui->otherpane()->get_ui()->invalidate_ui();
        }
    }
}

void aim_rebuild( aim_transaction_ui_t *ui, pane_mutex_t *mutex )
{
    aim_advuilist_sourced_t::slotidx_t lidx = 0;
    aim_advuilist_sourced_t::slotidx_t ridx = 0;
    aim_advuilist_sourced_t::icon_t licon = 0;
    aim_advuilist_sourced_t::icon_t ricon = 0;
    std::tie( lidx, licon ) = ui->left()->getSource();
    std::tie( ridx, ricon ) = ui->right()->getSource();
    set_mutex( mutex, lidx, licon, false );
    ui->left()->rebuild();
    set_mutex( mutex, lidx, licon, true );
    // make sure our panes don't use the same source even if they end up using the same slot
    set_mutex( mutex, ridx, ricon, lidx == ridx and licon == ricon );
    ui->right()->rebuild();
    set_mutex( mutex, ridx, ricon, true );
}

void setup_for_aim( aim_advuilist_t *myadvuilist, aim_stats_t *stats )
{
    using sorter_t = typename aim_advuilist_t::sorter_t;
    using grouper_t = typename aim_advuilist_t::grouper_t;
    using filter_t = typename aim_advuilist_t::filter_t;

    aim_default_columns( myadvuilist );
    myadvuilist->setcountingf( iloc_entry_counter );
    // we need to replace name sorter due to color tags
    myadvuilist->addSorter( sorter_t{ "Name", iloc_entry_name_sorter } );
    // use numeric sorters instead of advuilist's lexicographic ones
    myadvuilist->addSorter( sorter_t{ "amount", iloc_entry_count_sorter } );
    myadvuilist->addSorter( sorter_t{ "weight", iloc_entry_weight_sorter } );
    myadvuilist->addSorter( sorter_t{ "volume", iloc_entry_volume_sorter } );
    // extra sorters
    myadvuilist->addSorter( sorter_t{ "damage", iloc_entry_damage_sorter} );
    myadvuilist->addSorter( sorter_t{ "spoilage", iloc_entry_spoilage_sorter} );
    myadvuilist->addSorter( sorter_t{ "price", iloc_entry_price_sorter} );
    myadvuilist->addGrouper( grouper_t{ "category", iloc_entry_gsort, iloc_entry_glabel } );
    // FIXME: this string is duplicated from draw_item_filter_rules() because that function doesn't fit
    // anywhere in the current implementation of advuilist
    std::string const desc = string_format(
                                 "%s\n\n%s\n %s\n\n%s\n %s\n\n%s\n %s", _( "Type part of an item's name to filter it." ),
                                 _( "Separate multiple items with [<color_yellow>,</color>]." ), // NOLINT(cata-text-style): literal comma
                                 _( "Example: back,flash,aid, ,band" ), // NOLINT(cata-text-style): literal comma
                                 _( "To exclude items, place [<color_yellow>-</color>] in front." ),
                                 _( "Example: -pipe,-chunk,-steel" ),
                                 _( "Search [<color_yellow>c</color>]ategory, [<color_yellow>m</color>]aterial, "
                                    "[<color_yellow>q</color>]uality, [<color_yellow>n</color>]otes or "
                                    "[<color_yellow>d</color>]isassembled components." ),
                                 _( "Examples: c:food,m:iron,q:hammering,n:toolshelf,d:pipe" ) );
    myadvuilist->setfilterf( filter_t{ desc, iloc_entry_filter } );
    myadvuilist->on_rebuild(
    [stats]( bool first, iloc_entry const & it ) {
        iloc_entry_stats( stats, first, it );
    } );
    myadvuilist->on_redraw(
    [stats]( aim_advuilist_t *ui ) {
        aim_stats_printer( ui, stats );
    } );
    myadvuilist->get_ctxt()->register_action( ACTION_EXAMINE );
    myadvuilist->get_ctxt()->register_action( ACTION_ITEMS_DEFAULT );
    myadvuilist->get_ctxt()->register_action( ACTION_SAVE_DEFAULT );
    myadvuilist->get_ctxt()->register_action( TOGGLE_AUTO_PICKUP );
    myadvuilist->get_ctxt()->register_action( TOGGLE_FAVORITE );
}

void add_aim_sources( aim_advuilist_sourced_t *myadvuilist, pane_mutex_t *mutex )
{
    using source_t = aim_advuilist_sourced_t::source_t;
    using fsource_t = aim_advuilist_sourced_t::fsource_t;
    using fsourceb_t = aim_advuilist_sourced_t::fsourceb_t;
    using flabel_t = aim_advuilist_sourced_t::flabel_t;
    using icon_t = aim_advuilist_sourced_t::icon_t;

    fsource_t source_dummy = []() {
        return aim_container_t();
    };
    fsourceb_t _never = []() {
        return false;
    };

    // Cataclysm: Hacky Stuff Redux
    _sourcearray::size_type idx = 0;
    for( auto const &src : aimsources ) {
        fsource_t _fs;
        fsource_t _fsv;
        fsourceb_t _fsb;
        fsourceb_t _fsvb;
        char const *str = nullptr;
        icon_t icon = 0;
        tripoint off;
        std::tie( std::ignore, str, std::ignore, icon, off ) = src;

        if( icon != 0 ) {
            switch( idx ) {
                case CONT_IDX: {
                    _fs = source_dummy;
                    _fsb = _never;
                    break;
                }
                case DRAGGED_IDX: {
                    _fs = source_player_dragged;
                    _fsb = [ = ]() {
                        return !mutex->at( DRAGGED_IDX ) and source_player_dragged_avail();
                    };
                    break;
                }
                case INV_IDX: {
                    _fs = source_player_inv;
                    _fsb = [ = ]() {
                        return !mutex->at( INV_IDX );
                    };
                    break;
                }
                case ALL_IDX: {
                    _fs = [ = ]() {
                        return source_player_all( myadvuilist, mutex );
                    };
                    _fsb = [ = ]() {
                        return !mutex->at( ALL_IDX );
                    };
                    break;
                }
                case WORN_IDX: {
                    _fs = source_player_worn;
                    _fsb = [ = ]() {
                        return !mutex->at( WORN_IDX );
                    };
                    break;
                }
                default: {
                    _fs = [ = ]() {
                        return source_player_ground( off );
                    };
                    _fsb = [ = ]() {
                        return !mutex->at( idx ) and source_player_ground_avail( off );
                    };
                    _fsv = [ = ]() {
                        return source_player_vehicle( off );
                    };
                    _fsvb = [ = ]() {
                        return !mutex->at( idxtovehidx( idx ) ) and
                               source_player_vehicle_avail( off );
                    };
                    break;
                }
            }
            flabel_t const label = [ = ]() {
                return aim_sourcelabel( idx );
            };
            myadvuilist->addSource( idx, source_t{ label, icon, _fs, _fsb } );
            if( _fsv ) {
                flabel_t const vlabel = [ = ]() {
                    return aim_sourcelabel( idx, true );
                };
                myadvuilist->addSource( idx, source_t{ vlabel, SOURCE_VEHICLE_i, _fsv, _fsvb } );
            }
        }
        idx++;
    }
}

void aim_add_return_activity()
{
    // return to the AIM after player activities finish
    avatar &u = get_avatar();
    player_activity act_return( ACT_ADV_INVENTORY );
    act_return.auto_resume = true;
    u.assign_activity( act_return );
}

void aim_transfer( aim_transaction_ui_t *ui, aim_transaction_ui_t::select_t const &select )
{
    using slotidx_t = aim_advuilist_sourced_t::slotidx_t;
    using icon_t = aim_advuilist_sourced_t::icon_t;
    slotidx_t src = 0;
    slotidx_t dst = 0;
    icon_t srci = 0;
    icon_t dsti = 0;
    std::tie( src, srci ) = ui->curpane()->getSource();
    std::tie( dst, dsti ) = ui->otherpane()->getSource();

    // return to the AIM after player activities finish
    if( select.size() == 1 or !get_option<bool>( "CLOSE_ADV_INV" ) ) {
        aim_add_return_activity();
    }

    // select a valid destination if otherpane is showing the ALL source
    if( dst == ALL_IDX ) {
        int const newdst = query_destination();
        if( newdst < 0 ) {
            // transfer cancelled
            return;
        }
        dst = static_cast<slotidx_t>( newdst );
        dsti = std::get<icon_t>( aimsources[dst] );
    }

    if( dst == WORN_IDX ) {
        player_wear( select );
    } else if( src == WORN_IDX and dst == INV_IDX ) {
        player_take_off( select );
    } else if( src == WORN_IDX or src == INV_IDX ) {
        player_drop( select, slotidx_to_offset( dst ), is_vehicle( dsti ) );
    } else if( dst == INV_IDX ) {
        player_pick_up( select, is_vehicle( srci ) );
    } else {
        player_move_items( select, slotidx_to_offset( dst ), is_vehicle( dsti ) );
    }

    // close the transaction_ui so that player activities can run
    ui->pushevent( aim_transaction_ui_t::event::QUIT );
}

// FIXME: fragment this as it has grown quite large
void aim_ctxthandler( aim_transaction_ui_t *ui, std::string const &action, pane_mutex_t *mutex )
{
    using namespace advuilist_literals;
    using select_t = aim_transaction_ui_t::select_t;
    using slotidx_t = aim_advuilist_sourced_t::slotidx_t;
    select_t const peek = ui->curpane()->peek();

    // reset pane mutex on any source change
    if( action == ACTION_CYCLE_SOURCES or
        action.substr( 0, ACTION_SOURCE_PRFX_len ) == ACTION_SOURCE_PRFX ) {

        swap_panes_maybe( ui, action, mutex );

        change_columns( ui->curpane() );
        reset_mutex( ui, mutex );
        // rebuild other pane if it's set to the ALL source
        if( std::get<slotidx_t>( ui->otherpane()->getSource() ) == ALL_IDX ) {
            // this is ugly but it's required since we're rebuilding out of line
            mutex->at( ALL_IDX ) = false;
            ui->otherpane()->rebuild();
            mutex->at( ALL_IDX ) = true;
            ui->otherpane()->get_ui()->invalidate_ui();
        }

    } else if( action == ACTION_SAVE_DEFAULT ) {
        ui->savestate( &uistate.transfer_default );

    } else if( action == ACTION_ITEMS_DEFAULT ) {
        ui->curpane()->suspend();
        reset_mutex( mutex );
        ui->loadstate( &uistate.transfer_default, false );
        reset_mutex( ui, mutex );
        aim_rebuild( ui, mutex );
        ui->otherpane()->get_ui()->invalidate_ui();

    } else if( !peek.empty() ) {
        iloc_entry &entry = *std::get<aim_advuilist_t::ptr_t>( peek.front() );

        if( action == ACTION_EXAMINE ) {
            slotidx_t src = std::get<slotidx_t>( ui->curpane()->getSource() );
            if( src == INV_IDX or src == WORN_IDX ) {
                aim_add_return_activity();
                ui->pushevent( aim_transaction_ui_t::event::QUIT );
                ui->curpane()->suspend();
                ui->curpane()->hide();
                ui->otherpane()->hide();

                std::pair<point, point> const dim = ui->otherpane()->get_size();
                game::inventory_item_menu_position const side =
                    ui->curpane() == ui->left() ? game::LEFT_OF_INFO : game::RIGHT_OF_INFO;
                g->inventory_item_menu(
                    entry.stack.front(), [ = ] { return dim.second.x; }, [ = ] { return dim.first.x; },
                    side );
            } else {
                iloc_entry_examine( ui->otherpane()->get_window(), entry );
            }

        } else if( action == TOGGLE_AUTO_PICKUP ) {
            item const *it = entry.stack.front().get_item();
            bool const has = get_auto_pickup().has_rule( it );
            if( !has ) {
                get_auto_pickup().add_rule( it );
            } else {
                get_auto_pickup().remove_rule( it );
            }

        } else if( action == TOGGLE_FAVORITE ) {
            bool const isfavorite = entry.stack.front()->is_favorite;
            for( item_location &it : entry.stack ) {
                it->set_favorite( !isfavorite );
            }
        }
    }
}

} // namespace

void create_advanced_inv( bool resume )
{
    using mytrui_t = transaction_ui<aim_container_t>;

    static std::unique_ptr<mytrui_t> mytrui;
    static pane_mutex_t pane_mutex{};
    static aim_stats_t lstats{ 0_kilogram, 0_liter };
    static aim_stats_t rstats{ 0_kilogram, 0_liter };
    static bool full_screen{ get_option<bool>( "AIM_WIDTH" ) };
    bool const _fs = get_option<bool>( "AIM_WIDTH" );
    if( !mytrui ) {
        std::pair<point, point> size = aim_size( full_screen );

        mytrui = std::make_unique<mytrui_t>( aimlayout, size.first, size.second,
                                             "ADVANCED_INVENTORY", point{3, 1} );
        mytrui->on_resize( [&]( mytrui_t *ui ) {
            std::pair<point, point> size = aim_size( full_screen );
            ui->resize( size.first, size.second );
        } );

        setup_for_aim( mytrui->left(), &lstats );
        setup_for_aim( mytrui->right(), &rstats );
        add_aim_sources( mytrui->left(), &pane_mutex );
        add_aim_sources( mytrui->right(), &pane_mutex );
        mytrui->on_select( aim_transfer );
        mytrui->setctxthandler( [&]( aim_transaction_ui_t *ui, std::string const & action ) {
            aim_ctxthandler( ui, action, &pane_mutex );
        } );
        mytrui->loadstate( &uistate.transfer_save, false );

    } else if( full_screen != _fs ) {
        full_screen = _fs;
        std::pair<point, point> size = aim_size( full_screen );
        mytrui->resize( size.first, size.second );

    }

    reset_mutex( &pane_mutex );
    if( !resume and get_option<bool>( "OPEN_DEFAULT_ADV_INV" ) ) {
        mytrui->loadstate( &uistate.transfer_default, false );
    } else {
        mytrui->loadstate( &uistate.transfer_save, false );
    }

    reset_mutex( &*mytrui, &pane_mutex );
    aim_rebuild( &*mytrui, &pane_mutex );

    mytrui->show();
    mytrui->savestate( &uistate.transfer_save );
}
