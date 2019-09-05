#include "clzones.h"

#include <climits>
#include <iosfwd>
#include <iterator>
#include <list>
#include <tuple>
#include <string>
#include <algorithm>

#include "avatar.h"
#include "cata_utility.h"
#include "construction.h"
#include "debug.h"
#include "game.h"
#include "iexamine.h"
#include "item_category.h"
#include "item_search.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "messages.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "output.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "uistate.h"
#include "vehicle.h"
#include "item.h"
#include "player.h"
#include "vpart_position.h"
#include "faction.h"

zone_manager::zone_manager()
{
    types.emplace( zone_type_id( "NO_AUTO_PICKUP" ),
                   zone_type( translate_marker( "No Auto Pickup" ),
                              translate_marker( "You won't auto-pickup items inside the zone." ) ) );
    types.emplace( zone_type_id( "NO_NPC_PICKUP" ),
                   zone_type( translate_marker( "No NPC Pickup" ),
                              translate_marker( "Friendly NPCs don't pickup items inside the zone." ) ) );
    types.emplace( zone_type_id( "NPC_RETREAT" ),
                   zone_type( translate_marker( "NPC Retreat" ),
                              translate_marker( "When fleeing, friendly NPCs will attempt to retreat toward this zone if it is within 60 tiles." ) ) );
    types.emplace( zone_type_id( "NPC_NO_INVESTIGATE" ),
                   zone_type( translate_marker( "NPC Ignore Sounds" ),
                              translate_marker( "Friendly NPCs won't investigate unseen sounds coming from this zone." ) ) );
    types.emplace( zone_type_id( "NPC_INVESTIGATE_ONLY" ),
                   zone_type( translate_marker( "NPC Investigation Area" ),
                              translate_marker( "Friendly NPCs will investigate unseen sounds only if they come from inside this area." ) ) );
    types.emplace( zone_type_id( "LOOT_UNSORTED" ),
                   zone_type( translate_marker( "Loot: Unsorted" ),
                              translate_marker( "Place to drop unsorted loot. You can use \"sort out loot\" zone-action to sort items inside. It can overlap with Loot zones of different types." ) ) );
    types.emplace( zone_type_id( "LOOT_FOOD" ),
                   zone_type( translate_marker( "Loot: Food" ),
                              translate_marker( "Destination for comestibles. If more specific food zone is not defined, all food is moved here." ) ) );
    types.emplace( zone_type_id( "LOOT_PFOOD" ),
                   zone_type( translate_marker( "Loot: P.Food" ),
                              translate_marker( "Destination for perishable comestibles. Does include perishable drinks if such zone is not specified." ) ) );
    types.emplace( zone_type_id( "LOOT_DRINK" ),
                   zone_type( translate_marker( "Loot: Drink" ),
                              translate_marker( "Destination for drinks. Does include perishable drinks if such zone is not specified." ) ) );
    types.emplace( zone_type_id( "LOOT_PDRINK" ),
                   zone_type( translate_marker( "Loot: P.Drink" ),
                              translate_marker( "Destination for perishable drinks." ) ) );
    types.emplace( zone_type_id( "LOOT_GUNS" ),
                   zone_type( translate_marker( "Loot: Guns" ),
                              translate_marker( "Destination for guns, bows and similar weapons." ) ) );
    types.emplace( zone_type_id( "LOOT_MAGAZINES" ),
                   zone_type( translate_marker( "Loot: Magazines" ),
                              translate_marker( "Destination for gun magazines." ) ) );
    types.emplace( zone_type_id( "LOOT_AMMO" ),
                   zone_type( translate_marker( "Loot: Ammo" ),
                              translate_marker( "Destination for ammo." ) ) );
    types.emplace( zone_type_id( "LOOT_WEAPONS" ),
                   zone_type( translate_marker( "Loot: Weapons" ),
                              translate_marker( "Destination for melee weapons." ) ) );
    types.emplace( zone_type_id( "LOOT_TOOLS" ),
                   zone_type( translate_marker( "Loot: Tools" ),
                              translate_marker( "Destination for tools." ) ) );
    types.emplace( zone_type_id( "LOOT_CLOTHING" ),
                   zone_type( translate_marker( "Loot: Clothing" ),
                              translate_marker( "Destination for clothing. Does include filthy clothing if such zone is not specified." ) ) );
    types.emplace( zone_type_id( "LOOT_FCLOTHING" ),
                   zone_type( translate_marker( "Loot: F.Clothing" ),
                              translate_marker( "Destination for filthy clothing." ) ) );
    types.emplace( zone_type_id( "LOOT_DRUGS" ),
                   zone_type( translate_marker( "Loot: Drugs" ),
                              translate_marker( "Destination for drugs and other medical items." ) ) );
    types.emplace( zone_type_id( "LOOT_BOOKS" ),
                   zone_type( translate_marker( "Loot: Books" ),
                              translate_marker( "Destination for books and magazines." ) ) );
    types.emplace( zone_type_id( "LOOT_MODS" ),
                   zone_type( translate_marker( "Loot: Mods" ),
                              translate_marker( "Destination for firearm modifications and similar items." ) ) );
    types.emplace( zone_type_id( "LOOT_MUTAGENS" ),
                   zone_type( translate_marker( "Loot: Mutagens" ),
                              translate_marker( "Destination for mutagens, serums, and purifiers." ) ) );
    types.emplace( zone_type_id( "LOOT_BIONICS" ),
                   zone_type( translate_marker( "Loot: Bionics" ),
                              translate_marker( "Destination for Compact Bionics Modules aka CBMs." ) ) );
    types.emplace( zone_type_id( "LOOT_VEHICLE_PARTS" ),
                   zone_type( translate_marker( "Loot: V.Parts" ),
                              translate_marker( "Destination for vehicle parts." ) ) );
    types.emplace( zone_type_id( "LOOT_OTHER" ),
                   zone_type( translate_marker( "Loot: Other" ),
                              translate_marker( "Destination for other miscellaneous items." ) ) );
    types.emplace( zone_type_id( "LOOT_FUEL" ),
                   zone_type( translate_marker( "Loot: Fuel" ),
                              translate_marker( "Destination for gasoline, diesel, lamp oil and other substances used as a fuel." ) ) );
    types.emplace( zone_type_id( "LOOT_SEEDS" ),
                   zone_type( translate_marker( "Loot: Seeds" ),
                              translate_marker( "Destination for seeds, stems and similar items." ) ) );
    types.emplace( zone_type_id( "LOOT_CHEMICAL" ),
                   zone_type( translate_marker( "Loot: Chemical" ),
                              translate_marker( "Destination for chemicals." ) ) );
    types.emplace( zone_type_id( "LOOT_SPARE_PARTS" ),
                   zone_type( translate_marker( "Loot: S.Parts" ),
                              translate_marker( "Destination for spare parts." ) ) );
    types.emplace( zone_type_id( "LOOT_ARTIFACTS" ),
                   zone_type( translate_marker( "Loot: Artifacts" ),
                              translate_marker( "Destination for artifacts" ) ) );
    types.emplace( zone_type_id( "LOOT_CORPSE" ),
                   zone_type( translate_marker( "Loot: Corpses" ),
                              translate_marker( "Destination for corpses" ) ) );
    types.emplace( zone_type_id( "LOOT_ARMOR" ),
                   zone_type( translate_marker( "Loot: Armor" ),
                              translate_marker( "Destination for armor. Does include filthy armor if such zone is not specified." ) ) );
    types.emplace( zone_type_id( "LOOT_FARMOR" ),
                   zone_type( translate_marker( "Loot: F.Armor" ),
                              translate_marker( "Destination for filthy armor." ) ) );
    types.emplace( zone_type_id( "LOOT_WOOD" ),
                   zone_type( translate_marker( "Loot: Wood" ),
                              translate_marker( "Destination for firewood and items that can be used as such." ) ) );
    types.emplace( zone_type_id( "LOOT_CUSTOM" ),
                   zone_type( translate_marker( "Loot: Custom" ),
                              translate_marker( "Destination for loot with a custom filter that you can modify." ) ) );
    types.emplace( zone_type_id( "LOOT_IGNORE" ),
                   zone_type( translate_marker( "Loot: Ignore" ),
                              translate_marker( "Items inside of this zone are ignored by \"sort out loot\" zone-action." ) ) );
    types.emplace( zone_type_id( "SOURCE_FIREWOOD" ),
                   zone_type( translate_marker( "Source: Firewood" ),
                              translate_marker( "Source for firewood or other flammable materials in this zone may be used to automatically refuel fires. "
                                      "This will be done to maintain light during long-running tasks such as crafting, reading or waiting." ) ) );
    types.emplace( zone_type_id( "CONSTRUCTION_BLUEPRINT" ),
                   zone_type( translate_marker( "Construction: Blueprint" ),
                              translate_marker( "Designate a blueprint zone for construction." ) ) );
    types.emplace( zone_type_id( "FARM_PLOT" ),
                   zone_type( translate_marker( "Farm: Plot" ),
                              translate_marker( "Designate a farm plot for tilling and planting." ) ) );
    types.emplace( zone_type_id( "CHOP_TREES" ),
                   zone_type( translate_marker( "Chop Trees" ),
                              translate_marker( "Designate an area to chop down trees." ) ) );
    types.emplace( zone_type_id( "FISHING_SPOT" ),
                   zone_type( translate_marker( "Fishing Spot" ),
                              translate_marker( "Designate an area to fish from." ) ) );
    types.emplace( zone_type_id( "VEHICLE_DECONSTRUCT" ),
                   zone_type( translate_marker( "Vehicle Deconstruct Zone" ),
                              translate_marker( "Any vehicles in this area are marked for deconstruction." ) ) );
    types.emplace( zone_type_id( "CAMP_FOOD" ),
                   zone_type( translate_marker( "Basecamp: Food" ),
                              translate_marker( "Items in this zone will be added to a basecamp's food supply in the Distribute Food mission." ) ) );

}

