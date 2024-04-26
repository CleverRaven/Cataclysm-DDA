#include "inventory_ui.h"

#include <cstdint>
#include <optional>

#include "activity_actor_definitions.h"
#include "basecamp.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "enums.h"
#include "flag.h"
#include "inventory.h"
#include "input.h"
#include "item.h"
#include "item_category.h"
#include "item_pocket.h"
#include "item_search.h"
#include "item_stack.h"
#include "item_tname.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_selector.h"
#include "memory_fast.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "point.h"
#include "popup.h"
#include "ret_val.h"
#include "sdltiles.h"
#include "localized_comparator.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "trade_ui.h"
#include "translations.h"
#include "type_id.h"
#include "uistate.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"

#if defined(__ANDROID__)
#include <SDL_keyboard.h>
#endif

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

static const item_category_id item_category_BIONIC_FUEL_SOURCE( "BIONIC_FUEL_SOURCE" );
static const item_category_id item_category_INTEGRATED( "INTEGRATED" );
static const item_category_id item_category_ITEMS_WORN( "ITEMS_WORN" );
static const item_category_id item_category_WEAPON_HELD( "WEAPON_HELD" );

namespace
{
using startup_timer = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;
startup_timer tp_start;

void debug_print_timer( startup_timer const &tp, std::string const &msg = "inv_ui setup took" )
{
    startup_timer const tp_now =
        std::chrono::time_point_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() );
    add_msg_debug( debugmode::DF_GAME, "%s: %i ms", msg, ( tp_now - tp ).count() );
}

// using item_name_t = std::pair<std::string, std::string>;
struct item_name_t {
    std::string sort_key;
    std::string full_name;
    unsigned int contents_count{};
};
using name_cache_t = std::unordered_map<item const *, item_name_t>;
name_cache_t item_name_cache;
int item_name_cache_users = 0;

item_name_t &get_cached_name( item const *it )
{
    auto iter = item_name_cache.find( it );
    if( iter == item_name_cache.end() ) {
        return item_name_cache
               .emplace( it, item_name_t{ remove_color_tags( it->tname( 1, tname::tname_sort_key ) ),
                                          remove_color_tags( it->tname( 1, true ) ),
                                          it->aggregated_contents().count } )
               .first->second;
    }

    return iter->second;
}

// get topmost visible parent in an unbroken chain
item_location get_topmost_parent( item_location const &topmost, item_location const &loc,
                                  inventory_selector_preset const &preset )
{
    return preset.is_shown( loc ) ? topmost ? topmost : loc : item_location{};
}

using parent_path_t = std::vector<item_location>;
parent_path_t path_to_top( inventory_entry const &e, inventory_selector_preset const &pr )
{
    item_location it = e.any_item();
    parent_path_t path{ it };
    while( it.has_parent() ) {
        it = it.parent_item();
        if( pr.is_shown( it ) ) {
            path.emplace_back( it );
        }
    }
    return path;
}

using pred_t = std::function<bool( inventory_entry const & )>;
void move_if( std::vector<inventory_entry> &src, std::vector<inventory_entry> &dst,
              pred_t const &pred )
{
    for( auto it = src.begin(); it != src.end(); ) {
        if( pred( *it ) ) {
            if( it->is_item() ) {
                dst.emplace_back( std::move( *it ) );
            }
            it = src.erase( it );
        } else {
            ++it;
        }
    }
}

bool always_yes( const inventory_entry & )
{
    return true;
}

bool return_item( const inventory_entry &entry )
{
    return entry.is_item();
}

bool is_container( const item_location &loc )
{
    return loc.where() == item_location::type::container;
}

bool is_worn_id( item_category_id const &id )
{
    return id == item_category_ITEMS_WORN || id == item_category_INTEGRATED ||
           id == item_category_BIONIC_FUEL_SOURCE;
}

item_category const *wielded_worn_category( item_location const &loc, Character const &u )
{
    if( loc == u.get_wielded_item() ) {
        return &item_category_WEAPON_HELD.obj();
    }
    if( u.is_worn( *loc ) || ( loc.has_parent() && is_worn_ablative( loc.parent_item(), loc ) ) ) {
        if( loc->has_flag( flag_BIONIC_FUEL_SOURCE ) ) {
            return &item_category_BIONIC_FUEL_SOURCE.obj();
        }
        if( loc->has_flag( flag_INTEGRATED ) ) {
            return &item_category_INTEGRATED.obj();
        }
        return &item_category_ITEMS_WORN.obj();
    }
    return nullptr;
}

} // namespace

bool is_worn_ablative( item_location const &container, item_location const &child )
{
    // if the item is in an ablative pocket then put it with the item it is in
    // first do a short circuit test if the parent has ablative pockets at all
    return container->is_ablative() && container->is_worn_by_player() &&
           child.parent_pocket()->get_pocket_data()->ablative;
}

/** The maximum distance from the screen edge, to snap a window to it */
static const size_t max_win_snap_distance = 4;
/** The minimal gap between two cells */
static const int min_cell_gap = 2;
/** The gap between two cells when screen space is limited*/
static const int normal_cell_gap = 4;
/** The minimal gap between the first cell and denial */
static const int min_denial_gap = 2;
/** The minimal gap between two columns */
static const int min_column_gap = 2;
/** The gap between two columns when there's enough space, but they are not centered */
static const int normal_column_gap = 8;
/**
 * The minimal occupancy ratio to align columns to the center
 * @see inventory_selector::get_columns_occupancy_ratio()
 */
static const double min_ratio_to_center = 0.85;

bool inventory_selector::skip_unselectable = false;

struct navigation_mode_data {
    navigation_mode next_mode;
    translation name;
    nc_color color;
};

struct inventory_input {
    std::string action;
    int ch = 0;
    inventory_entry *entry;
};

struct container_data {
    units::volume actual_capacity;
    units::volume total_capacity;
    units::mass actual_capacity_weight;
    units::mass total_capacity_weight;
    units::length max_containable_length;

    std::string to_formatted_string( const bool compact = true ) const {
        std::string string_to_format;
        if( compact ) {
            string_to_format = _( "%s/%s : %s/%s : max %s" );
        } else {
            string_to_format = _( "(remains %s, %s) max length %s" );
        }
        return string_format( string_to_format,
                              unit_to_string( total_capacity - actual_capacity, true, true ),
                              unit_to_string( total_capacity_weight - actual_capacity_weight, true, true ),
                              unit_to_string( max_containable_length, true ) );
    }
};

bool inventory_entry::operator==( const inventory_entry &other ) const
{
    return get_category_ptr() == other.get_category_ptr() && locations == other.locations;
}

class selection_column_preset : public inventory_selector_preset
{
    public:
        selection_column_preset() = default;
        std::string get_caption( const inventory_entry &entry ) const override {
            std::string res;
            const size_t available_count = entry.get_available_count();
            const item_location &item = entry.any_item();

            if( entry.chosen_count > 0 && entry.chosen_count < available_count ) {
                //~ %1$d: chosen count, %2$d: available count
                res += string_format( pgettext( "count", "%1$d of %2$d" ), entry.chosen_count,
                                      available_count ) + " ";
            } else if( available_count != 1 ) {
                res += string_format( "%d ", available_count );
            }
            if( item->is_money() ) {
                cata_assert( available_count == entry.get_stack_size() );
                if( entry.chosen_count > 0 && entry.chosen_count < available_count ) {
                    res += item->display_money( available_count, item->ammo_remaining(),
                                                entry.get_selected_charges() );
                } else {
                    res += item->display_money( available_count, item->ammo_remaining() );
                }
            } else {
                res += item->display_name( available_count );
            }
            return res;
        }

        nc_color get_color( const inventory_entry &entry ) const override {
            Character &player_character = get_player_character();
            if( entry.is_item() ) {
                if( entry.any_item() == player_character.get_wielded_item() ) {
                    return c_light_blue;
                } else if( player_character.is_worn( *entry.any_item() ) ) {
                    return c_cyan;
                }
            }
            return inventory_selector_preset::get_color( entry );
        }
};

void inventory_selector_save_state::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "uimode", uimode );
    json.end_object();
}
void inventory_selector_save_state::deserialize( JsonObject const &jo )
{
    jo.read( "uimode", uimode );
}

namespace io
{
template <>
std::string enum_to_string<inventory_selector::uimode>( inventory_selector::uimode mode )
{
    switch( mode ) {
        case inventory_selector::uimode::hierarchy:
            return "hierarchy";
        case inventory_selector::uimode::last:
        case inventory_selector::uimode::categories:
            break;
    }
    return "categories";
}
} // namespace io

static inventory_selector_save_state inventory_sel_default_state{ inventory_selector::uimode::categories };
inventory_selector_save_state inventory_ui_default_state{ inventory_selector::uimode::categories };
inventory_selector_save_state pickup_sel_default_state{ inventory_selector::uimode::categories };
inventory_selector_save_state pickup_ui_default_state{ inventory_selector::uimode::hierarchy };

void save_inv_state( JsonOut &json )
{
    json.member( "inventory_sel_default_state", inventory_sel_default_state );
    json.member( "inventory_ui_state", inventory_ui_default_state );
    json.member( "pickup_sel_state", pickup_sel_default_state );
    json.member( "pickup_ui_state", pickup_ui_default_state );
}

void load_inv_state( const JsonObject &jo )
{
    jo.read( "inventory_sel_default_state", inventory_sel_default_state );
    jo.read( "inventory_ui_state", inventory_ui_default_state );
    jo.read( "pickup_sel_state", pickup_sel_default_state );
    jo.read( "pickup_ui_state", pickup_ui_default_state );
}

void uistatedata::serialize( JsonOut &json ) const
{
    const unsigned int input_history_save_max = 25;
    json.start_object();

    transfer_save.serialize( json, "transfer_save_" );
    save_inv_state( json );

    /**** if you want to save whatever so it's whatever when the game is started next, declare here and.... ****/
    // non array stuffs
    json.member( "ags_pay_gas_selected_pump", ags_pay_gas_selected_pump );
    json.member( "adv_inv_container_location", adv_inv_container_location );
    json.member( "adv_inv_container_index", adv_inv_container_index );
    json.member( "adv_inv_container_in_vehicle", adv_inv_container_in_vehicle );
    json.member( "adv_inv_container_type", adv_inv_container_type );
    json.member( "adv_inv_container_content_type", adv_inv_container_content_type );
    json.member( "unload_auto_contain", unload_auto_contain );
    json.member( "hide_entries_override", hide_entries_override );
    json.member( "editmap_nsa_viewmode", editmap_nsa_viewmode );
    json.member( "overmap_blinking", overmap_blinking );
    json.member( "overmap_show_overlays", overmap_show_overlays );
    json.member( "overmap_show_map_notes", overmap_show_map_notes );
    json.member( "overmap_show_land_use_codes", overmap_show_land_use_codes );
    json.member( "overmap_show_city_labels", overmap_show_city_labels );
    json.member( "overmap_show_hordes", overmap_show_hordes );
    json.member( "overmap_show_revealed_omts", overmap_show_revealed_omts );
    json.member( "overmap_show_forest_trails", overmap_show_forest_trails );
    json.member( "vmenu_show_items", vmenu_show_items );
    json.member( "list_item_sort", list_item_sort );
    json.member( "list_item_filter_active", list_item_filter_active );
    json.member( "list_item_downvote_active", list_item_downvote_active );
    json.member( "list_item_priority_active", list_item_priority_active );
    json.member( "construction_filter", construction_filter );
    json.member( "last_construction", last_construction );
    json.member( "construction_tab", construction_tab );
    json.member( "hidden_recipes", hidden_recipes );
    json.member( "favorite_recipes", favorite_recipes );
    json.member( "expanded_recipes", expanded_recipes );
    json.member( "read_recipes", read_recipes );
    json.member( "recent_recipes", recent_recipes );
    json.member( "bionic_ui_sort_mode", bionic_sort_mode );
    json.member( "overmap_debug_weather", overmap_debug_weather );
    json.member( "overmap_visible_weather", overmap_visible_weather );
    json.member( "overmap_debug_mongroup", overmap_debug_mongroup );
    json.member( "distraction_noise", distraction_noise );
    json.member( "distraction_pain", distraction_pain );
    json.member( "distraction_attack", distraction_attack );
    json.member( "distraction_hostile_close", distraction_hostile_close );
    json.member( "distraction_hostile_spotted", distraction_hostile_spotted );
    json.member( "distraction_conversation", distraction_conversation );
    json.member( "distraction_asthma", distraction_asthma );
    json.member( "distraction_dangerous_field", distraction_dangerous_field );
    json.member( "distraction_weather_change", distraction_weather_change );
    json.member( "distraction_hunger", distraction_hunger );
    json.member( "distraction_thirst", distraction_thirst );
    json.member( "distraction_temperature", distraction_temperature );
    json.member( "distraction_mutation", distraction_mutation );
    json.member( "distraction_oxygen", distraction_oxygen );
    json.member( "numpad_navigation", numpad_navigation );

    json.member( "input_history" );
    json.start_object();
    for( const auto &e : input_history ) {
        json.member( e.first );
        const std::vector<std::string> &history = e.second;
        json.start_array();
        int save_start = 0;
        if( history.size() > input_history_save_max ) {
            save_start = history.size() - input_history_save_max;
        }
        for( std::vector<std::string>::const_iterator hit = history.begin() + save_start;
             hit != history.end(); ++hit ) {
            json.write( *hit );
        }
        json.end_array();
    }
    json.end_object(); // input_history

    json.member( "lastreload", lastreload );

    json.end_object();
}

void uistatedata::deserialize( const JsonObject &jo )
{
    jo.allow_omitted_members();

    transfer_save.deserialize( jo, "transfer_save_" );
    load_inv_state( jo );
    // the rest
    jo.read( "ags_pay_gas_selected_pump", ags_pay_gas_selected_pump );
    jo.read( "adv_inv_container_location", adv_inv_container_location );
    jo.read( "adv_inv_container_index", adv_inv_container_index );
    jo.read( "adv_inv_container_in_vehicle", adv_inv_container_in_vehicle );
    jo.read( "adv_inv_container_type", adv_inv_container_type );
    jo.read( "adv_inv_container_content_type", adv_inv_container_content_type );
    jo.read( "unload_auto_contain", unload_auto_contain );
    jo.read( "hide_entries_override", hide_entries_override );
    jo.read( "editmap_nsa_viewmode", editmap_nsa_viewmode );
    jo.read( "overmap_blinking", overmap_blinking );
    jo.read( "overmap_show_overlays", overmap_show_overlays );
    jo.read( "overmap_show_map_notes", overmap_show_map_notes );
    jo.read( "overmap_show_land_use_codes", overmap_show_land_use_codes );
    jo.read( "overmap_show_city_labels", overmap_show_city_labels );
    jo.read( "overmap_show_hordes", overmap_show_hordes );
    jo.read( "overmap_show_revealed_omts", overmap_show_revealed_omts );
    jo.read( "overmap_show_forest_trails", overmap_show_forest_trails );
    jo.read( "hidden_recipes", hidden_recipes );
    jo.read( "favorite_recipes", favorite_recipes );
    jo.read( "expanded_recipes", expanded_recipes );
    jo.read( "read_recipes", read_recipes );
    jo.read( "recent_recipes", recent_recipes );
    jo.read( "bionic_ui_sort_mode", bionic_sort_mode );
    jo.read( "overmap_debug_weather", overmap_debug_weather );
    jo.read( "overmap_visible_weather", overmap_visible_weather );
    jo.read( "overmap_debug_mongroup", overmap_debug_mongroup );
    jo.read( "distraction_noise", distraction_noise );
    jo.read( "distraction_pain", distraction_pain );
    jo.read( "distraction_attack", distraction_attack );
    jo.read( "distraction_hostile_close", distraction_hostile_close );
    jo.read( "distraction_hostile_spotted", distraction_hostile_spotted );
    jo.read( "distraction_conversation", distraction_conversation );
    jo.read( "distraction_asthma", distraction_asthma );
    jo.read( "distraction_dangerous_field", distraction_dangerous_field );
    jo.read( "distraction_weather_change", distraction_weather_change );
    jo.read( "distraction_hunger", distraction_hunger );
    jo.read( "distraction_thirst", distraction_thirst );
    jo.read( "distraction_temperature", distraction_temperature );
    jo.read( "distraction_mutation", distraction_mutation );
    jo.read( "distraction_oxygen", distraction_oxygen );
    jo.read( "numpad_navigation", numpad_navigation );

    if( !jo.read( "vmenu_show_items", vmenu_show_items ) ) {
        // This is an old save: 1 means view items, 2 means view monsters,
        // -1 means uninitialized
        vmenu_show_items = jo.get_int( "list_item_mon", -1 ) != 2;
    }

    jo.read( "list_item_sort", list_item_sort );
    jo.read( "list_item_filter_active", list_item_filter_active );
    jo.read( "list_item_downvote_active", list_item_downvote_active );
    jo.read( "list_item_priority_active", list_item_priority_active );

    jo.read( "construction_filter", construction_filter );
    jo.read( "last_construction", last_construction );
    jo.read( "construction_tab", construction_tab );

    for( const JsonMember member : jo.get_object( "input_history" ) ) {
        std::vector<std::string> &v = gethistory( member.name() );
        v.clear();
        for( const std::string line : member.get_array() ) {
            v.push_back( line );
        }
    }
    // fetch list_item settings from input_history
    if( !gethistory( "item_filter" ).empty() ) {
        list_item_filter = gethistory( "item_filter" ).back();
    }
    if( !gethistory( "list_item_downvote" ).empty() ) {
        list_item_downvote = gethistory( "list_item_downvote" ).back();
    }
    if( !gethistory( "list_item_priority" ).empty() ) {
        list_item_priority = gethistory( "list_item_priority" ).back();
    }

    jo.read( "lastreload", lastreload );
}

static const selection_column_preset selection_preset{};

bool inventory_entry::is_hidden( std::optional<bool> const &hide_entries_override ) const
{
    if( !is_item() ) {
        return false;
    }

    if( is_collation_entry() && collation_meta->collapsed ) {
        return true;
    }

    if( !topmost_parent ) {
        return false;
    }

    item_location item = locations.front();
    if( hide_entries_override && topmost_parent && topmost_parent->is_container() ) {
        return *hide_entries_override;
    }
    while( item.has_parent() && item != topmost_parent ) {
        if( item.parent_pocket()->settings.is_collapsed() ) {
            return true;
        }
        item = item.parent_item();
    }
    return false;
}

int inventory_entry::get_total_charges() const
{
    int result = 0;
    for( const item_location &location : locations ) {
        result += location->charges;
    }
    return result;
}

int inventory_entry::get_selected_charges() const
{
    cata_assert( chosen_count <= locations.size() );
    int result = 0;
    for( size_t i = 0; i < chosen_count; ++i ) {
        const item_location &location = locations[i];
        result += location->charges;
    }
    return result;
}

size_t inventory_entry::get_available_count() const
{
    if( locations.size() == 1 ) {
        return any_item()->count();
    } else {
        return locations.size();
    }
}

int inventory_entry::get_invlet() const
{
    if( custom_invlet != INT_MIN ) {
        return custom_invlet;
    }
    if( !is_item() ) {
        return '\0';
    }
    return any_item()->invlet;
}

