#include "clzones.h"
#include "game.h"
#include "json.h"
#include "debug.h"
#include "output.h"
#include "cata_utility.h"
#include "translations.h"
#include "ui.h"
#include "string_input_popup.h"
#include "line.h"
#include "item.h"
#include "itype.h"
#include "item_category.h"

#include <iostream>

zone_manager::zone_manager()
{
    types.emplace( zone_type_id( "NO_AUTO_PICKUP" ),
                   zone_type( translate_marker( "No Auto Pickup" ) ) );
    types.emplace( zone_type_id( "NO_NPC_PICKUP" ), zone_type( translate_marker( "No NPC Pickup" ) ) );
    types.emplace( zone_type_id( "LOOT_UNSORTED" ), zone_type( translate_marker( "Loot: Unsorted" ) ) );
    types.emplace( zone_type_id( "LOOT_FOOD" ), zone_type( translate_marker( "Loot: Food" ) ) );
    types.emplace( zone_type_id( "LOOT_PFOOD" ), zone_type( translate_marker( "Loot: P.Food" ) ) );
    types.emplace( zone_type_id( "LOOT_DRINK" ), zone_type( translate_marker( "Loot: Drink" ) ) );
    types.emplace( zone_type_id( "LOOT_PDRINK" ), zone_type( translate_marker( "Loot: P.Drink" ) ) );
    types.emplace( zone_type_id( "LOOT_GUNS" ), zone_type( translate_marker( "Loot: Guns" ) ) );
    types.emplace( zone_type_id( "LOOT_MAGAZINES" ),
                   zone_type( translate_marker( "Loot: Magazines" ) ) );
    types.emplace( zone_type_id( "LOOT_AMMO" ), zone_type( translate_marker( "Loot: Ammo" ) ) );
    types.emplace( zone_type_id( "LOOT_WEAPONS" ), zone_type( translate_marker( "Loot: Weapons" ) ) );
    types.emplace( zone_type_id( "LOOT_TOOLS" ), zone_type( translate_marker( "Loot: Tools" ) ) );
    types.emplace( zone_type_id( "LOOT_CLOTHING" ), zone_type( translate_marker( "Loot: Clothing" ) ) );
    types.emplace( zone_type_id( "LOOT_FCLOTHING" ),
                   zone_type( translate_marker( "Loot: F.Clothing" ) ) );
    types.emplace( zone_type_id( "LOOT_DRUGS" ), zone_type( translate_marker( "Loot: Drugs" ) ) );
    types.emplace( zone_type_id( "LOOT_BOOKS" ), zone_type( translate_marker( "Loot: Books" ) ) );
    types.emplace( zone_type_id( "LOOT_MODS" ), zone_type( translate_marker( "Loot: Mods" ) ) );
    types.emplace( zone_type_id( "LOOT_MUTAGENS" ), zone_type( translate_marker( "Loot: Mutagens" ) ) );
    types.emplace( zone_type_id( "LOOT_BIONICS" ), zone_type( translate_marker( "Loot: Bionics" ) ) );
    types.emplace( zone_type_id( "LOOT_VEHICLE_PARTS" ),
                   zone_type( translate_marker( "Loot: V.Parts" ) ) );
    types.emplace( zone_type_id( "LOOT_OTHER" ), zone_type( translate_marker( "Loot: Other" ) ) );
    types.emplace( zone_type_id( "LOOT_FUEL" ), zone_type( translate_marker( "Loot: Fuel" ) ) );
    types.emplace( zone_type_id( "LOOT_SEEDS" ), zone_type( translate_marker( "Loot: Seeds" ) ) );
    types.emplace( zone_type_id( "LOOT_CHEMICAL" ), zone_type( translate_marker( "Loot: Chemical" ) ) );
    types.emplace( zone_type_id( "LOOT_SPARE_PARTS" ),
                   zone_type( translate_marker( "Loot: S.Parts" ) ) );
    types.emplace( zone_type_id( "LOOT_ARTIFACTS" ),
                   zone_type( translate_marker( "Loot: Artifacts" ) ) );
    types.emplace( zone_type_id( "LOOT_ARMOR" ), zone_type( translate_marker( "Loot: Armor" ) ) );
    types.emplace( zone_type_id( "LOOT_FARMOR" ), zone_type( translate_marker( "Loot: F.Armor" ) ) );
    types.emplace( zone_type_id( "LOOT_WOOD" ), zone_type( translate_marker( "Loot: Wood" ) ) );
    types.emplace( zone_type_id( "LOOT_IGNORE" ), zone_type( translate_marker( "Loot: Ignore" ) ) );
}

std::string zone_type::name() const
{
    return _( name_.c_str() );
}

std::string zone_manager::query_name( std::string default_name )
{
    return string_input_popup()
           .title( _( "Zone name:" ) )
           .width( 55 )
           .text( default_name )
           .max_length( 15 )
           .query_string();
}

zone_type_id zone_manager::query_type()
{
    const auto &types = get_manager().get_types();
    uimenu as_m;
    as_m.text = _( "Select zone type:" );

    size_t i = 0;
    for( const auto &type : types ) {
        as_m.addentry( i++, true, MENU_AUTOASSIGN, type.second.name() );
    }

    as_m.query();
    size_t index = as_m.ret;

    auto iter = types.begin();
    std::advance( iter, index );

    return iter->first;
}

void zone_manager::zone_data::set_name()
{
    const std::string new_name = get_manager().query_name( name );

    name = ( new_name.empty() ) ? _( "<no name>" ) : new_name;
}

void zone_manager::zone_data::set_type()
{
    type = get_manager().query_type();

    get_manager().cache_data();
}

