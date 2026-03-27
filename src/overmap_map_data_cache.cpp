#include "overmap_map_data_cache.h"

#include <array>
#include <bitset>
#include <memory>
#include <string>

#include "cata_assert.h"
#include "debug.h"
#include "generic_factory.h"
#include "string_id.h"

// "Placeholder map data" is a set of map_data_summary objects that are used when
// we don't need to have precise data, so that we don't bloat memory and saves.

namespace
{
generic_factory<map_data_summary> placeholder_map_data( "placeholder map data" );
} // namespace

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

void map_data_placeholders::finalize()
{
    placeholder_map_data.finalize();
}

std::shared_ptr<const map_data_summary> map_data_placeholders::get_ptr(
    string_id<map_data_summary> id )
{
    // placeholder_map_data owns the placeholder entries, but overmap::layer[].map_cache ALSO holds
    // a bunch of shared_ptrs that will reference these entries, and we don't want them
    // deallocating when an overmap is destructed. It's set up this way because we want
    // overmap::layer[].map_cache to eventually own references to non-placeholder map_data_summary
    // entries and deallocate them without having explicit lifetime code.
    std::shared_ptr<const map_data_summary> dummy{ nullptr };
    return std::shared_ptr<const map_data_summary> { dummy, &id.obj() };
}

void map_data_summary::load( const JsonObject &jo, const std::string_view & )
{
    std::array<std::string, 24> rows;
    mandatory( jo, was_loaded, "grid", rows );
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