nc_color inventory_entry::get_invlet_color() const
{
    if( !is_selectable() ) {
        return c_dark_gray;
    } else if( get_player_character().inv->assigned_invlet.count( get_invlet() ) ) {
        return c_yellow;
    } else {
        return c_white;
    }
}

void inventory_entry::update_cache()
{
    item_name_t &names = get_cached_name( &*any_item() );
    cached_name = &names.sort_key;
    contents_count = names.contents_count;
    cached_name_full = &names.full_name;
}

void inventory_entry::cache_denial( inventory_selector_preset const &preset ) const
{
    if( !denial ) {
        denial.emplace( preset.get_denial( *this ) );
        enabled = denial->empty();
    }
    if( is_collation_header() ) {
        collation_meta->enabled = denial->empty();
    }
}

const item_category *inventory_entry::get_category_ptr() const
{
    if( custom_category != nullptr ) {
        return custom_category;
    }
    if( !is_item() ) {
        return nullptr;
    }
    return &any_item()->get_category_of_contents();
}

inventory_column::inventory_column( const inventory_selector_preset &preset ) :
    preset( preset )
{
    hide_entries_override = uistate.hide_entries_override;
    cells.resize( preset.get_cells_count() );
}

bool inventory_column::activatable() const
{
    return std::any_of( entries.begin(), entries.end(), [this]( const inventory_entry & e ) {
        return e.is_highlightable( skip_unselectable );
    } );
}

inventory_entry *inventory_column::find_by_invlet( int invlet ) const
{
    for( const inventory_entry &elem : entries ) {
        if( elem.is_item() && elem.get_invlet() == invlet ) {
            return const_cast<inventory_entry *>( &elem );
        }
    }
    return nullptr;
}

inventory_entry *inventory_column::find_by_location( item_location const &loc, bool hidden ) const
{
    std::vector<inventory_entry> const &ents = hidden ? entries_hidden : entries;
    for( const inventory_entry &elem : ents ) {
        if( elem.is_item() ) {
            for( const item_location &it : elem.locations ) {
                if( it == loc ) {
                    return const_cast<inventory_entry *>( &elem );
                }
            }
        }
    }
    return nullptr;
}

size_t inventory_column::get_width() const
{
    return std::max( get_cells_width(), reserved_width );
}

size_t inventory_column::get_height() const
{
    return std::min( entries.size(), height );
}

void inventory_column::toggle_skip_unselectable( const bool skip )
{
    skip_unselectable = skip;
}

inventory_selector_preset::inventory_selector_preset()
{
    append_cell(
    std::function<std::string( const inventory_entry & )>( [ this ]( const inventory_entry & entry ) {
        return get_caption( entry );
    } ) );
}

bool inventory_selector_preset::sort_compare( const inventory_entry &lhs,
        const inventory_entry &rhs ) const
{
    auto const sort_key = []( inventory_entry const & e ) {
        return std::make_tuple( *e.cached_name, e.contents_count, *e.cached_name_full,
                                e.any_item()->link_sort_key(), e.generation );
    };
    return localized_compare( sort_key( lhs ), sort_key( rhs ) );
}

bool inventory_selector_preset::cat_sort_compare( const inventory_entry &lhs,
        const inventory_entry &rhs ) const
{
    return *lhs.get_category_ptr() < *rhs.get_category_ptr();
}

nc_color inventory_selector_preset::get_color( const inventory_entry &entry ) const
{
    return entry.is_item() ? entry.any_item()->color_in_inventory() : c_magenta;
}

std::function<bool( const inventory_entry & )> inventory_selector_preset::get_filter(
    const std::string &filter ) const
{
    auto item_filter = basic_item_filter( filter );

    return [item_filter]( const inventory_entry & e ) {
        return item_filter( *e.any_item() );
    };
}

std::string inventory_selector_preset::get_caption( const inventory_entry &entry ) const
{
    size_t count = entry.get_stack_size();
    std::string disp_name;
    if( entry.any_item()->is_money() ) {
        disp_name = entry.any_item()->display_money( count, entry.any_item()->ammo_remaining() );
    } else if( entry.is_collation_header() && entry.any_item()->count_by_charges() ) {
        item temp( *entry.any_item() );
        temp.charges = entry.get_total_charges();
        disp_name = temp.display_name();
        count = 1;
    } else {
        disp_name = entry.any_item()->display_name( count );
    }

    return ( count > 1 ) ? string_format( "%d %s", count, disp_name ) : disp_name;
}

std::string inventory_selector_preset::get_denial( const inventory_entry &entry ) const
{
    return entry.is_item() ? get_denial( entry.any_item() ) : std::string();
}

std::string inventory_selector_preset::get_cell_text( const inventory_entry &entry,
        size_t cell_index ) const
{
    if( cell_index >= cells.size() ) {
        debugmsg( "Invalid cell index %d.", cell_index );
        return "it's a bug!";
    }
    if( !entry ) {
        return std::string();
    } else if( entry.is_item() ) {
        std::string text = cells[cell_index].get_text( entry );
        const item &actual_item = *entry.locations.front();
        const std::string info_display = get_option<std::string>( "DETAILED_CONTAINERS" );
        // if we want no additional info skip this
        if( info_display != "NONE" ) {
            // if we want additional info for all items or it is worn then add the additional info
            if( info_display == "ALL" || ( info_display == "WORN" &&
                                           is_worn_id( entry.get_category_ptr()->get_id() ) &&
                                           actual_item.is_worn_by_player() ) ) {
                if( cell_index == 0 && !text.empty() &&
                    actual_item.is_container() && actual_item.has_unrestricted_pockets() ) {
                    const units::volume total_capacity = actual_item.get_total_capacity( true );
                    const units::mass total_capacity_weight = actual_item.get_total_weight_capacity( true );
                    const units::length max_containable_length = actual_item.max_containable_length( true );

                    const units::volume actual_capacity = actual_item.get_total_contained_volume( true );
                    const units::mass actual_capacity_weight = actual_item.get_total_contained_weight( true );

                    container_data container_data = {
                        actual_capacity,
                        total_capacity,
                        actual_capacity_weight,
                        total_capacity_weight,
                        max_containable_length
                    };
                    std::string formatted_string = container_data.to_formatted_string( false );

                    text = text + string_format( " %s", formatted_string );
                }
            }
        }
        return text;
    } else if( cell_index != 0 ) {
        return replace_colors( cells[cell_index].title );
    } else {
        return entry.get_category_ptr()->name_header();
    }
}

bool inventory_selector_preset::is_stub_cell( const inventory_entry &entry,
        size_t cell_index ) const
{
    if( !entry.is_item() ) {
        return false;
    }
    const std::string &text = entry.get_entry_cell_cache( *this ).text[ cell_index ];
    return text.empty() || text == cells[cell_index].stub;
}

void inventory_selector_preset::append_cell( const
        std::function<std::string( const item_location & )> &func,
        const std::string &title, const std::string &stub )
{
    // Don't capture by reference here. The func should be able to die earlier than the object itself
    append_cell( std::function<std::string( const inventory_entry & )>( [ func ](
    const inventory_entry & entry ) {
        return func( entry.any_item() );
    } ), title, stub );
}

void inventory_selector_preset::append_cell( const
        std::function<std::string( const inventory_entry & )> &func,
        const std::string &title, const std::string &stub )
{
    const auto iter = std::find_if( cells.begin(), cells.end(), [ &title ]( const cell_t &cell ) {
        return cell.title == title;
    } );
    if( iter != cells.end() ) {
        debugmsg( "Tried to append a duplicate cell \"%s\": ignored.", title.c_str() );
        return;
    }
    cells.emplace_back( func, title, stub );
}

std::string inventory_selector_preset::cell_t::get_text( const inventory_entry &entry ) const
{
    return replace_colors( func( entry ) );
}

const item_location &inventory_holster_preset::get_holster() const
{
    return holster;
}

bool inventory_holster_preset::is_shown( const item_location &contained ) const
{
    if( contained == holster ) {
        return false;
    }
    if( contained.eventually_contains( holster ) || holster.eventually_contains( contained ) ) {
        return false;
    }
    if( !is_container( contained ) && contained->made_of( phase_id::LIQUID ) &&
        !contained->is_frozen_liquid() ) {
        // spilt liquid cannot be picked up
        return false;
    }
    //only hide if it is in the toplevel of holster (to allow shuffling of items inside a bag)
    for( const item *it : holster->all_items_top() ) {
        if( it == contained.get_item() ) {
            return false;
        }
    }

    return true;
}

std::string inventory_holster_preset::get_denial( const item_location &it ) const
{
    if( who->is_worn( *it ) ) {
        ret_val<void> const ret = who->can_takeoff( *it );
        if( !ret.success() ) {
            return ret.str();
        }
    }

    if( it->is_bucket_nonempty() ) {
        return "item would spill";
    }

    item item_copy( *it );
    item_copy.charges = 1;
    item_location parent = it.has_parent() ? it.parent_item() : item_location();

    ret_val<void> ret = holster->can_contain( item_copy, false, false, true, false, parent );
    if( !ret.success() ) {
        return !ret.str().empty() ? ret.str() : "item can't be stored there";
    }

    ret = holster.parents_can_contain_recursive( &item_copy );
    if( !ret.success() ) {
        return !ret.str().empty() ? ret.str() : "item can't be stored there";
    }

    return {};
}

void inventory_column::highlight( size_t new_index, scroll_direction dir )
{
    if( new_index < entries.size() ) {
        if( !entries[new_index].is_highlightable( skip_unselectable ) ) {
            new_index = next_highlightable_index( new_index, dir );
        }

        highlighted_index = new_index;
        page_offset = ( new_index == static_cast<size_t>( -1 ) ) ?
                      0 : highlighted_index - highlighted_index % entries_per_page;
    }
}

size_t inventory_column::next_highlightable_index( size_t index, scroll_direction dir ) const
{
    if( entries.empty() ) {
        return index;
    }
    // limit index to the space of the size of entries
    index = index % entries.size();
    size_t new_index = index;
    do {
        // 'new_index' incremented by 'dir' using division remainder (number of entries) to loop over the entries.
        // Negative step '-k' (backwards) is equivalent to '-k + N' (forward), where:
        //     N = entries.size()  - number of elements,
        //     k = |step|          - absolute step (k <= N).
        new_index = ( new_index + static_cast<int>( dir ) + entries.size() ) % entries.size();
    } while( new_index != index &&
             !entries[new_index].is_highlightable( skip_unselectable ) );

    if( !entries[new_index].is_highlightable( skip_unselectable ) ) {
        return static_cast<size_t>( -1 );
    }

    return new_index;
}

void inventory_column::move_selection( scroll_direction dir )
{
    size_t index = highlighted_index;

    do {
        index = next_highlightable_index( index, dir );
    } while( index != highlighted_index && is_selected_by_category( entries[index] ) );

    highlight( index, dir );
}

void inventory_column::move_selection_page( scroll_direction dir )
{
    size_t index = highlighted_index;

    do {
        const size_t next_index = next_highlightable_index( index, dir );
        const bool flipped = next_index == highlighted_index ||
                             ( next_index > highlighted_index ) != ( static_cast<int>( dir ) > 0 );

        if( flipped && page_of( next_index ) == page_index() ) {
            break; // If flipped and still on the same page - no need to flip
        }

        index = next_index;
    } while( page_of( next_highlightable_index( index, dir ) ) == page_index() );

    highlight( index, dir );
}

void inventory_column::scroll_selection_page( scroll_direction dir )
{
    size_t index = highlighted_index;

    while( page_of( index = next_highlightable_index( index, dir ) ) == page_index() ) {

        if( index == highlighted_index && page_of( index ) == page_index() ) {
            break; // If flipped and still on the same page - no need to flip
        }
    }

    highlight( index, dir );
}

size_t inventory_column::get_entry_cell_width( const inventory_entry &entry,
        size_t cell_index ) const
{
    size_t res = utf8_width( entry.get_entry_cell_cache( preset ).text[cell_index], true );

    if( cell_index == 0 ) {
        res += get_entry_indent( entry );
    }

    return res;
}

size_t inventory_column::get_cells_width() const
{
    return std::accumulate( cells.begin(), cells.end(), static_cast<size_t>( 0 ), []( size_t lhs,
    const cell_t &cell ) {
        return lhs + cell.current_width;
    } );
}

void inventory_column::set_filter( const std::string &/* filter */ )
{
    paging_is_valid = false;
}

void selection_column::set_filter( const std::string & )
{
    // always show all selected items
    inventory_column::set_filter( std::string() );
}

void inventory_entry::make_entry_cell_cache(
    inventory_selector_preset const &preset, bool update_only ) const
{
    if( update_only && !entry_cell_cache ) {
        return;
    }
    entry_cell_cache.emplace( entry_cell_cache_t{
        preset.get_color( *this ),
        { preset.get_cells_count(), std::string() } } );

    for( size_t i = 0, n = preset.get_cells_count(); i < n; ++i ) {
        entry_cell_cache->text[i] = preset.get_cell_text( *this, i );
    }
}

void inventory_entry::reset_entry_cell_cache() const
{
    entry_cell_cache.reset();
}

const inventory_entry::entry_cell_cache_t &inventory_entry::get_entry_cell_cache(
    inventory_selector_preset const &preset ) const
{
    //lang check here is needed to rebuild cache when using "Toggle language to English" option
    if( !entry_cell_cache ||
        entry_cell_cache->lang_version != detail::get_current_language_version() ) {
        make_entry_cell_cache( preset, false );
        cache_denial( preset );
        cata_assert( entry_cell_cache.has_value() );
        entry_cell_cache->lang_version = detail::get_current_language_version();
    }

    return *entry_cell_cache;
}

void inventory_column::set_width( const size_t new_width )
{
    int width_gap = get_width() - new_width;
    // Now adjust the width if we must
    while( width_gap != 0 ) {
        const int step = width_gap > 0 ? -1 : 1;
        // Should return true when lhs < rhs
        const auto cmp_for_expansion = []( const cell_t &lhs, const cell_t &rhs ) {
            return lhs.visible() && lhs.gap() < rhs.gap();
        };
        // Should return true when lhs < rhs
        const auto cmp_for_shrinking = []( const cell_t &lhs, const cell_t &rhs ) {
            if( !lhs.visible() ) {
                return false;
            }
            if( rhs.gap() <= min_cell_gap ) {
                return lhs.current_width < rhs.current_width;
            } else {
                return lhs.gap() < rhs.gap();
            }
        };
        const auto &cell = step > 0
                           ? std::min_element( cells.begin(), cells.end(), cmp_for_expansion )
                           : std::max_element( cells.begin(), cells.end(), cmp_for_shrinking );

        if( cell == cells.end() || !cell->visible() ) {
            break; // This is highly unlikely to happen, but just in case
        }
        cell->current_width += step;
        width_gap += step;
    }
    reserved_width = new_width;
}

void inventory_column::set_height( size_t new_height )
{
    if( height != new_height ) {
        if( new_height <= 1 ) {
            debugmsg( "Unable to assign height <= 1 (was %zd).", new_height );
            return;
        }
        height = new_height;
        entries_per_page = new_height;
        paging_is_valid = false;
    }
}

void inventory_column::expand_to_fit( inventory_entry &entry, bool with_denial )
{
    if( !entry ) {
        return;
    }

    entry.cache_denial( preset );
    cata_assert( entry.denial.has_value() );
    const std::string &denial = *entry.denial;

    for( size_t i = 0, num = with_denial && denial.empty() ? cells.size() : 1; i < num; ++i ) {
        auto &cell = cells[i];

        cell.real_width = std::max( cell.real_width, get_entry_cell_width( entry, i ) );

        // Don't reveal the cell for headers and stubs
        if( cell.visible() || ( entry.is_item() && !preset.is_stub_cell( entry, i ) ) ) {
            const size_t cell_gap = i > 0 ? normal_cell_gap : 0;
            cell.current_width = std::max( cell.current_width, cell_gap + cell.real_width );
        }
    }

    if( with_denial && !denial.empty() ) {
        reserved_width = std::max( get_entry_cell_width( entry, 0 ) + min_denial_gap + utf8_width( denial,
                                   true ),
                                   reserved_width );
    }
}

void inventory_column::reset_width( const std::vector<inventory_column *> & )
{
    for( inventory_column::cell_t &elem : cells ) {
        elem = cell_t();
    }
    reserved_width = 0;
    for( inventory_entry &elem : entries ) {
        expand_to_fit( elem );
    }
}

size_t inventory_column::page_of( size_t index ) const
{
    cata_assert( entries_per_page ); // To appease static analysis
    // NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
    return index / entries_per_page;
}

size_t inventory_column::page_of( const inventory_entry &entry ) const
{
    return page_of( std::distance( entries.begin(), std::find( entries.begin(), entries.end(),
                                   entry ) ) );
}
bool inventory_column::has_available_choices() const
{
    if( !allows_selecting() || !activatable() ) {
        return false;
    }
    return std::any_of( entries.begin(), entries.end(), []( inventory_entry const & e ) {
        return e.is_item() && e.enabled;
    } );
}

bool inventory_column::is_selected( const inventory_entry &entry ) const
{
    return ( entry.is_selectable() && entry == get_highlighted() ) || ( multiselect &&
            is_selected_by_category( entry ) );
}

bool inventory_column::is_highlighted( const inventory_entry &entry ) const
{
    return entry == get_highlighted() || ( multiselect && is_selected_by_category( entry ) );
}

bool inventory_column::is_selected_by_category( const inventory_entry &entry ) const
{
    return entry.is_selectable() && mode == navigation_mode::CATEGORY
           && entry.get_category_ptr() == get_highlighted().get_category_ptr()
           && page_of( entry ) == page_index();
}

const inventory_entry &inventory_column::get_highlighted() const
{
    if( highlighted_index >= entries.size() || !entries[highlighted_index].is_item() ) {
        // clang complains if we use the default constructor here
        static const inventory_entry dummy( nullptr );
        return dummy;
    }
    return entries[highlighted_index];
}

inventory_entry &inventory_column::get_highlighted()
{
    return const_cast<inventory_entry &>( const_cast<const inventory_column *>
                                          ( this )->get_highlighted() );
}

std::vector<inventory_entry *> inventory_column::get_all_selected() const
{
    const auto filter_to_selected = [&]( const inventory_entry & entry ) {
        return is_selected( entry );
    };
    return get_entries( filter_to_selected );
}

void inventory_column::_get_entries( get_entries_t *res, entries_t const &ent,
                                     const ffilter_t &filter_func ) const
{
    if( allows_selecting() ) {
        for( const inventory_entry &elem : ent ) {
            if( filter_func( elem ) ) {
                res->push_back( const_cast<inventory_entry *>( &elem ) );
            }
        }
    }
}

inventory_column::get_entries_t inventory_column::get_entries( const ffilter_t &filter_func,
        bool include_hidden ) const
{
    get_entries_t res;

    if( allows_selecting() ) {
        _get_entries( &res, entries, filter_func );
        if( include_hidden ) {
            _get_entries( &res, entries_hidden, filter_func );
        }
    }

    return res;
}

