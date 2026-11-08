#include <string>
#include <vector>

#include "cata_catch.h"
#include "mapdata.h"
#include "enum_conversions.h"
#include "generic_factory.h"

TEST_CASE( "ter_furn_flag_bitflags_agree_with_string_flags", "[mapdata][flag]" )
{
    auto enum_match = []( const map_data_common_t &obj, const std::string & id ) {
        for( int i = 0; i < static_cast<int>( enum_traits<ter_furn_flag>::last ); ++i ) {
            const ter_furn_flag flag = static_cast<ter_furn_flag>( i );
            const std::string flag_str = io::enum_to_string( flag );
            INFO( "Checking mapdata flag: " << flag_str );
            CAPTURE( id );
            CHECK( obj.has_flag( flag ) == obj.has_flag( flag_str ) );
        }
    };

    for( const furn_t &obj : mapdata::furniture_data.get_all() ) {
        enum_match( obj, obj.id.str() );
    }

    for( const ter_t &obj : mapdata::terrain_data.get_all() ) {
        enum_match( obj, obj.id.str() );
    }
}

TEST_CASE( "ter_furn_flag_enum_string_round_trip", "[mapdata][flag]" )
{
    for( int i = 0; i < static_cast<int>( enum_traits<ter_furn_flag>::last ); ++i ) {
        const ter_furn_flag flag = static_cast<ter_furn_flag>( i );
        const std::string flag_str = io::enum_to_string( flag );
        INFO( "Checking mapdata flag: " << flag_str );
        CHECK( io::string_to_enum<ter_furn_flag>( flag_str ) == flag );
    }
}
