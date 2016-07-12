#include "catch/catch.hpp"

#include "game.h"
#include "npc.h"
#include "item_factory.h"

static void test_internal( const npc& who, const item &gun )
{
    WHEN( "the gun it is aimed" ) {
        THEN( "the effective range is the same or better" ) {
            REQUIRE( who.gun_effective_range( gun, 0 ) <=
                     who.gun_effective_range( gun, 1 ) );
        }
    }

    WHEN( "a higher accuracy is requested" ) {
        THEN( "the effective range is worse" ) {
            REQUIRE( who.gun_effective_range( gun, 0, 50, accuracy_grazing ) >
                     who.gun_effective_range( gun, 0, 50, accuracy_critical  ) );
        }
    }

    WHEN( "a higher certainty is requested" ) {
        THEN( "the effective range is worse" ) {
            REQUIRE( who.gun_effective_range( gun, 0, 50 ) >
                     who.gun_effective_range( gun, 0, 80 ) );
        }
    }
}

TEST_CASE( "gun_aiming", "[gun] [aim]" ) {
    const int gun_skill    = 4; // marksmanship
    const int weapon_skill = 4; // relevant weapon (eg. rifle)

    // Note that GIVEN statements cannot appear within loops
    GIVEN( "A typical survivor with a loded gun" ) {

        auto who = npc::standard();
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

                who.recoil = MIN_RECOIL;
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