void inventory_column::set_stack_favorite( inventory_entry &entry,
        const bool favorite )
{
    for( item_location &loc : entry.locations ) {
        loc->set_favorite( favorite );
    }
    entry.make_entry_cell_cache( preset );
}

void inventory_column::set_collapsed( inventory_entry &entry, const bool collapse )
{
    bool collapsed = false;
    if( entry.is_collation_header() ) {
        entry.collation_meta->collapsed = collapse;
        collapsed = true;
    } else {
        std::vector<item_location> &locations = entry.locations;

        for( item_location &loc : locations ) {
            for( item_pocket *pocket : loc->get_all_standard_pockets() ) {
                pocket->settings.set_collapse( collapse );
                collapsed = true;
            }
        }
    }

    if( collapsed ) {
        entry.collapsed = collapse;
        paging_is_valid = false;
        entry.make_entry_cell_cache( preset );
    }
}

void inventory_column::on_input( const inventory_input &input )
{
    if( !empty() && active ) {
        if( input.action == "DOWN" ) {
            move_selection( scroll_direction::FORWARD );
        } else if( input.action == "UP" ) {
            move_selection( scroll_direction::BACKWARD );
        } else if( input.action == "PAGE_DOWN" ) {
            move_selection_page( scroll_direction::FORWARD );
        } else if( input.action == "PAGE_UP" ) {
            move_selection_page( scroll_direction::BACKWARD );
        } else if( input.action == "SCROLL_DOWN" ) {
            scroll_selection_page( scroll_direction::FORWARD );
        } else if( input.action == "SCROLL_UP" ) {
            scroll_selection_page( scroll_direction::BACKWARD );
        } else if( input.action == "HOME" ) {
            highlight( 0, scroll_direction::FORWARD );
        } else if( input.action == "END" ) {
            highlight( entries.size() - 1, scroll_direction::BACKWARD );
        } else if( input.action == "TOGGLE_FAVORITE" ) {
            inventory_entry &selected = get_highlighted();
            set_stack_favorite( selected, !selected.any_item()->is_favorite );
        } else if( input.action == "SHOW_HIDE_CONTENTS" ) {
            inventory_entry &selected = get_highlighted();
            selected.collapsed ? set_collapsed( selected, false ) : set_collapsed( selected, true );
        }
    }

    if( input.action == "TOGGLE_FAVORITE" ) {
        // Favoriting items in one column may change item names in another column
        // if that column contains an item that contains the favorited item. So
        // we invalidate every column on TOGGLE_FAVORITE action.
        paging_is_valid = false;
    }
}

void inventory_column::on_change( const inventory_entry &/* entry */ )
{
    // stub
}

inventory_entry *inventory_column::add_entry( const inventory_entry &entry )
{
    entries_t &dest = entry.is_hidden( hide_entries_override ) ? entries_hidden : entries;
    if( auto it = std::find( dest.begin(), dest.end(), entry ); it != dest.end() ) {
        debugmsg( "Tried to add a duplicate entry." );
        return &*it;
    }
    paging_is_valid = false;
    if( entry.is_item() ) {
        item_location entry_item = entry.locations.front();

        auto entry_with_loc = std::find_if( dest.begin(),
        dest.end(), [&entry, &entry_item, this]( const inventory_entry & e ) {
            if( !e.is_item() ) {
                return false;
            }
            item_location found_entry_item = e.locations.front();
            return !e.is_collated() &&
                   e.get_category_ptr() == entry.get_category_ptr() &&
                   entry_item.where() == found_entry_item.where() &&
                   entry_item.position() == found_entry_item.position() &&
                   entry_item.parent_item() == found_entry_item.parent_item() &&
                   entry_item->is_collapsed() == found_entry_item->is_collapsed() &&
                   entry_item->link_length() == found_entry_item->link_length() &&
                   entry_item->max_link_length() == found_entry_item->max_link_length() &&
                   entry_item->display_stacked_with( *found_entry_item, preset.get_checking_components() );
        } );
        if( entry_with_loc != dest.end() ) {
            std::vector<item_location> &locations = entry_with_loc->locations;
            std::move( entry.locations.begin(), entry.locations.end(), std::back_inserter( locations ) );
            return &*entry_with_loc;
        }
    }

    dest.emplace_back( entry );
    inventory_entry &newent = dest.back();
    newent.update_cache();

    return &newent;
}

void inventory_column::move_entries_to( inventory_column &dest )
{
    std::move( entries.begin(), entries.end(), std::back_inserter( dest.entries ) );
    std::move( entries_hidden.begin(), entries_hidden.end(),
               std::back_inserter( dest.entries_hidden ) );
    dest.paging_is_valid = false;
    clear();
}

bool inventory_column::sort_compare( inventory_entry const &lhs, inventory_entry const &rhs )
{
    if( lhs.is_selectable() != rhs.is_selectable() ) {
        return lhs.is_selectable(); // Disabled items always go last
    }
    Character &player_character = get_player_character();
    // Place favorite items and items with an assigned inventory letter first,
    // since the player cared enough to assign them
    const bool left_has_invlet =
        player_character.inv->assigned_invlet.count( lhs.any_item()->invlet ) != 0;
    const bool right_has_invlet =
        player_character.inv->assigned_invlet.count( rhs.any_item()->invlet ) != 0;
    if( left_has_invlet != right_has_invlet ) {
        return left_has_invlet;
    }
    const bool left_fav = lhs.any_item()->is_favorite;
    const bool right_fav = rhs.any_item()->is_favorite;
    if( left_fav != right_fav ) {
        return left_fav;
    }

    return preset.sort_compare( lhs, rhs );
}

bool inventory_column::indented_sort_compare( inventory_entry const &lhs,
        inventory_entry const &rhs )
{
    // place children below all parents
    parent_path_t const path_lhs = path_to_top( lhs, preset );
    parent_path_t const path_rhs = path_to_top( rhs, preset );
    parent_path_t::size_type const common_depth = std::min( path_lhs.size(), path_rhs.size() );
    parent_path_t::size_type li = path_lhs.size() - common_depth;
    parent_path_t::size_type ri = path_rhs.size() - common_depth;
    item_location p_lhs = path_lhs[li];
    item_location p_rhs = path_rhs[ri];
    if( p_lhs == p_rhs ) {
        return path_lhs.size() < path_rhs.size();
    }
    // otherwise sort the entries below their lowest common ancestor
    while( li < path_lhs.size() && path_lhs[li] != path_rhs[ri] ) {
        p_lhs = path_lhs[li++];
        p_rhs = path_rhs[ri++];
    }

    inventory_entry ep_lhs( { p_lhs }, nullptr, true, 0, lhs.generation );
    inventory_entry ep_rhs( { p_rhs }, nullptr, true, 0, rhs.generation );
    ep_lhs.update_cache();
    ep_rhs.update_cache();
    return sort_compare( ep_lhs, ep_rhs );
}

bool inventory_column::collated_sort_compare( inventory_entry const &lhs,
        inventory_entry const &rhs )
{
    if( lhs.is_collated() && lhs.collation_meta == rhs.collation_meta ) {
        if( lhs.is_collation_header() ) {
            return true;
        }
        if( rhs.is_collation_header() ) {
            return false;
        }
        return sort_compare( lhs, rhs );
    }

    item_location p_lhs = lhs.is_collated() ? lhs.collation_meta->tip : lhs.any_item();
    item_location p_rhs = rhs.is_collated() ? rhs.collation_meta->tip : rhs.any_item();

    inventory_entry ep_lhs(
    { p_lhs }, nullptr, lhs.is_collated() ? lhs.collation_meta->enabled : lhs.is_selectable(),
    0, lhs.generation );
    inventory_entry ep_rhs(
    { p_rhs }, nullptr, rhs.is_collated() ? rhs.collation_meta->enabled : rhs.is_selectable(),
    0, rhs.generation );
    ep_lhs.update_cache();
    ep_rhs.update_cache();
    if( indent_entries() ) {
        return indented_sort_compare( ep_lhs, ep_rhs );
    }
    return sort_compare( ep_lhs, ep_rhs );
}

void inventory_column::collate()
{
    for( auto outer = entries.begin(); outer != entries.end(); ++outer ) {
        if( !outer->is_item() || outer->is_collated() || outer->chevron ) {
            continue;
        }
        for( auto e = std::next( outer ); e != entries.end(); ) {
            if( e->is_item() && e->get_category_ptr() == outer->get_category_ptr() &&
                e->any_item()->is_favorite == outer->any_item()->is_favorite &&
                e->any_item()->typeId() == outer->any_item()->typeId() &&
                std::min( 0, e->any_item()->link_length() ) == std::min( 0, outer->any_item()->link_length() ) &&
                e->any_item()->max_link_length() == outer->any_item()->max_link_length() &&
                ( !indent_entries() ||
                  e->any_item().parent_item() == outer->any_item().parent_item() ) &&
                ( e->is_collation_header() || !e->chevron ) &&
                e->any_item()->is_same_relic( *outer->any_item() ) ) {

                if( !outer->is_collated() ) {
                    outer->collation_meta = std::make_shared<collation_meta_t>(
                                                collation_meta_t{ outer->any_item(), true, outer->is_selectable() } );

                    entries_hidden.emplace_back( *outer );

                    outer->chevron = true;
                    set_collapsed( *outer, true );
                    outer->reset_entry_cell_cache(); // needed when switching UI modes
                }
                e->collation_meta = outer->collation_meta;
                std::copy( e->locations.begin(), e->locations.end(),
                           std::back_inserter( outer->locations ) );
                entries_hidden.emplace_back( std::move( *e ) );
                e = entries.erase( e );
            } else {
                ++e;
            }
        }
    }
    _collated = true;
}

void inventory_column::_reset_collation( entries_t &entries )
{
    for( auto iter = entries.begin(); iter != entries.end(); ) {
        if( iter->is_collation_header() ) {
            iter = entries.erase( iter );
        } else {
            iter->reset_collation();
            ++iter;
        }
    }
}

void inventory_column::uncollate()
{
    if( _collated ) {
        _reset_collation( entries );
        _reset_collation( entries_hidden );
        _collated = false;
    }
}

void inventory_column::prepare_paging( const std::string &filter )
{
    if( paging_is_valid ) {
        return;
    }

    const auto filter_fn = filter_from_string<inventory_entry>(
    filter, [this]( const std::string & filter ) {
        return preset.get_filter( filter );
    } );

    const auto is_visible = [&filter_fn, &filter, this]( inventory_entry const & it ) {
        return it.is_item() &&
               ( filter_fn( it ) &&
                 ( ( !filter.empty() && !it.is_collation_entry() ) || !it.is_hidden( hide_entries_override ) ) );
    };
    const auto is_not_visible = [&is_visible, this]( inventory_entry const & it ) {
        it.cache_denial( preset ); // do it here since we're looping over all visible entries anyway
        return !is_visible( it );
    };

    // restore entries revealed by SHOW_HIDE_CONTENTS or filter
    move_if( entries_hidden, entries, is_visible );
    // remove entries hidden by SHOW_HIDE_CONTENTS
    move_if( entries, entries_hidden, is_not_visible );

    // Then sort them with respect to categories
    std::stable_sort( entries.begin(), entries.end(),
    [this]( const inventory_entry & lhs, const inventory_entry & rhs ) {
        if( *lhs.get_category_ptr() == *rhs.get_category_ptr() ) {
            if( _collated ) {
                return collated_sort_compare( lhs, rhs );
            }
            if( indent_entries() ) {
                return indented_sort_compare( lhs, rhs );
            }

            return sort_compare( lhs, rhs );
        }
        return preset.cat_sort_compare( lhs, rhs );
    } );

    if( !_collated && collate_entries() ) {
        collate();
    }

    // Recover categories
    const item_category *current_category = nullptr;
    for( auto iter = entries.begin(); iter != entries.end(); ++iter ) {
        if( iter->get_category_ptr() == current_category ) {
            continue;
        }
        current_category = iter->get_category_ptr();
        iter = entries.insert( iter, inventory_entry( current_category ) );
    }
    // Determine the new height.
    entries_per_page = height;
    if( entries.size() > entries_per_page ) {
        entries_per_page -= 1;  // Make room for the page number.
        for( size_t i = entries_per_page - 1; i < entries.size(); i += entries_per_page ) {
            auto iter = std::next( entries.begin(), i );
            if( iter->is_category() ) {
                // The last item on the page must not be a category.
                entries.insert( iter, inventory_entry() );
            } else {
                // The first item on the next page must be a category.
                iter = std::next( iter );
                if( iter != entries.end() && iter->is_item() ) {
                    entries.insert( iter, inventory_entry( iter->get_category_ptr() ) );
                }
            }
        }
    }
    paging_is_valid = true;
    // Select the uppermost possible entry
    const size_t ind = highlighted_index >= entries.size() ? 0 : highlighted_index;
    highlight( ind, ind ? scroll_direction::BACKWARD : scroll_direction::FORWARD );
}

void inventory_column::clear()
{
    entries.clear();
    entries_hidden.clear();
    paging_is_valid = false;
}

bool inventory_column::highlight( const item_location &loc, bool front_only )
{
    for( size_t index = 0; index < entries.size(); ++index ) {
        inventory_entry &ent = entries[index];
        if( ent.is_item() &&
            ( ( !front_only && std::find( ent.locations.begin(), ent.locations.end(), loc ) !=
                ent.locations.end() ) ||
              ( !ent.is_collation_header() && ent.locations.front() == loc ) ) ) {
            highlight( index, scroll_direction::FORWARD );
            return true;
        }
    }
    return false;
}

size_t inventory_column::get_entry_indent( const inventory_entry &entry ) const
{
    if( !entry.is_item() ) {
        return 0;
    }

    size_t res = 2;
    if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
        res += 2;
    }
    if( allows_selecting() && activatable() && multiselect ) {
        res += 2;
    }
    if( entry.is_item() ) {
        if( collate_entries() && entry.is_collation_entry() ) {
            res += 2;
        }
        if( indent_entries() ) {
            res += entry.indent;
        }
    }

    return res;
}

int inventory_column::reassign_custom_invlets( const Character &p, int min_invlet, int max_invlet )
{
    int cur_invlet = min_invlet;
    for( inventory_entry &elem : entries ) {
        // Only items on map/in vehicles: those that the player does not possess.
        if( elem.is_selectable() && !p.has_item( *elem.any_item() ) ) {
            elem.custom_invlet = cur_invlet <= max_invlet ? cur_invlet++ : '\0';
        }
    }
    return cur_invlet;
}

int inventory_column::reassign_custom_invlets( int cur_idx, const std::string_view pickup_chars )
{
    for( inventory_entry &elem : entries ) {
        // Only items on map/in vehicles: those that the player does not possess.
        if( elem.is_selectable() && elem.any_item()->invlet <= '\0' ) {
            elem.custom_invlet =
                static_cast<uint8_t>(
                    cur_idx < static_cast<int>( pickup_chars.size() ) ?
                    pickup_chars[cur_idx] : '\0'
                );
            cur_idx++;
        }
    }
    return cur_idx;
}

void inventory_column::draw( const catacurses::window &win, const point &p,
                             std::vector<std::pair<inclusive_rectangle<point>, inventory_entry *>> &rect_entry_map )
{
    if( !visible() ) {
        return;
    }
    const auto available_cell_width = [ this ]( inventory_entry const & entry, size_t cell_index ) {
        const size_t displayed_width = cells[cell_index].current_width;
        const size_t real_width = get_entry_cell_width( entry, cell_index );

        return displayed_width > real_width ? displayed_width - real_width : 0;
    };

    // Do the actual drawing
    for( size_t index = page_offset, line = 0; index < entries.size() &&
         line < entries_per_page; ++index, ++line ) {
        inventory_entry &entry = entries[index];

        if( !entry ) {
            continue;
        }
        const inventory_entry::entry_cell_cache_t &entry_cell_cache = entry.get_entry_cell_cache( preset );

        int x1 = p.x + get_entry_indent( entry );
        int x2 = p.x + std::max( static_cast<int>( reserved_width - get_cells_width() ), 0 );
        int yy = p.y + line;

        const bool selected = active && is_highlighted( entry );

        const int hx_max = p.x + get_width();
        inclusive_rectangle<point> rect = inclusive_rectangle<point>( point( x1, yy ),
                                          point( hx_max - 1, yy ) );
        rect_entry_map.emplace_back( rect,
                                     &entry );

        if( selected && visible_cells() > 1 ) {
            for( int hx = x1; hx < hx_max; ++hx ) {
                mvwputch( win, point( hx, yy ), h_white, ' ' );
            }
        }

        cata_assert( entry.denial.has_value() );
        const std::string &denial = *entry.denial;

        if( !denial.empty() ) {
            // Determine the width available for the first cell to print, then use that to trim the denial
            const size_t first_cell_width = std::min( get_entry_cell_width( entry, 0 ),
                                            x2 + cells[0].current_width - min_denial_gap );
            const size_t max_denial_width = std::max( static_cast<int>( get_width() - ( min_denial_gap +
                                            first_cell_width ) ), 0 );
            const size_t denial_width = std::min( max_denial_width, static_cast<size_t>( utf8_width( denial,
                                                  true ) ) );

            if( denial_width > 0 ) {
                /* Need to determine exact point to start printing from the left, since just trimming produces
                 * artifacts on languages with wide characters, and right_print expects only a second column */
                std::string trimmed = trim_by_length( denial, denial_width );
                const int x = p.x + get_width() - utf8_width( remove_color_tags( trimmed ) );
                nc_color temp = c_red;
                print_colored_text( win, point( x, yy ), temp, c_red,
                                    selected ? hilite_string( colorize( trimmed, c_red ) ) : trimmed );
                entry.cached_denial_space = denial_width;
            }
        }

        size_t count = denial.empty() ? cells.size() : 1;

        for( size_t cell_index = 0; cell_index < count; ++cell_index ) {
            if( !cells[cell_index].visible() ) {
                continue; // Don't show empty cells
            }

            if( line != 0 && cell_index != 0 && entry.is_category() ) {
                break; // Don't show duplicated titles
            }

            x2 += cells[cell_index].current_width;
            std::string text = entry_cell_cache.text[cell_index];

            size_t text_width = utf8_width( text, true );
            size_t text_gap = cell_index > 0 ? std::max( cells[cell_index].gap(), min_cell_gap ) : 0;
            size_t available_width = x2 - x1 - text_gap;

            if( text_width > available_width ) {
                // See if we can steal some of the needed width from an adjacent cell
                if( cell_index == 0 && count >= 2 ) {
                    available_width += available_cell_width( entry, 1 );
                } else if( cell_index > 0 ) {
                    available_width += available_cell_width( entry, cell_index - 1 );
                }
                text_width = std::min( text_width, available_width );
            }

            if( text_width > 0 ) {
                const int text_x = cell_index == 0 ? x1 : x2 -
                                   text_width; // Align either to the left or to the right

                const std::string &hl_option = get_option<std::string>( "INVENTORY_HIGHLIGHT" );
                if( cell_index == 0 && entry.chevron ) {
                    bool const hide_override = hide_entries_override && entry.any_item()->is_container();
                    nc_color const col = entry.is_collation_header() ? c_light_blue : hide_override ?
                                         *hide_entries_override ? c_red : c_green : c_dark_gray;
                    bool const stat = entry.is_collation_entry() ||
                                      !hide_override ? entry.collapsed : *hide_entries_override;
                    trim_and_print( win, point( text_x - 1, yy ), 1, col,
                                    stat ? "▶" : "▼" );
                }
                if( entry.is_item() && ( selected || !entry.is_selectable() ) ) {
                    trim_and_print( win, point( text_x, yy ), text_width, selected ? h_white : c_dark_gray,
                                    remove_color_tags( text ) );
                } else if( entry.is_item() && entry.highlight_as_parent ) {
                    if( hl_option == "symbol" ) {
                        trim_and_print( win, point( text_x - 1, yy ), 1, h_white, "<" );
                        trim_and_print( win, point( text_x, yy ), text_width, entry_cell_cache.color, text );
                    } else {
                        trim_and_print( win, point( text_x, yy ), text_width, c_white_white,
                                        remove_color_tags( text ) );
                    }
                    entry.highlight_as_parent = false;
                } else if( entry.is_item() && entry.highlight_as_child ) {
                    if( hl_option == "symbol" ) {
                        trim_and_print( win, point( text_x - 1, yy ), 1, h_white, ">" );
                        trim_and_print( win, point( text_x, yy ), text_width, entry_cell_cache.color, text );
                    } else {
                        trim_and_print( win, point( text_x, yy ), text_width, c_black_white,
                                        remove_color_tags( text ) );
                    }
                    entry.highlight_as_child = false;
                } else {
                    trim_and_print( win, point( text_x, yy ), text_width, entry_cell_cache.color, text );
                }
            }

            x1 = x2;
        }

        if( entry.is_item() ) {
            int xx = p.x;
            if( entry.get_invlet() != '\0' ) {
                mvwputch( win, point( p.x, yy ), entry.get_invlet_color(), entry.get_invlet() );
            }
            xx += 2;
            if( get_option<bool>( "ITEM_SYMBOLS" ) ) {
                const nc_color color = entry.any_item()->color();
                mvwputch( win, point( xx, yy ), color, entry.any_item()->symbol() );
                xx += 2;
            }
            if( allows_selecting() && activatable() && multiselect ) {
                if( entry.chosen_count == 0 ) {
                    mvwputch( win, point( xx, yy ), c_dark_gray, '-' );
                } else if( entry.chosen_count >= entry.get_available_count() ) {
                    mvwputch( win, point( xx, yy ), c_light_green, '+' );
                } else {
                    mvwputch( win, point( xx, yy ), c_light_green, '#' );
                }
            }
        }
    }

    if( pages_count() > 1 ) {
        mvwprintw( win, p + point( 0, height - 1 ), _( "Page %d/%d" ), page_index() + 1, pages_count() );
    }
}

