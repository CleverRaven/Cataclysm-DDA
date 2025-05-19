#include "clzones.h"

#include <algorithm>
#include <climits>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <string>
#include <tuple>

#include "avatar.h"
#include "calendar.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "construction.h"
#include "construction_group.h"
#include "debug.h"
#include "field_type.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "iexamine.h"
#include "item.h"
#include "item_category.h"
#include "item_group.h"
#include "item_pocket.h"
#include "item_search.h"
#include "itype.h"
#include "json.h"
#include "localized_comparator.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "memory_fast.h"
#include "output.h"
#include "path_info.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "uilist.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "visitable.h"
#include "vpart_position.h"

static const faction_id faction_your_followers( "your_followers" );

static const item_category_id item_category_food( "food" );

static const itype_id itype_disassembly( "disassembly" );

static const zone_type_id zone_type_AUTO_DRINK( "AUTO_DRINK" );
static const zone_type_id zone_type_AUTO_EAT( "AUTO_EAT" );
static const zone_type_id zone_type_CAMP_FOOD( "CAMP_FOOD" );
static const zone_type_id zone_type_CONSTRUCTION_BLUEPRINT( "CONSTRUCTION_BLUEPRINT" );
static const zone_type_id zone_type_DISASSEMBLE( "DISASSEMBLE" );
static const zone_type_id zone_type_FARM_PLOT( "FARM_PLOT" );
static const zone_type_id zone_type_LOOT_CORPSE( "LOOT_CORPSE" );
static const zone_type_id zone_type_LOOT_CUSTOM( "LOOT_CUSTOM" );
static const zone_type_id zone_type_LOOT_DEFAULT( "LOOT_DEFAULT" );
static const zone_type_id zone_type_LOOT_DRINK( "LOOT_DRINK" );
static const zone_type_id zone_type_LOOT_FOOD( "LOOT_FOOD" );
static const zone_type_id zone_type_LOOT_IGNORE( "LOOT_IGNORE" );
static const zone_type_id zone_type_LOOT_ITEM_GROUP( "LOOT_ITEM_GROUP" );
static const zone_type_id zone_type_LOOT_PDRINK( "LOOT_PDRINK" );
static const zone_type_id zone_type_LOOT_PFOOD( "LOOT_PFOOD" );
static const zone_type_id zone_type_LOOT_SEEDS( "LOOT_SEEDS" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_LOOT_WOOD( "LOOT_WOOD" );
static const zone_type_id zone_type_NO_AUTO_PICKUP( "NO_AUTO_PICKUP" );
static const zone_type_id zone_type_NO_NPC_PICKUP( "NO_NPC_PICKUP" );
static const zone_type_id zone_type_SOURCE_FIREWOOD( "SOURCE_FIREWOOD" );
static const zone_type_id zone_type_STRIP_CORPSES( "STRIP_CORPSES" );
static const zone_type_id zone_type_UNLOAD_ALL( "UNLOAD_ALL" );

const std::vector<zone_type_id> ignorable_zone_types = {
    zone_type_AUTO_EAT,
    zone_type_AUTO_DRINK,
    zone_type_DISASSEMBLE,
    zone_type_SOURCE_FIREWOOD,
};

static const std::unordered_map< std::string, zone_type_id> legacy_zone_types = {
    {"zone_disassemble", zone_type_DISASSEMBLE},
    {"zone_strip", zone_type_STRIP_CORPSES},
    {"zone_unload_all", zone_type_UNLOAD_ALL}
};

zone_manager::zone_manager()
{
    for( const zone_type &zone : zone_type::get_all() ) {
        types.emplace( zone.id, zone );
    }
}

void zone_manager::clear()
{
    zones.clear();
    added_vzones.clear();
    changed_vzones.clear();
    removed_vzones.clear();
    // Do not clear types since it is needed for the next games.
    area_cache.clear();
    vzone_cache.clear();
}

std::string zone_type::name() const
{
    return name_.translated();
}

std::string zone_type::desc() const
{
    return desc_.translated();
}

field_type_str_id zone_type::get_field() const
{
    return field_;
}

namespace
{
generic_factory<zone_type> zone_type_factory( "zone_type" );
} // namespace

template<>
const zone_type &string_id<zone_type>::obj() const
{
    return zone_type_factory.obj( *this );
}

template<>
bool string_id<zone_type>::is_valid() const
{
    return zone_type_factory.is_valid( *this );
}

const std::vector<zone_type> &zone_type::get_all()
{
    return zone_type_factory.get_all();
}

void zone_type::load_zones( const JsonObject &jo, const std::string &src )
{
    zone_type_factory.load( jo, src );
}

void zone_type::reset()
{
    zone_type_factory.reset();
}

void zone_type::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "display_field", field_ );
    optional( jo, was_loaded, "description", desc_, translation() );
    optional( jo, was_loaded, "can_be_personal", can_be_personal );
    optional( jo, was_loaded, "hidden", hidden );
}

shared_ptr_fast<zone_options> zone_options::create( const zone_type_id &type )
{
    if( type == zone_type_FARM_PLOT ) {
        return make_shared_fast<plot_options>();
    } else if( type == zone_type_CONSTRUCTION_BLUEPRINT ) {
        return make_shared_fast<blueprint_options>();
    } else if( type == zone_type_LOOT_CUSTOM || type == zone_type_LOOT_ITEM_GROUP ) {
        return make_shared_fast<loot_options>();
    } else if( type == zone_type_UNLOAD_ALL ) {
        return make_shared_fast<unload_options>();
    } else if( std::find( ignorable_zone_types.begin(), ignorable_zone_types.end(),
                          type ) != ignorable_zone_types.end() ) {
        return make_shared_fast<ignorable_options>();
    }

    return make_shared_fast<zone_options>();
}

bool zone_options::is_valid( const zone_type_id &type, const zone_options &options )
{
    if( type == zone_type_FARM_PLOT ) {
        return dynamic_cast<const plot_options *>( &options ) != nullptr;
    } else if( type == zone_type_CONSTRUCTION_BLUEPRINT ) {
        return dynamic_cast<const blueprint_options *>( &options ) != nullptr;
    } else if( type == zone_type_LOOT_CUSTOM || type == zone_type_LOOT_ITEM_GROUP ) {
        return dynamic_cast<const loot_options *>( &options ) != nullptr;
    } else if( type == zone_type_UNLOAD_ALL ) {
        return dynamic_cast<const unload_options *>( &options ) != nullptr;
    } else if( std::find( ignorable_zone_types.begin(), ignorable_zone_types.end(),
                          type ) != ignorable_zone_types.end() ) {
        return dynamic_cast<const ignorable_options *>( &options ) != nullptr;
    }

    // ensure options is not derived class for the rest of zone types
    return !options.has_options();
}

construction_id blueprint_options::get_final_construction(
    const std::vector<construction> &list_constructions,
    const construction_id &idx,
    std::set<construction_id> &skip_index
)
{
    const construction &con = idx.obj();
    if( con.post_terrain.empty() ) {
        return idx;
    }

    for( int i = 0; i < static_cast<int>( list_constructions.size() ); ++i ) {
        if( construction_id( i ) == idx || skip_index.find( construction_id( i ) ) != skip_index.end() ) {
            continue;
        }
        const construction &con_next = list_constructions[i];
        if( con.group == con_next.group &&
            ( con_next.pre_terrain.find( con.post_terrain ) != con_next.pre_terrain.end() ) ) {
            skip_index.insert( idx );
            return get_final_construction( list_constructions, construction_id( i ), skip_index );
        }
    }

    return idx;
}

