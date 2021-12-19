#include <string>

#include "cata_catch.h"
#include "item.h"
#include "map.h"

static const itype_id itype_test_baseball( "test_baseball" );
static const itype_id itype_test_baseball_half_degradation( "test_baseball_half_degradation" );
static const itype_id itype_test_baseball_x2_degradation( "test_baseball_x2_degradation" );

static constexpr int max_iters = 4000;
static constexpr tripoint spawn_pos( HALF_MAPSIZE_X - 1, HALF_MAPSIZE_Y, 0 );

static float get_avg_degradation( const itype_id &it, int count, int damage )
{
    if( count <= 0 ) {
        return 0.f;
    }
    map &m = get_map();
    m.spawn_item( spawn_pos, it, count, 0, calendar::turn, damage );
    REQUIRE( m.i_at( spawn_pos ).size() == static_cast<unsigned>( count ) );
    float deg = 0.f;
    for( const item &it : m.i_at( spawn_pos ) ) {
        deg += it.degradation();
    }
    m.i_clear( spawn_pos );
    return deg / count;
}

TEST_CASE( "Degradation on spawned items", "[item][degradation]" )
{
    SECTION( "Non-spawned items have no degradation" ) {
        item norm( itype_test_baseball );
        item half( itype_test_baseball_half_degradation );
        item doub( itype_test_baseball_x2_degradation );
        CHECK( norm.degradation() == 0 );
        CHECK( half.degradation() == 0 );
        CHECK( doub.degradation() == 0 );
    }

    SECTION( "Items spawned with -1000 damage" ) {
        float norm = get_avg_degradation( itype_test_baseball, max_iters, -1000 );
        float half = get_avg_degradation( itype_test_baseball_half_degradation, max_iters, -1000 );
        float doub = get_avg_degradation( itype_test_baseball_x2_degradation, max_iters, -1000 );
        CHECK( norm == Approx( 0.0 ).epsilon( 0.001 ) );
        CHECK( half == Approx( 0.0 ).epsilon( 0.001 ) );
        CHECK( doub == Approx( 0.0 ).epsilon( 0.001 ) );
    }

    SECTION( "Items spawned with 0 damage" ) {
        float norm = get_avg_degradation( itype_test_baseball, max_iters, 0 );
        float half = get_avg_degradation( itype_test_baseball_half_degradation, max_iters, 0 );
        float doub = get_avg_degradation( itype_test_baseball_x2_degradation, max_iters, 0 );
        CHECK( norm == Approx( 0.0 ).epsilon( 0.001 ) );
        CHECK( half == Approx( 0.0 ).epsilon( 0.001 ) );
        CHECK( doub == Approx( 0.0 ).epsilon( 0.001 ) );
    }

    SECTION( "Items spawned with 2000 damage" ) {
        float norm = get_avg_degradation( itype_test_baseball, max_iters, 2000 );
        float half = get_avg_degradation( itype_test_baseball_half_degradation, max_iters, 2000 );
        float doub = get_avg_degradation( itype_test_baseball_x2_degradation, max_iters, 2000 );
        CHECK( norm == Approx( 1000.0 ).epsilon( 0.1 ) );
        CHECK( half == Approx( 500.0 ).epsilon( 0.1 ) );
        CHECK( doub == Approx( 2000.0 ).epsilon( 0.1 ) );
    }

    SECTION( "Items spawned with 4000 damage" ) {
        float norm = get_avg_degradation( itype_test_baseball, max_iters, 4000 );
        float half = get_avg_degradation( itype_test_baseball_half_degradation, max_iters, 4000 );
        float doub = get_avg_degradation( itype_test_baseball_x2_degradation, max_iters, 4000 );
        CHECK( norm == Approx( 2000.0 ).epsilon( 0.1 ) );
        CHECK( half == Approx( 1000.0 ).epsilon( 0.1 ) );
        CHECK( doub == Approx( 4000.0 ).epsilon( 0.1 ) );
    }
}