size_t inventory_column::visible_cells() const
{
    return std::count_if( cells.begin(), cells.end(), []( const cell_t &elem ) {
        return elem.visible();
    } );
}

selection_column::selection_column( const std::string &id, const std::string &name ) :
    inventory_column( selection_preset ),
    selected_cat( id, no_translation( name ), translation(), 0 )
{
    hide_entries_override = { false };
}

selection_column::~selection_column() = default;

void selection_column::reset_width( const std::vector<inventory_column *> &all_columns )
{
    inventory_column::reset_width( all_columns );

    for( const inventory_column *const col : all_columns ) {
        if( col && !dynamic_cast<const selection_column *>( col ) ) {
            for( inventory_entry *const ent : col->get_entries( always_yes ) ) {
                if( ent ) {
                    expand_to_fit( *ent, false );
                }
            }
        }
    }
}

void selection_column::prepare_paging( const std::string & )
{
    // always show all selected items
    inventory_column::prepare_paging( std::string() );

    if( entries.empty() ) { // Category must always persist
        entries.emplace_back( &*selected_cat );
        expand_to_fit( entries.back(), false );
    }

    if( !last_changed.is_null() ) {
        const auto iter = std::find( entries.begin(), entries.end(), last_changed );
        if( iter != entries.end() ) {
            highlight( std::distance( entries.begin(), iter ), scroll_direction::FORWARD );
        }
        last_changed = inventory_entry();
    }
}

void selection_column::on_change( const inventory_entry &entry )
{
    inventory_entry my_entry( entry, &*selected_cat );

    auto iter = std::find( entries.begin(), entries.end(), my_entry );

    if( iter == entries.end() ) {
        if( my_entry.chosen_count == 0 ) {
            return; // Not interested.
        }
        add_entry( my_entry )->make_entry_cell_cache( preset );
        paging_is_valid = false;
        last_changed = my_entry;
    } else if( iter->chosen_count != my_entry.chosen_count ) {
        if( my_entry.chosen_count > 0 ) {
            iter->chosen_count = my_entry.chosen_count;
            iter->make_entry_cell_cache( preset );
        } else {
            iter = entries.erase( iter );
        }
        paging_is_valid = false;
        if( iter != entries.end() ) {
            last_changed = *iter;
        }
    }
}
const item_category *inventory_selector::naturalize_category( const item_category &category,
        const tripoint &pos )
{
    const auto find_cat_by_id = [ this ]( const item_category_id & id ) {
        const auto iter = std::find_if( categories.begin(),
        categories.end(), [ &id ]( const item_category & cat ) {
            return cat.get_id() == id;
        } );
        return iter != categories.end() ? &*iter : nullptr;
    };

    const int dist = rl_dist( u.pos(), pos );

    if( dist != 0 ) {
        const std::string suffix = direction_suffix( u.pos(), pos );
        const item_category_id id = item_category_id( string_format( "%s_%s", category.get_id().c_str(),
                                    suffix.c_str() ) );

        const auto *existing = find_cat_by_id( id );
        if( existing != nullptr ) {
            return existing;
        }

        const std::string name = string_format( "%s %s", category.name_header(), suffix.c_str() );
        const int sort_rank = category.sort_rank() + dist;
        const item_category new_category( id, no_translation( name ), translation(), sort_rank );

        categories.push_back( new_category );
    } else {
        const item_category *const existing = find_cat_by_id( category.get_id() );
        if( existing != nullptr ) {
            return existing;
        }

        categories.push_back( category );
    }

    return &categories.back();
}

inventory_entry *inventory_selector::add_entry( inventory_column &target_column,
        std::vector<item_location> &&locations,
        const item_category *custom_category,
        const size_t chosen_count, item_location const &topmost_parent,
        bool chevron )
{
    if( !preset.is_shown( locations.front() ) ) {
        return nullptr;
    }

    is_empty = false;
    inventory_entry entry( locations, custom_category,
                           true, chosen_count,
                           entry_generation_number++, topmost_parent, chevron );

    entry.collapsed = locations.front()->is_collapsed();
    inventory_entry *const ret = target_column.add_entry( entry );

    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->mark_resize();
    }

    return ret;
}

bool inventory_selector::add_entry_rec( inventory_column &entry_column,
                                        inventory_column &children_column, item_location &loc,
                                        item_category const *entry_category,
                                        item_category const *children_category,
                                        item_location const &topmost_parent, int indent )
{
    inventory_column temp_children( preset );
    bool vis_contents =
        add_contained_items( loc, temp_children, children_category,
                             get_topmost_parent( topmost_parent, loc, preset ),
                             preset.is_shown( loc ) ? indent + 2 : indent );
    inventory_entry *const nentry = add_entry( entry_column, std::vector<item_location>( 1, loc ),
                                    entry_category, 0, topmost_parent );
    if( nentry != nullptr ) {
        nentry->chevron = vis_contents;
        nentry->indent = indent;
        vis_contents = true;
    }
    temp_children.move_entries_to( children_column );

    return vis_contents;
}

bool inventory_selector::drag_drop_item( item *sourceItem, item *destItem )
{
    if( sourceItem == destItem ) {
        return false;
    }
    auto pockets = destItem->get_all_contained_pockets();
    if( pockets.empty() ) {
        popup( _( "Destination has no pockets." ) );
        return false;
    }
    int base_move_cost = 0;
    auto parentContainers = u.parents( *sourceItem );
    for( item *parent : parentContainers ) {
        base_move_cost += parent->obtain_cost( *sourceItem );
    }
    std::vector<item_pocket *> containable_pockets;
    uilist action_menu;
    action_menu.text = _( "Select pocket" );
    int index = 0;
    bool any_containable = false;
    for( item_pocket *pocket : pockets ) {
        std::string pocket_text = string_format( _( "%s - %s/%s | %d moves" ),
                                  pocket->get_description().translated(), vol_to_info( "", "", pocket->contains_volume() ).sValue,
                                  vol_to_info( "", "", pocket->max_contains_volume() ).sValue, pocket->moves() + base_move_cost );
        action_menu.addentry( index++, true, std::optional<input_event>(), pocket_text );
        ret_val<item_pocket::contain_code> contain = pocket->can_contain( *sourceItem );
        if( contain.success() ) {
            any_containable = true;
        } else {
            action_menu.entries.back().enabled = false;
        }
    }
    if( !any_containable ) {
        tname::segment_bitset segs( tname::unprefixed_tname );
        segs.set( tname::segments::CONTENTS, false );
        popup( string_format( _( "%s contains no pockets that can contain item." ),
                              destItem->tname( 1, segs ) ) );
        return false;
    }
    action_menu.query();
    if( action_menu.ret >= 0 && static_cast<size_t>( action_menu.ret ) < pockets.size() ) {
        ret_val<item_pocket::contain_code> contain = pockets[action_menu.ret]->can_contain( *sourceItem );
        if( contain.success() ) {
            // storage should mimick character inserting
            u.store( pockets[action_menu.ret], *sourceItem, true, base_move_cost );
            return true;
        } else {
            popup( _( "Cannot put item in pocket: %s" ), contain.str() );
            return false;
        }
    }

    return false;
}

bool inventory_selector::add_contained_items( item_location &container )
{
    return add_contained_items( container, own_inv_column );
}

bool inventory_selector::add_contained_items( item_location &container, inventory_column &column,
        const item_category *const custom_category, item_location const &topmost_parent, int indent )
{
    if( container->has_flag( STATIC( flag_id( "NO_UNLOAD" ) ) ) ) {
        return false;
    }

    std::list<item *> const items = preset.get_pocket_type() == pocket_type::LAST
                                    ? container->all_items_top()
                                    : container->all_items_top( preset.get_pocket_type() );

    bool vis_top = false;
    inventory_column temp( preset );
    for( item *it : items ) {
        item_location child( container, it );
        item_category const *hacked_cat = custom_category;
        inventory_column *hacked_col = &column;
        if( is_worn_ablative( container, child ) ) {
            hacked_cat = &item_category_ITEMS_WORN.obj();
            hacked_col = &own_gear_column;
        }
        vis_top |= add_entry_rec( *hacked_col, temp, child, hacked_cat, custom_category,
                                  topmost_parent, indent );
    }
    temp.move_entries_to( column );
    return vis_top;
}

void inventory_selector::add_contained_gunmods( Character &you, item &gun )
{
    auto filter_irremovable = []( std::vector<item *> &gunmods ) {
        gunmods.erase(
            std::remove_if(
        gunmods.begin(), gunmods.end(), []( const item * i ) {
            return i->is_irremovable();
        } ),
        gunmods.end() );
    };

    std::vector<item *> mods = gun.gunmods();
    item_location gun_item_location( you, &gun );

    filter_irremovable( mods );

    for( item *mod : mods ) {
        item_location gunmod_item_location( gun_item_location, mod );
        add_entry( own_inv_column, std::vector<item_location>( 1, gunmod_item_location ) );
    }
}

void inventory_selector::add_contained_ebooks( item_location &container )
{
    if( !container->is_ebook_storage() ) {
        return;
    }

    for( item *it : container->get_contents().ebooks() ) {
        item_location child( container, it );
        add_entry( own_inv_column, std::vector<item_location>( 1, child ) );
    }
}

void inventory_selector::add_character_items( Character &character )
{
    item_location weapon = character.get_wielded_item();
    bool const hierarchy = _uimode == uimode::hierarchy;
    if( weapon ) {
        add_entry_rec( own_gear_column, hierarchy ? own_gear_column : own_inv_column, weapon,
                       &item_category_WEAPON_HELD.obj(),
                       hierarchy ? &item_category_WEAPON_HELD.obj() : nullptr );
    }
    for( item_location &worn_item : character.top_items_loc() ) {
        item_category const *const custom_cat = wielded_worn_category( worn_item, u );
        add_entry_rec( own_gear_column, hierarchy ? own_gear_column : own_inv_column, worn_item,
                       custom_cat, hierarchy ? custom_cat : nullptr );
    }
    if( !hierarchy ) {
        own_inv_column.set_indent_entries_override( false );
    }
}

void inventory_selector::add_map_items( const tripoint &target )
{
    map &here = get_map();
    if( here.accessible_items( target ) ) {
        map_stack items = here.i_at( target );
        const std::string name = to_upper_case( here.name( target ) );
        const item_category map_cat( name, no_translation( name ), translation(), 100 );
        _add_map_items( target, map_cat, items, [target]( item & it ) {
            return item_location( map_cursor( target ), &it );
        } );
    }
}

void inventory_selector::add_vehicle_items( const tripoint &target )
{
    const std::optional<vpart_reference> ovp = get_map().veh_at( target ).cargo();
    if( !ovp ) {
        return;
    }
    vehicle_part &vp = ovp->part();
    vehicle_stack items = ovp->items();
    const std::string name = to_upper_case( remove_color_tags( vp.name() ) );
    const item_category vehicle_cat( name, no_translation( name ), translation(), 200 );
    const vehicle_cursor cursor( ovp->vehicle(), ovp->part_index() );
    _add_map_items( target, vehicle_cat, items, [&cursor]( item & it ) {
        return item_location( cursor, &it );
    } );
}

void inventory_selector::_add_map_items( tripoint const &target, item_category const &cat,
        item_stack &items, std::function<item_location( item & )> const &floc )
{
    bool const hierarchy = _uimode == uimode::hierarchy;
    item_category const *const custom_cat = hierarchy ? naturalize_category( cat, target ) : nullptr;
    inventory_column *const col = _categorize_map_items ? &own_inv_column : &map_column;

    inventory_column temp( preset );
    inventory_column temp_cont( preset );
    for( item &it : items ) {
        item_location loc = floc( it );
        add_entry_rec( temp, temp_cont, loc, custom_cat, custom_cat );
    }

    temp.move_entries_to( *col );
    temp_cont.move_entries_to( *col );

    if( !hierarchy ) {
        col->set_indent_entries_override( false );
    }
}

void inventory_selector::add_nearby_items( int radius )
{
    if( radius >= 0 ) {
        map &here = get_map();
        for( const tripoint &pos : closest_points_first( u.pos(), radius ) ) {
            // can not reach this -> can not access its contents
            if( u.pos() != pos && !here.clear_path( u.pos(), pos, rl_dist( u.pos(), pos ), 1, 100 ) ) {
                continue;
            }
            add_map_items( pos );
            add_vehicle_items( pos );
        }
    }
}

void inventory_selector::add_remote_map_items( tinymap *remote_map, const tripoint &target )
{
    map_stack items = remote_map->i_at( target );
    const std::string name = to_upper_case( remote_map->name( target ) );
    const item_category map_cat( name, no_translation( name ), translation(), 100 );
    _add_map_items( target, map_cat, items, [target]( item & it ) {
        return item_location( map_cursor( target ), &it );
    } );
}

void inventory_selector::add_basecamp_items( const basecamp &camp )
{
    std::unordered_set<tripoint_abs_ms> tiles = camp.get_storage_tiles();
    map &here = get_map();
    for( tripoint_abs_ms tile : tiles ) {
        add_map_items( here.bub_from_abs( tile ).raw() );
    }
}

void inventory_selector::clear_items()
{
    is_empty = true;
    for( inventory_column *&column : columns ) {
        column->clear();
    }
    own_inv_column.clear();
    own_gear_column.clear();
    map_column.clear();
}

bool inventory_selector::highlight( const item_location &loc, bool hidden, bool front_only )
{
    bool res = false;

    for( size_t i = 0; i < columns.size(); ++i ) {
        inventory_column *elem = columns[i];
        if( !elem->visible() ) {
            continue;
        }
        bool found = false;
        if( hidden ) {
            if( inventory_entry *ent = elem->find_by_location( loc, true ) ) {
                item_location const &parent = ent->is_collation_entry() ? ent->collation_meta->tip :
                                              ent->topmost_parent;
                if( !parent ) {
                    continue;
                }
                inventory_entry *pent = elem->find_by_location( parent );
                if( elem != nullptr ) {
                    elem->set_collapsed( *pent, false );
                    prepare_layout();
                    found = elem->highlight( loc, true );
                }
            }
        } else {
            found = elem->highlight( loc, front_only );
        }
        if( found && !res && elem->activatable() ) {
            set_active_column( i );
            res = true;
        }
    }

    return res;
}

item_location inventory_selector::get_collation_next() const
{
    inventory_column &col = get_active_column();
    inventory_entry &ent = col.get_highlighted();
    if( !ent.is_collated() ) {
        return {};
    }
    if( ent.is_collation_header() ) {
        return ent.locations.front();
    }

    item_location const &loc_cur = ent.locations.front();
    inventory_entry const &p_ent = *col.find_by_location( ent.collation_meta->tip );
    auto iter = std::find( p_ent.locations.begin(), p_ent.locations.end(), loc_cur );
    if( iter == --p_ent.locations.end() ) {
        return *--iter;
    }

    return *++iter;
}

bool inventory_selector::highlight_one_of( const std::vector<item_location> &locations,
        bool hidden )
{
    prepare_layout();
    for( const item_location &loc : locations ) {
        if( loc && highlight( loc, hidden ) ) {
            return true;
        }
    }
    return false;
}

inventory_entry *inventory_selector::find_entry_by_invlet( int invlet ) const
{
    for( const inventory_column *elem : columns ) {
        inventory_entry *const res = elem->find_by_invlet( invlet );
        if( res != nullptr ) {
            return res;
        }
    }
    return nullptr;
}

inventory_entry *inventory_selector::find_entry_by_coordinate( const point &coordinate ) const
{
    for( auto pair : rect_entry_map ) {
        if( pair.first.contains( coordinate ) ) {
            return pair.second;
        }
    }
    return nullptr;
}

inventory_entry *inventory_selector::find_entry_by_location( item_location &loc ) const
{
    for( const inventory_column *elem : columns ) {
        if( elem->allows_selecting() ) {
            inventory_entry *const res = elem->find_by_location( loc );
            if( res != nullptr ) {
                return res;
            }
        }
    }
    return nullptr;
}

// FIXME: if columns are merged due to low screen width, they will not be splitted
// once screen width becomes enough for the columns.
void inventory_selector::rearrange_columns( size_t client_width )
{
    const inventory_entry &prev_entry = get_highlighted();
    const item_location prev_selection = prev_entry.is_item() ?
                                         prev_entry.any_item() : item_location::nowhere;
    bool const front_only = prev_entry.is_collation_entry();
    if( ( is_overflown( client_width ) || force_single_column ) && !own_gear_column.empty() ) {
        if( own_inv_column.empty() ) {
            own_inv_column.set_indent_entries_override( own_gear_column.indent_entries() );
        }
        own_gear_column.move_entries_to( own_inv_column );
        own_inv_column.reset_width( {} );
    }
    if( ( is_overflown( client_width ) || force_single_column ) && !map_column.empty() ) {
        if( own_inv_column.empty() ) {
            own_inv_column.set_indent_entries_override( map_column.indent_entries() );
        }
        map_column.move_entries_to( own_inv_column );
        own_inv_column.reset_width( {} );
    }
    if( prev_selection ) {
        highlight( prev_selection, false, front_only );
    }
}

