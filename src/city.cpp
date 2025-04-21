#include "city.h"

#include <algorithm>
#include <climits>
#include <vector>

#include "coordinates.h"
#include "debug.h"
#include "generic_factory.h"
#include "options.h"
#include "rng.h"
#include "text_snippets.h"

generic_factory<city> &get_city_factory()
{
    static generic_factory<city> city_factory( "city" );
    return city_factory;
}

/** @relates string_id */
template<>
const city &string_id<city>::obj() const
{
    return get_city_factory().obj( *this );
}

/** @relates string_id */
template<>
bool string_id<city>::is_valid() const
{
    return get_city_factory().is_valid( *this );
}

void city::load_city( const JsonObject &jo, const std::string &src )
{
    get_city_factory().load( jo, src );
}

void city::finalize()
{
    for( city &c : const_cast<std::vector<city>&>( city::get_all() ) ) {
        if( c.name.empty() ) {
            c.name = SNIPPET.expand( "<city_name>" );
        }
        if( c.population == 0 ) {
            c.population = rng( 1, INT_MAX );
        }
        if( c.size == -1 ) {
            c.size = rng( 1, 16 );
        }
    }
}

void city::check_consistency()
{
    get_city_factory().check();
}

const std::vector<city> &city::get_all()
{
    return get_city_factory().get_all();
}

void city::reset()
{
    get_city_factory().reset();
}

void city::load( const JsonObject &jo, std::string_view )
{

    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "database_id", database_id );
    optional( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "population", population );
    optional( jo, was_loaded, "size", size );
    mandatory( jo, was_loaded, "pos_om", pos_om );
    mandatory( jo, was_loaded, "pos", pos );
}

void city::check() const
{
    if( get_option<bool>( "SELECT_STARTING_CITY" ) && city::get_all().empty() ) {
        debugmsg( "Overmap cities need to be defined when `SELECT_STARTING_CITY` option is set to `true`!" );
    }
}

city::city( const point_om_omt &P, int const S )
    : pos( P )
    , size( S )
    , name( SNIPPET.expand( "<city_name>" ) )
{
}

int city::get_distance_from( const tripoint_om_omt &p ) const
{
    return std::max( static_cast<int>( trig_dist( p, tripoint_om_omt{ pos, 0 } ) ) - size, 0 );
}
