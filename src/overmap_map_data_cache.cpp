#include "overmap_map_data_cache.h"

#include <array>

#include "assign.h"
#include "cata_assert.h"
#include "generic_factory.h"
#include "overmap_map_data_cache.h"
#include "string_id.h"

// "Placeholder map data" is a set of map_data_summary objects that are used when
// we don't need to have precise data, so that we don't bloat memory and saves.

namespace
{

generic_factory<map_data_summary> placeholder_map_data( "placeholder map data" );

}

template<>
const map_data_summary &string_id<map_data_summary>::obj() const
{
    return placeholder_map_data.obj( *this );
}

template<>
bool string_id<map_data_summary>::is_valid() const
{
    return placeholder_map_data.is_valid( *this );
}

void map_data_placeholders::load( const JsonObject &jo, const std::string &src )
{
    placeholder_map_data.load( jo, src );
}

void map_data_placeholders::reset()
{
    placeholder_map_data.reset();
}

void map_data_summary::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";
    std::array<std::string, 24> rows;
    assign( jo, "grid", rows, strict );
    placeholder = true;
    int idx = 0;
    cata_assert( rows.size() == 24 );
    for( std::string &row : rows ) {
        cata_assert( row.size() == 24 );
        for( char &elem : row ) {
            switch( elem ) {
                case '0':
                    passable.set( idx, false );
                    break;
                case '1':
                    passable.set( idx, true );
                    break;
                default:
                    debugmsg( "Unknown value %c in map data placeholder", elem ); // error
            }
            ++idx;
        }
    }
}