void inventory_selector::prepare_layout( size_t client_width, size_t client_height )
{
    // This block adds categories and should go before any width evaluations
    const bool initial = get_active_column().get_highlighted_index() == static_cast<size_t>( -1 );
    for( inventory_column *&elem : columns ) {
        elem->set_height( client_height );
        elem->prepare_paging( filter );
        elem->reset_width( columns );
    }

    // Handle screen overflow
    rearrange_columns( client_width );
    if( initial ) {
        get_active_column().highlight( 0, scroll_direction::FORWARD );
    }
    // If we have a single column and it occupies more than a half of
    // the available with -> expand it
    auto visible_columns = get_visible_columns();
    if( visible_columns.size() == 1 && are_columns_centered( client_width ) ) {
        visible_columns.front()->set_width( client_width );
    }

    reassign_custom_invlets();

    refresh_active_column();
}

void inventory_selector::reassign_custom_invlets()
{
    if( invlet_type_ == SELECTOR_INVLET_DEFAULT || invlet_type_ == SELECTOR_INVLET_NUMERIC ) {
        bool use_num_invlet = uistate.numpad_navigation ? false : use_invlet;
        int min_invlet = static_cast<uint8_t>( use_num_invlet ? '0' : '\0' );
        for( inventory_column *elem : columns ) {
            elem->prepare_paging();
            min_invlet = elem->reassign_custom_invlets( u, min_invlet, use_num_invlet ? '9' : '\0' );
        }
    } else if( invlet_type_ == SELECTOR_INVLET_ALPHA ) {
        const std::string all_pickup_chars = use_invlet ?
                                             "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ:;" : "";
        std::string pickup_chars = ctxt.get_available_single_char_hotkeys( all_pickup_chars );
        int cur_idx = 0;
        auto elemfilter = []( const inventory_entry & e ) {
            return e.is_item() && e.any_item()->invlet > '\0';
        };
        // First pass -> remove letters taken by user-set invlets
        for( inventory_column *elem : columns ) {
            for( inventory_entry *e : elem->get_entries( elemfilter ) ) {
                const char c = e->any_item()->invlet;
                if( pickup_chars.find_first_of( c ) != std::string::npos ) {
                    pickup_chars.erase( std::remove( pickup_chars.begin(), pickup_chars.end(), c ),
                                        pickup_chars.end() );
                }
            }
        }
        for( inventory_column *elem : columns ) {
            elem->prepare_paging();
            cur_idx = elem->reassign_custom_invlets( cur_idx, pickup_chars );
        }
    }
}

void inventory_selector::prepare_layout()
{
    startup_timer const tp_prep =
        std::chrono::time_point_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() );

    const auto snap = []( size_t cur_dim, size_t max_dim ) {
        return cur_dim + 2 * max_win_snap_distance >= max_dim ? max_dim : cur_dim;
    };

    const int nc_width = 2 * ( 1 + border );
    const int nc_height = get_header_height() + 1 + 2 * border;

    prepare_layout( TERMX - nc_width, TERMY - nc_height );

    int const win_width =
        _fixed_size.x < 0 ? snap( get_layout_width() + nc_width, TERMX ) : _fixed_size.x;
    int const win_height =
        _fixed_size.y < 0
        ? snap( std::max<int>( get_layout_height() + nc_height, FULL_SCREEN_HEIGHT ), TERMY )
        : _fixed_size.y;

    prepare_layout( win_width - nc_width, win_height - nc_height );

    resize_window( win_width, win_height );

    debug_print_timer( tp_prep, "prepare_layout took" );
}

shared_ptr_fast<ui_adaptor> inventory_selector::create_or_get_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( !current_ui ) {
        ui = current_ui = make_shared_fast<ui_adaptor>();
        current_ui->on_screen_resize( [this]( ui_adaptor & ) {
            prepare_layout();
        } );
        current_ui->mark_resize();

        current_ui->on_redraw( [this]( const ui_adaptor & ) {
            refresh_window();
        } );
    }
    return current_ui;
}

size_t inventory_selector::get_layout_width() const
{
    const size_t min_hud_width = std::max( get_header_min_width(), get_footer_min_width() );
    const auto visible_columns = get_visible_columns();
    const size_t gaps = visible_columns.size() > 1 ? normal_column_gap * ( visible_columns.size() - 1 )
                        : 0;

    return std::max( get_columns_width( visible_columns ) + gaps, min_hud_width );
}

size_t inventory_selector::get_layout_height() const
{
    const auto visible_columns = get_visible_columns();
    // Find and return the highest column's height.
    const auto iter = std::max_element( visible_columns.begin(), visible_columns.end(),
    []( const inventory_column * lhs, const inventory_column * rhs ) {
        return lhs->get_height() < rhs->get_height();
    } );

    return iter != visible_columns.end() ? ( *iter )->get_height() : 1;
}

size_t inventory_selector::get_header_height() const
{
    return display_stats || !hint.empty()
           ? std::max<size_t>( 3, 2 + std::count( hint.begin(), hint.end(), '\n' ) )
           : 1;
}

size_t inventory_selector::get_header_min_width() const
{
    const size_t titles_width = std::max( utf8_width( title, true ),
                                          utf8_width( hint, true ) );
    if( !display_stats ) {
        return titles_width;
    }

    size_t stats_width = 0;
    for( const std::string &elem : get_stats() ) {
        stats_width = std::max( static_cast<size_t>( utf8_width( elem, true ) ), stats_width );
    }

    return titles_width + stats_width + ( stats_width != 0 ? 3 : 0 );
}

size_t inventory_selector::get_footer_min_width() const
{
    size_t result = 0;
    navigation_mode m = mode;

    do {
        result = std::max( static_cast<size_t>( utf8_width( get_footer( m ).first, true ) ) + 2 + 4,
                           result );
        m = get_navigation_data( m ).next_mode;
    } while( m != mode );

    return result;
}

void inventory_selector::draw_header( const catacurses::window &w ) const
{
    trim_and_print( w, point( border + 1, border ), getmaxx( w ) - 2 * ( border + 1 ), c_white, title );
    fold_and_print( w, point( border + 1, border + 1 ), getmaxx( w ) - 2 * ( border + 1 ), c_dark_gray,
                    hint );

    int const bottom = border + get_header_height();
    mvwhline( w, point( border, bottom ), LINE_OXOX, getmaxx( w ) - 2 * border );

    if( display_stats ) {
        size_t y = border;
        for( const std::string &elem : get_stats() ) {
            right_print( w, y++, border + 1, c_dark_gray, elem );
        }
    }
    if( uistate.numpad_navigation ) {
        right_print( w, bottom, border + 1, c_dark_gray, _( "Numpad ON" ) );
    }
}

inventory_selector::stat display_stat( const std::string &caption, int cur_value, int max_value,
                                       const std::function<std::string( int )> &disp_func )
{
    const nc_color color = cur_value > max_value ? c_red : c_light_gray;
    return {{
            caption,
            colorize( disp_func( cur_value ), color ), "/",
            colorize( disp_func( max_value ), c_light_gray )
        }};
}

inventory_selector::stat inventory_selector::get_weight_and_length_stat(
    units::mass weight_carried, units::mass weight_capacity,
    const units::length &longest_length )
{
    std::string length_weight_caption = string_format( _( "Longest Length (%s): %s Weight (%s):" ),
                                        length_units( longest_length ), colorize( std::to_string( convert_length( longest_length ) ),
                                                c_light_gray ), weight_units() );
    return display_stat( length_weight_caption, to_gram( weight_carried ),
    to_gram( weight_capacity ), []( int w ) {
        return string_format( "%.1f", round_up( convert_weight( units::from_gram( w ) ), 1 ) );
    } );
}
inventory_selector::stat inventory_selector::get_volume_stat( const units::volume
        &volume_carried, const units::volume &volume_capacity, const units::volume &largest_free_volume )
{
    std::string volume_caption = string_format( _( "Free Volume (%s): %s Volume (%s):" ),
                                 volume_units_abbr(),
                                 colorize( format_volume( largest_free_volume ), c_light_gray ),
                                 volume_units_abbr() );
    return display_stat( volume_caption, units::to_milliliter( volume_carried ),
    units::to_milliliter( volume_capacity ), []( int v ) {
        return format_volume( units::from_milliliter( v ) );
    } );
}
inventory_selector::stat inventory_selector::get_holster_stat( const units::volume
        &holster_volume, int used_holsters, int total_holsters )
{

    std::string holster_caption = string_format( _( "Free Holster Volume (%s): %s Used Holsters:" ),
                                  volume_units_abbr(),
                                  colorize( format_volume( holster_volume ), c_light_gray ) );

    return display_stat( holster_caption, used_holsters, total_holsters, []( int v ) {
        return string_format( "%d", v );
    } );
}

inventory_selector::stats inventory_selector::get_weight_and_volume_and_holster_stats(
    units::mass weight_carried, units::mass weight_capacity,
    const units::volume &volume_carried, const units::volume &volume_capacity,
    const units::length &longest_length, const units::volume &largest_free_volume,
    const units::volume &holster_volume, int used_holsters, int total_holsters )
{
    return { {
            get_weight_and_length_stat( weight_carried, weight_capacity, longest_length ),
            get_volume_stat( volume_carried, volume_capacity, largest_free_volume ),
            get_holster_stat( holster_volume, used_holsters, total_holsters )
        }
    };
}

inventory_selector::stats inventory_selector::get_raw_stats() const
{
    return get_weight_and_volume_and_holster_stats( u.weight_carried(), u.weight_capacity(),
            u.volume_carried(), u.volume_capacity(),
            u.max_single_item_length(), u.max_single_item_volume(),
            u.free_holster_volume(), u.used_holsters(), u.total_holsters() );
}

std::vector<std::string> inventory_selector::get_stats() const
{
    // Stats consist of arrays of cells.
    const size_t num_stats = 3;
    const std::array<stat, num_stats> stats = get_raw_stats();
    // Streams for every stat.
    std::array<std::string, num_stats> lines;
    std::array<size_t, num_stats> widths;
    // Add first cells and spaces after them.
    for( size_t i = 0; i < stats.size(); ++i ) {
        lines[i] += string_format( "%s", stats[i][0] ) + " ";
    }
    // Now add the rest of the cells and align them to the right.
    for( size_t j = 1; j < stats.front().size(); ++j ) {
        // Calculate actual cell width for each stat.
        std::transform( stats.begin(), stats.end(), widths.begin(),
        [j]( const stat & elem ) {
            return utf8_width( elem[j], true );
        } );
        // Determine the max width.
        const size_t max_w = *std::max_element( widths.begin(), widths.end() );
        // Align all stats in this cell with spaces.
        for( size_t i = 0; i < stats.size(); ++i ) {
            if( max_w > widths[i] ) {
                lines[i] += std::string( max_w - widths[i], ' ' );
            }
            lines[i] += string_format( "%s", stats[i][j] );
        }
    }
    // Construct the final result.
    return std::vector<std::string>( lines.begin(), lines.end() );
}

void inventory_selector::resize_window( int width, int height )
{
    point origin = _fixed_origin;
    if( origin.x < 0 || origin.y < 0 ) {
        origin = { ( TERMX - width ) / 2, ( TERMY - height ) / 2 };
    }
    w_inv = catacurses::newwin( height, width, origin );
    if( spopup ) {
        spopup->window( w_inv, point( 4, getmaxy( w_inv ) - 1 ), ( getmaxx( w_inv ) / 2 ) - 4 );
    }
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->position_from_window( w_inv );
    }
}

void inventory_selector::refresh_window()
{
    cata_assert( w_inv );

    if( get_option<std::string>( "INVENTORY_HIGHLIGHT" ) != "disable" ) {
        highlight();
    }

    werase( w_inv );

    draw_frame( w_inv );
    draw_header( w_inv );
    draw_columns( w_inv );
    draw_footer( w_inv );

    wnoutrefresh( w_inv );
}

std::pair< bool, std::string > inventory_selector::query_string( const std::string &val,
        bool end_with_toggle )
{
    spopup = std::make_unique<string_input_popup>();
    spopup->max_length( 256 )
    .text( val );
    spopup->identifier( "item_filter" ).hist_use_uilist( false );
    if( end_with_toggle ) {
        for( input_event const &iev : inp_mngr.get_input_for_action( "TOGGLE_ENTRY", "INVENTORY" ) ) {
            spopup->add_callback( iev.get_first_input(), [this]() {
                spopup->confirm();
                return true;
            } );
        }
    };

    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->mark_resize();
    }

    do {
        ui_manager::redraw();
        spopup->query_string( /*loop=*/false );
    } while( !spopup->confirmed() && !spopup->canceled() );

    std::string rval;
    bool confirmed = spopup->confirmed();
    if( confirmed ) {
        rval = spopup->text();
    }

    spopup.reset();
    return std::make_pair( confirmed, rval );
}

void inventory_selector::query_set_filter()
{
    std::pair< bool, std::string > query = query_string( filter );
    if( query.first ) {
        set_filter( query.second );
    }
}

int inventory_selector::query_count( char init, bool end_with_toggle )
{
    std::string sinit = init != 0 ? std::string( 1, init ) : std::string();
    std::pair< bool, std::string > query = query_string( sinit, end_with_toggle );
    int ret = -1;
    if( query.first ) {
        try {
            ret = std::stoi( query.second );
        } catch( const std::invalid_argument &e ) {
            // TODO Tell User they did a bad
            ret = -1;
        } catch( const std::out_of_range &e ) {
            ret = INT_MAX;
        }
    }

    return ret;
}

void inventory_selector::set_filter( const std::string &str )
{
    filter = str;
    for( inventory_column *const elem : columns ) {
        elem->set_filter( filter );
    }
}

std::string inventory_selector::get_filter() const
{
    return filter;
}

void inventory_selector::draw_columns( const catacurses::window &w )
{
    const auto columns = get_visible_columns();

    const int screen_width = getmaxx( w ) - 2 * ( border + 1 );
    const bool centered = are_columns_centered( screen_width );

    const int free_space = screen_width - get_columns_width( columns );
    const int max_gap = ( columns.size() > 1 ) ? free_space / ( static_cast<int>
                        ( columns.size() ) - 1 ) :
                        free_space;
    const int gap = centered ? max_gap : std::min<int>( max_gap, normal_column_gap );
    const int gap_rounding_error = centered && columns.size() > 1
                                   ? free_space % ( columns.size() - 1 ) : 0;

    size_t x = border + 1;
    size_t y = get_header_height() + border + 1;
    size_t active_x = 0;

    rect_entry_map.clear();
    for( inventory_column * const &elem : columns ) {
        if( &elem == &columns.back() ) {
            x += gap_rounding_error;
        }
        if( !is_active_column( *elem ) ) {
            elem->draw( w, point( x, y ), rect_entry_map );
        } else {
            active_x = x;
        }
        x += elem->get_width() + gap;
    }

    get_active_column().draw( w, point( active_x, y ), rect_entry_map );
    if( empty() ) {
        center_print( w, getmaxy( w ) / 2, c_dark_gray, _( "Your inventory is empty." ) );
    }
}

void inventory_selector::draw_frame( const catacurses::window &w ) const
{
    draw_border( w );

    const int y = border + get_header_height();
    mvwhline( w, point( 0, y ), LINE_XXXO, 1 );
    mvwhline( w, point( getmaxx( w ) - border, y ), LINE_XOXX, 1 );
}

std::pair<std::string, nc_color> inventory_selector::get_footer( navigation_mode m ) const
{
    if( has_available_choices() ) {
        return std::make_pair( get_navigation_data( m ).name.translated(),
                               get_navigation_data( m ).color );
    }
    return std::make_pair( _( "There are no available choices" ), i_red );
}

void inventory_selector::draw_footer( const catacurses::window &w ) const
{
    if( spopup ) {
        mvwprintz( w_inv, point( 2, getmaxy( w_inv ) - 1 ), c_cyan, "< " );
        mvwprintz( w_inv, point( ( getmaxx( w_inv ) / 2 ) - 4, getmaxy( w_inv ) - 1 ), c_cyan, " >" );

        spopup->query_string( /*loop=*/false, /*draw_only=*/true );
    } else {
        int filter_offset = 0;
        if( has_available_choices() || !filter.empty() ) {
            std::string text = string_format( filter.empty() ? _( "[%s] Filter" ) : _( "[%s] Filter: " ),
                                              ctxt.get_desc( "INVENTORY_FILTER" ) );
            filter_offset = utf8_width( text + filter ) + 6;

            mvwprintz( w, point( 2, getmaxy( w ) - border ), c_light_gray, "< " );
            wprintz( w, c_light_gray, text );
            wprintz( w, c_white, filter );
            wprintz( w, c_light_gray, " >" );
        }

        right_print( w, getmaxy( w ) - border, border + 1, c_light_gray,
                     string_format( "< [%s] %s >", ctxt.get_desc( "VIEW_CATEGORY_MODE" ),
                                    io::enum_to_string( _uimode ) ) );
        const auto footer = get_footer( mode );
        if( !footer.first.empty() ) {
            const int string_width = utf8_width( footer.first );
            const int x1 = filter_offset + std::max( getmaxx( w ) - string_width - filter_offset, 0 ) / 2;
            const int x2 = x1 + string_width - 1;
            const int y = getmaxy( w ) - border;

            mvwprintz( w, point( x1, y ), footer.second, footer.first );
            mvwputch( w, point( x1 - 1, y ), c_light_gray, ' ' );
            mvwputch( w, point( x2 + 1, y ), c_light_gray, ' ' );
            mvwputch( w, point( x1 - 2, y ), c_light_gray, LINE_XOXX );
            mvwputch( w, point( x2 + 2, y ), c_light_gray, LINE_XXXO );
        }
    }
}

