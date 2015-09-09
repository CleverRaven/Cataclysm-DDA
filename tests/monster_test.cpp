#include "catch/catch.hpp"

#include "creature.h"
#include "creature_tracker.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "mtype.h"
#include "options.h"

#include <string>
#include <vector>

std::ostream& operator << ( std::ostream& os, tripoint const& value ) {
    os << "(" << value.x << "," << value.y << "," << value.z << ")";
    return os;
}

int turns_to_destination( const std::string &monster_type,
                          const tripoint &start, const tripoint &end )
{
    monster temp_monster( mtype_id(monster_type), start);
    // Bypassing game::add_zombie() since it sometimes upgrades the monster instantly.
    g->critter_tracker->add( temp_monster );
    monster &test_monster = g->critter_tracker->find( 0 );
    test_monster.set_dest( end );
    // Get it riled up.
    test_monster.anger = 100;
    test_monster.set_moves( 0 );
    int turn = 0;
    for( ; test_monster.pos() != test_monster.move_target() && turn < 1000; ++turn ) {
        test_monster.mod_moves( test_monster.get_speed() );
        while( test_monster.moves >= 0 ) {
            test_monster.move();
        }
    }
    g->remove_zombie( 0 );
    return turn;
}

class stat {
private:
    int _n;
    int _sum;
    int _max;
    int _min;

public:
    stat() : _n(0), _sum(0), _max(INT_MIN), _min(INT_MAX) {}
    
    void add( int new_val ) {
        _n++;
        _sum += new_val;
        _max = std::max(_max, new_val);
        _min = std::min(_min, new_val);
    }
    int sum() { return _sum; }
    int max() { return _max; }
    int min() { return _min; }
    int avg() { return _sum / _n; }
};

// Verify that the named monster has the expected effective speed, which is greatly reduced
// due to wasted motion from shambling.
// This is an assertion that an average (i.e. no fleet) survivor with no encumbrance
// will be able to out-walk (not run, walk) the given monster
// if their speed is higher than the monster's speed stat.
void check_shamble_speed( const std::string monster_type, const tripoint &destination )
{
    // Scale the scaling factor based on the ratio of diagonal to cardinal steps.
    const float slope = (destination.x < destination.y) ?
        (destination.x / destination.y) : (destination.y / destination.x);
    const float diagonal_multiplier = 1.0 + (OPTIONS["CIRCLEDIST"] ? (slope * 0.41) : 0.0);
    const float mon_speed = (float)monster( mtype_id( monster_type ) ).get_speed();
    INFO( monster_type << " " << destination );
    // Wandering makes things nondeterministic, so look at the distribution rather than a target number.
    stat move_stats;
    for( int i = 0; i < 10; ++i ) {
        move_stats.add( turns_to_destination( monster_type, {0, 0, 0}, destination ) );
        if( ((move_stats.avg() * mon_speed) / (10000.0 * diagonal_multiplier)) ==
            Approx(1.0).epsilon(0.04) ) {
            break;
        }
    }
    CHECK( ((move_stats.avg() * mon_speed) / (10000.0 * diagonal_multiplier)) ==
           Approx(1.0).epsilon(0.04) );
}

void monster_check() {
    const float diagonal_multiplier = (OPTIONS["CIRCLEDIST"] ? 1.41 : 1.0);
    // Have a monster walk some distance in a direction and measure how long it takes.
    int vert_move = turns_to_destination( "mon_pig", {0,0,0}, {100,0,0} );
    CHECK( vert_move <= 100 + 2 );
    CHECK( vert_move >= 100 - 2 );
    int horiz_move = turns_to_destination( "mon_pig", {0,0,0}, {0,100,0} );
    CHECK( horiz_move <= 100 + 2 );
    CHECK( horiz_move >= 100 - 2 );
    int diag_move = turns_to_destination( "mon_pig", {0,0,0}, {100,100,0} );
    CHECK( diag_move <= (100 * diagonal_multiplier) + 3 );
    CHECK( diag_move >= (100 * diagonal_multiplier) - 3 );

    check_shamble_speed( "mon_zombie", {100, 0, 0} );
    check_shamble_speed( "mon_zombie", {0, 100, 0} );
    check_shamble_speed( "mon_zombie", {100, 0, 0} );
    check_shamble_speed( "mon_zombie_dog", {100, 0, 0} );
    check_shamble_speed( "mon_zombie_dog", {0, 100, 0} );
    check_shamble_speed( "mon_zombie_dog", {100, 100, 0} );
    check_shamble_speed( "mon_zombear", {100, 0, 0} );
    check_shamble_speed( "mon_zombear", {0, 100, 0} );
    check_shamble_speed( "mon_zombear", {100, 100, 0} );
    check_shamble_speed( "mon_jabberwock", {100, 0, 0} );
    check_shamble_speed( "mon_jabberwock", {0, 100, 0} );
    check_shamble_speed( "mon_jabberwock", {100, 100, 0} );
}

// Characterization test for monster movement speed.
// It's not necessarally the one true speed for monsters, we just want notice if it changes.
TEST_CASE("monster_speed") {
    // Remove all the obstacles.
    const int mapsize = g->m.getmapsize() * SEEX;
    for( int x = 0; x < mapsize; ++x ) {
        for( int y = 0; y < mapsize; ++y ) {
            g->m.set(x, y, t_grass, f_null);
        }
    }
    // Remove any interfering monsters.
    while( g->num_zombies() ) {
        g->remove_zombie( 0 );
    }
    OPTIONS["CIRCLEDIST"].setValue("true");
    trigdist = true;
    monster_check();
    OPTIONS["CIRCLEDIST"].setValue("false");
    trigdist = false;
    monster_check();

    // Check the angles between 0deg and 45deg
    for( int x = 0; x <= 100; ++x ) {
        check_shamble_speed( "mon_zombie", {x, 100, 0} );
    }
}