blueprint_options::query_con_result blueprint_options::query_con()
{
    construction_id con_index = construction_menu( true );
    if( con_index.is_valid() ) {
        const std::vector<construction> &list_constructions = get_constructions();
        std::set<construction_id> skip_index;
        con_index = get_final_construction( list_constructions, con_index, skip_index );

        const construction &chosen = con_index.obj();

        const construction_group_str_id &chosen_group = chosen.group;
        const std::string &chosen_mark = chosen.post_terrain;

        if( con_index != index || chosen_group != group || chosen_mark != mark ) {
            group = chosen_group;
            mark = chosen_mark;
            index = con_index;
            return changed;
        } else {
            return successful;
        }
    } else {
        return canceled;
    }
}

ignorable_options::query_ignorable_result ignorable_options::query_ignorable()
{
    ignore_contents = query_yn( _( "Ignore items in this area when sorting?" ) );
    return changed;
}

loot_options::query_loot_result loot_options::query_loot()
{
    string_input_popup()
    .title( _( "Filter:" ) )
    .description( item_filter_rule_string( item_filter_type::FILTER ) + "\n\n" )
    .desc_color( c_white )
    .width( 55 )
    .identifier( "item_filter" )
    .max_length( 256 )
    .edit( mark );
    return changed;
}

unload_options::query_unload_result unload_options::query_unload()
{
    molle = query_yn( _( "Detach MOLLE attached pouches?" ) );
    mods = query_yn(
               _( "Detach mods from weapons?  (Be careful as you may not have the skills to reattach them)" ) );
    sparse_only = query_yn( string_format(
                                _( "Avoid unloading items stacks (not charges) greater than a certain amount?  (Amount defined in next window)" ) ) );
    if( sparse_only ) {
        int threshold = 20;
        if( query_int( threshold, true,
                       _( "What is the maximum stack size to unload?" ) ) ) {
            if( sparse_threshold < 1 ) {
                sparse_threshold = 1;
            } else {
                sparse_threshold = threshold;
            }
        } else {
            return canceled;
        }
    }
    always_unload = query_yn(
                        _( "Always unload?  (Unload even if the container has a valid sorting location)" ) );
    return changed;
}

plot_options::query_seed_result plot_options::query_seed()
{
    Character &player_character = get_player_character();
    std::vector<item *> seed_inv = player_character.cache_get_items_with( "is_seed", &item::is_seed );
    zone_manager &mgr = zone_manager::get_manager();
    map &here = get_map();
    const std::unordered_set<tripoint_abs_ms> zone_src_set =
        mgr.get_near( zone_type_LOOT_SEEDS, here.get_abs( player_character.pos_bub() ),
                      MAX_VIEW_DISTANCE );
    for( const tripoint_abs_ms &elem : zone_src_set ) {
        tripoint_bub_ms elem_loc = here.get_bub( elem );
        for( item &it : here.i_at( elem_loc ) ) {
            if( it.is_seed() ) {
                seed_inv.push_back( &it );
            }
        }
    }
    std::vector<seed_tuple> seed_entries = iexamine::get_seed_entries( seed_inv );
    seed_entries.emplace( seed_entries.begin(), itype_id::NULL_ID(), _( "No seed" ), 0 );

    int seed_index = iexamine::query_seed( seed_entries );

    if( seed_index > 0 && seed_index < static_cast<int>( seed_entries.size() ) ) {
        const auto &seed_entry = seed_entries[seed_index];
        const itype_id &new_seed = std::get<0>( seed_entry );
        itype_id new_mark;

        item it = item( new_seed );
        if( it.is_seed() ) {
            new_mark = it.type->seed->fruit_id;
        } else {
            new_mark = seed;
        }

        if( new_seed != seed || new_mark != mark ) {
            seed = new_seed;
            mark = new_mark;
            return changed;
        } else {
            return successful;
        }
    } else if( seed_index == 0 ) {
        // No seeds
        if( !seed.is_empty() || !mark.is_empty() ) {
            seed = itype_id();
            mark = itype_id();
            return changed;
        } else {
            return successful;
        }
    } else {
        return canceled;
    }
}

bool blueprint_options::query_at_creation()
{
    return query_con() != canceled;
}

bool ignorable_options::query_at_creation()
{
    return query_ignorable() != canceled;
}

bool loot_options::query_at_creation()
{
    return query_loot() != canceled;
}

bool plot_options::query_at_creation()
{
    return query_seed() != canceled;
}

bool unload_options::query_at_creation()
{
    return query_unload() != canceled;
}

bool blueprint_options::query()
{
    return query_con() == changed;
}

bool ignorable_options::query()
{
    return query_ignorable() == changed;
}

bool loot_options::query()
{
    return query_loot() == changed;
}

bool plot_options::query()
{
    return query_seed() == changed;
}

bool unload_options::query()
{
    return query_unload() == changed;
}

std::string blueprint_options::get_zone_name_suggestion() const
{
    if( group ) {
        return group->name();
    }

    return _( "No construction" );
}

std::string loot_options::get_zone_name_suggestion() const
{
    if( !mark.empty() ) {
        return string_format( _( "Loot: Custom: %s" ), mark );
    }
    return _( "Loot: Custom: No Filter" );
}

std::string plot_options::get_zone_name_suggestion() const
{
    if( !seed.is_empty() ) {
        const itype_id type = itype_id( seed );
        item it = item( type );
        if( it.is_seed() ) {
            return it.type->seed->plant_name.translated();
        } else {
            return item::nname( type );
        }
    }

    return _( "No seed" );
}

std::string unload_options::get_zone_name_suggestion() const
{

    return string_format( "%s%s%s%s%s", _( "Unload: " ),
                          mods ? _( "mods, " ) : "",
                          molle ? _( "MOLLE, " ) : "",
                          sparse_only ? string_format( _( "ignore stacks over %i, " ), sparse_threshold ) : "",
                          always_unload ? _( "unload all" ) : _( "unload unmatched" ) );
}

std::vector<std::pair<std::string, std::string>> blueprint_options::get_descriptions() const
{
    std::vector<std::pair<std::string, std::string>> options =
                std::vector<std::pair<std::string, std::string>>();
    options.emplace_back( _( "Construct: " ),
                          group ? group->name() : _( "No Construction" ) );

    return options;
}

std::vector<std::pair<std::string, std::string>> ignorable_options::get_descriptions() const
{
    std::vector<std::pair<std::string, std::string>> options;
    options.emplace_back( _( "Ignore contents: " ),
                          string_format( "%s", ignore_contents ? _( "True" ) : _( "False" ) ) );
    return options;
}

std::vector<std::pair<std::string, std::string>> loot_options::get_descriptions() const
{
    std::vector<std::pair<std::string, std::string>> options;
    options.emplace_back( _( "Filter: " ),
                          !mark.empty() ? mark : _( "No filter" ) );

    return options;
}

std::vector<std::pair<std::string, std::string>> plot_options::get_descriptions() const
{
    auto options = std::vector<std::pair<std::string, std::string>>();
    options.emplace_back(
        _( "Plant seed: " ),
        !seed.is_empty() ? item::nname( itype_id( seed ) ) : _( "No seed" ) );

    return options;
}

std::vector<std::pair<std::string, std::string>> unload_options::get_descriptions() const
{
    std::vector<std::pair<std::string, std::string>> options;
    options.emplace_back( _( "Unload: " ),
                          string_format( "%s%s%s%s",
                                         mods ? _( "mods " ) : "",
                                         molle ? _( "MOLLE " ) : "",
                                         sparse_only ? string_format( _( "ignore stacks over %i, " ), sparse_threshold ) : "",
                                         always_unload ? _( "unload all" ) : _( "unload unmatched" ) ) );

    return options;
}

void blueprint_options::serialize( JsonOut &json ) const
{
    json.member( "mark", mark );
    json.member( "group", group );
    json.member( "index", index.id() );
}