inventory_selector::inventory_selector( Character &u, const inventory_selector_preset &preset )
    : u( u )
    , preset( preset )
    , ctxt( "INVENTORY", keyboard_mode::keychar )
    , active_column_index( 0 )
    , mode( navigation_mode::ITEM )
    , own_inv_column( preset )
    , own_gear_column( preset )
    , map_column( preset )
    , _uimode( preset.save_state == nullptr ? inventory_sel_default_state.uimode :
               preset.save_state->uimode )
{
    item_name_cache_users++;
    tp_start =
        std::chrono::time_point_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() );
    ctxt.register_action( "COORDINATE" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "DOWN", to_translation( "Next item" ) );
    ctxt.register_action( "UP", to_translation( "Previous item" ) );
    ctxt.register_action( "SCROLL_UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "SCROLL_DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Page down" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Page up" ) );
    ctxt.register_action( "NEXT_COLUMN", to_translation( "Next column" ) );
    ctxt.register_action( "PREV_COLUMN", to_translation( "Previous column" ) );
    ctxt.register_action( "CONFIRM", to_translation( "Confirm your selection" ) );
    ctxt.register_action( "QUIT", to_translation( "Cancel" ) );
    ctxt.register_action( "CATEGORY_SELECTION", to_translation( "Switch category selection mode" ) );
    ctxt.register_action( "TOGGLE_FAVORITE", to_translation( "Toggle favorite" ) );
    ctxt.register_action( "HOME", to_translation( "Home" ) );
    ctxt.register_action( "END", to_translation( "End" ) );
    ctxt.register_action( "CLICK_AND_DRAG" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "VIEW_CATEGORY_MODE" );
    ctxt.register_action( "TOGGLE_NUMPAD_NAVIGATION" );
    ctxt.register_action( "ANY_INPUT" ); // For invlets
    ctxt.register_action( "INVENTORY_FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "SHOW_HIDE_CONTENTS", to_translation( "Show/hide contents" ) );
    ctxt.register_action( "SHOW_HIDE_CONTENTS_ALL" );
    ctxt.register_action( "EXAMINE_CONTENTS" );
    ctxt.register_action( "TOGGLE_SKIP_UNSELECTABLE" );
    ctxt.register_action( "ORGANIZE_MENU" );

    append_column( own_inv_column );
    append_column( map_column );
    append_column( own_gear_column );

    for( inventory_column *column : columns ) {
        column->toggle_skip_unselectable( skip_unselectable );
    }
}

inventory_selector::~inventory_selector()
{
    item_name_cache_users--;
    if( item_name_cache_users <= 0 ) {
        item_name_cache.clear();
    }
    if( preset.save_state == nullptr ) {
        inventory_sel_default_state.uimode = _uimode;
    } else {
        preset.save_state->uimode = _uimode;
    }
}

bool inventory_selector::empty() const
{
    return is_empty;
}

bool inventory_selector::has_available_choices() const
{
    return std::any_of( columns.begin(), columns.end(), []( const inventory_column * element ) {
        return element->has_available_choices();
    } );
}

uint64_t inventory_selector::item_entry_count() const
{
    uint64_t count = 0;
    for( const inventory_column *col : columns ) {
        count += col->get_entries( return_item, true ).size();
    }

    return count;
}

drop_location inventory_selector::get_only_choice() const
{
    if( item_entry_count() != 1 ) {
        debugmsg( "inventory_selector::get_only_choice called with more that one choice" );
        return drop_location();
    }

    for( const inventory_column *col : columns ) {
        const std::vector<inventory_entry *> ent = col->get_entries( return_item, true );
        if( !ent.empty() ) {
            return { ent.front()->any_item(), static_cast<int>( ent.front()->get_available_count() ) };
        }
    }

    return drop_location();
}

inventory_input inventory_selector::get_input()
{
    std::string const &action = ctxt.handle_input();
    int const ch = ctxt.get_raw_input().get_first_input();
    return process_input( action, ch );
}

inventory_input inventory_selector::process_input( const std::string &action, int ch )
{
    inventory_input res{ action, ch, nullptr };

    if( res.action == "SELECT" || res.action == "COORDINATE" || res.action == "MOUSE_MOVE" ||
        res.action == "CLICK_AND_DRAG" ) {
        std::optional<point> o_p = ctxt.get_coordinates_text( w_inv );
        if( o_p ) {
            point p = o_p.value();
            if( window_contains_point_relative( w_inv, p ) ) {
                res.entry = find_entry_by_coordinate( p );
                if( res.entry != nullptr && res.entry->is_selectable() ) {
                    return res;
                }
                if( res.entry == nullptr && res.action == "SELECT" ) {
                    p.x++;
                    res.entry = find_entry_by_coordinate( p );
                    // if the user has clicked the coordinate just to the left of a pocket, they are clicking the pocket's expander icon
                    if( res.entry != nullptr && res.entry->is_selectable() && res.entry->chevron ) {
                        res.action = "SHOW_HIDE_CONTENTS";
                        return res;
                    }
                }
            }
        }
    }

    res.entry = find_entry_by_invlet( res.ch );
    if( res.entry != nullptr && !res.entry->is_selectable() ) {
        res.entry = nullptr;
    }
    return res;
}

void inventory_column::cycle_hide_override()
{
    if( hide_entries_override ) {
        if( *hide_entries_override ) {
            hide_entries_override = std::nullopt;
        } else {
            hide_entries_override = true;
        }
    } else {
        hide_entries_override = false;
    }
    uistate.hide_entries_override = hide_entries_override;
}

void selection_column::cycle_hide_override()
{
    // never hide entries
}

void inventory_selector::on_input( const inventory_input &input )
{
    if( input.action == "CATEGORY_SELECTION" ) {
        toggle_navigation_mode();
    } else if( input.action == "PREV_COLUMN" ) {
        toggle_active_column( scroll_direction::BACKWARD );
    } else if( input.action == "NEXT_COLUMN" ) {
        toggle_active_column( scroll_direction::FORWARD );
    } else if( input.action == "VIEW_CATEGORY_MODE" ) {
        toggle_categorize_contained();
    } else if( input.action == "TOGGLE_NUMPAD_NAVIGATION" ) {
        uistate.numpad_navigation = !uistate.numpad_navigation;
        reassign_custom_invlets();
    } else if( input.action == "EXAMINE_CONTENTS" ) {
        const inventory_entry &selected = get_active_column().get_highlighted();
        if( selected ) {
            //TODO: Should probably be any_item() rather than direct front() access, but that seems to lock us into const item_location, which various functions are unprepared for
            item_location sitem = selected.locations.front();
            inventory_examiner examine_contents( u, sitem );
            examine_contents.add_contained_items( sitem );
            int examine_result = examine_contents.execute();
            if( examine_result == EXAMINED_CONTENTS_WITH_CHANGES ) {
                //The user changed something while examining, so rebuild paging
                for( inventory_column *elem : columns ) {
                    elem->invalidate_paging();
                }
            } else if( examine_result == NO_CONTENTS_TO_EXAMINE ) {
                action_examine( sitem );
            }
        }
    } else if( input.action == "EXAMINE" ) {
        const inventory_entry &selected = get_active_column().get_highlighted();
        if( selected ) {
            const item_location &sitem = selected.any_item();
            action_examine( sitem );
        }
    } else if( input.action == "INVENTORY_FILTER" ) {
        query_set_filter();
        ui.lock()->mark_resize();
    } else if( input.action == "RESET_FILTER" ) {
        set_filter( "" );
        ui.lock()->mark_resize();
    } else if( input.action == "TOGGLE_SKIP_UNSELECTABLE" ) {
        toggle_skip_unselectable();
    } else {
        for( inventory_column *elem : columns ) {
            elem->on_input( input );
        }
        refresh_active_column(); // Columns can react to actions by losing their activation capacity
        if( input.action == "TOGGLE_FAVORITE" ) {
            // Favoriting items changes item name length which may require resizing
            shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
            if( current_ui ) {
                current_ui->mark_resize();
            }
        }
        if( input.action == "SHOW_HIDE_CONTENTS_ALL" ) {
            for( inventory_column *col : columns ) {
                col->cycle_hide_override();
            }
        }
        if( input.action == "SHOW_HIDE_CONTENTS" || input.action == "SHOW_HIDE_CONTENTS_ALL" ) {
            shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
            for( inventory_column * const &col : columns ) {
                col->invalidate_paging();
            }
            if( current_ui ) {
                std::vector<item_location> inv = get_highlighted().locations;
                current_ui->mark_resize();
                highlight_one_of( inv );
            }
        }
    }
}

void inventory_selector::on_change( const inventory_entry &entry )
{
    for( inventory_column *&elem : columns ) {
        elem->on_change( entry );
    }
    refresh_active_column(); // Columns can react to changes by losing their activation capacity
}

std::vector<inventory_column *> inventory_selector::get_visible_columns() const
{
    std::vector<inventory_column *> res( columns.size() );
    const auto iter = std::copy_if( columns.begin(), columns.end(), res.begin(),
    []( const inventory_column * e ) {
        return e->visible();
    } );
    res.resize( std::distance( res.begin(), iter ) );
    return res;
}

inventory_column &inventory_selector::get_column( size_t index ) const
{
    if( index >= columns.size() ) {
        static inventory_column dummy( preset );
        return dummy;
    }
    return *columns[index];
}

void inventory_selector::set_active_column( size_t index )
{
    if( index < columns.size() && index != active_column_index && get_column( index ).activatable() ) {
        get_active_column().on_deactivate();
        active_column_index = index;
        get_active_column().on_activate();
    }
}

void inventory_selector::toggle_skip_unselectable()
{
    skip_unselectable = !skip_unselectable;
    for( inventory_column *col : columns ) {
        col->toggle_skip_unselectable( skip_unselectable );
    }
}

size_t inventory_selector::get_columns_width( const std::vector<inventory_column *> &columns ) const
{
    return std::accumulate( columns.begin(), columns.end(), static_cast< size_t >( 0 ),
    []( const size_t &lhs, const inventory_column * column ) {
        return lhs + column->get_width();
    } );
}

double inventory_selector::get_columns_occupancy_ratio( size_t client_width ) const
{
    const auto visible_columns = get_visible_columns();
    const int free_width = client_width - get_columns_width( visible_columns )
                           - min_column_gap * std::max( static_cast<int>( visible_columns.size() ) - 1, 0 );
    return 1.0 - static_cast<double>( free_width ) / client_width;
}

bool inventory_selector::are_columns_centered( size_t client_width ) const
{
    return get_columns_occupancy_ratio( client_width ) >= min_ratio_to_center;
}

bool inventory_selector::is_overflown( size_t client_width ) const
{
    return get_columns_occupancy_ratio( client_width ) > 1.0;
}

void inventory_selector::_categorize( inventory_column &col )
{
    // Remove custom category and allow entries to categorize by their item's category
    for( inventory_entry *entry : col.get_entries( return_item, true ) ) {
        const item_location loc = entry->any_item();
        // ensure top-level equipped entries don't lose their special categories
        const item_category *custom_category = wielded_worn_category( loc, u );

        entry->set_custom_category( custom_category );
    }
    col.set_indent_entries_override( false );
    col.invalidate_paging();
    col.uncollate();
}

void inventory_selector::_uncategorize( inventory_column &col )
{
    for( inventory_entry *entry : col.get_entries( return_item, true ) ) {
        // find the topmost parent of the entry's item and categorize it by that
        // to form the hierarchy
        item_location ancestor = entry->any_item();
        while( ancestor.has_parent() ) {
            ancestor = ancestor.parent_item();
        }

        const item_category *custom_category = nullptr;
        if( ancestor.where() != item_location::type::character ) {
            const std::string name = to_upper_case( remove_color_tags( ancestor.describe() ) );
            const item_category map_cat( name, no_translation( name ), translation(), 100 );
            custom_category = naturalize_category( map_cat, ancestor.position() );
        } else {
            custom_category = wielded_worn_category( ancestor, u );
        }

        entry->set_custom_category( custom_category );
    }
    col.clear_indent_entries_override();
    col.invalidate_paging();
    col.uncollate();
}

void inventory_selector::toggle_categorize_contained()
{
    std::vector<item_location> highlighted;
    if( get_highlighted().is_item() ) {
        highlighted = get_highlighted().locations;
    }

    if( _uimode == uimode::hierarchy ) {
        inventory_column replacement_column;

        // split entries into either worn/held gear or contained items
        for( inventory_entry *entry : own_gear_column.get_entries( return_item, true ) ) {
            const item_location loc = entry->any_item();
            inventory_column *col = is_container( loc ) && !is_worn_ablative( loc.parent_item(), loc ) ?
                                    &own_inv_column : &replacement_column;
            col->add_entry( *entry );
        }
        own_gear_column.clear();
        replacement_column.move_entries_to( own_gear_column );

        for( inventory_column *col : columns ) {
            _categorize( *col );
        }
        _uimode = uimode::categories;
    } else {
        // move all entries into one big gear column and turn into hierarchy
        own_inv_column.move_entries_to( own_gear_column );
        for( inventory_column *col : columns ) {
            _uncategorize( *col );
        }
        _uimode = uimode::hierarchy;
    }

    if( !highlighted.empty() ) {
        highlight_one_of( highlighted );
    }

    // needs to be called now so that new invlets can be assigned
    // and subclasses w/ selection columns can then re-populate entries
    // using the new invlets
    prepare_layout();

    // invalidate, but dont mark resize, to avoid re-calling prepare_layout()
    // and as a consequence reassign_custom_invlets()
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->invalidate_ui();
    }
}

void inventory_selector::toggle_active_column( scroll_direction dir )
{
    if( columns.empty() ) {
        return;
    }

    size_t index = active_column_index;

    do {
        switch( dir ) {
            case scroll_direction::FORWARD:
                index = index + 1 < columns.size() ? index + 1 : 0;
                break;
            case scroll_direction::BACKWARD:
                index = index > 0 ? index - 1 : columns.size() - 1;
                break;
        }
    } while( index != active_column_index && !get_column( index ).activatable() );

    set_active_column( index );
}

void inventory_selector::toggle_navigation_mode()
{
    mode = get_navigation_data( mode ).next_mode;
    for( inventory_column *&elem : columns ) {
        elem->on_mode_change( mode );
    }
}

void inventory_selector::append_column( inventory_column &column )
{
    column.on_mode_change( mode );

    if( columns.empty() ) {
        column.on_activate();
    }

    columns.push_back( &column );
}

const navigation_mode_data &inventory_selector::get_navigation_data( navigation_mode m ) const
{
    static const std::map<navigation_mode, navigation_mode_data> mode_data = {
        { navigation_mode::ITEM,     { navigation_mode::CATEGORY, translation(),                               c_light_gray } },
        { navigation_mode::CATEGORY, { navigation_mode::ITEM,     to_translation( "Category selection mode" ), h_white  } }
    };

    return mode_data.at( m );
}

std::string inventory_selector::action_bound_to_key( char key ) const
{
    return ctxt.input_to_action( input_event( key, input_event_t::keyboard_char ) );
}

item_location inventory_pick_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    debug_print_timer( tp_start );
    item_location startDragItem;
    bool dragActive = false;
    while( true ) {
        ui_manager::redraw();
        const inventory_input input = get_input();

        if( input.entry != nullptr ) {
            if( drag_enabled && input.action == "CLICK_AND_DRAG" ) {
                if( input.entry->is_item() ) {
                    dragActive = true;
                    startDragItem = input.entry->locations.front();
                }
            } else if( input.action == "MOUSE_MOVE" ) {
                if( !dragActive && highlight( input.entry->any_item() ) ) {
                    ui_manager::redraw();
                }
            } else if( input.action == "SELECT" ) {
                dragActive = false;
                item_location startDragItemCpy = startDragItem;
                startDragItem = item_location();
                item_location endDragItem;
                if( input.entry->is_item() ) {
                    endDragItem = input.entry->locations.front();
                    if( startDragItemCpy && endDragItem && startDragItemCpy != endDragItem ) {
                        if( drag_drop_item( startDragItemCpy.get_item(), endDragItem.get_item() ) ) {
                            clear_items();
                            add_character_items( u );
                        }
                    } else {
                        return input.entry->any_item();
                    }
                }
            } else if( input.action != "MOUSE_MOVE" && input.action != "COORDINATE" ) {
                return input.entry->any_item();
            } else {
                if( !dragActive && highlight( input.entry->any_item() ) ) {
                    ui_manager::redraw();
                }
                on_input( input );
            }
        } else if( input.action == "ORGANIZE_MENU" ) {
            u.worn.organize_items_menu();
            return item_location();
        } else if( input.action == "QUIT" ) {
            return item_location();
        } else if( input.action == "CONFIRM" ) {
            const inventory_entry &highlighted = get_active_column().get_highlighted();
            if( highlighted && highlighted.is_selectable() ) {
                return highlighted.any_item();
            }
        } else {
            on_input( input );
        }
    }
}

inventory_selector::stats container_inventory_selector::get_raw_stats() const
{
    return get_weight_and_volume_and_holster_stats( loc->get_total_contained_weight(),
            loc->get_total_weight_capacity(),
            loc->get_total_contained_volume(), loc->get_total_capacity(),
            loc->max_containable_length(), loc->max_containable_volume(),
            loc->get_total_holster_volume() - loc->get_used_holster_volume(),
            loc->get_used_holsters(), loc->get_total_holsters() );
}

ammo_inventory_selector::ammo_inventory_selector( Character &you,
        const item_location &reload_loc, const inventory_selector_preset &preset ) :
    inventory_selector( you, preset ), reload_loc( reload_loc )
{
    ctxt.register_action( "INCREASE_COUNT" );
    ctxt.register_action( "DECREASE_COUNT" );

    force_single_column = true;
}

std::vector<item_location> get_possible_reload_targets( const item_location &target )
{
    std::vector<item_location> opts;
    opts.emplace_back( target );

    if( target->magazine_current() ) {
        opts.emplace_back( target, const_cast<item *>( target->magazine_current() ) );
    }

    for( const item *mod : target->gunmods() ) {
        item_location mod_loc( target, const_cast<item *>( mod ) );
        opts.emplace_back( mod_loc );
        if( mod->magazine_current() ) {
            opts.emplace_back( mod_loc, const_cast<item *>( mod->magazine_current() ) );
        }
    }

    return opts;
}

// todo: this should happen when the entries are created, but that's a different refactoring
void ammo_inventory_selector::set_all_entries_chosen_count()
{
    for( inventory_column *col : columns ) {
        for( inventory_entry *entry : col->get_entries( return_item, true ) ) {
            for( const item_location &loc : get_possible_reload_targets( reload_loc ) ) {
                item_location it = entry->any_item();
                if( loc->can_reload_with( *it, true ) ) {
                    item::reload_option tmp_opt( &u, loc, it );
                    int count = entry->get_available_count();
                    if( it->has_flag( flag_SPEEDLOADER ) || it->has_flag( flag_SPEEDLOADER_CLIP ) ) {
                        count = it->ammo_remaining();
                    }
                    tmp_opt.qty( count );
                    entry->chosen_count = tmp_opt.qty();
                    break;
                }
            }
        }
    }
}

void ammo_inventory_selector::mod_chosen_count( inventory_entry &entry, int value )
{
    if( !entry.any_item()->is_ammo() ) {
        return;
    }
    for( const item_location &loc : get_possible_reload_targets( reload_loc ) ) {
        if( loc->can_reload_with( *entry.any_item(), true ) ) {
            item::reload_option tmp_opt( &u, loc, entry.any_item() );
            tmp_opt.qty( entry.chosen_count + value );
            entry.chosen_count = tmp_opt.qty();
            break;
        }
    }

    entry.make_entry_cell_cache( preset );
    on_change( entry );
}

