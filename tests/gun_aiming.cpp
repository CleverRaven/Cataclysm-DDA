#include "catch/catch.hpp"

#include "game.h"
#include "npc.h"
#include "item_factory.h"

static void test_internal( const npc& who, const item &gun )
{
    THEN( "the effective range is correctly calcuated" ) {
        // calculate range for 50% chance of critical hit at arbitrary recoil
        for ( double recoil = 0; recoil < 1000; recoil += 100 ) {
            double range = who.gun_current_range( gun, recoil, 50, accuracy_critical );
            double max_range = who.gun_current_range( gun, 0, 0, 0 );
            double dispersion = who.get_weapon_dispersion( gun ) + recoil;

            // approx_hit_chance uses an linear interpolation which is not particularly accurate
            // at sub-tile resolution, so just check that the bracketing integer ranges are OK
            double chance0 = who.projectile_attack_chance( dispersion, ( int )range, accuracy_critical );
            double chance1 = who.projectile_attack_chance( dispersion, ( int )( range + 1 ), accuracy_critical );

            INFO( "Recoil: " << recoil );
            INFO( "Range: " << range );
            INFO( "Dispersion: " << dispersion );
            INFO( "Chance (left): " << chance0 );
            INFO( "Chance (right): " << chance1 );

            if ( range == 0 ) {
                REQUIRE( chance1 >= 0.5 );
            } else if ( range == max_range ) {
                REQUIRE( chance0 <= 0.5 );
            } else {
                REQUIRE( chance0 >= 0.5 );
                REQUIRE( chance1 <= 0.5 );
            }
        }
    }

    THEN( "the snapshot range is less than the effective range" ) {
        REQUIRE( who.gun_engagement_range( gun, player::engagement::snapshot ) <=
                 who.gun_engagement_range( gun, player::engagement::effective ) );
    }

    THEN( "the effective range is less than maximum range" ) {
        REQUIRE( who.gun_engagement_range( gun, player::engagement::effective ) <=
                 who.gun_engagement_range( gun, player::engagement::maximum ) );
    }

    WHEN( "the gun it is aimed" ) {
        double penalty = MIN_RECOIL;
        double aimed = penalty - who.aim_per_move( gun, penalty );

        THEN( "recoil is the the same or less" ) {
            REQUIRE( aimed <= penalty );

            AND_THEN( "the effective range is the same or better" ) {
                REQUIRE( who.gun_current_range( gun, penalty ) <=
                         who.gun_current_range( gun, aimed ) );
            }
        }
    }

    WHEN( "a higher accuracy is requested" ) {
        THEN( "the effective range is worse" ) {
            REQUIRE( who.gun_current_range( gun, MIN_RECOIL, 50, accuracy_grazing ) >
                     who.gun_current_range( gun, MIN_RECOIL, 50, accuracy_critical  ) );
        }
    }

    WHEN( "a higher certainty is requested" ) {
        THEN( "the effective range is worse" ) {
            REQUIRE( who.gun_current_range( gun, MIN_RECOIL, 50 ) >
                     who.gun_current_range( gun, MIN_RECOIL, 80 ) );
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

        for( const auto& e : item_controller->get_all_itypes() ) {
            if( e.second.gun ) {
                item gun( e.first );
                if( !gun.magazine_integral() ) {
                    gun.emplace_back( gun.magazine_default() );
                }
                gun.ammo_set( default_ammo( gun.ammo_type() ), gun.ammo_capacity() );

                who.set_skill_level( gun.gun_skill(), weapon_skill );

                INFO( "GUN: " << gun.tname() );
                INFO( "AMMO " << gun.ammo_current() );

                REQUIRE( gun.is_gun() );
                REQUIRE( gun.ammo_sufficient() );

                test_internal( who, gun );

                // @todo acceptance tests here
            }
        }
    }
}