void blueprint_options::deserialize( const JsonObject &jo_zone )
{
    jo_zone.read( "mark", mark );
    jo_zone.read( "group", group );
    if( jo_zone.has_int( "index" ) ) {
        // Oops, saved incorrectly as an int id by legacy code. Just load it and hope for the best
        index = construction_id( jo_zone.get_int( "index" ) );
    } else {
        index = construction_str_id( jo_zone.get_string( "index" ) ).id();
    }
}

void ignorable_options::serialize( JsonOut &json ) const
{
    json.member( "ignore_contents", ignore_contents );
}

void ignorable_options::deserialize( const JsonObject &jo_zone )
{
    jo_zone.read( "ignore_contents", ignore_contents );
}

void loot_options::serialize( JsonOut &json ) const
{
    json.member( "mark", mark );
}

void loot_options::deserialize( const JsonObject &jo_zone )
{
    jo_zone.read( "mark", mark );
}

void plot_options::serialize( JsonOut &json ) const
{
    json.member( "mark", mark );
    json.member( "seed", seed );
}

void plot_options::deserialize( const JsonObject &jo_zone )
{
    jo_zone.read( "mark", mark );
    jo_zone.read( "seed", seed );
}

void unload_options::serialize( JsonOut &json ) const
{
    json.member( "mark", mark );
    json.member( "mods", mods );
    json.member( "molle", molle );
    json.member( "sparse_only", sparse_only );
    json.member( "sparse_threshold", sparse_threshold );
    json.member( "always_unload", always_unload );
}

void unload_options::deserialize( const JsonObject &jo_zone )
{
    jo_zone.read( "mark", mark );
    jo_zone.read( "mods", mods );
    jo_zone.read( "molle", molle );
    jo_zone.read( "sparse_only", sparse_only );
    jo_zone.read( "sparse_threshold", sparse_threshold );
    jo_zone.read( "always_unload", always_unload );
}

std::optional<std::string> zone_manager::query_name( const std::string &default_name ) const
{
    string_input_popup popup;
    popup
    .title( _( "Zone name:" ) )
    .width( 55 )
    .text( default_name )
    .query();
    if( popup.canceled() ) {
        return {};
    } else {
        return popup.text();
    }
}

static std::string wrap60( const std::string &text )
{
    return string_join( foldstring( text, 60 ), "\n" );
}

std::optional<zone_type_id> zone_manager::query_type( bool personal ) const
{
    const auto &types = get_manager().get_types();

    // Copy zone types into an array and sort by name
    std::vector<std::pair<zone_type_id, zone_type>> types_vec;
    // only add personal functioning zones for personal
    if( personal ) {
        for( const auto &tmp : types ) {
            if( tmp.second.can_be_personal ) {
                types_vec.emplace_back( tmp );
            }
        }
    } else {
        std::copy_if( types.begin(), types.end(), std::back_inserter( types_vec ),
        []( std::pair<zone_type_id, zone_type> const & it ) {
            return !it.first.is_valid() || !it.first->hidden;
        } );
    }
    std::sort( types_vec.begin(), types_vec.end(),
    []( const std::pair<zone_type_id, zone_type> &lhs, const std::pair<zone_type_id, zone_type> &rhs ) {
        return localized_compare( lhs.second.name(), rhs.second.name() );
    } );

    uilist as_m;
    as_m.desc_enabled = true;
    as_m.title = _( "Select zone type:" );

    size_t i = 0;
    for( const auto &pair : types_vec ) {
        const zone_type &type = pair.second;

        as_m.addentry_desc( i++, true, MENU_AUTOASSIGN, type.name(), wrap60( type.desc() ) );
    }

    as_m.query();
    if( as_m.ret < 0 ) {
        return {};
    }
    size_t index = as_m.ret;

    auto iter = types_vec.begin();
    std::advance( iter, index );

    return iter->first;
}

bool zone_data::set_name()
{
    const auto maybe_name = zone_manager::get_manager().query_name( name );
    if( maybe_name.has_value() ) {
        auto new_name = maybe_name.value();
        if( new_name.empty() ) {
            new_name = _( "<no name>" );
        }
        if( name != new_name ) {
            zone_manager::get_manager().zone_edited( *this );
            name = new_name;
            return true;
        }
    }
    return false;
}

bool zone_data::set_type()
{
    const auto maybe_type = zone_manager::get_manager().query_type( is_personal );
    if( maybe_type.has_value() && maybe_type.value() != type ) {
        shared_ptr_fast<zone_options> new_options = zone_options::create( maybe_type.value() );
        if( new_options->query_at_creation() ) {
            zone_manager::get_manager().zone_edited( *this );
            type = maybe_type.value();
            options = new_options;
            zone_manager::get_manager().cache_data();
            return true;
        }
    }
    // False positive from memory leak detection on shared_ptr_fast
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    return false;
}

void zone_data::set_position( const std::pair<tripoint_abs_ms, tripoint_abs_ms> &position,
                              const bool manual, bool update_avatar, bool skip_cache_update, bool suppress_display_update )
{
    if( is_vehicle && manual ) {
        debugmsg( "Tried moving a lootzone bound to a vehicle part" );
        return;
    }

    if( is_personal ) {
        debugmsg( "Tried moving a personal lootzone to absolute location" );
        return;
    }

    bool adjust_display = is_displayed && !suppress_display_update;

    if( adjust_display ) {
        toggle_display();
    }

    start = position.first;
    end = position.second;

    if( adjust_display ) {
        toggle_display();
    }

    if( !skip_cache_update ) {
        zone_manager::get_manager().cache_data( update_avatar );
    }
}

void zone_data::set_position( const std::pair<tripoint_rel_ms, tripoint_rel_ms> &position,
                              const bool manual, bool update_avatar, bool skip_cache_update, bool suppress_display_update )
{
    if( is_vehicle && manual ) {
        debugmsg( "Tried moving a lootzone bound to a vehicle part" );
        return;
    }

    if( !is_personal ) {
        debugmsg( "Tried moving a normal lootzone to relative location" );
        return;
    }

    bool adjust_display = is_displayed && !suppress_display_update;

    if( adjust_display ) {
        toggle_display();
    }

    personal_start = position.first;
    personal_end = position.second;

    if( adjust_display ) {
        toggle_display();
    }

    if( !skip_cache_update ) {
        zone_manager::get_manager().cache_data( update_avatar );
    }
}

void zone_data::set_enabled( const bool enabled_arg )
{
    zone_manager::get_manager().zone_edited( *this );
    enabled = enabled_arg;
}

void zone_data::set_temporary_disabled( const bool enabled_arg )
{
    temporarily_disabled = enabled_arg;
}

// This operations can presumably be defined using templates. It should also already exist somewhere else.
static std::pair<tripoint_abs_ms, tripoint_abs_ms> get_corners( tripoint_abs_ms a,
        tripoint_abs_ms b )
{
    const tripoint_abs_ms start = tripoint_abs_ms( std::min( a.x(), b.x() ), std::min( a.y(), b.y() ),
                                  std::min( a.z(), b.z() ) );
    const tripoint_abs_ms end = tripoint_abs_ms( std::max( a.x(), b.x() ), std::max( a.y(), b.y() ),
                                std::max( a.z(), b.z() ) );
    return { start, end };
}

