#include <string>

#include "cata_catch.h"
#include "type_id.h"

static const oter_str_id oter_sewer_es( "sewer_es" );
static const oter_str_id oter_sewer_esw( "sewer_esw" );
static const oter_str_id oter_sewer_ew( "sewer_ew" );
static const oter_str_id oter_sewer_ne( "sewer_ne" );
static const oter_str_id oter_sewer_nes( "sewer_nes" );
static const oter_str_id oter_sewer_nesw( "sewer_nesw" );
static const oter_str_id oter_sewer_new( "sewer_new" );
static const oter_str_id oter_sewer_ns( "sewer_ns" );
static const oter_str_id oter_sewer_nsw( "sewer_nsw" );
static const oter_str_id oter_sewer_sw( "sewer_sw" );
static const oter_str_id oter_sewer_wn( "sewer_wn" );

static bool connects_to( const oter_id &there, int dir )
{
    switch( dir ) {
        // South
        case 2:
            if( there == oter_sewer_ns || there == oter_sewer_es || there == oter_sewer_sw ||
                there == oter_sewer_nes || there == oter_sewer_nsw || there == oter_sewer_esw ||
                there == oter_sewer_nesw ) {
                return true;
            }
            return false;
        // West
        case 3:
            if( there == oter_sewer_ew || there == oter_sewer_sw || there == oter_sewer_wn ||
                there == oter_sewer_new || there == oter_sewer_nsw || there == oter_sewer_esw ||
                there == oter_sewer_nesw ) {
                return true;
            }
            return false;
        // North
        case 0:
            if( there == oter_sewer_ns || there == oter_sewer_ne || there == oter_sewer_wn ||
                there == oter_sewer_nes || there == oter_sewer_new || there == oter_sewer_nsw ||
                there == oter_sewer_nesw ) {
                return true;
            }
            return false;
        // East
        case 1:
            if( there == oter_sewer_ew || there == oter_sewer_ne || there == oter_sewer_es ||
                there == oter_sewer_nes || there == oter_sewer_new || there == oter_sewer_esw ||
                there == oter_sewer_nesw ) {
                return true;
            }
            return false;
        default:
            return false;
    }
}

TEST_CASE( "connects_to", "[mapgen][connects]" )
{
    // connects_to returns true if a given oter connects to a given cardinal
    // compass direction, identified by an integer, clockwise from 0:North

    int north = 0;
    int east = 1;
    int south = 2;
    int west = 3;

    // oter suffixes must be one of the following (order matters):
    // ne, ns, es, nes, wn, ew, new, sw, nsw, esw, nesw

    SECTION( "two-way connections" ) {
        // North/South
        CHECK( connects_to( oter_id( "sewer_ns" ), north ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_ns" ), east ) );
        CHECK( connects_to( oter_id( "sewer_ns" ), south ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_ns" ), west ) );

        // East/West
        CHECK_FALSE( connects_to( oter_id( "sewer_ew" ), north ) );
        CHECK( connects_to( oter_id( "sewer_ew" ), east ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_ew" ), south ) );
        CHECK( connects_to( oter_id( "sewer_ew" ), west ) );

        // North/East
        CHECK( connects_to( oter_id( "sewer_ne" ), north ) );
        CHECK( connects_to( oter_id( "sewer_ne" ), east ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_ne" ), south ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_ne" ), west ) );

        // East/South
        CHECK_FALSE( connects_to( oter_id( "sewer_es" ), north ) );
        CHECK( connects_to( oter_id( "sewer_es" ), east ) );
        CHECK( connects_to( oter_id( "sewer_es" ), south ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_es" ), west ) );

        // South/West
        CHECK_FALSE( connects_to( oter_id( "sewer_sw" ), north ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_sw" ), east ) );
        CHECK( connects_to( oter_id( "sewer_sw" ), south ) );
        CHECK( connects_to( oter_id( "sewer_sw" ), west ) );

        // West/North
        CHECK( connects_to( oter_id( "sewer_wn" ), north ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_wn" ), east ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_wn" ), south ) );
        CHECK( connects_to( oter_id( "sewer_wn" ), west ) );
    }

    SECTION( "three-way connections" ) {
        // North/East/South
        CHECK( connects_to( oter_id( "sewer_nes" ), north ) );
        CHECK( connects_to( oter_id( "sewer_nes" ), east ) );
        CHECK( connects_to( oter_id( "sewer_nes" ), south ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_nes" ), west ) );

        // East/South/West
        CHECK_FALSE( connects_to( oter_id( "sewer_esw" ), north ) );
        CHECK( connects_to( oter_id( "sewer_esw" ), east ) );
        CHECK( connects_to( oter_id( "sewer_esw" ), south ) );
        CHECK( connects_to( oter_id( "sewer_esw" ), west ) );

        // South/West/North
        CHECK( connects_to( oter_id( "sewer_nsw" ), north ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_nsw" ), east ) );
        CHECK( connects_to( oter_id( "sewer_nsw" ), south ) );
        CHECK( connects_to( oter_id( "sewer_nsw" ), west ) );

        // West/North/East
        CHECK( connects_to( oter_id( "sewer_new" ), north ) );
        CHECK( connects_to( oter_id( "sewer_new" ), east ) );
        CHECK_FALSE( connects_to( oter_id( "sewer_new" ), south ) );
        CHECK( connects_to( oter_id( "sewer_new" ), west ) );
    }

    SECTION( "four-way connections" ) {
        // North/East/South/West
        CHECK( connects_to( oter_id( "sewer_nesw" ), north ) );
        CHECK( connects_to( oter_id( "sewer_nesw" ), east ) );
        CHECK( connects_to( oter_id( "sewer_nesw" ), south ) );
        CHECK( connects_to( oter_id( "sewer_nesw" ), west ) );
    }
}
