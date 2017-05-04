#include "catch/catch.hpp"

#include "game.h"
#include "npc.h"
#include "item_factory.h"

static void test_internal( const npc& who, const std::vector<item> &guns )
{
    WHEN( "the gun it is aimed" ) {
        double penalty = MIN_RECOIL;

        for( const auto &gun : guns ) {
            double aimed = penalty - who.aim_per_move( gun, penalty );

            CAPTURE( gun.tname() );
            CHECK( aimed <= penalty );
            CHECK( who.gun_current_range( gun, penalty ) <=
                   who.gun_current_range( gun, aimed ) );
        }
    }

    WHEN( "a higher accuracy is requested" ) {
        THEN( "the effective range is worse" ) {
            for( const auto &gun : guns ) {
                CAPTURE( gun.tname() );
                if( who.gun_current_range( gun, MIN_RECOIL, 50, accuracy_critical ) < gun.gun_range( &who ) ) {
                    CHECK( who.gun_current_range( gun, MIN_RECOIL, 50, accuracy_grazing ) >
                           who.gun_current_range( gun, MIN_RECOIL, 50, accuracy_critical ) );
                } else {
                    CHECK( who.gun_current_range( gun, MIN_RECOIL, 50, accuracy_grazing ) >=
                           who.gun_current_range( gun, MIN_RECOIL, 50, accuracy_critical ) );
                }
            }
        }
    }

    WHEN( "a higher certainty is requested" ) {
        THEN( "the effective range is worse" ) {
            for( const auto &gun : guns ) {
                CAPTURE( gun.tname() );
                if( who.gun_current_range( gun, MIN_RECOIL, 80 ) < gun.gun_range( &who ) ) {
                    CHECK( who.gun_current_range( gun, MIN_RECOIL, 50 ) >
                           who.gun_current_range( gun, MIN_RECOIL, 80 ) );
                } else {
                    CHECK( who.gun_current_range( gun, MIN_RECOIL, 50 ) >=
                           who.gun_current_range( gun, MIN_RECOIL, 80 ) );
                }
            }
        }
    }
}

TEST_CASE( "gun_aiming", "[gun] [aim]" ) {
    const int gun_skill    = 4; // marksmanship
    const int weapon_skill = 4; // relevant weapon (eg. rifle)

    // Note that GIVEN statements cannot appear within loops
    GIVEN( "A typical survivor with a loaded gun" ) {

        standard_npc who;
        who.setpos( tripoint( 0, 0, 0 ) );
        who.wear_item( item( "gloves_lsurvivor" ) );
        who.wear_item( item( "mask_lsurvivor" ) );
        who.set_skill_level( skill_id( "gun" ), gun_skill );

        WHEN( "the gun ranges are examined" ) {
            std::vector<item> guns;
            for( const itype *e : item_controller->all() ) {
                if( e->gun ) {
                    guns.emplace_back( e );
                    auto &gun = guns.back();
                    if( !gun.magazine_integral() ) {
                        gun.emplace_back( gun.magazine_default() );
                    }
                    gun.ammo_set( default_ammo( gun.ammo_type() ), gun.ammo_capacity() );

                    who.set_skill_level( gun.gun_skill(), weapon_skill );

                    CAPTURE( gun.tname() );
                    CAPTURE( gun.ammo_current() );
                    REQUIRE( gun.is_gun() );
                    REQUIRE( gun.ammo_sufficient() );
                }
            }

            test_internal( who, guns );
        }

        // @todo acceptance tests here
    }
}