void zone_data::refresh_display() const
{
    if( this->is_vehicle ) {
        popup( colorize( _( "Zones tied to vehicles cannot be displayed" ), c_magenta ) );
        return;
    }

    std::unique_ptr<tinymap> p_update_tmap = std::make_unique<tinymap>();
    tinymap &update_tmap = *p_update_tmap;

    std::pair<tripoint_abs_ms, tripoint_abs_ms> bounds = get_corners( get_start_point(),
            get_end_point() );
    const tripoint_abs_ms start = bounds.first;
    const tripoint_abs_ms end = bounds.second;

    const tripoint_abs_omt omt_start_pos = coords::project_to<coords::omt>( start );
    const tripoint_abs_omt omt_end_pos = coords::project_to<coords::omt>( end );
    const tripoint_omt_ms start_remainder = tripoint_omt_ms( ( get_start_point() - coords::project_to
                                            < coords::ms >
                                            ( omt_start_pos ) ).raw() );
    const tripoint_omt_ms end_remainder = tripoint_omt_ms( ( get_end_point() - coords::project_to
                                          < coords::ms >
                                          ( omt_end_pos ) ).raw() );

    zone_type_id type = this->get_type();

    field_type_str_id field = fd_null;
    static const std::vector<zone_type> &all_zone_types = zone_type::get_all();
    for( const zone_type &zone : all_zone_types ) {
        if( zone.id == type ) {
            field = zone.get_field();
            break;
        }
    }

    if( field != fd_null ) {
        tripoint_omt_ms start_ms;
        tripoint_omt_ms end_ms;

        for( int i = omt_start_pos.x(); i <= omt_end_pos.x(); i++ ) {
            for( int k = omt_start_pos.y(); k <= omt_end_pos.y(); k++ ) {
                //  We assume the Z coordinate will remain fixed
                update_tmap.load( tripoint_abs_omt{ i, k, omt_start_pos.z() }, false );

                start_ms = tripoint_omt_ms( i > omt_start_pos.x() ? 0 : start_remainder.x(),
                                            k > omt_start_pos.y() ? 0 : start_remainder.y(), start_remainder.z() );
                end_ms = tripoint_omt_ms( i < omt_end_pos.x() ? SEEX * 2 - 1 : end_remainder.x(),
                                          k < omt_end_pos.y() ? SEEY * 2 - 1 : end_remainder.y(), end_remainder.z() );

                for( tripoint_omt_ms pt : update_tmap.points_in_rectangle( start_ms, end_ms ) ) {
                    if( is_displayed ) {
                        update_tmap.add_field( pt, field, 1, time_duration::from_turns( 0 ), false );
                    } else {
                        update_tmap.delete_field( pt, field );
                    }
                }
            }
        }
    }
}

void zone_data::toggle_display()
{
    if( this->is_vehicle ) {
        popup( colorize( _( "Zones tied to vehicles cannot be displayed" ), c_magenta ) );
        return;
    }

    is_displayed = !is_displayed;

    this->refresh_display();

    // Take care of the situation where parts of overlapping zones were erased by the toggling.
    if( !is_displayed ) {

        const point_abs_ms start = { std::min( this->start.x(), this->end.x() ),
                                     std::min( this->start.y(), this->end.y() )
                                   };

        const point_abs_ms end = { std::max( this->start.x(), this->end.x() ),
                                   std::max( this->start.y(), this->end.y() )
                                 };

        const inclusive_rectangle<point_abs_ms> zone_rectangle( start, end );

        for( zone_manager::ref_zone_data zone : zone_manager::get_manager().get_zones() ) {
            // Assumes zones are a single Z level only. Also, inclusive_cuboid doesn't have an overlap function...
            if( zone.get().get_is_displayed() && zone.get().get_type() == this->get_type() &&
                this->start.z() == zone.get().get_start_point().z() ) {
                const point_abs_ms candidate_begin = zone.get().get_start_point().xy();
                const point_abs_ms candidate_stop = zone.get().get_end_point().xy();

                const point_abs_ms candidate_start = { std::min( candidate_begin.x(),
                                                       candidate_stop.x() ),
                                                       std::min( candidate_begin.y(), candidate_stop.y() )
                                                     };

                const point_abs_ms candidate_end = { std::max( candidate_begin.x(),
                                                     candidate_stop.x() ),
                                                     std::max( candidate_begin.y(), candidate_stop.y() )
                                                   };

                const inclusive_rectangle<point_abs_ms> candidate_rectangle( candidate_start, candidate_end );

                if( zone_rectangle.overlaps( candidate_rectangle ) ) {
                    zone.get().refresh_display();
                }
            }
        }
    }
}

void zone_data::set_is_vehicle( const bool is_vehicle_arg )
{
    is_vehicle = is_vehicle_arg;
}

tripoint_abs_ms zone_data::get_center_point() const
{
    return midpoint( get_start_point(), get_end_point() );
}

std::string zone_manager::get_name_from_type( const zone_type_id &type ) const
{
    const auto &iter = types.find( type );
    if( iter != types.end() ) {
        return iter->second.name();
    }

    return "Unknown Type";
}

bool zone_manager::has_type( const zone_type_id &type ) const
{
    return types.count( type ) > 0;
}

bool zone_manager::has_defined( const zone_type_id &type, const faction_id &fac ) const
{
    const auto &type_iter = area_cache.find( zone_data::make_type_hash( type, fac ) );
    return type_iter != area_cache.end();
}

void zone_manager::cache_data( bool update_avatar )
{
    area_cache.clear();
    avatar &player_character = get_avatar();
    tripoint_abs_ms cached_shift = player_character.pos_abs();
    for( zone_data &elem : zones ) {
        if( !elem.get_enabled() ) {
            continue;
        }

        // update the current cached locations for each personal zone
        // if we are flagged to update the locations with this cache
        if( elem.get_is_personal() && update_avatar ) {
            elem.update_cached_shift( cached_shift );
        }

        const std::string &type_hash = elem.get_type_hash();
        auto &cache = area_cache[type_hash];

        // Draw marked area
        for( const tripoint_abs_ms &p : tripoint_range<tripoint_abs_ms>(
                 elem.get_start_point(), elem.get_end_point() ) ) {
            cache.insert( p );
        }
    }
}

void zone_manager::reset_disabled()
{
    bool changed = false;
    for( zone_data &elem : zones ) {
        if( elem.get_temporarily_disabled() ) {
            elem.set_enabled( true );
            elem.set_temporary_disabled( false );
            changed = true;
        }
    }

    if( changed ) {
        cache_data();
    }
}

void zone_manager::cache_avatar_location()
{
    avatar &player_character = get_avatar();
    tripoint_abs_ms cached_shift = player_character.pos_abs();
    for( zone_data &elem : zones ) {
        // update the current cached locations for each personal zone
        if( elem.get_is_personal() ) {
            elem.update_cached_shift( cached_shift );
        }
    }
}

void zone_manager::cache_vzones( map *pmap )
{
    vzone_cache.clear();
    map &here = pmap == nullptr ? get_map() : *pmap;
    auto vzones = here.get_vehicle_zones( here.get_abs_sub().z() );
    for( zone_data *elem : vzones ) {
        if( !elem->get_enabled() ) {
            continue;
        }

        const std::string &type_hash = elem->get_type_hash();
        auto &cache = vzone_cache[type_hash];

        // TODO: looks very similar to the above cache_data - maybe merge it?

        // Draw marked area
        for( const tripoint_abs_ms &p : tripoint_range<tripoint_abs_ms>(
                 elem->get_start_point(), elem->get_end_point() ) ) {
            cache.insert( p );
        }
    }
}

std::unordered_set<tripoint_abs_ms> zone_manager::get_point_set( const zone_type_id &type,
        const faction_id &fac ) const
{
    const auto &type_iter = area_cache.find( zone_data::make_type_hash( type, fac ) );
    if( type_iter == area_cache.end() ) {
        return std::unordered_set<tripoint_abs_ms>();
    }

    return type_iter->second;
}

std::unordered_set<tripoint_bub_ms> zone_manager::get_point_set_loot( const tripoint_abs_ms &where,
        int radius, const faction_id &fac ) const
{
    return get_point_set_loot( where, radius, false, fac );
}

