#include "catch/catch.hpp"

#include "creature.h"
#include "creature_tracker.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "mtype.h"

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
// if their speed is higher than @effective_speed.
void check_shamble_speed( const std::string monster_type, float effective_speed )
{
    // Wandering makes things nondeterministic, so look at the distribution rather than a target number.
    stat horizontal_move;
    stat vertical_move;
    stat diagonal_move;
    for( int i = 0; i < 10; ++i ) {
        horizontal_move.add( turns_to_destination( monster_type, {0, 0, 0}, {100,0,0} ) );
        vertical_move.add( turns_to_destination( monster_type, {0, 0, 0}, {0,100,0} ) );
        diagonal_move.add( turns_to_destination( monster_type, {0, 0, 0}, {100,100,0} ) );
    }
    float speed_factor = 100.0 / (float)monster( mtype_id( monster_type ) ).get_speed();
    float expected_move_points = 10000.0 / effective_speed;
    CAPTURE( horizontal_move.avg() );
    CHECK( horizontal_move.min() >= 100 * speed_factor );
    CHECK( horizontal_move.avg() <= expected_move_points + 4 );
    CHECK( horizontal_move.avg() >= expected_move_points - 4 );
    CHECK( horizontal_move.max() <= 200 * speed_factor );
    CHECK( vertical_move.min() >= 100 * speed_factor );
    CHECK( vertical_move.avg() <= expected_move_points + 4 );
    CHECK( vertical_move.avg() >= expected_move_points - 4 );
    CHECK( vertical_move.max() <= 200 * speed_factor );
    CHECK( diagonal_move.min() >= 141 * speed_factor );
    CHECK( diagonal_move.avg() <= (expected_move_points * 1.63) + 4 );
    CHECK( diagonal_move.avg() >= (expected_move_points * 1.63) - 4 );
    CHECK( diagonal_move.max() <= 350 * speed_factor );
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
    // Have a monster walk some distance in a direction and measure how long it takes.
    int vert_move = turns_to_destination( "mon_pig", {0,0,0}, {100,0,0} );
    CHECK( vert_move <= 100 + 2 );
    CHECK( vert_move >= 100 - 2 );
    int horiz_move = turns_to_destination( "mon_pig", {0,0,0}, {0,100,0} );
    CHECK( horiz_move <= 100 + 2 );
    CHECK( horiz_move >= 100 - 2 );
    int diag_move = turns_to_destination( "mon_pig", {0,0,0}, {100,100,0} );
    CHECK( diag_move <= 141 + 3 );
    CHECK( diag_move >= 141 - 3 );

    check_shamble_speed( "mon_zombie", 80 );
    check_shamble_speed( "mon_zombear", 110 );
    check_shamble_speed( "mon_zombie_dog", 150 );
    check_shamble_speed( "mon_jabberwock", 180 );
}