drop_location ammo_inventory_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    debug_print_timer( tp_start );
    while( true ) {
        ui_manager::redraw();
        const inventory_input input = get_input();

        if( input.entry != nullptr ) {
            if( input.action == "MOUSE_MOVE" ) {
                if( highlight( input.entry->any_item() ) ) {
                    ui_manager::redraw();
                }
            } else if( input.action == "ANY_INPUT" || input.action == "SELECT" ) {
                return { input.entry->any_item(), static_cast<int>( input.entry->chosen_count ) };
            } else {
                if( highlight( input.entry->any_item() ) ) {
                    ui_manager::redraw();
                }
                on_input( input );
            }
        } else if( input.action == "QUIT" ) {
            return drop_location();
        } else if( input.action == "CONFIRM" ) {
            const inventory_entry &highlighted = get_active_column().get_highlighted();
            if( highlighted && highlighted.is_selectable() ) {
                return { highlighted.any_item(), static_cast<int>( highlighted.chosen_count ) };
            }
        } else if( input.action == "INCREASE_COUNT" ) {
            inventory_entry &highlighted = get_active_column().get_highlighted();
            mod_chosen_count( highlighted, 1 );
        } else if( input.action == "DECREASE_COUNT" ) {
            inventory_entry &highlighted = get_active_column().get_highlighted();
            mod_chosen_count( highlighted, -1 );
        } else {
            on_input( input );
        }
    }
}

void inventory_selector::action_examine( const item_location &sitem )
{
    // Code below pulled from the action_examine function in advanced_inv.cpp
    std::vector<iteminfo> vThisItem;
    std::vector<iteminfo> vDummy;

    sitem->info( true, vThisItem );
    vThisItem.insert( vThisItem.begin(),
    { {}, string_format( _( "Location: %s" ), sitem.describe( &u ) ) } );

    item_info_data data( sitem->tname(), sitem->type_name(), vThisItem, vDummy );
    data.handle_scrolling = true;
    draw_item_info( [&]() -> catacurses::window {
        int maxwidth = std::max( FULL_SCREEN_WIDTH, TERMX );
        int width = std::min( 80, maxwidth );
        return catacurses::newwin( 0, width, point( maxwidth / 2 - width / 2, 0 ) ); },
    data ).get_first_input();
}

void inventory_selector::highlight()
{
    const inventory_entry &selected = get_active_column().get_highlighted();
    if( !selected.is_item() ) {
        return;
    }
    item_location parent = item_location::nowhere;
    bool selected_has_parent = false;
    if( selected.is_item() && selected.any_item().has_parent() ) {
        parent = selected.any_item().parent_item();
        selected_has_parent = true;
    }
    for( const inventory_column *column : get_all_columns() ) {
        for( inventory_entry *entry : column->get_entries( return_item ) ) {
            // Find parent of selected.
            if( selected_has_parent ) {
                // Check if parent is in a stack.
                for( const item_location &test_loc : entry->locations ) {
                    if( test_loc == parent ) {
                        entry->highlight_as_parent = true;
                        break;
                    }
                }
            }
            // Find contents of selected.
            if( !entry->any_item().has_parent() ) {
                continue;
            }
            // More than one item can be highlighted when selected container is stacked.
            for( const item_location &location : selected.locations ) {
                if( entry->any_item().parent_item() == location ) {
                    entry->highlight_as_child = true;
                }
            }
        }
    }
}

inventory_multiselector::inventory_multiselector( Character &p,
        const inventory_selector_preset &preset,
        const std::string &selection_column_title,
        const GetStats &get_stats,
        const bool allow_select_contained ) :
    inventory_selector( p, preset ),
    allow_select_contained( allow_select_contained ),
    selection_col( new selection_column( "SELECTION_COLUMN", selection_column_title ) ),
    get_stats( get_stats )
{
    ctxt.register_action( "TOGGLE_ENTRY", to_translation( "Mark/unmark selected item" ) );
    ctxt.register_action( "MARK_WITH_COUNT",
                          to_translation( "Mark a specific amount of selected item" ) );
    ctxt.register_action( "TOGGLE_NON_FAVORITE", to_translation( "Mark/unmark non-favorite items" ) );
    ctxt.register_action( "INCREASE_COUNT" );
    ctxt.register_action( "DECREASE_COUNT" );

    max_chosen_count = std::numeric_limits<decltype( max_chosen_count )>::max();
    set_invlet_type( inventory_selector::SELECTOR_INVLET_ALPHA );

    for( inventory_column * const &elem : get_all_columns() ) {
        elem->set_multiselect( true );
    }
    append_column( *selection_col );
}

void inventory_multiselector::toggle_entry( inventory_entry &entry, size_t count )
{
    set_chosen_count( entry, count );
    on_toggle();
    selection_col->prepare_paging();
}

void inventory_multiselector::rearrange_columns( size_t client_width )
{
    selection_col->set_visibility( true );
    inventory_selector::rearrange_columns( client_width );
    selection_col->set_visibility( !is_overflown( client_width ) );
}

void inventory_multiselector::set_chosen_count( inventory_entry &entry, size_t count )
{
    const item_location &it = entry.any_item();

    /* Since we're modifying selection of this entry, we need to clear out
       anything that's been set before.
     */
    for( const item_location &loc : entry.locations ) {
        for( auto iter = to_use.begin(); iter != to_use.end(); ) {
            if( iter->first == loc ) {
                iter = to_use.erase( iter );
            } else {
                ++iter;
            }
        }
    }

    if( count == 0 ) {
        entry.chosen_count = 0;
    } else {
        size_t size_before = to_use.size();
        entry.chosen_count = std::min( {count, max_chosen_count, entry.get_available_count() } );
        if( it->count_by_charges() ) {
            auto iter = find_if( to_use.begin(),
            to_use.begin() + size_before, [&it]( const drop_location & drop ) {
                return drop.first == it;
            } );
            if( iter == to_use.begin() + size_before ) {
                to_use.emplace_back( it, static_cast<int>( entry.chosen_count ) );
            }
        } else {
            for( const item_location &loc : entry.locations ) {
                if( count == 0 ) {
                    break;
                }
                auto iter = find_if( to_use.begin(),
                to_use.begin() + size_before, [&loc]( const drop_location & drop ) {
                    return drop.first == loc;
                } );
                if( iter == to_use.begin() + size_before ) {
                    to_use.emplace_back( loc, 1 );
                }
                count--;
            }
        }
    }

    on_change( entry );
}

void inventory_multiselector::toggle_entries( int &count, const toggle_mode mode )
{
    std::vector<inventory_entry *> selected;
    switch( mode ) {
        case toggle_mode::SELECTED:
            selected = get_active_column().get_all_selected();
            break;
        case toggle_mode::NON_FAVORITE_NON_WORN: {
            const auto filter_to_nonfavorite_and_nonworn = [this]( const inventory_entry & entry ) {
                return entry.is_selectable() &&
                       !entry.any_item()->is_favorite &&
                       !u.is_worn( *entry.any_item() );
            };

            selected = get_active_column().get_entries( filter_to_nonfavorite_and_nonworn );
        }
    }

    // Deal with entries that can be highlighted but not selected (e.g. items too large to pick up)
    inventory_entry &highlighted_entry = get_active_column().get_highlighted();
    if( !highlighted_entry.is_selectable() && highlighted_entry.is_item() ) {
        cata_assert( highlighted_entry.denial.has_value() );
        const std::string &denial = *highlighted_entry.denial;

        if( !denial.empty() ) {
            const std::string assembled = highlighted_entry.any_item().get_item()->display_name() + ":\n"
                                          + colorize( denial, c_red );
            if( static_cast<size_t>( utf8_width( denial ) ) > highlighted_entry.cached_denial_space ) {
                popup( assembled, PF_GET_KEY );
            }
        }
        count = 0;
        return;
    }

    // Deal with anything else that can't be selected
    if( selected.empty() ) {
        count = 0;
        return;
    }

    // No amount entered, select all
    if( count == 0 ) {
        bool select_nonfav = true;
        bool select_fav = true;
        switch( mode ) {
            case toggle_mode::SELECTED: {
                count = INT_MAX;

                // Any non favorite item to select?
                select_nonfav = std::any_of( selected.begin(), selected.end(),
                []( const inventory_entry * elem ) {
                    return ( !elem->any_item()->is_favorite ) && elem->chosen_count == 0;
                } );

                // Otherwise, any favorite item to select?
                select_fav = !select_nonfav && std::any_of( selected.begin(), selected.end(),
                []( const inventory_entry * elem ) {
                    return elem->any_item()->is_favorite && elem->chosen_count == 0;
                } );
                break;
            }
            case toggle_mode::NON_FAVORITE_NON_WORN: {
                const bool clear = std::none_of( selected.begin(), selected.end(),
                []( const inventory_entry * elem ) {
                    return elem->chosen_count > 0;
                } );

                if( clear ) {
                    count = max_chosen_count;
                }
                break;
            }
        }

        for( inventory_entry *elem : selected ) {
            const bool is_favorite = elem->any_item()->is_favorite;
            if( ( select_nonfav && !is_favorite ) || ( select_fav && is_favorite ) ) {
                set_chosen_count( *elem, count );
            } else if( !select_nonfav && !select_fav ) {
                // Every element is selected, unselect all
                set_chosen_count( *elem, 0 );
            }
        }
        // Select the entered amount
    } else {
        for( inventory_entry *elem : selected ) {
            set_chosen_count( *elem, count );
        }
    }

    if( !allow_select_contained ) {
        deselect_contained_items();
    }

    selection_col->prepare_paging();
    count = 0;
    on_toggle();
}

drop_locations inventory_multiselector::execute( bool allow_empty )
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    debug_print_timer( tp_start );
    while( true ) {
        ui_manager::redraw();

        const inventory_input input = get_input();

        if( input.action == "CONFIRM" ) {
            if( to_use.empty() && !allow_empty ) {
                popup_getkey( _( "No items were selected.  Use %s to select them." ),
                              ctxt.get_desc( "TOGGLE_ENTRY" ) );
                continue;
            }
            break;
        }

        if( input.action == "QUIT" ) {
            return drop_locations();
        }

        on_input( input );
    }
    drop_locations dropped_pos_and_qty;
    for( const std::pair<item_location, int> &drop_pair : to_use ) {
        dropped_pos_and_qty.push_back( drop_pair );
    }

    return dropped_pos_and_qty;
}

inventory_compare_selector::inventory_compare_selector( Character &p ) :
    inventory_multiselector( p, default_preset, _( "ITEMS TO COMPARE" ) ) {}

std::pair<const item *, const item *> inventory_compare_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    debug_print_timer( tp_start );
    while( true ) {
        ui_manager::redraw();

        const inventory_input input = get_input();

        inventory_entry *just_selected = nullptr;

        if( input.entry != nullptr ) {
            highlight( input.entry->any_item() );
            if( input.action == "SELECT" || input.action == "ANY_INPUT" ) {
                toggle_entry( input.entry );
                just_selected = input.entry;
            } else if( input.action != "MOUSE_MOVE" ) {
                inventory_selector::on_input( input );
            }
        } else if( input.action == "TOGGLE_ENTRY" ) {
            const auto selection( get_active_column().get_all_selected() );

            for( inventory_entry * const &elem : selection ) {
                if( elem->chosen_count == 0 || selection.size() == 1 ) {
                    toggle_entry( elem );
                    just_selected = elem;
                    if( compared.size() == 2 ) {
                        break;
                    }
                }
            }
        } else if( input.action == "CONFIRM" ) {
            popup_getkey( _( "You need two items for comparison.  Use %s to select them." ),
                          ctxt.get_desc( "TOGGLE_ENTRY" ) );
        } else if( input.action == "QUIT" ) {
            return std::make_pair( nullptr, nullptr );
        } else if( input.action == "TOGGLE_FAVORITE" ) {
            // TODO: implement favoriting in multi selection menus while maintaining selection
        } else {
            inventory_selector::on_input( input );
        }

        if( compared.size() == 2 ) {
            const auto res = std::make_pair( compared[0], compared[1] );
            // Clear second selected entry to prevent comparison reopening too
            // soon
            if( just_selected ) {
                toggle_entry( just_selected );
            }
            return res;
        }
    }
}

void inventory_compare_selector::toggle_entry( inventory_entry *entry )
{
    const item *it = &*entry->any_item();
    const auto iter = std::find( compared.begin(), compared.end(), it );

    entry->chosen_count = iter == compared.end() ? 1 : 0;

    if( entry->chosen_count != 0 ) {
        compared.push_back( it );
    } else {
        compared.erase( iter );
    }

    on_change( *entry );
}

inventory_selector::stats inventory_multiselector::get_raw_stats() const
{
    if( get_stats ) {
        return get_stats( to_use );
    }
    return stats{{ stat{{ "", "", "", "" }}, stat{{ "", "", "", "" }} }};
}

inventory_drop_selector::inventory_drop_selector( Character &p,
        const inventory_selector_preset &preset,
        const std::string &selection_column_title,
        const bool warn_liquid ) :
    inventory_multiselector( p, preset, selection_column_title ),
    warn_liquid( warn_liquid )
{
#if defined(__ANDROID__)
    // allow user to type a drop number without dismissing virtual keyboard after each keypress
    ctxt.allow_text_entry = true;
#endif
}

inventory_insert_selector::inventory_insert_selector( Character &p,
        const inventory_holster_preset &preset,
        const std::string &selection_column_title,
        const bool warn_liquid ) :
    inventory_drop_selector( p, preset, selection_column_title, warn_liquid )
{
#if defined(__ANDROID__)
    // allow user to type a drop number without dismissing virtual keyboard after each keypress
    ctxt.allow_text_entry = true;
#endif
}

void inventory_multiselector::deselect_contained_items()
{
    std::vector<item_location> inventory_items;
    for( std::pair<item_location, int> &drop : to_use ) {
        item_location loc_front = drop.first;
        inventory_items.push_back( loc_front );
    }
    for( item_location loc_container : inventory_items ) {
        if( !loc_container->empty() ) {
            for( inventory_column *col : get_all_columns() ) {
                for( inventory_entry *selected : col->get_entries( []( const inventory_entry & entry ) {
                return entry.chosen_count > 0;
            } ) ) {
                    if( !selected->is_item() ) {
                        continue;
                    }
                    for( const item *item_contained : loc_container->all_items_ptr() ) {
                        for( const item_location &selected_loc : selected->locations ) {
                            if( selected_loc.get_item() == item_contained ) {
                                set_chosen_count( *selected, 0 );
                            }
                        }
                    }
                }
            }
        }
    }
    for( inventory_column *col : get_all_columns() ) {
        for( inventory_entry *selected : col->get_entries(
        []( const inventory_entry & entry ) {
        return entry.is_item() && entry.chosen_count > 0 && entry.locations.front()->is_frozen_liquid() &&
                   //Frozen liquids can be selected if it have the SHREDDED flag.
                   !entry.locations.front()->has_flag( STATIC( flag_id( "SHREDDED" ) ) ) &&
                   (
                       ( //Frozen liquids on the map are not selectable if they can't be crushed.
                           entry.locations.front().where() == item_location::type::map &&
                           !get_player_character().can_crush_frozen_liquid( entry.locations.front() ).success() ) ||
                       ( //Weapon in hand is can selectable.
                           entry.locations.front().where() == item_location::type::character &&
                           !entry.locations.front().has_parent() &&
                           entry.locations.front() != get_player_character().used_weapon() ) ||
                       ( //Frozen liquids are unselectable if they don't have SHREDDED flag and can't be crushed in a container.
                           entry.locations.front().has_parent() &&
                           entry.locations.front().where() == item_location::type::container &&
                           !get_player_character().can_crush_frozen_liquid( entry.locations.front() ).success() )
                   );
        } ) ) {
            set_chosen_count( *selected, 0 );
        }
    }
}

void inventory_multiselector::toggle_categorize_contained()
{
    selection_col->clear();
    inventory_selector::toggle_categorize_contained();

    for( inventory_column *col : get_all_columns() ) {
        for( inventory_entry *entry : col->get_entries( return_item, true ) ) {
            if( entry->chosen_count > 0 ) {
                toggle_entry( *entry, entry->chosen_count );
            }
        }
    }
}

void inventory_multiselector::on_input( const inventory_input &input )
{
    if( input.entry != nullptr ) { // Single Item from mouse
        highlight( input.entry->any_item() );
        if( input.action == "SELECT" || input.action == "ANY_INPUT" ) {
            toggle_entries( count );
        }
    }
    if( input.action == "TOGGLE_NON_FAVORITE" ) {
        toggle_entries( count, toggle_mode::NON_FAVORITE_NON_WORN );
    } else if( input.action == "MARK_WITH_COUNT" ) { // Set count and mark selected with specific key
        int query_result = query_count();
        if( query_result >= 0 ) {
            toggle_entries( query_result, toggle_mode::SELECTED );
        }
    } else if( !uistate.numpad_navigation && input.ch >= '0' && input.ch <= '9' ) {
        int query_result = query_count( input.ch, true );
        if( query_result >= 0 ) {
            toggle_entries( query_result, toggle_mode::SELECTED );
        }
    } else if( input.action == "TOGGLE_ENTRY" ) { // Mark selected
        toggle_entries( count, toggle_mode::SELECTED );
    } else if( input.action == "INCREASE_COUNT" || input.action == "DECREASE_COUNT" ) {
        inventory_entry &entry = get_active_column().get_highlighted();
        if( entry.is_selectable() ) {
            size_t const count = entry.chosen_count;
            size_t const max = entry.get_available_count();
            size_t const newcount = input.action == "INCREASE_COUNT"
                                    ? count < max ? count + 1 : max
                                    : count > 1 ? count - 1 : 0;
            toggle_entry( entry, newcount );
        }
    } else if( input.action == "VIEW_CATEGORY_MODE" ) {
        toggle_categorize_contained();
    } else {
        inventory_selector::on_input( input );
    }
}

static const haul_selector_preset haul_preset {};

inventory_haul_selector::inventory_haul_selector( Character &p ) :
    inventory_multiselector( p, haul_preset, _( "ITEMS TO HAUL" ) ) {}

void inventory_haul_selector::apply_selection( std::vector<item_location> &items )
{
    // Finding an entry is cheap, changing it is expensive, so find everything first then modify once
    std::unordered_map<inventory_entry *, int> counts;
    for( item_location &item : items ) {
        inventory_entry *entry = find_entry_by_location( item );
        if( counts.count( entry ) ) {
            counts.at( entry ) += 1;
        } else {
            counts.emplace( entry, 1 );
        }
    }
    for( std::pair<inventory_entry *, int> count : counts ) {
        // count_by_charges items will be moved all at once anyway, this is just to make it look a bit better
        if( count.first->locations.size() == 1 && count.first->locations[0]->count_by_charges() ) {
            set_chosen_count( *count.first, inventory_multiselector::max_chosen_count );
        } else {
            set_chosen_count( *count.first, count.second );
        }
    }
}

bool haul_selector_preset::is_shown( const item_location &item ) const
{
    if( item.where() == item_location::type::container ) {
        return false;
    }

    return true;
}

std::string haul_selector_preset::get_denial( const item_location &item ) const
{
    if( item.where() == item_location::type::container ) {
        return _( "Cannot haul contents of containers" );
    }
    return "";
}