std::unordered_set<tripoint_bub_ms> zone_manager::get_point_set_loot( const tripoint_abs_ms &where,
        int radius, bool npc_search, const faction_id &fac ) const
{
    std::unordered_set<tripoint_bub_ms> res;
    map &here = get_map();
    for( const std::pair<std::string, std::unordered_set<tripoint_abs_ms>> cache : area_cache ) {
        zone_type_id type = zone_data::unhash_type( cache.first );
        faction_id z_fac = zone_data::unhash_fac( cache.first );
        if( fac == z_fac && type.str().substr( 0, 4 ) == "LOOT" ) {
            for( tripoint_abs_ms point : cache.second ) {
                if( square_dist( where, point ) <= radius ) {
                    res.emplace( here.get_bub( point ) );
                }
            }
        }
    }
    for( const std::pair<std::string, std::unordered_set<tripoint_abs_ms>> cache : vzone_cache ) {
        zone_type_id type = zone_data::unhash_type( cache.first );
        faction_id z_fac = zone_data::unhash_fac( cache.first );
        if( fac == z_fac && type.str().substr( 0, 4 ) == "LOOT" ) {
            for( tripoint_abs_ms point : cache.second ) {
                if( square_dist( where, point ) <= radius ) {
                    res.emplace( here.get_bub( point ) );
                }
            }
        }
    }

    if( npc_search ) {
        for( const std::pair<std::string, std::unordered_set<tripoint_abs_ms>> cache : vzone_cache ) {
            zone_type_id type = zone_data::unhash_type( cache.first );
            if( type == zone_type_NO_NPC_PICKUP ) {
                for( tripoint_abs_ms point : cache.second ) {
                    res.erase( here.get_bub( point ) );
                }
            }
        }
    }

    return res;
}

std::unordered_set<tripoint_abs_ms> zone_manager::get_vzone_set( const zone_type_id &type,
        const faction_id &fac ) const
{
    //Only regenerate the vehicle zone cache if any vehicles have moved
    const auto &type_iter = vzone_cache.find( zone_data::make_type_hash( type, fac ) );
    if( type_iter == vzone_cache.end() ) {
        return std::unordered_set<tripoint_abs_ms>();
    }

    return type_iter->second;
}

bool zone_manager::has( const zone_type_id &type, const tripoint_abs_ms &where,
                        const faction_id &fac ) const
{
    const auto &point_set = get_point_set( type, fac );
    const auto &vzone_set = get_vzone_set( type, fac );
    return point_set.find( where ) != point_set.end() || vzone_set.find( where ) != vzone_set.end();
}

bool zone_manager::has_near( const zone_type_id &type, const tripoint_abs_ms &where, int range,
                             const faction_id &fac ) const
{
    const auto &point_set = get_point_set( type, fac );
    for( const tripoint_abs_ms &point : point_set ) {
        if( square_dist( point, where ) <= range ) {
            return true;
        }
    }

    const auto &vzone_set = get_vzone_set( type, fac );
    for( const tripoint_abs_ms &point : vzone_set ) {
        if( point.z() == where.z() ) {
            if( square_dist( point, where ) <= range ) {
                return true;
            }
        }
    }

    return false;
}

std::vector<zone_data const *> zone_manager::get_near_zones( const zone_type_id &type,
        const tripoint_abs_ms &where, int range,
        const faction_id &fac ) const
{
    std::vector<zone_data const *> ret;
    for( const zone_data &zone : zones ) {
        if( square_dist( zone.get_center_point(), where ) <= range && zone.get_type() == type &&
            zone.get_faction() == fac ) {
            ret.emplace_back( &zone );
        }
    }

    map &here = get_map();
    auto vzones = here.get_vehicle_zones( here.get_abs_sub().z() );
    for( const zone_data *zone : vzones ) {
        if( square_dist( zone->get_center_point(), where ) <= range && zone->get_type() == type &&
            zone->get_faction() == fac ) {
            ret.emplace_back( zone );
        }
    }

    return ret;
}

bool zone_manager::has_loot_dest_near( const tripoint_abs_ms &where ) const
{
    for( const auto &ztype : get_manager().get_types() ) {
        const zone_type_id &type = ztype.first;
        if( type == zone_type_CAMP_FOOD || type == zone_type_FARM_PLOT ||
            type == zone_type_LOOT_UNSORTED || type == zone_type_LOOT_IGNORE ||
            type == zone_type_CONSTRUCTION_BLUEPRINT ||
            type == zone_type_NO_AUTO_PICKUP || type == zone_type_NO_NPC_PICKUP ) {
            continue;
        }
        if( has_near( type, where ) ) {
            return true;
        }
    }
    return false;
}

const zone_data *zone_manager::get_zone_at( const tripoint_abs_ms &where,
        const zone_type_id &type, const faction_id &fac ) const
{
    std::vector<zone_data const *> ret = get_zones_at( where, type, fac );
    if( !ret.empty() ) {
        return ret.front();
    }

    return nullptr;
}

std::vector<zone_data const *> zone_manager::get_zones_at( const tripoint_abs_ms &where,
        const zone_type_id &type, const faction_id &fac ) const
{
    std::vector<zone_data const *> ret;
    for( const zone_data &zone : zones ) {
        if( zone.has_inside( where ) && zone.get_type() == type && zone.get_faction() == fac ) {
            ret.emplace_back( &zone );
        }
    }
    map &here = get_map();
    auto vzones = here.get_vehicle_zones( here.get_abs_sub().z() );
    for( const zone_data *zone : vzones ) {
        if( zone->has_inside( where ) && zone->get_type() == type && zone->get_faction() == fac ) {
            ret.emplace_back( zone );
        }
    }
    return ret;
}

bool zone_manager::custom_loot_has( const tripoint_abs_ms &where, const item *it,
                                    const zone_type_id &ztype, const faction_id &fac ) const
{
    std::vector<zone_data const *> const zones = get_zones_at( where, ztype, fac );
    if( zones.empty() || !it ) {
        return false;
    }
    item const *const check_it = it->this_or_single_content();
    for( zone_data const *zone : zones ) {
        loot_options const &options = dynamic_cast<const loot_options &>( zone->get_options() );
        std::string const filter_string = options.get_mark();
        bool has = false;
        if( ztype == zone_type_LOOT_CUSTOM ) {
            auto const z = item_filter_from_string( filter_string );
            has = z( *check_it ) || ( check_it != it && z( *it ) );
        } else if( ztype == zone_type_LOOT_ITEM_GROUP ) {
            has = item_group::group_contains_item( item_group_id( filter_string ),
                                                   check_it->typeId() ) ||
                  ( check_it != it &&
                    item_group::group_contains_item( item_group_id( filter_string ),
                            it->typeId() ) );
        }
        if( has ) {
            return true;
        }
    }

    return false;
}

std::unordered_set<tripoint_abs_ms> zone_manager::get_near( const zone_type_id &type,
        const tripoint_abs_ms &where, int range, const item *it, const faction_id &fac ) const
{
    const auto &point_set = get_point_set( type, fac );
    std::unordered_set<tripoint_abs_ms> near_point_set;

    for( const tripoint_abs_ms &point : point_set ) {
        if( square_dist( point, where ) <= range ) {
            if( ( type != zone_type_LOOT_CUSTOM && type != zone_type_LOOT_ITEM_GROUP ) ||
                ( it != nullptr && custom_loot_has( point, it, type, fac ) ) ) {
                near_point_set.insert( point );
            }
        }
    }

    const auto &vzone_set = get_vzone_set( type, fac );
    for( const tripoint_abs_ms &point : vzone_set ) {
        if( point.z() == where.z() ) {
            if( square_dist( point, where ) <= range ) {
                if( ( type != zone_type_LOOT_CUSTOM && type != zone_type_LOOT_ITEM_GROUP ) ||
                    ( it != nullptr && custom_loot_has( point, it, type, fac ) ) ) {
                    near_point_set.insert( point );
                }
            }
        }
    }

    return near_point_set;
}