std::string zone_type::name() const
{
    return _( name_ );
}

std::string zone_type::desc() const
{
    return _( desc_ );
}

std::shared_ptr<zone_options> zone_options::create( const zone_type_id &type )
{
    if( type == zone_type_id( "FARM_PLOT" ) ) {
        return std::make_shared<plot_options>();
    } else if( type == zone_type_id( "CONSTRUCTION_BLUEPRINT" ) ) {
        return std::make_shared<blueprint_options>();
    } else if( type == zone_type_id( "LOOT_CUSTOM" ) ) {
        return std::make_shared<loot_options>();
    }

    return std::make_shared<zone_options>();
}

bool zone_options::is_valid( const zone_type_id &type, const zone_options &options )
{
    if( type == zone_type_id( "FARM_PLOT" ) ) {
        return dynamic_cast<const plot_options *>( &options ) != nullptr ;
    } else if( type == zone_type_id( "CONSTRUCTION_BLUEPRINT" ) ) {
        return dynamic_cast<const blueprint_options *>( &options ) != nullptr ;
    } else if( type == zone_type_id( "LOOT_CUSTOM" ) ) {
        return dynamic_cast<const loot_options *>( &options ) != nullptr ;
    }

    // ensure options is not derived class for the rest of zone types
    return !options.has_options();
}