drop_locations inventory_drop_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    debug_print_timer( tp_start );
    while( true ) {
        ui_manager::redraw();

        const inventory_input input = get_input();
        if( input.action == "CONFIRM" ) {
            if( to_use.empty() ) {
                popup_getkey( _( "No items were selected.  Use %s to select them." ),
                              ctxt.get_desc( "TOGGLE_ENTRY" ) );
                continue;
            }
            break;
        }

        if( input.action == "QUIT" ) {
            return drop_locations();
        }

        on_input( input );
    }

    drop_locations dropped_pos_and_qty;

    enum class drop_liquid {
        ask, no, yes
    } should_drop_liquid = drop_liquid::ask;

    for( const std::pair<item_location, int> &drop_pair : to_use ) {
        bool should_drop = true;
        if( drop_pair.first->made_of_from_type( phase_id::LIQUID ) &&
            !drop_pair.first->is_frozen_liquid() ) {
            if( should_drop_liquid == drop_liquid::ask ) {
                if( !warn_liquid || query_yn(
                        _( "You are dropping liquid from its container.  You might not be able to pick it back up.  Really do so?" ) ) ) {
                    should_drop_liquid = drop_liquid::yes;
                } else {
                    should_drop_liquid = drop_liquid::no;
                }
            }
            if( should_drop_liquid == drop_liquid::no ) {
                should_drop = false;
            }
        }
        if( should_drop ) {
            dropped_pos_and_qty.push_back( drop_pair );
        }
    }

    return dropped_pos_and_qty;
}

inventory_selector::stats inventory_drop_selector::get_raw_stats() const
{
    return get_weight_and_volume_and_holster_stats(
               u.weight_carried_with_tweaks( to_use ),
               u.weight_capacity(),
               u.volume_carried_with_tweaks( to_use ),
               u.volume_capacity_with_tweaks( to_use ),
               u.max_single_item_length(), u.max_single_item_volume(),
               u.free_holster_volume(), u.used_holsters(), u.total_holsters() );
}

inventory_selector::stats inventory_insert_selector::get_raw_stats() const
{
    units::mass selected_weight = units::mass();
    units::volume selected_volume = units::volume();
    const item_location &holster = static_cast<const inventory_holster_preset &>
                                   ( preset ).get_holster();
    const std::vector<inventory_column *> &columns = get_all_columns();
    int holstered_items = 0;
    units::volume holster_volume = units::volume();
    units::mass holster_weight = units::mass();
    std::vector<const item_pocket *> used_pockets = std::vector<const item_pocket *>();
    for( const inventory_column *c : columns ) {
        if( c == nullptr ) {
            continue;
        }
        if( c->allows_selecting() ) {
            const inventory_column::get_entries_t entries = c->get_entries( always_yes );
            for( const inventory_entry *e : entries ) {
                if( e == nullptr ) {
                    continue;
                }
                if( e->chosen_count == 0 ) {
                    continue;
                }
                const item *item_to_insert = e->any_item().get_item();
                if( item_to_insert == nullptr ) {
                    continue;
                }
                units::mass w = item_to_insert->weight();
                units::volume v = item_to_insert->volume();
                int overflow_counter = e->chosen_count;
                for( const item_pocket *p : holster.get_item()->get_all_contained_pockets() ) {
                    bool pocket_already_used = false;
                    for( const item_pocket *used_pocket : used_pockets ) {
                        if( p == used_pocket ) {
                            pocket_already_used = true;
                            break;
                        }
                    }
                    if( !pocket_already_used && p->is_holster() && !p->holster_full() ) {
                        ret_val<item_pocket::contain_code> contain = p->can_contain( *item_to_insert );
                        if( contain.success() ) {
                            bool has_better_pocket = false;
                            for( const item_pocket *other_pocket : holster.get_item()->get_all_contained_pockets() ) {
                                if( p == other_pocket ) {
                                    continue;
                                }
                                if( p->better_pocket( *other_pocket, *item_to_insert, false ) ) {
                                    has_better_pocket = true;
                                }
                            }
                            if( has_better_pocket ) {
                                continue;
                            }
                            holstered_items += 1;
                            holster_weight += w;
                            holster_volume += v;
                            used_pockets.push_back( p );
                            overflow_counter -= 1;
                            if( overflow_counter <= 0 ) {
                                break;
                            }
                        }
                    }
                }
                selected_weight += w * overflow_counter;
                selected_volume += v * overflow_counter;
            }
        }
    }

    units::mass contained_weight = holster->get_total_contained_weight() + selected_weight +
                                   holster_weight +
                                   holster->get_used_holster_weight();
    units::mass total_weight = holster->get_total_weight_capacity() + holster_weight;
    units::volume contained_volume = holster->get_total_contained_volume() + selected_volume +
                                     holster_volume;
    units::volume total_volume = holster->get_total_capacity();
    return get_weight_and_volume_and_holster_stats( contained_weight,
            total_weight,
            contained_volume,
            total_volume,
            holster->max_containable_length(),
            holster->max_containable_volume(),
            holster->get_total_holster_volume() - ( holster->get_used_holster_volume() + holster_volume ),
            holster->get_used_holsters() + holstered_items,
            holster->get_total_holsters() );
}

pickup_selector::pickup_selector( Character &p, const inventory_selector_preset &preset,
                                  const std::string &selection_column_title, const std::optional<tripoint> &where ) :
    inventory_multiselector( p, preset, selection_column_title ), where( where )
{
    ctxt.register_action( "WEAR" );
    ctxt.register_action( "WIELD" );
#if defined(__ANDROID__)
    // allow user to type a drop number without dismissing virtual keyboard after each keypress
    ctxt.allow_text_entry = true;
#endif

    set_hint( string_format(
                  _( "%s wield %s wear\n%s expand %s all\n%s examine %s/%s/%s quantity (or type number then %s)" ),
                  colorize( ctxt.get_desc( "WIELD" ), c_yellow ),
                  colorize( ctxt.get_desc( "WEAR" ), c_yellow ),
                  colorize( ctxt.get_desc( "SHOW_HIDE_CONTENTS" ), c_yellow ),
                  colorize( ctxt.get_desc( "SHOW_HIDE_CONTENTS_ALL" ), c_yellow ),
                  colorize( ctxt.get_desc( "EXAMINE" ), c_yellow ),
                  colorize( ctxt.get_desc( "MARK_WITH_COUNT" ), c_yellow ),
                  colorize( ctxt.get_desc( "INCREASE_COUNT" ), c_yellow ),
                  colorize( ctxt.get_desc( "DECREASE_COUNT" ), c_yellow ),
                  colorize( ctxt.get_desc( "TOGGLE_ENTRY" ), c_yellow ) ) );
}

void pickup_selector::apply_selection( std::vector<drop_location> selection )
{
    for( drop_location &loc : selection ) {
        inventory_entry *entry = find_entry_by_location( loc.first );
        if( entry != nullptr ) {
            set_chosen_count( *entry, loc.second + entry->chosen_count );
        }
    }
}

drop_locations pickup_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    debug_print_timer( tp_start );

    while( true ) {
        ui_manager::redraw();

        const inventory_input input = get_input();

        if( input.action == "CONFIRM" ) {
            if( to_use.empty() ) {
                popup_getkey( _( "No items were selected.  Use %s to select them." ),
                              ctxt.get_desc( "TOGGLE_ENTRY" ) );
                continue;
            }
            break;
        } else if( input.action == "WIELD" ) {
            if( wield( count ) ) {
                return drop_locations();
            }
        } else if( input.action == "WEAR" ) {
            if( wear() ) {
                return drop_locations();
            }
        } else if( input.action == "QUIT" ) {
            return drop_locations();
        } else {
            on_input( input );
        }
    }
    drop_locations dropped_pos_and_qty;
    for( const std::pair<item_location, int> &drop_pair : to_use ) {
        dropped_pos_and_qty.push_back( drop_pair );
    }

    return dropped_pos_and_qty;
}

bool pickup_selector::wield( int &count )
{
    inventory_entry &selected = get_active_column().get_highlighted();
    if( !selected.is_item() ) {
        return false;
    }

    item_location it = selected.any_item();
    if( count == 0 ) {
        count = INT_MAX;
    }
    int charges = std::min( it->charges, count );

    if( u.can_wield( *it ).success() ) {
        remove_from_to_use( it );
        add_reopen_activity();
        u.assign_activity( wield_activity_actor( it, charges ) );
        return true;
    } else {
        popup_getkey( u.can_wield( *it ).c_str() );
    }

    return false;
}

bool pickup_selector::wear()
{
    inventory_entry &selected = get_active_column().get_highlighted();
    if( !selected.is_item() ) {
        return false;
    }

    std::vector<item_location> items{ selected.any_item() };
    std::vector<int> quantities{ 0 };

    if( u.can_wear( *items.front() ).success() ) {
        remove_from_to_use( items.front() );
        add_reopen_activity();
        u.assign_activity( wear_activity_actor( items, quantities ) );
        return true;
    } else {
        popup_getkey( _( "You can't wear the %s." ), items.front()->display_name() );
    }

    return false;
}

void pickup_selector::add_reopen_activity()
{
    u.assign_activity( pickup_menu_activity_actor( where, to_use ) );
    u.activity.auto_resume = true;
}

void pickup_selector::remove_from_to_use( item_location &it )
{
    for( auto iter = to_use.begin(); iter < to_use.end(); ) {
        if( iter->first == it ) {
            to_use.erase( iter );
            return;
        } else {
            iter++;
        }
    }
}

void pickup_selector::reassign_custom_invlets()
{
    if( invlet_type() == SELECTOR_INVLET_DEFAULT ) {
        set_invlet_type( SELECTOR_INVLET_ALPHA );
    }
    inventory_selector::reassign_custom_invlets();
}

inventory_selector::stats pickup_selector::get_raw_stats() const
{
    units::mass weight;
    units::volume volume;

    for( const drop_location &loc : to_use ) {
        if( loc.first->count_by_charges() ) {
            item copy( *loc.first.get_item() );
            copy.charges = loc.second;
            weight += copy.weight();
            volume += copy.volume();
        } else {
            weight += loc.first->weight() * loc.second;
            volume += loc.first->volume() * loc.second;
        }
    }

    return get_weight_and_volume_and_holster_stats(
               u.weight_carried() + weight,
               u.weight_capacity(),
               u.volume_carried() + volume,
               u.volume_capacity(),
               u.max_single_item_length(),
               u.max_single_item_volume(),
               u.free_holster_volume(),
               u.used_holsters(),
               u.total_holsters() );
}

bool inventory_examiner::check_parent_item()
{
    return !( parent_item->is_container_empty() || empty() );
}

int inventory_examiner::cleanup() const
{
    if( changes_made ) {
        return EXAMINED_CONTENTS_WITH_CHANGES;
    } else {
        return EXAMINED_CONTENTS_UNCHANGED;
    }
}

void inventory_examiner::draw_item_details( const item_location &sitem )
{
    std::vector<iteminfo> vThisItem;
    std::vector<iteminfo> vDummy;

    sitem->info( true, vThisItem );

    item_info_data data( sitem->tname(), sitem->type_name(), vThisItem, vDummy, examine_window_scroll );
    data.without_getch = true;

    draw_item_info( w_examine, data );
}

void inventory_examiner::force_max_window_size()
{
    constexpr int border_width = 1;
    _fixed_size = { TERMX / 3 + 2 * border_width, TERMY };
    _fixed_origin = point_zero;
}

int inventory_examiner::execute()
{
    if( !check_parent_item() ) {
        return NO_CONTENTS_TO_EXAMINE;
    }
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    debug_print_timer( tp_start );

    ui_adaptor ui_examine;

    ui_examine.on_screen_resize( [&]( ui_adaptor & ui_examine ) {
        force_max_window_size();
        ui->mark_resize();

        int const width = TERMX - _fixed_size.x;
        int const height = TERMY;
        point const start_position = point( TERMX - width, 0 );

        scroll_item_info_lines = TERMY / 2;

        w_examine = catacurses::newwin( height, width, start_position );
        ui_examine.position_from_window( w_examine );
    } );
    ui_examine.mark_resize();

    ui_examine.on_redraw( [&]( const ui_adaptor & ) {
        const inventory_entry &selected = get_active_column().get_highlighted();
        if( selected ) {
            if( selected_item != selected.any_item() ) {
                //A new item has been selected, reset scrolling
                examine_window_scroll = 0;
                selected_item = selected.any_item();
            }
            draw_item_details( selected_item );
        }
    } );

    while( true ) {
        /* Since ui_examine is the most recently created ui_adaptor, it will always be redrawn.
         The item list will only be redrawn when specifically invalidated */
        ui_manager::redraw();

        const inventory_input input = get_input();

        if( input.entry != nullptr ) {
            if( highlight( input.entry->any_item() ) ) {
                ui_manager::redraw();
            }
            if( input.action == "SELECT" ) {
                return cleanup();
            }
        }

        if( input.action == "QUIT" || input.action == "CONFIRM" ) {
            return cleanup();
        } else if( input.action == "PAGE_UP" || input.action == "SCROLL_UP" ) {
            examine_window_scroll -= scroll_item_info_lines;
        } else if( input.action == "PAGE_DOWN" || input.action == "SCROLL_DOWN" ) {
            examine_window_scroll += scroll_item_info_lines;
        } else {
            ui->invalidate_ui(); //The player is probably doing something that requires updating the base window
            if( input.action == "SHOW_HIDE_CONTENTS" ) {
                changes_made = true;
            }
            on_input( input );
        }
    }
}

void inventory_examiner::setup()
{
    if( parent_item == item_location::nowhere ) {
        set_title( "ERROR: Item not found" );
    } else {
        set_title( parent_item->display_name() );
    }
}

unload_selector::unload_selector( Character &p,
                                  const inventory_selector_preset &preset ) : inventory_pick_selector( p, preset )
{
    ctxt.register_action( "CONTAIN_MODE" );
    set_hint( hint_string() );
}

std::string unload_selector::hint_string()
{
    std::string mode = uistate.unload_auto_contain ? _( "Auto" ) : _( "Manual" );
    return string_format(
               _( "[<color_yellow>%s</color>] Confirm [<color_yellow>%s</color>] Cancel [<color_yellow>%s</color>] Contain mode(<color_yellow>%s</color>)" ),
               ctxt.get_desc( "CONFIRM" ), ctxt.get_desc( "QUIT" ), ctxt.get_desc( "CONTAIN_MODE" ), mode );
}

std::pair<item_location, bool> unload_selector::execute()
{
    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();
    item_location startDragItem;
    bool dragActive = false;
    while( true ) {
        ui_manager::redraw();
        const inventory_input input = get_input();
        if( input.entry != nullptr ) {
            if( drag_enabled && input.action == "CLICK_AND_DRAG" ) {
                if( input.entry->is_item() ) {
                    dragActive = true;
                    startDragItem = input.entry->locations.front();
                }
            } else if( input.action == "MOUSE_MOVE" ) {
                if( !dragActive && highlight( input.entry->any_item() ) ) {
                    ui_manager::redraw();
                }
            } else if( input.action == "SELECT" ) {
                dragActive = false;
                item_location startDragItemCpy = startDragItem;
                startDragItem = item_location();
                item_location endDragItem;
                if( input.entry->is_item() ) {
                    endDragItem = input.entry->locations.front();
                    if( startDragItemCpy && endDragItem && startDragItemCpy != endDragItem ) {
                        if( drag_drop_item( startDragItemCpy.get_item(), endDragItem.get_item() ) ) {
                            clear_items();
                            add_character_items( u );
                        }
                    } else {
                        return {input.entry->any_item(), uistate.unload_auto_contain};
                    }
                }
            } else if( input.action == "ANY_INPUT" ) {
                return {input.entry->any_item(), uistate.unload_auto_contain};
            } else {
                if( !dragActive && highlight( input.entry->any_item() ) ) {
                    ui_manager::redraw();
                }
                on_input( input );
            }
        } else if( input.action == "QUIT" ) {
            return { item_location(), uistate.unload_auto_contain };
        } else if( input.action == "CONFIRM" ) {
            const inventory_entry &highlighted = get_active_column().get_highlighted();
            if( highlighted && highlighted.is_selectable() ) {
                return { highlighted.any_item(), uistate.unload_auto_contain };
            }
        } else if( input.action == "CONTAIN_MODE" ) {
            uistate.unload_auto_contain = !uistate.unload_auto_contain;
            set_hint( hint_string() );
        } else {
            on_input( input );
        }
    }
}

trade_selector::trade_selector( trade_ui *parent, Character &u,
                                inventory_selector_preset const &preset,
                                std::string const &selection_column_title,
                                point const &size, point const &origin )
    : inventory_drop_selector( u, preset, selection_column_title ), _parent( parent ),
      _ctxt_trade( "INVENTORY", keyboard_mode::keychar )
{
    _ctxt_trade.register_action( ACTION_SWITCH_PANES );
    _ctxt_trade.register_action( ACTION_TRADE_CANCEL );
    _ctxt_trade.register_action( ACTION_TRADE_OK );
    _ctxt_trade.register_action( ACTION_AUTOBALANCE );
    _ctxt_trade.register_action( ACTION_BANKBALANCE );
    _ctxt_trade.register_action( "ANY_INPUT" );
    // duplicate this action in the parent ctxt so it shows up in the keybindings menu
    // CANCEL and OK are already set in inventory_selector
    ctxt.register_action( ACTION_SWITCH_PANES );
    ctxt.register_action( ACTION_AUTOBALANCE );
    ctxt.register_action( ACTION_BANKBALANCE );
    resize( size, origin );
    _ui = create_or_get_ui_adaptor();
}

trade_selector::select_t trade_selector::to_trade() const
{
    return to_use;
}

void trade_selector::execute()
{
    debug_print_timer( tp_start );
    bool exit = false;

    get_active_column().on_activate();

    while( !exit ) {
        _ui->invalidate_ui();
        ui_manager::redraw_invalidated();
        std::string const &action = _ctxt_trade.handle_input();
        if( action == ACTION_SWITCH_PANES ) {
            _parent->pushevent( trade_ui::event::SWITCH );
            get_active_column().on_deactivate();
            exit = true;
        } else if( action == ACTION_TRADE_OK ) {
            _parent->pushevent( trade_ui::event::TRADEOK );
            exit = true;
        } else if( action == ACTION_TRADE_CANCEL ) {
            _parent->pushevent( trade_ui::event::TRADECANCEL );
            exit = true;
        } else if( action == ACTION_AUTOBALANCE ) {
            _parent->autobalance();
        } else if( action == ACTION_BANKBALANCE ) {
            _parent->bank_balance();
        } else {
            input_event const iev = _ctxt_trade.get_raw_input();
            inventory_input const input =
                process_input( ctxt.input_to_action( iev ), iev.get_first_input() );
            inventory_drop_selector::on_input( input );
            if( input.action == "HELP_KEYBINDINGS" ) {
                ctxt.display_menu();
            }
        }
    }
}

void trade_selector::on_toggle()
{
    _parent->recalc_values_cpane();
}

void trade_selector::resize( point const &size, point const &origin )
{
    _fixed_size = size;
    _fixed_origin = origin;
    if( _ui ) {
        _ui->mark_resize();
    }
}

shared_ptr_fast<ui_adaptor> trade_selector::get_ui() const
{
    return _ui;
}

input_context const *trade_selector::get_ctxt() const
{
    return &_ctxt_trade;
}

void inventory_selector::categorize_map_items( bool toggle )
{
    _categorize_map_items = toggle;
}