std::optional<tripoint_abs_ms> zone_manager::get_nearest( const zone_type_id &type,
        const tripoint_abs_ms &where, int range, const faction_id &fac ) const
{
    if( range < 0 ) {
        return std::nullopt;
    }

    tripoint_abs_ms nearest_pos( INT_MIN, INT_MIN, INT_MIN );
    int nearest_dist = range + 1;
    const std::unordered_set<tripoint_abs_ms> &point_set = get_point_set( type, fac );
    for( const tripoint_abs_ms &p : point_set ) {
        int cur_dist = square_dist( p, where );
        if( cur_dist < nearest_dist ) {
            nearest_dist = cur_dist;
            nearest_pos = p;
            if( nearest_dist == 0 ) {
                return nearest_pos;
            }
        }
    }

    const std::unordered_set<tripoint_abs_ms> &vzone_set = get_vzone_set( type, fac );
    for( const tripoint_abs_ms &p : vzone_set ) {
        int cur_dist = square_dist( p, where );
        if( cur_dist < nearest_dist ) {
            nearest_dist = cur_dist;
            nearest_pos = p;
            if( nearest_dist == 0 ) {
                return nearest_pos;
            }
        }
    }
    if( nearest_dist > range ) {
        return std::nullopt;
    }
    return nearest_pos;
}

zone_type_id zone_manager::get_near_zone_type_for_item( const item &it,
        const tripoint_abs_ms &where, int range, const faction_id &fac ) const
{
    const item_category &cat = it.get_category_of_contents();

    if( has_near( zone_type_LOOT_CUSTOM, where, range, fac ) ) {
        if( !get_near( zone_type_LOOT_CUSTOM, where, range, &it, fac ).empty() ) {
            return zone_type_LOOT_CUSTOM;
        }
    }
    if( has_near( zone_type_LOOT_ITEM_GROUP, where, range, fac ) ) {
        if( !get_near( zone_type_LOOT_ITEM_GROUP, where, range, &it, fac ).empty() ) {
            return zone_type_LOOT_ITEM_GROUP;
        }
    }
    if( it.has_flag( STATIC( flag_id( "FIREWOOD" ) ) ) ) {
        if( has_near( zone_type_LOOT_WOOD, where, range, fac ) ) {
            return zone_type_LOOT_WOOD;
        }
    }
    if( it.is_corpse() ) {
        if( has_near( zone_type_LOOT_CORPSE, where, range, fac ) ) {
            return zone_type_LOOT_CORPSE;
        }
    }
    if( it.typeId() == itype_disassembly ) {
        if( has_near( zone_type_DISASSEMBLE, where, range, fac ) ) {
            return zone_type_DISASSEMBLE;
        }
    }

    std::optional<zone_type_id> zone_check_first = cat.priority_zone( it );
    if( zone_check_first && has_near( *zone_check_first, where, range, fac ) ) {
        return *zone_check_first;
    }

    std::optional<zone_type_id> zone_cat = cat.zone();
    if( zone_cat && has_near( *zone_cat, where, range, fac ) ) {
        return *cat.zone();
    }

    if( cat.get_id() == item_category_food ) {

        const item *it_food = nullptr;
        bool perishable = false;
        // Look for food, and whether any contents which will spoil if left out.
        // Food crafts and food without comestible, like MREs, will fall down to LOOT_FOOD.
        it.visit_items( [&it_food, &perishable]( const item * node, const item * parent ) {
            if( node && node->is_food() ) {
                it_food = node;

                if( node->goes_bad() ) {
                    float spoil_multiplier = 1.0f;
                    if( parent ) {
                        const item_pocket *parent_pocket = parent->contained_where( *node );
                        if( parent_pocket ) {
                            spoil_multiplier = parent_pocket->spoil_multiplier();
                        }
                    }
                    if( spoil_multiplier > 0.0f ) {
                        perishable = true;
                    }
                }
            }
            return VisitResponse::NEXT;
        } );

        if( it_food != nullptr ) {
            if( it_food->get_comestible()->comesttype == "DRINK" ) {
                if( perishable && has_near( zone_type_LOOT_PDRINK, where, range, fac ) ) {
                    if( !get_near( zone_type_LOOT_PDRINK, where, range, &it, fac ).empty() ) {
                        return zone_type_LOOT_PDRINK;
                    }
                } else if( has_near( zone_type_LOOT_DRINK, where, range, fac ) ) {
                    if( !get_near( zone_type_LOOT_DRINK, where, range, &it, fac ).empty() ) {
                        return zone_type_LOOT_DRINK;
                    }
                }
            }

            if( perishable && has_near( zone_type_LOOT_PFOOD, where, range, fac ) ) {
                if( !get_near( zone_type_LOOT_PFOOD, where, range, &it, fac ).empty() ) {
                    return zone_type_LOOT_PFOOD;
                }
            }
        }
        if( !get_near( zone_type_LOOT_FOOD, where, range, &it, fac ).empty() ) {
            return zone_type_LOOT_FOOD;
        }
    }

    if( has_near( zone_type_LOOT_DEFAULT, where, range, fac ) ) {
        if( !get_near( zone_type_LOOT_DEFAULT, where, range, &it, fac ).empty() ) {
            return zone_type_LOOT_DEFAULT;
        }
    }

    return zone_type_id();
}

std::vector<zone_data> zone_manager::get_zones( const zone_type_id &type,
        const tripoint_abs_ms &where, const faction_id &fac ) const
{
    auto zones = std::vector<zone_data>();

    for( const zone_data &zone : this->zones ) {
        if( zone.get_type() == type && zone.get_faction() == fac ) {
            if( zone.has_inside( where ) ) {
                zones.emplace_back( zone );
            }
        }
    }

    return zones;
}

const zone_data *zone_manager::get_zone_at( const tripoint_abs_ms &where, bool loot_only,
        const faction_id &fac ) const
{
    map &here = get_map();

    auto const check = [&fac, loot_only, &where]( zone_data const & z ) {
        return z.get_faction() == fac &&
               ( !loot_only || z.get_type().str().substr( 0, 4 ) == "LOOT" ) &&
               z.has_inside( where );
    };
    for( auto it = zones.rbegin(); it != zones.rend(); ++it ) {
        if( check( *it ) ) {
            return &*it;
        }
    }
    auto const vzones = here.get_vehicle_zones( here.get_abs_sub().z() );
    for( zone_data *it : vzones ) {
        if( check( *it ) ) {
            return &*it;
        }
    }
    return nullptr;
}

const zone_data *zone_manager::get_bottom_zone(
    const tripoint_abs_ms &where, const faction_id &fac ) const
{
    for( auto it = zones.rbegin(); it != zones.rend(); ++it ) {
        const zone_data &zone = *it;
        if( zone.get_faction() != fac ) {
            continue;
        }

        if( zone.has_inside( where ) ) {
            return &zone;
        }
    }
    map &here = get_map();
    auto vzones = here.get_vehicle_zones( here.get_abs_sub().z() );
    for( auto it = vzones.rbegin(); it != vzones.rend(); ++it ) {
        const zone_data *zone = *it;
        if( zone->get_faction() != fac ) {
            continue;
        }

        if( zone->has_inside( where ) ) {
            return zone;
        }
    }

    return nullptr;
}