int blueprint_options::get_final_construction(
    const std::vector<construction> &list_constructions,
    int idx,
    std::set<int> &skip_index
)
{
    const construction &con = list_constructions[idx];
    if( con.post_terrain.empty() ) {
        return idx;
    }

    if( string_ends_with( con.post_terrain, "_half" ) ||
        string_ends_with( con.post_terrain, "_halfway" ) ) {
        for( int i = 0; i < static_cast<int>( list_constructions.size() ); ++i ) {
            if( i == idx || skip_index.find( i ) != skip_index.end() ) {
                continue;
            }
            const construction &con_next =  list_constructions[i];
            if( con.description == con_next.description &&
                con.post_terrain == con_next.pre_terrain ) {
                skip_index.insert( idx );
                return get_final_construction( list_constructions, i, skip_index );
            }
        }
    }
    return idx;
}

blueprint_options::query_con_result blueprint_options::query_con()
{
    int con_index = construction_menu( true );
    if( con_index > -1 ) {
        const std::vector<construction> &list_constructions = get_constructions();
        std::set<int> skip_index;
        con_index = get_final_construction( list_constructions, con_index, skip_index );

        const construction &chosen = list_constructions[con_index];

        const std::string &chosen_desc = chosen.description;
        const std::string &chosen_mark = chosen.post_terrain;

        if( con_index != index || chosen_desc != con || chosen_mark != mark ) {
            con = chosen_desc;
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

loot_options::query_loot_result loot_options::query_loot()
{
    int w_height = TERMY / 2;

    const int w_width = TERMX / 2;
    const int w_y0 = ( TERMY > w_height ) ? ( TERMY - w_height ) / 4 : 0;
    const int w_x0 = ( TERMX > w_width ) ? ( TERMX - w_width ) / 2 : 0;

    catacurses::window w_con = catacurses::newwin( w_height, w_width, point( w_x0, w_y0 ) );
    draw_item_filter_rules( w_con, 1, w_height - 1, item_filter_type::FILTER );
    string_input_popup()
    .title( _( "Filter:" ) )
    .width( 55 )
    .identifier( "item_filter" )
    .max_length( 256 )
    .edit( mark );
    return changed;
}

plot_options::query_seed_result plot_options::query_seed()
{
    player &p = g->u;

    std::vector<item *> seed_inv = p.items_with( []( const item & itm ) {
        return itm.is_seed();
    } );

    std::vector<seed_tuple> seed_entries = iexamine::get_seed_entries( seed_inv );
    seed_entries.emplace( seed_entries.begin(), seed_tuple( itype_id( "null" ), _( "No seed" ), 0 ) );

    int seed_index = iexamine::query_seed( seed_entries );

    if( seed_index > 0 && seed_index < static_cast<int>( seed_entries.size() ) ) {
        const auto &seed_entry = seed_entries[seed_index];
        const auto &new_seed = std::get<0>( seed_entry );
        std::string new_mark;

        item it = item( itype_id( new_seed ) );
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
    } else if( seed_index == 0 ) { // No seeds
        if( !seed.empty() || !mark.empty() ) {
            seed.clear();
            mark.clear();
            return changed;
        } else {
            return successful;
        }
    } else {
        return canceled;
    }
}

bool loot_options::query_at_creation()
{
    return query_loot() != canceled;
}

bool loot_options::query()
{
    return query_loot() == changed;
}

std::string loot_options::get_zone_name_suggestion() const
{
    if( !mark.empty() ) {
        return string_format( _( "Loot: Custom : %s" ), mark );
    }
    return _( "Loot: Custom : No Filter" );
}

std::vector<std::pair<std::string, std::string>> loot_options::get_descriptions() const
{
    std::vector<std::pair<std::string, std::string>> options;
    options.emplace_back( std::make_pair( _( "Loot: Custom: " ),
                                          !mark.empty() ? mark : _( "No filter" ) ) );

    return options;
}

void loot_options::serialize( JsonOut &json ) const
{
    json.member( "mark", mark );
}

void loot_options::deserialize( JsonObject &jo_zone )
{
    jo_zone.read( "mark", mark );
}

bool blueprint_options::query_at_creation()
{
    return query_con() != canceled;
}

bool plot_options::query_at_creation()
{
    return query_seed() != canceled;
}

bool blueprint_options::query()
{
    return query_con() == changed;
}

bool plot_options::query()
{
    return query_seed() == changed;
}

std::string blueprint_options::get_zone_name_suggestion() const
{
    if( !con.empty() ) {
        return con;
    }

    return _( "No construction" );
}

std::string plot_options::get_zone_name_suggestion() const
{
    if( !seed.empty() ) {
        auto type = itype_id( seed );
        item it = item( type );
        if( it.is_seed() ) {
            return it.type->seed->plant_name;
        } else {
            return item::nname( type );
        }
    }

    return _( "No seed" );
}

std::vector<std::pair<std::string, std::string>> blueprint_options::get_descriptions() const
{
    std::vector<std::pair<std::string, std::string>> options =
                std::vector<std::pair<std::string, std::string>>();
    options.emplace_back( std::make_pair( _( "Construct: " ),
                                          !con.empty() ? con : _( "No Construction" ) ) );

    return options;
}

std::vector<std::pair<std::string, std::string>> plot_options::get_descriptions() const
{
    auto options = std::vector<std::pair<std::string, std::string>>();
    options.emplace_back( std::make_pair( _( "Plant seed: " ),
                                          !seed.empty() ? item::nname( itype_id( seed ) ) : _( "No seed" ) ) );

    return options;
}

void blueprint_options::serialize( JsonOut &json ) const
{
    json.member( "mark", mark );
    json.member( "con", con );
    json.member( "index", index );
}

void blueprint_options::deserialize( JsonObject &jo_zone )
{
    jo_zone.read( "mark", mark );
    jo_zone.read( "con", con );
    jo_zone.read( "index", index );
}

void plot_options::serialize( JsonOut &json ) const
{
    json.member( "mark", mark );
    json.member( "seed", seed );
}

void plot_options::deserialize( JsonObject &jo_zone )
{
    jo_zone.read( "mark", mark );
    jo_zone.read( "seed", seed );
}

cata::optional<std::string> zone_manager::query_name( const std::string &default_name ) const
{
    string_input_popup popup;
    popup
    .title( _( "Zone name:" ) )
    .width( 55 )
    .text( default_name )
    .max_length( 15 )
    .query();
    if( popup.canceled() ) {
        return {};
    } else {
        return popup.text();
    }
}

cata::optional<zone_type_id> zone_manager::query_type() const
{
    const auto &types = get_manager().get_types();
    uilist as_m;
    as_m.desc_enabled = true;
    as_m.text = _( "Select zone type:" );

    size_t i = 0;
    for( const auto &pair : types ) {
        const auto &type = pair.second;

        as_m.addentry_desc( i++, true, MENU_AUTOASSIGN, type.name(), type.desc() );
    }

    as_m.query();
    if( as_m.ret < 0 ) {
        return {};
    }
    size_t index = as_m.ret;

    auto iter = types.begin();
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
    const auto maybe_type = zone_manager::get_manager().query_type();
    if( maybe_type.has_value() && maybe_type.value() != type ) {
        auto new_options = zone_options::create( maybe_type.value() );
        if( new_options->query_at_creation() ) {
            zone_manager::get_manager().zone_edited( *this );
            type = maybe_type.value();
            options = new_options;
            zone_manager::get_manager().cache_data();
            return true;
        }
    }
    return false;
}

void zone_data::set_position( const std::pair<tripoint, tripoint> &position,
                              const bool manual )
{
    if( is_vehicle && manual ) {
        debugmsg( "Tried moving a lootzone bound to a vehicle part" );
        return;
    }
    start = position.first;
    end = position.second;

    zone_manager::get_manager().cache_data();
}

void zone_data::set_enabled( const bool enabled_arg )
{
    zone_manager::get_manager().zone_edited( *this );
    enabled = enabled_arg;
}

void zone_data::set_is_vehicle( const bool is_vehicle_arg )
{
    is_vehicle = is_vehicle_arg;
}

tripoint zone_data::get_center_point() const
{
    return tripoint( ( start.x + end.x ) / 2, ( start.y + end.y ) / 2, ( start.z + end.z ) / 2 );
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

void zone_manager::cache_data()
{
    area_cache.clear();

    for( auto &elem : zones ) {
        if( !elem.get_enabled() ) {
            continue;
        }

        const std::string &type_hash = elem.get_type_hash();
        auto &cache = area_cache[type_hash];

        tripoint start = elem.get_start_point();
        tripoint end = elem.get_end_point();

        // Draw marked area
        for( int x = start.x; x <= end.x; ++x ) {
            for( int y = start.y; y <= end.y; ++y ) {
                for( int z = start.z; z <= end.z; ++z ) {
                    cache.insert( tripoint( x, y, z ) );
                }
            }
        }
    }
}

void zone_manager::cache_vzones()
{
    vzone_cache.clear();
    auto vzones = g->m.get_vehicle_zones( g->get_levz() );
    for( auto elem : vzones ) {
        if( !elem->get_enabled() ) {
            continue;
        }

        const std::string &type_hash = elem->get_type_hash();
        auto &cache = area_cache[type_hash];

        tripoint start = elem->get_start_point();
        tripoint end = elem->get_end_point();

        // Draw marked area
        for( int x = start.x; x <= end.x; ++x ) {
            for( int y = start.y; y <= end.y; ++y ) {
                for( int z = start.z; z <= end.z; ++z ) {
                    cache.insert( tripoint( x, y, z ) );
                }
            }
        }
    }
}

std::unordered_set<tripoint> zone_manager::get_point_set( const zone_type_id &type,
        const faction_id &fac ) const
{
    const auto &type_iter = area_cache.find( zone_data::make_type_hash( type, fac ) );
    if( type_iter == area_cache.end() ) {
        return std::unordered_set<tripoint>();
    }

    return type_iter->second;
}

std::unordered_set<tripoint> zone_manager::get_point_set_loot( const tripoint &where,
        int radius, const faction_id &fac ) const
{
    return get_point_set_loot( where, radius, false, fac );
}

std::unordered_set<tripoint> zone_manager::get_point_set_loot( const tripoint &where,
        int radius, bool npc_search, const faction_id &fac ) const
{
    ( void )fac;
    std::unordered_set<tripoint> res;
    for( const tripoint elem : g->m.points_in_radius( g->m.getlocal( where ), radius ) ) {
        const zone_data *zone = get_zone_at( g->m.getabs( elem ) );
        // if not a LOOT zone
        if( ( !zone ) || ( zone->get_type().str().substr( 0, 4 ) != "LOOT" ) ) {
            continue;
        }
        if( npc_search && ( has( zone_type_id( "NO_NPC_PICKUP" ), elem ) ) ) {
            continue;
        }
        res.insert( elem );
    }
    return res;
}

std::unordered_set<tripoint> zone_manager::get_vzone_set( const zone_type_id &type,
        const faction_id &fac ) const
{
    //Only regenerate the vehicle zone cache if any vehicles have moved
    const auto &type_iter = vzone_cache.find( zone_data::make_type_hash( type, fac ) );
    if( type_iter == vzone_cache.end() ) {
        return std::unordered_set<tripoint>();
    }

    return type_iter->second;
}

bool zone_manager::has( const zone_type_id &type, const tripoint &where,
                        const faction_id &fac ) const
{
    const auto &point_set = get_point_set( type, fac );
    const auto &vzone_set = get_vzone_set( type, fac );
    return point_set.find( where ) != point_set.end() || vzone_set.find( where ) != vzone_set.end();
}

bool zone_manager::has_near( const zone_type_id &type, const tripoint &where, int range,
                             const faction_id &fac ) const
{
    const auto &point_set = get_point_set( type, fac );
    for( auto &point : point_set ) {
        if( point.z == where.z ) {
            if( square_dist( point, where ) <= range ) {
                return true;
            }
        }
    }

    const auto &vzone_set = get_vzone_set( type, fac );
    for( auto &point : vzone_set ) {
        if( point.z == where.z ) {
            if( square_dist( point, where ) <= range ) {
                return true;
            }
        }
    }

    return false;
}

bool zone_manager::has_loot_dest_near( const tripoint &where ) const
{
    for( const auto &ztype : get_manager().get_types() ) {
        const zone_type_id &type = ztype.first;
        if( type == zone_type_id( "CAMP_FOOD" ) || type == zone_type_id( "FARM_PLOT" ) ||
            type == zone_type_id( "LOOT_UNSORTED" ) || type == zone_type_id( "LOOT_IGNORE" ) ||
            type == zone_type_id( "CONSTRUCTION_BLUEPRINT" ) ||
            type == zone_type_id( "NO_AUTO_PICKUP" ) || type == zone_type_id( "NO_NPC_PICKUP" ) ) {
            continue;
        }
        if( has_near( type, where ) ) {
            return true;
        }
    }
    return false;
}

const zone_data *zone_manager::get_zone_at( const tripoint &where, const zone_type_id &type ) const
{
    for( auto it = zones.rbegin(); it != zones.rend(); ++it ) {
        const auto &zone = *it;

        if( zone.has_inside( where ) && zone.get_type() == type ) {
            return &zone;
        }
    }
    auto vzones = g->m.get_vehicle_zones( g->get_levz() );
    for( auto it = vzones.rbegin(); it != vzones.rend(); ++it ) {
        const auto &zone = *it;

        if( zone->has_inside( where ) && zone->get_type() == type ) {
            return zone;
        }
    }
    return nullptr;
}

bool zone_manager::custom_loot_has( const tripoint &where, const item *it ) const
{
    auto zone = get_zone_at( where, zone_type_id( "LOOT_CUSTOM" ) );
    if( !zone || !it ) {
        return false;
    }
    const loot_options options = dynamic_cast<const loot_options &>( zone->get_options() );
    std::string filter_string = options.get_mark();
    auto z = item_filter_from_string( filter_string );

    return z( *it );
}

std::unordered_set<tripoint> zone_manager::get_near( const zone_type_id &type,
        const tripoint &where, int range, const item *it, const faction_id &fac ) const
{
    const auto &point_set = get_point_set( type, fac );
    auto near_point_set = std::unordered_set<tripoint>();

    for( auto &point : point_set ) {
        if( point.z == where.z ) {
            if( square_dist( point, where ) <= range ) {
                if( it && has( zone_type_id( "LOOT_CUSTOM" ), point ) ) {
                    if( custom_loot_has( point, it ) ) {
                        near_point_set.insert( point );
                    }
                } else {
                    near_point_set.insert( point );
                }
            }
        }
    }

    const auto &vzone_set = get_vzone_set( type, fac );
    for( auto &point : vzone_set ) {
        if( point.z == where.z ) {
            if( square_dist( point, where ) <= range ) {
                if( it && has( zone_type_id( "LOOT_CUSTOM" ), point ) ) {
                    if( custom_loot_has( point, it ) ) {
                        near_point_set.insert( point );
                    }
                } else {
                    near_point_set.insert( point );
                }
            }
        }
    }

    return near_point_set;
}

cata::optional<tripoint> zone_manager::get_nearest( const zone_type_id &type, const tripoint &where,
        int range, const faction_id &fac ) const
{
    if( range < 0 ) {
        return cata::nullopt;
    }

    tripoint nearest_pos = tripoint( INT_MIN, INT_MIN, INT_MIN );
    int nearest_dist = range + 1;
    const std::unordered_set<tripoint> &point_set = get_point_set( type, fac );
    for( const tripoint &p : point_set ) {
        int cur_dist = square_dist( p, where );
        if( cur_dist < nearest_dist ) {
            nearest_dist = cur_dist;
            nearest_pos = p;
            if( nearest_dist == 0 ) {
                return nearest_pos;
            }
        }
    }

    const std::unordered_set<tripoint> &vzone_set = get_vzone_set( type, fac );
    for( const tripoint &p : vzone_set ) {
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
        return cata::nullopt;
    }
    return nearest_pos;
}

zone_type_id zone_manager::get_near_zone_type_for_item( const item &it,
        const tripoint &where, int range ) const
{
    auto cat = it.get_category();
    if( has_near( zone_type_id( "LOOT_CUSTOM" ), where ) ) {
        for( const auto elem : get_near( zone_type_id( "LOOT_CUSTOM" ), where, 60, &it ) ) {
            ( void )elem;
            return zone_type_id( "LOOT_CUSTOM" );
        }
    }
    if( it.has_flag( "FIREWOOD" ) ) {
        if( has_near( zone_type_id( "LOOT_WOOD" ), where, range ) ) {
            return zone_type_id( "LOOT_WOOD" );
        }
    }
    if( it.is_corpse() ) {
        if( has_near( zone_type_id( "LOOT_CORPSE" ), where, range ) ) {
            return zone_type_id( "LOOT_CORPSE" );
        }
    }

    if( cat.id() == "food" ) {
        const bool preserves = it.is_food_container() && it.type->container->preserves;
        const auto &it_food = it.is_food_container() ? it.contents.front() : it;

        if( it_food.is_food() ) { // skip food without comestible, like MREs
            if( it_food.get_comestible()->comesttype == "DRINK" ) {
                if( !preserves && it_food.goes_bad() && has_near( zone_type_id( "LOOT_PDRINK" ), where, range ) ) {
                    return zone_type_id( "LOOT_PDRINK" );
                } else if( has_near( zone_type_id( "LOOT_DRINK" ), where, range ) ) {
                    return zone_type_id( "LOOT_DRINK" );
                }
            }

            if( !preserves && it_food.goes_bad() && has_near( zone_type_id( "LOOT_PFOOD" ), where, range ) ) {
                return zone_type_id( "LOOT_PFOOD" );
            }
        }

        return zone_type_id( "LOOT_FOOD" );
    }
    if( cat.id() == "guns" ) {
        return zone_type_id( "LOOT_GUNS" );
    }
    if( cat.id() == "magazines" ) {
        return zone_type_id( "LOOT_MAGAZINES" );
    }
    if( cat.id() == "ammo" ) {
        return zone_type_id( "LOOT_AMMO" );
    }
    if( cat.id() == "weapons" ) {
        return zone_type_id( "LOOT_WEAPONS" );
    }
    if( cat.id() == "tools" ) {
        return zone_type_id( "LOOT_TOOLS" );
    }
    if( cat.id() == "clothing" ) {
        if( it.is_filthy() && has_near( zone_type_id( "LOOT_FCLOTHING" ), where, range ) ) {
            return zone_type_id( "LOOT_FCLOTHING" );
        }
        return zone_type_id( "LOOT_CLOTHING" );
    }
    if( cat.id() == "drugs" ) {
        return zone_type_id( "LOOT_DRUGS" );
    }
    if( cat.id() == "books" ) {
        return zone_type_id( "LOOT_BOOKS" );
    }
    if( cat.id() == "mods" ) {
        return zone_type_id( "LOOT_MODS" );
    }
    if( cat.id() == "mutagen" ) {
        return zone_type_id( "LOOT_MUTAGENS" );
    }
    if( cat.id() == "bionics" ) {
        return zone_type_id( "LOOT_BIONICS" );
    }
    if( cat.id() == "veh_parts" ) {
        return zone_type_id( "LOOT_VEHICLE_PARTS" );
    }
    if( cat.id() == "other" ) {
        return zone_type_id( "LOOT_OTHER" );
    }
    if( cat.id() == "fuel" ) {
        return zone_type_id( "LOOT_FUEL" );
    }
    if( cat.id() == "seeds" ) {
        return zone_type_id( "LOOT_SEEDS" );
    }
    if( cat.id() == "chems" ) {
        return zone_type_id( "LOOT_CHEMICAL" );
    }
    if( cat.id() == "spare_parts" ) {
        return zone_type_id( "LOOT_SPARE_PARTS" );
    }
    if( cat.id() == "artifacts" ) {
        return zone_type_id( "LOOT_ARTIFACTS" );
    }
    if( cat.id() == "armor" ) {
        if( it.is_filthy() && has_near( zone_type_id( "LOOT_FARMOR" ), where, range ) ) {
            return zone_type_id( "LOOT_FARMOR" );
        }
        return zone_type_id( "LOOT_ARMOR" );
    }

    return zone_type_id();
}

std::vector<zone_data> zone_manager::get_zones( const zone_type_id &type,
        const tripoint &where, const faction_id &fac ) const
{
    auto zones = std::vector<zone_data>();

    for( const auto &zone : this->zones ) {
        if( zone.get_type() == type && zone.get_faction() == fac ) {
            if( zone.has_inside( where ) ) {
                zones.emplace_back( zone );
            }
        }
    }

    return zones;
}

const zone_data *zone_manager::get_zone_at( const tripoint &where ) const
{
    for( auto it = zones.rbegin(); it != zones.rend(); ++it ) {
        const auto &zone = *it;

        if( zone.has_inside( where ) ) {
            return &zone;
        }
    }
    return nullptr;
}

const zone_data *zone_manager::get_bottom_zone( const tripoint &where,
        const faction_id &fac ) const
{
    for( auto it = zones.rbegin(); it != zones.rend(); ++it ) {
        const auto &zone = *it;
        if( zone.get_faction() != fac ) {
            continue;
        }

        if( zone.has_inside( where ) ) {
            return &zone;
        }
    }
    auto vzones = g->m.get_vehicle_zones( g->get_levz() );
    for( auto it = vzones.rbegin(); it != vzones.rend(); ++it ) {
        const auto zone = *it;
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
void zone_manager::create_vehicle_loot_zone( vehicle &vehicle, const point &mount_point,
        zone_data &new_zone )
{
    //create a vehicle loot zone
    new_zone.set_is_vehicle( true );
    auto nz = vehicle.loot_zones.emplace( mount_point, new_zone );
    g->m.register_vehicle_zone( &vehicle, g->get_levz() );
    vehicle.zones_dirty = false;
    added_vzones.push_back( &nz->second );
    cache_vzones();
}

void zone_manager::add( const std::string &name, const zone_type_id &type, const faction_id &fac,
                        const bool invert, const bool enabled, const tripoint &start,
                        const tripoint &end, std::shared_ptr<zone_options> options )
{
    zone_data new_zone = zone_data( name, type, fac, invert, enabled, start, end, options );
    //the start is a vehicle tile with cargo space
    if( const cata::optional<vpart_reference> vp = g->m.veh_at( g->m.getlocal(
                start ) ).part_with_feature( "CARGO", false ) ) {
        // TODO:Allow for loot zones on vehicles to be larger than 1x1
        if( start == end && query_yn( _( "Bind this zone to the cargo part here?" ) ) ) {
            // TODO: refactor zone options for proper validation code
            if( type == zone_type_id( "FARM_PLOT" ) || type == zone_type_id( "CONSTRUCTION_BLUEPRINT" ) ) {
                popup( _( "You cannot add that type of zone to a vehicle." ), PF_NONE );
                return;
            }

            create_vehicle_loot_zone( vp->vehicle(), vp->mount(), new_zone );
            return;
        }
    }

    //Create a regular zone
    zones.push_back( new_zone );
    cache_data();
}

bool zone_manager::remove( zone_data &zone )
{
    for( auto it = zones.begin(); it != zones.end(); ++it ) {
        if( &zone == &*it ) {
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

    if( !g->m.deregister_vehicle_zone( zone ) ) {
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

void zone_manager::rotate_zones( map &target_map, const int turns )
{
    if( turns == 0 ) {
        return;
    }
    const tripoint a_start = target_map.getabs( tripoint_zero );
    const tripoint a_end = target_map.getabs( tripoint( 23, 23, 0 ) );
    const point dim( 24, 24 );
    for( zone_data &zone : zones ) {
        const tripoint z_start = zone.get_start_point();
        const tripoint z_end = zone.get_end_point();
        if( ( a_start.x <= z_start.x && a_start.y <= z_start.y ) &&
            ( a_end.x > z_start.x && a_end.y >= z_start.y ) &&
            ( a_start.x <= z_end.x && a_start.y <= z_end.y ) &&
            ( a_end.x >= z_end.x && a_end.y >= z_end.y ) ) {
            tripoint z_l_start3 = target_map.getlocal( z_start );
            tripoint z_l_end3 = target_map.getlocal( z_end );
            // don't rotate centered squares
            if( z_l_start3.x == z_l_start3.y && z_l_end3.x == z_l_end3.y && z_l_start3.x + z_l_end3.x == 23 ) {
                continue;
            }
            point z_l_start = z_l_start3.xy().rotate( turns, dim );
            point z_l_end = z_l_end3.xy().rotate( turns, dim );
            point new_z_start = target_map.getabs( z_l_start );
            point new_z_end = target_map.getabs( z_l_end );
            tripoint first = tripoint( std::min( new_z_start.x, new_z_end.x ),
                                       std::min( new_z_start.y, new_z_end.y ), a_start.z );
            tripoint second = tripoint( std::max( new_z_start.x, new_z_end.x ),
                                        std::max( new_z_start.y, new_z_end.y ), a_end.z );
            zone.set_position( std::make_pair( first, second ), false );
        }
    }
}

void zone_manager::start_sort( const std::vector<tripoint> &src_sorted )
{
    for( auto &src : src_sorted ) {
        num_processed[src] = 0;
    }
}

void zone_manager::end_sort()
{
    num_processed.clear();
}

bool zone_manager::is_sorting() const
{
    return !num_processed.empty();
}

int zone_manager::get_num_processed( const tripoint &src ) const
{
    auto it = num_processed.find( src );
    if( it != num_processed.end() ) {
        return it->second;
    }
    return 0;
}

void zone_manager::increment_num_processed( const tripoint &src )
{
    num_processed[src]++;
}

void zone_manager::decrement_num_processed( const tripoint &src )
{
    num_processed[src]--;
    if( num_processed[src] < 0 ) {
        num_processed[src] = 0;
    }
}

std::vector<zone_manager::ref_zone_data> zone_manager::get_zones( const faction_id &fac )
{
    auto zones = std::vector<ref_zone_data>();

    for( auto &zone : this->zones ) {
        if( zone.get_faction() == fac ) {
            zones.emplace_back( zone );
        }
    }

    auto vzones = g->m.get_vehicle_zones( g->get_levz() );

    for( auto zone : vzones ) {
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

    for( auto &zone : this->zones ) {
        if( zone.get_faction() == fac ) {
            zones.emplace_back( zone );
        }
    }

    auto vzones = g->m.get_vehicle_zones( g->get_levz() );

    for( auto zone : vzones ) {
        if( zone->get_faction() == fac ) {
            zones.emplace_back( *zone );
        }
    }

    return zones;
}

void zone_manager::serialize( JsonOut &json ) const
{
    json.write( zones );
}

void zone_manager::deserialize( JsonIn &jsin )
{
    jsin.read( zones );
    for( auto it = zones.begin(); it != zones.end(); ++it ) {
        const zone_type_id zone_type = it->get_type();
        if( !has_type( zone_type ) ) {
            zones.erase( it );
            debugmsg( "Invalid zone type: %s", zone_type.c_str() );
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
    json.member( "start", start );
    json.member( "end", end );
    get_options().serialize( json );
    json.end_object();
}

void zone_data::deserialize( JsonIn &jsin )
{
    JsonObject data = jsin.get_object();
    data.read( "name", name );
    data.read( "type", type );
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
    //Legacy support
    if( data.has_member( "start_x" ) ) {
        tripoint s;
        tripoint e;
        data.read( "start_x", s.x );
        data.read( "start_y", s.y );
        data.read( "start_z", s.z );
        data.read( "end_x", e.x );
        data.read( "end_y", e.y );
        data.read( "end_z", e.z );
        start = s;
        end = e;
    } else {
        data.read( "start", start );
        data.read( "end", end );
    }
    auto new_options = zone_options::create( type );
    new_options->deserialize( data );
    options = new_options;
}

bool zone_manager::save_zones()
{
    std::string savefile = g->get_player_base_save_path() + ".zones.json";

    added_vzones.clear();
    changed_vzones.clear();
    removed_vzones.clear();
    return write_to_file( savefile, [&]( std::ostream & fout ) {
        JsonOut jsout( fout );
        serialize( jsout );
    }, _( "zones date" ) );
}

void zone_manager::load_zones()
{
    std::string savefile = g->get_player_base_save_path() + ".zones.json";

    read_from_file_optional( savefile, [&]( std::istream & fin ) {
        JsonIn jsin( fin );
        deserialize( jsin );
    } );
    revert_vzones();
    added_vzones.clear();
    changed_vzones.clear();
    removed_vzones.clear();

    cache_data();
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
        changed_vzones.push_back( std::make_pair( zone_data( zone ), &zone ) );
    }
}

void zone_manager::revert_vzones()
{
    for( auto zone : removed_vzones ) {
        //Code is copied from add() to avoid yn query
        if( const cata::optional<vpart_reference> vp = g->m.veh_at( g->m.getlocal(
                    zone.get_start_point() ) ).part_with_feature( "CARGO", false ) ) {
            zone.set_is_vehicle( true );
            vp->vehicle().loot_zones.emplace( vp->mount(), zone );
            vp->vehicle().zones_dirty = false;
            g->m.register_vehicle_zone( &vp->vehicle(), g->get_levz() );
            cache_vzones();
        }
    }
    for( const auto &zpair : changed_vzones ) {
        *( zpair.second ) = zpair.first;
    }
    for( auto zone : added_vzones ) {
        remove( *zone );
    }
}