void zone_manager::zone_data::set_position( const std::pair<tripoint, tripoint> position )
{
    start = position.first;
    end = position.second;

    get_manager().cache_data();
}

void zone_manager::zone_data::set_enabled( const bool _enabled )
{
    enabled = _enabled;
}

tripoint zone_manager::zone_data::get_center_point() const
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

void zone_manager::cache_data()
{
    area_cache.clear();

    for( auto &elem : zones ) {
        if( !elem.get_enabled() ) {
            continue;
        }

        const zone_type_id &type = elem.get_type();
        auto &cache = area_cache[type];

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

std::unordered_set<tripoint> zone_manager::get_point_set( const zone_type_id &type ) const
{
    const auto &type_iter = area_cache.find( type );
    if( type_iter == area_cache.end() ) {
        return std::unordered_set<tripoint>();
    }

    return type_iter->second;;
}

bool zone_manager::has( const zone_type_id &type, const tripoint &where ) const
{
    const auto &point_set = get_point_set( type );
    return point_set.find( where ) != point_set.end();
}

bool zone_manager::has_near( const zone_type_id &type, const tripoint &where ) const
{
    const auto &point_set = get_point_set( type );

    for( auto &point : point_set ) {
        if( square_dist( point, where ) <= MAX_DISTANCE ) {
            return true;
        }
    }

    return false;
}

std::unordered_set<tripoint> zone_manager::get_near( const zone_type_id &type,
        const tripoint &where ) const
{
    const auto &point_set = get_point_set( type );
    auto near_point_set = std::unordered_set<tripoint>();

    for( auto &point : point_set ) {
        if( square_dist( point, where ) <= MAX_DISTANCE ) {
            near_point_set.insert( point );
        }
    }

    return near_point_set;
}

zone_type_id zone_manager::get_near_zone_type_for_item( const item &it,
        const tripoint &where ) const
{
    auto cat = it.get_category();

    auto typeId = it.typeId();
    if( typeId == "2x4" ||
        typeId == "log" ||
        typeId == "splinter" ||
        typeId == "stick" ) {
        if( has_near( zone_type_id( "LOOT_WOOD" ), where ) ) {
            return zone_type_id( "LOOT_WOOD" );
        }
    }

    if( cat.id() == "food" ) {
        const bool preserves = it.is_food_container() && it.type->container->preserves ? 1 : 0;
        const auto &it_food = it.is_food_container() ? it.contents.front() : it;

        if( it_food.is_food() ) { // skip food without comestible, like MREs
            if( it_food.type->comestible->comesttype == "DRINK" ) {
                if( !preserves && it_food.goes_bad() && has_near( zone_type_id( "LOOT_PDRINK" ), where ) ) {
                    return zone_type_id( "LOOT_PDRINK" );
                } else if( has_near( zone_type_id( "LOOT_DRINK" ), where ) ) {
                    return zone_type_id( "LOOT_DRINK" );
                }
            }

            if( !preserves && it_food.goes_bad() && has_near( zone_type_id( "LOOT_PFOOD" ), where ) ) {
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
        if( it.is_filthy() && has_near( zone_type_id( "LOOT_FCLOTHING" ), where ) ) {
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
        if( it.is_filthy() && has_near( zone_type_id( "LOOT_FARMOR" ), where ) ) {
            return zone_type_id( "LOOT_FARMOR" );
        }
        return zone_type_id( "LOOT_ARMOR" );
    }

    return zone_type_id();
}

void zone_manager::add( const std::string &name, const zone_type_id &type,
                        const bool invert, const bool enabled,
                        const tripoint &start, const tripoint &end )
{
    zones.push_back( zone_data( name, type, invert, enabled, start, end ) );
    cache_data();
}

void zone_manager::serialize( JsonOut &json ) const
{
    json.start_array();
    for( auto &elem : zones ) {
        json.start_object();

        json.member( "name", elem.get_name() );
        json.member( "type", elem.get_type() );
        json.member( "invert", elem.get_invert() );
        json.member( "enabled", elem.get_enabled() );

        tripoint start = elem.get_start_point();
        tripoint end = elem.get_end_point();

        json.member( "start_x", start.x );
        json.member( "start_y", start.y );
        json.member( "start_z", start.z );
        json.member( "end_x", end.x );
        json.member( "end_y", end.y );
        json.member( "end_z", end.z );

        json.end_object();
    }

    json.end_array();
}

void zone_manager::deserialize( JsonIn &jsin )
{
    zones.clear();

    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject jo_zone = jsin.get_object();

        const std::string name = jo_zone.get_string( "name" );
        const zone_type_id type( jo_zone.get_string( "type" ) );

        const bool invert = jo_zone.get_bool( "invert" );
        const bool enabled = jo_zone.get_bool( "enabled" );

        // Z-coordinates need to have a default value - old saves won't have those
        const int start_x = jo_zone.get_int( "start_x" );
        const int start_y = jo_zone.get_int( "start_y" );
        const int start_z = jo_zone.get_int( "start_z", 0 );
        const int end_x = jo_zone.get_int( "end_x" );
        const int end_y = jo_zone.get_int( "end_y" );
        const int end_z = jo_zone.get_int( "end_z", 0 );

        if( has_type( type ) ) {
            add( name, type, invert, enabled,
                 tripoint( start_x, start_y, start_z ),
                 tripoint( end_x, end_y, end_z ) );
        } else {
            debugmsg( "Invalid zone type: %s", type.c_str() );
        }
    }
}


bool zone_manager::save_zones()
{
    std::string savefile = g->get_player_base_save_path() + ".zones.json";

    return write_to_file_exclusive( savefile, [&]( std::ostream & fout ) {
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

    cache_data();
}