// CAREFUL: This function has the ability to move the passed in zone reference depending on
// which constructor of the key-value pair we use which depends on new_zone being an rvalue or lvalue and constness.
// If you are passing new_zone from a non-const iterator, be prepared for a move! This
// may break some iterators like map iterators if you are less specific!
void zone_manager::create_vehicle_loot_zone( vehicle &vehicle, const point_rel_ms &mount_point,
        zone_data &new_zone, map *pmap )
{
    //create a vehicle loot zone
    new_zone.set_is_vehicle( true );
    auto nz = vehicle.loot_zones.emplace( mount_point, new_zone );
    map &here = pmap == nullptr ? get_map() : *pmap;
    here.register_vehicle_zone( &vehicle, here.get_abs_sub().z() );
    added_vzones.push_back( &nz->second );
    cache_vzones( pmap );
}

void zone_manager::add( const std::string &name, const zone_type_id &type, const faction_id &fac,
                        const bool invert, const bool enabled, const tripoint_abs_ms &start,
                        const tripoint_abs_ms &end, const shared_ptr_fast<zone_options> &options,
                        bool silent, map *pmap )
{
    map &here = pmap == nullptr ? get_map() : *pmap;
    zone_data new_zone = zone_data( name, type, fac, invert, enabled, start, end, options );
    // only non personal zones can be vehicle zones
    optional_vpart_position const vp = here.veh_at( here.get_bub( start ) );
    if( vp && vp->vehicle().get_owner() == fac && vp.cargo() ) {
        // TODO:Allow for loot zones on vehicles to be larger than 1x1
        if( start == end &&
            ( silent || query_yn( _( "Bind this zone to the cargo part here?" ) ) ) ) {
            // TODO: refactor zone options for proper validation code
            if( !silent &&
                ( type == zone_type_FARM_PLOT || type == zone_type_CONSTRUCTION_BLUEPRINT ) ) {
                popup( _( "You cannot add that type of zone to a vehicle." ), PF_NONE );
                return;
            }

            create_vehicle_loot_zone( vp->vehicle(), vp->mount_pos(), new_zone, pmap );
            return;
        }
    }

    //Create a regular zone
    zones.push_back( new_zone );

    cache_data();
}

void zone_manager::add( const std::string &name, const zone_type_id &type, const faction_id &fac,
                        const bool invert, const bool enabled, const tripoint_rel_ms &start,
                        const tripoint_rel_ms &end, const shared_ptr_fast<zone_options> &options )
{
    zone_data new_zone = zone_data( name, type, fac, invert, enabled, start, end, options );

    //Create a regular zone
    zones.push_back( new_zone );
    num_personal_zones++;
    cache_data();
}

bool zone_manager::remove( zone_data &zone )
{
    for( auto it = zones.begin(); it != zones.end(); ++it ) {
        if( &zone == &*it ) {
            if( zone.get_is_displayed() ) {
                zone.toggle_display();
            }
            // if removing a personal zone reduce the number of counted personal zones
            if( it->get_is_personal() ) {
                num_personal_zones--;
            }
            zones.erase( it );
            return true;
        }
    }
    zone_data old_zone = zone_data( zone );
    //If the zone was previously edited this session
    //Move original data out of changed
    for( auto it = changed_vzones.begin(); it != changed_vzones.end(); ++it ) {
        if( it->second == &zone ) {
            old_zone = zone_data( it->first );
            changed_vzones.erase( it );
            break;
        }
    }
    bool added = false;
    //If the zone was added this session
    //remove from added, and don't add to removed
    for( auto it = added_vzones.begin(); it != added_vzones.end(); ++it ) {
        if( *it == &zone ) {
            added = true;
            added_vzones.erase( it );
            break;
        }
    }
    if( !added ) {
        removed_vzones.push_back( old_zone );
    }

    if( !get_map().deregister_vehicle_zone( zone ) ) {
        debugmsg( "Tried to remove a zone from an unloaded vehicle" );
        return false;
    }
    cache_vzones();
    return true;
}

void zone_manager::swap( zone_data &a, zone_data &b )
{
    if( a.get_is_vehicle() || b.get_is_vehicle() ) {
        //Current swap mechanic will change which vehicle the zone is on
        // TODO: track and update vehicle zone priorities?
        popup( _( "You cannot change the order of vehicle loot zones." ), PF_NONE );
        return;
    }
    std::swap( a, b );
}

namespace
{
void _rotate_zone( map &target_map, zone_data &zone, int turns )
{
    const point dim( SEEX * 2, SEEY * 2 );
    const tripoint_bub_ms a_start( 0, 0, target_map.get_abs_sub().z() );
    const tripoint_bub_ms a_end( SEEX * 2 - 1, SEEY * 2 - 1, a_start.z() );
    const tripoint_bub_ms z_start = target_map.get_bub( zone.get_start_point() );
    const tripoint_bub_ms z_end = target_map.get_bub( zone.get_end_point() );
    const inclusive_cuboid<tripoint_bub_ms> boundary( a_start, a_end );
    if( boundary.contains( z_start ) && boundary.contains( z_end ) ) {
        // don't rotate centered squares
        if( z_start.x() == z_start.y() && z_end.x() == z_end.y() &&
            z_start.x() + z_end.x() == a_end.x() ) {
            return;
        }
        point_bub_ms z_l_start = z_start.xy().rotate( turns, dim );
        point_bub_ms z_l_end = z_end.xy().rotate( turns, dim );
        tripoint_abs_ms first =
            target_map.get_abs( tripoint_bub_ms( std::min( z_l_start.x(), z_l_end.x() ),
                                std::min( z_l_start.y(), z_l_end.y() ),
                                z_start.z() ) );
        tripoint_abs_ms second =
            target_map.get_abs( tripoint_bub_ms( std::max( z_l_start.x(), z_l_end.x() ),
                                std::max( z_l_start.y(), z_l_end.y() ),
                                z_end.z() ) );
        zone.set_position( std::make_pair( first, second ), false, true, false, true );
    }
}

} // namespace

void zone_manager::rotate_zones( map &target_map, const int turns )
{
    if( turns == 0 ) {
        return;
    }

    for( zone_data &zone : zones ) {
        if( !zone.get_is_personal() && target_map.inbounds_z( zone.get_center_point().z() ) ) {
            _rotate_zone( target_map, zone, turns );
        }
    }

    for( int z_level = target_map.supports_zlevels() ? -OVERMAP_DEPTH : target_map.get_abs_sub().z();
         z_level <= ( target_map.supports_zlevels() ? OVERMAP_HEIGHT : target_map.get_abs_sub().z() );
         z_level++ ) {
        for( zone_data *zone : target_map.get_vehicle_zones( z_level ) ) {
            _rotate_zone( target_map, *zone, turns );
        }
    }
}

std::vector<zone_manager::ref_zone_data> zone_manager::get_zones( const faction_id &fac )
{
    auto zones = std::vector<ref_zone_data>();

    for( zone_data &zone : this->zones ) {
        if( zone.get_faction() == fac ) {
            zones.emplace_back( zone );
        }
    }

    map &here = get_map();
    auto vzones = here.get_vehicle_zones( here.get_abs_sub().z() );

    for( zone_data *zone : vzones ) {
        if( zone->get_faction() == fac ) {
            zones.emplace_back( *zone );
        }
    }

    return zones;
}

std::vector<zone_manager::ref_const_zone_data> zone_manager::get_zones(
    const faction_id &fac ) const
{
    auto zones = std::vector<ref_const_zone_data>();

    for( const zone_data &zone : this->zones ) {
        if( zone.get_faction() == fac ) {
            zones.emplace_back( zone );
        }
    }

    map &here = get_map();
    auto vzones = here.get_vehicle_zones( here.get_abs_sub().z() );

    for( zone_data *zone : vzones ) {
        if( zone->get_faction() == fac ) {
            zones.emplace_back( *zone );
        }
    }

    return zones;
}

bool zone_manager::has_personal_zones() const
{
    // if there are more than 0 personal zones
    return num_personal_zones > 0;
}

void zone_manager::serialize( JsonOut &json ) const
{
    json.write( zones );
}