static void add_x_dmg_levels( item &it, int lvls )
{
    it.mod_damage( -5000 );
    for( int i = 0; i < lvls; i++ ) {
        it.mod_damage( 1000 );
        it.mod_damage( -1000 );
    }
}

TEST_CASE( "Items that get damaged gain degradation", "[item][degradation]" )
{
    GIVEN( "An item with default degradation rate" ) {
        item it( itype_test_baseball );
        it.mod_damage( -1000 );
        REQUIRE( it.degrade_increments() == 50 );
        REQUIRE( it.degradation() == 0 );
        REQUIRE( it.damage() == -1000 );
        REQUIRE( it.damage_level() == -1 );

        WHEN( "Item loses 2 damage levels" ) {
            add_x_dmg_levels( it, 2 );
            THEN( "Item gains 10 percent as degradation, -1 damage level" ) {
                CHECK( it.degradation() == 200 );
                CHECK( it.damage_level() == -1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 10 damage levels" ) {
            add_x_dmg_levels( it, 10 );
            THEN( "Item gains 10 percent as degradation, 0 damage level" ) {
                CHECK( it.degradation() == 1000 );
                CHECK( it.damage_level() == 0 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 20 damage levels" ) {
            add_x_dmg_levels( it, 20 );
            THEN( "Item gains 10 percent as degradation, 1 damage level" ) {
                CHECK( it.degradation() == 2000 );
                CHECK( it.damage_level() == 1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }
    }

    GIVEN( "An item with half degradation rate" ) {
        item it( itype_test_baseball_half_degradation );
        it.mod_damage( -1000 );
        REQUIRE( it.degrade_increments() == 100 );
        REQUIRE( it.degradation() == 0 );
        REQUIRE( it.damage() == -1000 );
        REQUIRE( it.damage_level() == -1 );

        WHEN( "Item loses 2 damage levels" ) {
            add_x_dmg_levels( it, 2 );
            THEN( "Item gains 5 percent as degradation, -1 damage level" ) {
                CHECK( it.degradation() == 100 );
                CHECK( it.damage_level() == -1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 10 damage levels" ) {
            add_x_dmg_levels( it, 10 );
            THEN( "Item gains 5 percent as degradation, -1 damage level" ) {
                CHECK( it.degradation() == 500 );
                CHECK( it.damage_level() == -1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 20 damage levels" ) {
            add_x_dmg_levels( it, 20 );
            THEN( "Item gains 5 percent as degradation, 0 damage level" ) {
                CHECK( it.degradation() == 1000 );
                CHECK( it.damage_level() == 0 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }
    }

    GIVEN( "An item with double degradation rate" ) {
        item it( itype_test_baseball_x2_degradation );
        it.mod_damage( -1000 );
        REQUIRE( it.degrade_increments() == 25 );
        REQUIRE( it.degradation() == 0 );
        REQUIRE( it.damage() == -1000 );
        REQUIRE( it.damage_level() == -1 );

        WHEN( "Item loses 2 damage levels" ) {
            add_x_dmg_levels( it, 2 );
            THEN( "Item gains 20 percent as degradation, -1 damage level" ) {
                CHECK( it.degradation() == 400 );
                CHECK( it.damage_level() == -1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 10 damage levels" ) {
            add_x_dmg_levels( it, 10 );
            THEN( "Item gains 20 percent as degradation, 1 damage level" ) {
                CHECK( it.degradation() == 2000 );
                CHECK( it.damage_level() == 1 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }

        WHEN( "Item loses 20 damage levels" ) {
            add_x_dmg_levels( it, 20 );
            THEN( "Item gains 20 percent as degradation, 3 damage level" ) {
                CHECK( it.degradation() == 4000 );
                CHECK( it.damage_level() == 3 );
                CHECK( it.damage() == it.degradation() + it.min_damage() );
            }
        }
    }
}