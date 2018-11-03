#include "catch/catch.hpp"

#include "monster.h"
#include "mtype.h"

#include "creature_tracker.h"

TEST_CASE( "monster_enters_square_with_recently_dead_monster_then_other_monster_examines_dead_one",
           "[creature_tracker]" )
{
    Creature_tracker test_tracker;
    mtype_id zombie_type( "mon_zombie" );
    {
        monster dearly_departed( zombie_type, { 30, 30, 0 } );
        monster hapless_wanderer( zombie_type, { 30, 31, 0 } );
        monster schemer( mtype_id( "mon_chickenbot" ), { 40, 40, 0 } );

        test_tracker.add( dearly_departed );
        test_tracker.add( hapless_wanderer );
        test_tracker.add( schemer );
        CHECK( test_tracker.size() == 3 );
    }

    mfactions monster_factions;

    std::shared_ptr<monster> schemer_ptr = test_tracker.find( { 40, 40, 0 } );
    {
        std::shared_ptr<monster> departed_ptr = test_tracker.find( { 30, 30, 0 } );
        std::shared_ptr<monster> wanderer_ptr = test_tracker.find( { 30, 31, 0 } );
        monster_factions[ departed_ptr->faction ].insert( departed_ptr.get() );
        monster_factions[ departed_ptr->faction ].insert( wanderer_ptr.get() );

        departed_ptr->set_hp( 0 );
        CHECK( departed_ptr->is_dead() );
        CHECK( test_tracker.size() == 3 );

        test_tracker.update_pos( *wanderer_ptr, { 30, 30, 0 } );
        CHECK( test_tracker.find( { 30, 30, 0 } ) == wanderer_ptr );
        CHECK( test_tracker.size() == 2 );
    }

    CHECK( schemer_ptr->friendly == 0 );
    schemer_ptr->plan( monster_factions );
}