void zone_manager::deserialize( const JsonValue &jv )
{
    jv.read( zones );
    for( auto it = zones.begin(); it != zones.end(); ) {
        // need to keep track of number of personal zones on reload
        if( it->get_is_personal() ) {
            num_personal_zones++;
        }
        const zone_type_id zone_type = it->get_type();

        if( !has_type( zone_type ) ) {
            it = zones.erase( it );
            debugmsg( "Invalid zone type: %s", zone_type.c_str() );
        } else  if( it->get_faction() != faction_your_followers ) {
            it = zones.erase( it );
        } else {
            ++it;
        }
    }
}

void zone_data::serialize( JsonOut &json ) const
{
    json.start_object();
    json.member( "name", name );
    json.member( "type", type );
    json.member( "faction", faction );
    json.member( "invert", invert );
    json.member( "enabled", enabled );
    json.member( "is_vehicle", is_vehicle );
    json.member( "is_personal", is_personal );
    json.member( "cached_shift", cached_shift );
    if( is_personal ) {
        json.member( "start", personal_start );
        json.member( "end", personal_end );
    } else {
        json.member( "start", start );
        json.member( "end", end );
    }
    json.member( "is_displayed", is_displayed );
    options->serialize( json );
    json.end_object();
}

void zone_data::deserialize( const JsonObject &data )
{
    data.allow_omitted_members();
    data.read( "name", name );
    // handle legacy zone types
    zone_type_id temp_type;
    data.read( "type", temp_type );
    const auto find_result = legacy_zone_types.find( temp_type.str() );
    if( find_result != legacy_zone_types.end() ) {
        type =  find_result->second;
    } else {
        type = temp_type;
    }
    if( data.has_member( "faction" ) ) {
        data.read( "faction", faction );
    } else {
        faction = your_fac;
    }
    data.read( "invert", invert );
    data.read( "enabled", enabled );
    //Legacy support
    if( data.has_member( "is_vehicle" ) ) {
        data.read( "is_vehicle", is_vehicle );
    } else {
        is_vehicle = false;
    }
    if( data.has_member( "is_personal" ) ) {
        data.read( "is_personal", is_personal );
        data.read( "cached_shift", cached_shift );
    } else {
        is_personal = false;
        cached_shift = tripoint_abs_ms{};
    }
    //Legacy support
    if( data.has_member( "start_x" ) ) {
        tripoint_abs_ms s;
        tripoint_abs_ms e;
        data.read( "start_x", s.x() );
        data.read( "start_y", s.y() );
        data.read( "start_z", s.z() );
        data.read( "end_x", e.x() );
        data.read( "end_y", e.y() );
        data.read( "end_z", e.z() );
        if( is_personal ) {
            personal_start = tripoint_rel_ms( s.raw() );
            personal_end = tripoint_rel_ms( e.raw() );
        } else {
            start = s;
            end = e;
        }
    } else {
        if( is_personal ) {
            data.read( "start", personal_start );
            data.read( "end", personal_end );
        } else {
            data.read( "start", start );
            data.read( "end", end );
        }
    }
    if( data.has_member( "is_displayed" ) ) {
        data.read( "is_displayed", is_displayed );
    } else {
        is_displayed = false;
    }
    auto new_options = zone_options::create( type );
    new_options->deserialize( data );
    options = new_options;
}

namespace
{
cata_path _savefile( std::string const &suffix, bool player )
{
    if( player ) {
        return PATH_INFO::player_base_save_path() + string_format( ".zones%s.json", suffix );
    } else {
        return PATH_INFO::world_base_save_path() / string_format( "zones%s.json", suffix );
    }
}
} // namespace

bool zone_manager::save_zones( std::string const &suffix )
{
    cata_path const savefile = _savefile( suffix, true );

    added_vzones.clear();
    changed_vzones.clear();
    removed_vzones.clear();
    save_world_zones( suffix );
    return write_to_file( savefile, [&]( std::ostream & fout ) {
        JsonOut jsout( fout );
        serialize( jsout );
    }, _( "zones date" ) );
}

void zone_manager::load_zones( std::string const &suffix )
{
    cata_path const savefile = _savefile( suffix, true );

    const auto reader = [this]( const JsonValue & jv ) {
        deserialize( jv );
    };
    if( !read_from_file_optional_json( savefile, reader ) ) {
        // If no such file or failed to load, clear zones.
        zones.clear();
    }
    load_world_zones( suffix );
    revert_vzones();
    added_vzones.clear();
    changed_vzones.clear();
    removed_vzones.clear();

    cache_data();
    cache_vzones();
}

bool zone_manager::save_world_zones( std::string const &suffix )
{
    cata_path const savefile = _savefile( suffix, false );
    std::vector<zone_data> tmp;
    std::copy_if( zones.begin(), zones.end(), std::back_inserter( tmp ), []( const zone_data & z ) {
        return z.get_faction() != faction_your_followers;
    } );
    return write_to_file( savefile, [&]( std::ostream & fout ) {
        JsonOut jsout( fout );
        jsout.write( tmp );
    }, _( "zones date" ) );
}

void zone_manager::load_world_zones( std::string const &suffix )
{
    cata_path const savefile = _savefile( suffix, false );
    std::vector<zone_data> tmp;
    read_from_file_optional_json( savefile, [&]( const JsonValue & jsin ) {
        jsin.read( tmp );
        for( auto it = tmp.begin(); it != tmp.end(); ) {
            const zone_type_id zone_type = it->get_type();
            if( !has_type( zone_type ) ) {
                it = tmp.erase( it );
                debugmsg( "Invalid zone type: %s", zone_type.c_str() );
            } else if( it->get_faction() == faction_your_followers ) {
                it = tmp.erase( it );
            } else {
                ++it;
            }
        }
        std::copy( tmp.begin(), tmp.end(), std::back_inserter( zones ) );
    } );
}

void zone_manager::zone_edited( zone_data &zone )
{
    if( zone.get_is_vehicle() ) {
        //Check if this zone has already been stored
        for( auto &changed_vzone : changed_vzones ) {
            if( &zone == changed_vzone.second ) {
                return;
            }
        }
        //Add it to the list of changed zones
        changed_vzones.emplace_back( zone_data( zone ), &zone );
    }
}

void zone_manager::revert_vzones()
{
    map &here = get_map();
    for( zone_data zone : removed_vzones ) {
        //Code is copied from add() to avoid yn query
        const tripoint_bub_ms pos = here.get_bub( zone.get_start_point() );
        if( const std::optional<vpart_reference> vp = here.veh_at( pos ).cargo() ) {
            zone.set_is_vehicle( true );
            vp->vehicle().loot_zones.emplace( vp->mount_pos(), zone );
            here.register_vehicle_zone( &vp->vehicle(), here.get_abs_sub().z() );
            cache_vzones();
        }
    }
    for( const auto &zpair : changed_vzones ) {
        *( zpair.second ) = zpair.first;
    }
    for( zone_data *zone : added_vzones ) {
        remove( *zone );
    }
}

void mapgen_place_zone( tripoint_abs_ms const &start, tripoint_abs_ms const &end,
                        zone_type_id const &type,
                        faction_id const &fac, std::string const &name, std::string const &filter,
                        map *pmap )
{
    zone_manager &mgr = zone_manager::get_manager();
    auto options = zone_options::create( type );
    tripoint_abs_ms const s_ = std::min( start, end );
    tripoint_abs_ms const e_ = std::max( start, end );
    if( type == zone_type_LOOT_CUSTOM || type == zone_type_LOOT_ITEM_GROUP ) {
        if( dynamic_cast<loot_options *>( &*options ) != nullptr ) {
            dynamic_cast<loot_options *>( &*options )->set_mark( filter );
        }
    }
    mgr.add( name, type, fac, false, true, s_, e_, options, true, pmap );
}
