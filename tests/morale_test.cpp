#include "catch/catch.hpp"

#include "morale.h"
#include "morale_types.h"
#include "effect.h"
#include "game.h"
#include "itype.h"
#include "item.h"
#include "bodypart.h"

#include <string>

TEST_CASE( "player_morale" )
{
    static const efftype_id effect_took_prozac( "took_prozac" );

    player_morale m;

    GIVEN( "an empty morale" ) {
        CHECK( m.get_level() == 0 );
    }

    GIVEN( "temporary morale (food)" ) {
        m.add( MORALE_FOOD_GOOD, 20, 40, 20, 10 );
        m.add( MORALE_FOOD_BAD, -10, -20, 20, 10 );

        CHECK( m.has( MORALE_FOOD_GOOD ) == 20 );
        CHECK( m.has( MORALE_FOOD_BAD ) == -10 );
        CHECK( m.get_level() == 10 );

        WHEN( "it decays" ) {
            AND_WHEN( "it's just started" ) {
                m.decay( 10 );
                CHECK( m.has( MORALE_FOOD_GOOD ) == 20 );
                CHECK( m.has( MORALE_FOOD_BAD ) == -10 );
                CHECK( m.get_level() == 10 );
            }
            AND_WHEN( "it's halfway there" ) {
                m.decay( 15 );
                CHECK( m.has( MORALE_FOOD_GOOD ) == 10 );
                CHECK( m.has( MORALE_FOOD_BAD ) == -5 );
                CHECK( m.get_level() == 5 );
            }
            AND_WHEN( "it's finished" ) {
                m.decay( 20 );
                CHECK( m.has( MORALE_FOOD_GOOD ) == 0 );
                CHECK( m.has( MORALE_FOOD_BAD ) == 0 );
                CHECK( m.get_level() == 0 );
            }
        }

        WHEN( "it gets deleted" ) {
            AND_WHEN( "good one gets deleted" ) {
                m.remove( MORALE_FOOD_GOOD );
                CHECK( m.get_level() == -10 );
            }
            AND_WHEN( "bad one gets deleted" ) {
                m.remove( MORALE_FOOD_BAD );
                CHECK( m.get_level() == 20 );
            }
            AND_WHEN( "both get deleted" ) {
                m.remove( MORALE_FOOD_BAD );
                m.remove( MORALE_FOOD_GOOD );
                CHECK( m.get_level() == 0 );
            }
        }

        WHEN( "it gets cleared" ) {
            m.clear();
            CHECK( m.get_level() == 0 );
        }

        WHEN( "it's added/subtracted (no cap)" ) {
            m.add( MORALE_FOOD_GOOD, 10, 40, 20, 10, false );
            m.add( MORALE_FOOD_BAD, -10, -20, 20, 10, false );

            CHECK( m.has( MORALE_FOOD_GOOD ) == 30 );
            CHECK( m.has( MORALE_FOOD_BAD ) == -20 );
            CHECK( m.get_level() == 10 );

        }

        WHEN( "it's added/subtracted (with a cap)" ) {
            m.add( MORALE_FOOD_GOOD, 5, 10, 20, 10, true );
            m.add( MORALE_FOOD_BAD, -5, -10, 20, 10, true );

            CHECK( m.has( MORALE_FOOD_GOOD ) == 10 );
            CHECK( m.has( MORALE_FOOD_BAD ) == -10 );
            CHECK( m.get_level() == 0 );
        }
    }

    GIVEN( "persistent morale" ) {
        m.add_permanent( MORALE_PERM_MASOCHIST, 5 );

        CHECK( m.has( MORALE_PERM_MASOCHIST ) == 5 );

        WHEN( "it decays" ) {
            m.decay( 100 );
            THEN( "nothing happens" ) {
                CHECK( m.has( MORALE_PERM_MASOCHIST ) == 5 );
                CHECK( m.get_level() == 5 );
            }
        }
    }

    GIVEN( "OPTIMISTIC trait" ) {
        m.on_mutation_gain( "OPTIMISTIC" );
        CHECK( m.has( MORALE_PERM_OPTIMIST ) == 4 );
        CHECK( m.get_level() == 5 );

        WHEN( "lost the trait" ) {
            m.on_mutation_loss( "OPTIMISTIC" );
            CHECK( m.has( MORALE_PERM_OPTIMIST ) == 0 );
            CHECK( m.get_level() == 0 );
        }
    }

    GIVEN( "BADTEMPER trait" ) {
        m.on_mutation_gain( "BADTEMPER" );
        CHECK( m.has( MORALE_PERM_BADTEMPER ) == -4 );
        CHECK( m.get_level() == -5 );

        WHEN( "lost the trait" ) {
            m.on_mutation_loss( "BADTEMPER" );
            CHECK( m.has( MORALE_PERM_BADTEMPER ) == 0 );
            CHECK( m.get_level() == 0 );
        }
    }

    GIVEN( "killed an innocent" ) {
        m.add( MORALE_KILLED_INNOCENT, -100 );

        WHEN( "took prozac" ) {
            m.on_effect_int_change( effect_took_prozac, 1 );

            THEN( "it's not so bad" ) {
                CHECK( m.get_level() == -25 );

                AND_WHEN( "the effect ends" ) {
                    m.on_effect_int_change( effect_took_prozac, 0 );

                    THEN( "guilt returns" ) {
                        CHECK( m.get_level() == -100 );
                    }
                }
            }
        }
    }

    GIVEN( "a set of super fancy bride's clothes" ) {
        item dress_wedding( "dress_wedding", 0 ); // legs, torso | 8 + 2 | 10
        item veil_wedding( "veil_wedding", 0 );   // eyes, mouth | 4 + 2 | 6
        item heels( "heels", 0 );                 // feet        | 1 + 2 | 3

        m.on_item_wear( dress_wedding );
        m.on_item_wear( veil_wedding );
        m.on_item_wear( heels );

        WHEN( "not a stylish person" ) {
            THEN( "just don't care (even if man)" ) {
                CHECK( m.get_level() == 0 );
            }
        }

        WHEN( "a stylish person" ) {
            m.on_mutation_gain( "STYLISH" );

            CHECK( m.get_level() == 19 );

            AND_WHEN( "gets naked" ) {
                m.on_item_takeoff( heels ); // the queen took off her sandal ...
                CHECK( m.get_level() == 16 );
                m.on_item_takeoff( veil_wedding );
                CHECK( m.get_level() == 10 );
                m.on_item_takeoff( dress_wedding );
                CHECK( m.get_level() == 0 );
            }
            AND_WHEN( "tries to be even fancier" ) {
                item watch( "sf_watch", 0 );
                m.on_item_wear( watch );
                THEN( "there's a limit" ) {
                    CHECK( m.get_level() == 20 );
                }
            }
            AND_WHEN( "not anymore" ) {
                m.on_mutation_loss( "STYLISH" );
                CHECK( m.get_level() == 0 );
            }
        }
    }
}
