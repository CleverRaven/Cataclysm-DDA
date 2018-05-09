#include "clzones.h"
#include "game.h"
#include "json.h"
#include "debug.h"
#include "output.h"
#include "cata_utility.h"
#include "translations.h"
#include "ui.h"
#include "string_input_popup.h"

#include <iostream>

zone_manager::zone_manager()
{
    types.emplace( zone_type_id( "NO_AUTO_PICKUP" ),
                   zone_type( translate_marker( "No Auto Pickup" ) ) );
    types.emplace( zone_type_id( "NO_NPC_PICKUP" ), zone_type( translate_marker( "No NPC Pickup" ) ) );
}

std::string zone_type::name() const
{
    return _( name_.c_str() );
}

void zone_manager::zone_data::set_name()
{
    const std::string new_name = string_input_popup()
                                 .title( _( "Zone name:" ) )
                                 .width( 55 )
                                 .text( name )
                                 .max_length( 15 )
                                 .query_string();

    name = ( new_name.empty() ) ? _( "<no name>" ) : new_name;
}

void zone_manager::zone_data::set_type()
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
    type = iter->first;
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

bool zone_manager::has( const zone_type_id &type, const tripoint &where ) const
{
    const auto &type_iter = area_cache.find( type );
    if( type_iter == area_cache.end() ) {
        return false;
    }

    const auto &point_set = type_iter->second;;
    return point_set.find( where ) != point_set.end();
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
