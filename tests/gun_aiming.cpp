#include "catch/catch.hpp"

#include "game.h"
#include "npc.h"
#include "item_factory.h"

static void test_internal( const npc& who, const item &gun )
{
    THEN( "the effective range is correctly calcuated" ) {
        // calculate range for 50% chance of critical hit at arbitrary recoil
        double recoil = rng_float( 0, 1000 );
        double range = who.gun_current_range( gun, recoil, 50, accuracy_critical );

        // calculate actual accuracy at the given range
        double dispersion = ( who.get_weapon_dispersion( gun ) + recoil ) / 2;
        double missed_by = iso_tangent( range, dispersion );

        INFO( "Recoil: " << recoil );
        INFO( "Range: " << range );
        INFO( "Dispersion: " << dispersion );

        // require inverse calculation to agree with tolerance of 0.1%
        REQUIRE( std::abs( missed_by - accuracy_critical ) < accuracy_critical / 1000 );
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
            if( e.second->gun ) {
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
