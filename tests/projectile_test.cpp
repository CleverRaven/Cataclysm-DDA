#include "catch/catch.hpp"

#include "avatar.h"
#include "ballistics.h"
#include "dispersion.h"
#include "game.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "projectile.h"

static tripoint projectile_end_point( const std::vector<tripoint> &range, const item &gun,
                                      int speed, int proj_range )
{
    projectile test_proj;
    test_proj.speed = speed;
    test_proj.range = proj_range;
    test_proj.impact = gun.gun_damage();
    test_proj.proj_effects = gun.ammo_effects();
    test_proj.critical_multiplier = gun.ammo_data()->ammo->critical_multiplier;

    dealt_projectile_attack attack;

    attack = projectile_attack( test_proj, range[0], range[2], dispersion_sources(), &g->u, nullptr );

    return attack.end_point;
}

TEST_CASE( "projectiles_through_obstacles", "[projectile]" )
{
    clear_map();

    // Move the player out of the way of the test area
    g->u.setpos( { 2, 2, 0 } );

    // Ensure that a projectile fired from a gun can pass through a chain link fence
    // First, set up a test area - three tiles in a row
    // One on either side clear, with a chainlink fence in the middle
    std::vector<tripoint> range = { tripoint_zero, tripoint_east, tripoint( 2, 0, 0 ) };
    for( const tripoint &pt : range ) {
        REQUIRE( g->m.inbounds( pt ) );
        g->m.ter_set( pt, ter_id( "t_dirt" ) );
        g->m.furn_set( pt, furn_id( "f_null" ) );
        REQUIRE_FALSE( g->critter_at( pt ) );
        REQUIRE( g->m.is_transparent( pt ) );
    }

    // Set an obstacle in the way, a chain fence
    g->m.ter_set( range[1], ter_id( "t_chainfence" ) );

    // Create a gun to fire a projectile from
    item gun( itype_id( "m1a" ) );
    item mag( gun.magazine_default() );
    mag.ammo_set( itype_id( "308" ), 5 );
    gun.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );

    // Check that a bullet with the correct amount of speed can through obstacles
    CHECK( projectile_end_point( range, gun, 1000, 3 ) == range[2] );

    // But that a bullet without the correct amount cannot
    CHECK( projectile_end_point( range, gun, 10, 3 ) == range[0] );
}
