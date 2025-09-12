#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "game.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const flag_id json_flag_CASING( "CASING" );
static const flag_id json_flag_SPEEDLOADER( "SPEEDLOADER" );

static const itype_id itype_10mm_fmj( "10mm_fmj" );
static const itype_id itype_223( "223" );
static const itype_id itype_2lcanteen( "2lcanteen" );
static const itype_id itype_38_special( "38_special" );
static const itype_id itype_38_speedloader( "38_speedloader" );
static const itype_id itype_40_casing( "40_casing" );
static const itype_id itype_40_speedloader6( "40_speedloader6" );
static const itype_id itype_40sw( "40sw" );
static const itype_id itype_40x46mm_m118_casing( "40x46mm_m118_casing" );
static const itype_id itype_40x46mm_m433( "40x46mm_m433" );
static const itype_id itype_556( "556" );
static const itype_id itype_9mm( "9mm" );
static const itype_id itype_9mmfmj( "9mmfmj" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_bigback( "bigback" );
static const itype_id itype_bottle_plastic( "bottle_plastic" );
static const itype_id itype_bottle_plastic_small( "bottle_plastic_small" );
static const itype_id itype_bottle_twoliter( "bottle_twoliter" );
static const itype_id itype_bp_40sw( "bp_40sw" );
static const itype_id itype_cranberry_juice( "cranberry_juice" );
static const itype_id itype_debug_modular_m4_carbine( "debug_modular_m4_carbine" );
static const itype_id itype_glock_19( "glock_19" );
static const itype_id itype_glockbigmag( "glockbigmag" );
static const itype_id itype_glockmag( "glockmag" );
static const itype_id itype_jug_plastic( "jug_plastic" );
static const itype_id itype_m203( "m203" );
static const itype_id itype_milk( "milk" );
static const itype_id itype_pebble( "pebble" );
static const itype_id itype_stanag30( "stanag30" );
static const itype_id itype_sw_610( "sw_610" );
static const itype_id itype_sw_619( "sw_619" );
static const itype_id itype_water_clean( "water_clean" );

static void test_reloading( item &target, item &ammo, bool expect_success = true )
{
    item target_copy( target );

    Character &dummy = get_avatar();

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( itype_backpack, calendar::turn_zero ) );

    dummy.i_add( ammo );
    dummy.set_wielded_item( target );

    g->reload_wielded( false );

    INFO( "Tried to reload " << target.tname() << " with " << ammo.tname() );

    if( expect_success ) {
        CHECK( dummy.activity );
    } else {
        CHECK( !dummy.activity );
    }

    process_activity( dummy );

    // Quick and dirty check to see if anything was actually reloaded.
    // Doesn't check if right thing went to right place. But something did go somewhere
    item_contents contents_before = target.get_contents();
    item_contents contents_after = dummy.get_wielded_item()->get_contents();
    bool contents_changed = !contents_before.same_contents( contents_after );
    if( expect_success ) {
        CHECK( contents_changed );
    } else {
        CHECK( !contents_changed );
    }
}

TEST_CASE( "reload_magazines", "[reload]" )
{
    SECTION( "empty magazine" ) {

        item mag( itype_stanag30 );

        SECTION( "with one round" ) {
            item ammo( itype_556 );
            test_reloading( mag, ammo );
        }

        SECTION( "with large stack of rounds" ) {
            item ammo( itype_556, calendar::turn, 500 );
            test_reloading( mag, ammo );
        }

        SECTION( "with wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( mag, ammo, false );
        }
    }

    SECTION( "partially empty magazine" ) {

        item mag( itype_stanag30 );
        mag.put_in( item( itype_556, calendar::turn, 1 ), pocket_type::MAGAZINE );
        REQUIRE( mag.ammo_remaining( ) == 1 );

        SECTION( "with one round" ) {
            item ammo( itype_556 );
            test_reloading( mag, ammo );
        }

        SECTION( "with large stack of rounds" ) {
            item ammo( itype_556, calendar::turn, 500 );
            test_reloading( mag, ammo );
        }

        SECTION( "with one ammo of different type" ) {
            item ammo( itype_223 );
            test_reloading( mag, ammo );
        }

        SECTION( "with wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( mag, ammo, false );
        }
    }

    SECTION( "full magazine" ) {

        item mag( itype_stanag30 );
        mag.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );
        REQUIRE( mag.ammo_remaining( ) == 30 );

        SECTION( "with one round" ) {
            item ammo( itype_556 );
            test_reloading( mag, ammo, false );
        }

        SECTION( "with large stack of rounds" ) {
            item ammo( itype_556, calendar::turn, 500 );
            test_reloading( mag, ammo, false );
        }

        SECTION( "with one ammo of different type" ) {
            item ammo( itype_223 );
            test_reloading( mag, ammo, false );
        }

        SECTION( "with wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( mag, ammo, false );
        }
    }

}

TEST_CASE( "reload_gun_with_casings", "[reload],[gun]" )
{

    SECTION( "empty gun" ) {
        item gun( itype_sw_610 );

        SECTION( "with one round" ) {
            item ammo( itype_40sw );
            test_reloading( gun, ammo );
        }
    }

    SECTION( "gun with casings and ammo" ) {
        item gun( itype_sw_610 );
        gun.put_in( item( itype_40sw, calendar::turn, 3 ), pocket_type::MAGAZINE );
        gun.force_insert_item( item( itype_40_casing, calendar::turn, 3 ).set_flag( json_flag_CASING ),
                               pocket_type::MAGAZINE );

        SECTION( "with one round" ) {
            item ammo( itype_40sw );
            test_reloading( gun, ammo );
        }

        SECTION( "with large stack of rounds" ) {
            item ammo( itype_40sw, calendar::turn, 500 );
            test_reloading( gun, ammo );
        }

        SECTION( "with one ammo of different type" ) {
            item ammo( itype_bp_40sw );
            test_reloading( gun, ammo );
        }

        SECTION( "with one ammo of different ammo type" ) {
            item ammo( itype_10mm_fmj );
            test_reloading( gun, ammo, false );
        }

        SECTION( "with wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( gun, ammo, false );
        }
    }

    SECTION( "full of casings" ) {
        item gun( itype_sw_610 );
        gun.force_insert_item( item( itype_40_casing, calendar::turn, 6 ).set_flag( json_flag_CASING ),
                               pocket_type::MAGAZINE );

        SECTION( "with one round" ) {
            item ammo( itype_40sw );
            test_reloading( gun, ammo );
        }

        SECTION( "with large stack of rounds" ) {
            item ammo( itype_40sw, calendar::turn, 500 );
            test_reloading( gun, ammo );
        }

        SECTION( "with wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( gun, ammo, false );
        }
    }
}

TEST_CASE( "reload_gun_with_magazine", "[reload],[gun]" )
{
    SECTION( "empty gun" ) {
        item gun( itype_glock_19 );

        SECTION( "with empty magazine" ) {
            item mag( itype_glockmag );
            test_reloading( gun, mag );
        }

        SECTION( "with full magazine" ) {
            item mag( itype_glockmag );
            mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag );
        }

        SECTION( "with magazine of wrong ammo" ) {
            item mag( itype_glockmag );
            mag.force_insert_item( item( itype_556, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag, false );
        }
    }

    SECTION( "gun with empty magazine" ) {
        item gun( itype_glock_19 );
        gun.put_in( item( itype_glockmag ), pocket_type::MAGAZINE_WELL );

        SECTION( "with empty magazine" ) {
            item mag( itype_glockmag );
            test_reloading( gun, mag, false );
        }

        SECTION( "with full magazine" ) {
            item mag( itype_glockmag );
            mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag );
        }

        SECTION( "with magazine of wrong ammo" ) {
            item mag( itype_glockmag );
            mag.force_insert_item( item( itype_556, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag, false );
        }
    }

    SECTION( "gun with partially empty magazine" ) {
        item gun( itype_glock_19 );
        item old_mag( itype_glockmag );
        old_mag.put_in( item( itype_9mm, calendar::turn, 2 ), pocket_type::MAGAZINE );
        REQUIRE( old_mag.ammo_remaining( ) == 2 );
        gun.put_in( old_mag, pocket_type::MAGAZINE_WELL );
        REQUIRE( gun.ammo_remaining( ) == 2 );

        SECTION( "with empty magazine" ) {
            item mag( itype_glockmag );
            test_reloading( gun, mag );
        }

        SECTION( "with full magazine" ) {
            item mag( itype_glockmag );
            mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag );
        }

        SECTION( "with full magazine with different ammo" ) {
            item mag( itype_glockmag );
            mag.put_in( item( itype_9mmfmj, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag );
        }

        SECTION( "with magazine of wrong ammo" ) {
            item mag( itype_glockmag );
            mag.force_insert_item( item( itype_556, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag, false );
        }
    }

    SECTION( "gun with full magazine" ) {
        item gun( itype_glock_19 );
        item old_mag( itype_glockmag );
        old_mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
        REQUIRE( old_mag.ammo_remaining( ) == 15 );
        gun.put_in( old_mag, pocket_type::MAGAZINE_WELL );
        REQUIRE( gun.ammo_remaining( ) == 15 );

        SECTION( "with empty magazine" ) {
            item mag( itype_glockmag );
            test_reloading( gun, mag, false );
        }

        SECTION( "with full magazine" ) {
            item mag( itype_glockmag );
            mag.put_in( item( itype_9mm, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag, false );
        }

        SECTION( "with full magazine with different ammo" ) {
            item mag( itype_glockmag );
            mag.put_in( item( itype_9mmfmj, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag, false );
        }

        SECTION( "with magazine of wrong ammo" ) {
            item mag( itype_glockmag );
            mag.force_insert_item( item( itype_556, calendar::turn, 15 ), pocket_type::MAGAZINE );
            REQUIRE( mag.ammo_remaining( ) == 15 );
            test_reloading( gun, mag, false );
        }
    }
}

TEST_CASE( "liquid_reloading", "[reload]" )
{
    SECTION( "empty bottle" ) {
        item container( itype_bottle_plastic );

        SECTION( "with water" ) {
            item liquid_container( itype_bottle_plastic );
            liquid_container.put_in( item( itype_water_clean ), pocket_type::CONTAINER );
            test_reloading( container, liquid_container );
        }

        SECTION( "with lots of water" ) {
            item liquid_container( itype_bottle_twoliter );
            liquid_container.put_in( item( itype_water_clean, calendar::turn, 8 ),
                                     pocket_type::CONTAINER );
            test_reloading( container, liquid_container );
        }

        SECTION( "with frozen water" ) {
            item liquid( itype_water_clean );
            liquid.set_item_temperature( units::from_kelvin( 200 ) );
            REQUIRE( liquid.is_frozen_liquid() );
            test_reloading( container, liquid, false );
        }

        SECTION( "with frozen water from container" ) {
            item liquid( itype_water_clean );
            liquid.set_item_temperature( units::from_kelvin( 200 ) );
            REQUIRE( liquid.is_frozen_liquid() );

            item liquid_container( itype_bottle_plastic );
            liquid_container.put_in( liquid, pocket_type::CONTAINER );

            test_reloading( container, liquid_container, false );
        }

        SECTION( "with non-liquid" ) {
            item liquid( itype_9mm );
            test_reloading( container, liquid, false );
        }

        SECTION( "with non-liquid from container" ) {
            item liquid_container( itype_bottle_plastic );
            liquid_container.put_in( item( itype_9mm ), pocket_type::CONTAINER );
            test_reloading( container, liquid_container, false );
        }
    }

    // Note: The reloading doesn't care which item is what. The liquid can go from either bottle to the other.
    SECTION( "partially empty bottle" ) {

        item container( itype_bottle_twoliter );
        container.put_in( item( itype_water_clean, calendar::turn, 1 ), pocket_type::CONTAINER );
        REQUIRE( !container.is_container_empty() );
        REQUIRE( !container.is_container_full() );

        SECTION( "with one water" ) {
            item liquid_container( itype_bottle_plastic );
            liquid_container.put_in( item( itype_water_clean ), pocket_type::CONTAINER );
            test_reloading( container, liquid_container );
        }

        SECTION( "with lots of water" ) {
            item liquid_container( itype_bottle_twoliter );
            liquid_container.put_in( item( itype_water_clean, calendar::turn, 8 ),
                                     pocket_type::CONTAINER );
            test_reloading( container, liquid_container );
        }

        SECTION( "with frozen water" ) {
            item liquid( itype_water_clean );
            liquid.set_item_temperature( units::from_kelvin( 200 ) );
            REQUIRE( liquid.is_frozen_liquid() );
            test_reloading( container, liquid, false );
        }

        SECTION( "with frozen water from container" ) {
            item liquid( itype_water_clean );
            liquid.set_item_temperature( units::from_kelvin( 200 ) );
            REQUIRE( liquid.is_frozen_liquid() );

            item liquid_container( itype_bottle_plastic );
            liquid_container.put_in( liquid, pocket_type::CONTAINER );

            test_reloading( container, liquid_container, false );
        }

        SECTION( "with liquid of different type" ) {
            item liquid_container( itype_bottle_plastic );
            liquid_container.put_in( item( itype_cranberry_juice ), pocket_type::CONTAINER );
            test_reloading( container, liquid_container, false );
        }

        SECTION( "with non-liquid" ) {
            item liquid( itype_9mm );
            test_reloading( container, liquid, false );
        }
    }

    SECTION( "full bottle" ) {
        item container( itype_bottle_plastic );
        container.put_in( item( itype_water_clean, calendar::turn, 2 ), pocket_type::CONTAINER );
        REQUIRE( container.is_container_full() );

        SECTION( "with one water" ) {
            item liquid_container( itype_bottle_plastic_small );
            liquid_container.put_in( item( itype_water_clean ), pocket_type::CONTAINER );
            test_reloading( container, liquid_container, false );
        }

        SECTION( "with lots of water" ) {
            item liquid_container( itype_bottle_twoliter );
            liquid_container.put_in( item( itype_water_clean, calendar::turn, 8 ),
                                     pocket_type::CONTAINER );
            test_reloading( container, liquid_container, false );
        }

        SECTION( "with frozen water" ) {
            item liquid( itype_water_clean );
            liquid.set_item_temperature( units::from_kelvin( 200 ) );
            REQUIRE( liquid.is_frozen_liquid() );
            test_reloading( container, liquid, false );
        }

        SECTION( "with frozen water from container" ) {
            item liquid( itype_water_clean );
            liquid.set_item_temperature( units::from_kelvin( 200 ) );
            REQUIRE( liquid.is_frozen_liquid() );

            item liquid_container( itype_bottle_plastic );
            liquid_container.put_in( liquid, pocket_type::CONTAINER );

            test_reloading( container, liquid_container, false );
        }

        SECTION( "with liquid of different type" ) {
            item liquid_container( itype_bottle_plastic );
            liquid_container.put_in( item( itype_cranberry_juice ), pocket_type::CONTAINER );
            test_reloading( container, liquid_container, false );
        }

        SECTION( "with non-liquid" ) {
            item liquid( itype_9mm );
            test_reloading( container, liquid, false );
        }
    }
}

TEST_CASE( "speedloader_reloading", "[reload],[gun]" )
{
    SECTION( "empty gun" ) {
        item gun( itype_sw_610 );

        SECTION( "empty speedloader" ) {
            item speedloader( itype_40_speedloader6 );

            test_reloading( gun, speedloader, false );
        }

        SECTION( "partially empty speedloader" ) {
            item speedloader( itype_40_speedloader6 );
            speedloader.put_in( item( itype_40sw, calendar::turn, 3 ), pocket_type::MAGAZINE );

            REQUIRE( !speedloader.empty() );
            REQUIRE( !speedloader.is_magazine_full() );

            test_reloading( gun, speedloader );
        }

        SECTION( "full speedloader" ) {
            item speedloader( itype_40_speedloader6 );
            speedloader.put_in( item( itype_40sw, calendar::turn, 6 ), pocket_type::MAGAZINE );

            REQUIRE( speedloader.is_magazine_full() );

            test_reloading( gun, speedloader );
        }

        SECTION( "speedloader with wrong ammo" ) {
            item speedloader( itype_40_speedloader6 );
            speedloader.force_insert_item( item( itype_9mm, calendar::turn, 6 ),
                                           pocket_type::MAGAZINE );

            REQUIRE( speedloader.is_magazine_full() );

            test_reloading( gun, speedloader, false );
        }
    }

    SECTION( "full gun" ) {
        item gun( itype_sw_610 );
        gun.put_in( item( itype_40sw, calendar::turn, 6 ), pocket_type::MAGAZINE );
        REQUIRE( gun.is_magazine_full() );

        SECTION( "empty speedloader" ) {
            item speedloader( itype_40_speedloader6 );

            test_reloading( gun, speedloader, false );
        }

        SECTION( "partially empty speedloader" ) {
            item speedloader( itype_40_speedloader6 );
            speedloader.put_in( item( itype_40sw, calendar::turn, 3 ), pocket_type::MAGAZINE );

            REQUIRE( !speedloader.empty() );
            REQUIRE( !speedloader.is_magazine_full() );

            test_reloading( gun, speedloader, false );
        }

        SECTION( "full speedloader" ) {
            item speedloader( itype_40_speedloader6 );
            speedloader.put_in( item( itype_40sw, calendar::turn, 6 ), pocket_type::MAGAZINE );

            REQUIRE( speedloader.is_magazine_full() );

            test_reloading( gun, speedloader, false );
        }

        SECTION( "speedloader with wrong ammo" ) {
            item speedloader( itype_40_speedloader6 );
            speedloader.force_insert_item( item( itype_9mm, calendar::turn, 6 ),
                                           pocket_type::MAGAZINE );

            REQUIRE( speedloader.is_magazine_full() );

            test_reloading( gun, speedloader, false );
        }
    }

    SECTION( "gun full of casings" ) {
        item gun( itype_sw_610 );
        gun.force_insert_item( item( itype_40_casing, calendar::turn, 6 ).set_flag( json_flag_CASING ),
                               pocket_type::MAGAZINE );

        SECTION( "empty speedloader" ) {
            item speedloader( itype_40_speedloader6 );

            test_reloading( gun, speedloader, false );
        }

        SECTION( "partially empty speedloader" ) {
            item speedloader( itype_40_speedloader6 );
            speedloader.put_in( item( itype_40sw, calendar::turn, 3 ), pocket_type::MAGAZINE );

            REQUIRE( !speedloader.empty() );
            REQUIRE( !speedloader.is_magazine_full() );

            test_reloading( gun, speedloader );
        }

        SECTION( "full speedloader" ) {
            item speedloader( itype_40_speedloader6 );
            speedloader.put_in( item( itype_40sw, calendar::turn, 6 ), pocket_type::MAGAZINE );

            REQUIRE( speedloader.is_magazine_full() );

            test_reloading( gun, speedloader );
        }

        SECTION( "speedloader with wrong ammo" ) {
            item speedloader( itype_40_speedloader6 );
            speedloader.force_insert_item( item( itype_9mm, calendar::turn, 6 ),
                                           pocket_type::MAGAZINE );

            REQUIRE( speedloader.is_magazine_full() );

            test_reloading( gun, speedloader, false );
        }
    }
}

TEST_CASE( "gunmod_reloading", "[reload],[gun]" )
{
    SECTION( "empty gun and gunmod" ) {
        item gun( itype_debug_modular_m4_carbine );
        item mod( itype_m203 );
        gun.force_insert_item( mod, pocket_type::MOD );

        SECTION( "wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( gun, ammo, false );
        }

        SECTION( "full magazine for the gun" ) {
            item mag( itype_stanag30 );
            mag.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );

            test_reloading( gun, mag );
        }

        SECTION( "ammo for the mod" ) {
            item ammo( itype_40x46mm_m433 );

            test_reloading( gun, ammo );
        }
    }

    SECTION( "partially empty gun and empty gunmod" ) {
        item gun( itype_debug_modular_m4_carbine );
        item mod( itype_m203 );
        item mag1( itype_stanag30 );
        mag1.put_in( item( itype_556, calendar::turn, 10 ), pocket_type::MAGAZINE );

        gun.force_insert_item( mod, pocket_type::MOD );
        gun.put_in( mag1, pocket_type::MAGAZINE_WELL );

        SECTION( "wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( gun, ammo, false );
        }

        SECTION( "full magazine for the gun" ) {
            item mag( itype_stanag30 );
            mag.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );

            test_reloading( gun, mag );
        }

        SECTION( "ammo for the mod" ) {
            item ammo( itype_40x46mm_m433 );

            test_reloading( gun, ammo );
        }

        SECTION( "ammo for the gun" ) {
            item ammo( itype_556 );

            test_reloading( gun, ammo );
        }
    }

    SECTION( "partially empty gun and full gunmod" ) {
        item gun( itype_debug_modular_m4_carbine );
        item mod( itype_m203 );
        item mag1( itype_stanag30 );
        mag1.put_in( item( itype_556, calendar::turn, 10 ), pocket_type::MAGAZINE );
        mod.put_in( item( itype_40x46mm_m433, calendar::turn, 1 ), pocket_type::MAGAZINE );

        gun.force_insert_item( mod, pocket_type::MOD );
        gun.put_in( mag1, pocket_type::MAGAZINE_WELL );

        SECTION( "wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( gun, ammo, false );
        }

        SECTION( "full magazine for the gun" ) {
            item mag( itype_stanag30 );
            mag.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );

            test_reloading( gun, mag );
        }

        SECTION( "ammo for the mod" ) {
            item ammo( itype_40x46mm_m433 );

            test_reloading( gun, ammo, false );
        }

        SECTION( "ammo for the gun" ) {
            item ammo( itype_556 );

            test_reloading( gun, ammo );
        }
    }

    SECTION( "partially empty gun and gunmod with casing" ) {
        item gun( itype_debug_modular_m4_carbine );
        item mod( itype_m203 );
        item mag1( itype_stanag30 );
        mag1.put_in( item( itype_556, calendar::turn, 10 ), pocket_type::MAGAZINE );
        mod.force_insert_item( item( itype_40x46mm_m118_casing ).set_flag( json_flag_CASING ),
                               pocket_type::MAGAZINE );

        gun.force_insert_item( mod, pocket_type::MOD );
        gun.put_in( mag1, pocket_type::MAGAZINE_WELL );

        SECTION( "wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( gun, ammo, false );
        }

        SECTION( "full magazine for the gun" ) {
            item mag( itype_stanag30 );
            mag.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );

            test_reloading( gun, mag );
        }

        SECTION( "ammo for the mod" ) {
            item ammo( itype_40x46mm_m433 );

            test_reloading( gun, ammo );
        }

        SECTION( "ammo for the gun" ) {
            item ammo( itype_556 );

            test_reloading( gun, ammo );
        }
    }

    SECTION( "full gun and empty gunmod" ) {
        item gun( itype_debug_modular_m4_carbine );
        item mod( itype_m203 );
        item mag1( itype_stanag30 );
        mag1.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );

        gun.force_insert_item( mod, pocket_type::MOD );
        gun.put_in( mag1, pocket_type::MAGAZINE_WELL );

        SECTION( "wrong ammo" ) {
            item ammo( itype_9mm );
            test_reloading( gun, ammo, false );
        }

        SECTION( "full magazine for the gun" ) {
            item mag( itype_stanag30 );
            mag.put_in( item( itype_556, calendar::turn, 30 ), pocket_type::MAGAZINE );

            test_reloading( gun, mag, false );
        }

        SECTION( "ammo for the mod" ) {
            item ammo( itype_40x46mm_m433 );

            test_reloading( gun, ammo );
        }

        SECTION( "ammo for the gun" ) {
            item ammo( itype_556 );

            test_reloading( gun, ammo, false );
        }
    }
}

TEST_CASE( "reload_gun_with_integral_magazine", "[reload],[gun]" )
{
    Character &dummy = get_avatar();

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( itype_backpack, calendar::turn_zero ) );

    item_location ammo = dummy.i_add( item( itype_40sw, calendar::turn_zero, item::default_charges_tag{} ) );
    item_location gun = dummy.i_add( item( itype_sw_610, calendar::turn_zero, item::default_charges_tag{} ) );

    REQUIRE( dummy.has_item( *ammo ) );
    REQUIRE( gun->ammo_remaining( ) == 0 );
    REQUIRE( gun->magazine_integral() );

    bool success = gun->reload( dummy, ammo, ammo->charges );

    REQUIRE( success );
    REQUIRE( gun->remaining_ammo_capacity() == 0 );
}

TEST_CASE( "reload_gun_with_integral_magazine_using_speedloader", "[reload],[gun]" )
{
    Character &dummy = get_avatar();

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( itype_backpack, calendar::turn_zero ) );

    item_location speedloader = dummy.i_add( item( itype_38_speedloader, calendar::turn_zero, false ) );
    item_location ammo = dummy.i_add( item( itype_38_special, calendar::turn_zero,
                                            speedloader->remaining_ammo_capacity() ) );
    item_location gun = dummy.i_add( item( itype_sw_619, calendar::turn_zero, false ) );

    REQUIRE( dummy.has_item( *ammo ) );
    REQUIRE( gun->ammo_remaining( ) == 0 );
    REQUIRE( gun->magazine_integral() );
    REQUIRE( dummy.has_item( *speedloader ) );
    REQUIRE( speedloader->ammo_remaining( ) == 0 );
    REQUIRE( speedloader->has_flag( json_flag_SPEEDLOADER ) );

    bool speedloader_success = speedloader->reload( dummy, ammo, ammo->charges );

    REQUIRE( speedloader_success );
    REQUIRE( speedloader->remaining_ammo_capacity() == 0 );

    // This automatically selects the speedloader as ammo
    // as long as dummy has nothing else available.
    // If there are multiple options, it will crash from opening a ui.
    item::reload_option opt = dummy.select_ammo( gun );

    REQUIRE( opt );

    dummy.assign_activity( reload_activity_actor( std::move( opt ) ) );
    if( !!dummy.activity ) {
        process_activity( dummy );
    }

    //REQUIRE( success );
    REQUIRE( gun->remaining_ammo_capacity() == 0 );
    // Speedloader is still in inventory.
    REQUIRE( dummy.has_item( *speedloader ) );
}

TEST_CASE( "reload_gun_with_swappable_magazine", "[reload],[gun]" )
{
    Character &dummy = get_avatar();

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( itype_backpack, calendar::turn_zero ) );

    item_location ammo = dummy.i_add( item( itype_9mm, calendar::turn_zero, item::default_charges_tag{} ) );
    const cata::value_ptr<islot_ammo> &ammo_type = ammo->type->ammo;
    REQUIRE( ammo_type );

    const item mag( itype_glockmag, calendar::turn_zero, 0 );
    const cata::value_ptr<islot_magazine> &magazine_type = mag.type->magazine;
    REQUIRE( magazine_type );
    REQUIRE( magazine_type->type.count( ammo_type->type ) != 0 );

    item gun( itype_glock_19 );
    gun.put_in( mag, pocket_type::MAGAZINE_WELL );
    REQUIRE( gun.magazine_current() != nullptr );
    REQUIRE( gun.magazine_current()->ammo_types().count( ammo_type->type ) != 0 );
    dummy.i_add( gun );

    const std::vector<item *> guns = dummy.items_with( []( const item & it ) {
        return it.typeId() == itype_glock_19;
    } );
    REQUIRE( guns.size() == 1 );
    item &glock = *guns.front();
    REQUIRE( glock.magazine_current() != nullptr );
    // We're expecting the magazine to end up in the inventory.
    item_location glock_loc( dummy, &glock );
    REQUIRE( dummy.unload( glock_loc ) );
    const std::vector<item *> glock_mags = dummy.items_with( []( const item & it ) {
        return it.typeId() == itype_glockmag;
    } );
    REQUIRE( glock_mags.size() == 1 );
    item &magazine = *glock_mags.front();
    REQUIRE( magazine.ammo_remaining( ) == 0 );

    REQUIRE( dummy.has_item( *ammo ) );

    bool magazine_success = magazine.reload( dummy, ammo, ammo->charges );

    REQUIRE( magazine_success );
    REQUIRE( magazine.remaining_ammo_capacity() == 0 );

    REQUIRE( gun.ammo_remaining( ) == 0 );
    REQUIRE( gun.magazine_integral() == false );

    bool gun_success = gun.reload( dummy, item_location( dummy, &magazine ), 1 );

    CHECK( gun_success );
    REQUIRE( gun.remaining_ammo_capacity() == 0 );
}

static void reload_a_revolver( Character &dummy, item &gun, item &ammo )
{
    if( !dummy.is_wielding( gun ) ) {
        if( dummy.has_weapon() ) {
            // to avoid dispose_option in avatar::unwield()
            dummy.i_add( *dummy.get_wielded_item() );
            dummy.remove_weapon();
        }
        bool success = dummy.wield( gun );
        REQUIRE( success );
    }
    while( dummy.get_wielded_item()->remaining_ammo_capacity() > 0 ) {
        g->reload_weapon( false );
        REQUIRE( dummy.activity );
        process_activity( dummy );
        CAPTURE( dummy.get_wielded_item()->typeId() );
        CAPTURE( ammo.typeId() );
        REQUIRE( !dummy.get_wielded_item()->empty() );
        REQUIRE( dummy.get_wielded_item()->ammo_current() == ammo.type->get_id() );
    }
}

TEST_CASE( "automatic_reloading_action", "[reload],[gun]" )
{
    Character &dummy = get_avatar();

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( itype_backpack, calendar::turn_zero ) );

    GIVEN( "an unarmed player" ) {
        REQUIRE( !dummy.is_armed() );
        WHEN( "the player triggers auto reload" ) {
            g->reload_weapon( false );
            THEN( "No activity is generated" ) {
                CHECK( !dummy.activity );
            }
        }
    }

    GIVEN( "a player armed with a revolver and ammo for it" ) {
        item_location ammo = dummy.i_add( item( itype_40sw, calendar::turn_zero, 100 ) );
        REQUIRE( ammo->is_ammo() );

        dummy.set_wielded_item( item( itype_sw_610, calendar::turn_zero, 0 ) );
        REQUIRE( dummy.get_wielded_item()->ammo_remaining( ) == 0 );
        REQUIRE( dummy.get_wielded_item().can_reload_with( ammo, false ) );

        WHEN( "the player triggers auto reload until the revolver is full" ) {
            reload_a_revolver( dummy, *dummy.get_wielded_item(), *ammo );
            REQUIRE( dummy.find_reloadables().empty() );
            WHEN( "the player triggers auto reload again" ) {
                g->reload_weapon( false );
                THEN( "no activity is generated" ) {
                    CHECK( !dummy.activity );
                }
            }
        }
        GIVEN( "the player has another gun with ammo" ) {
            item_location gun2 = dummy.i_add( item( itype_sw_610, calendar::turn_zero, 0 ) );
            REQUIRE( gun2->ammo_remaining( ) == 0 );
            REQUIRE( ammo->charges >= gun2->ammo_capacity( ammo->ammo_type() ) );
            REQUIRE( dummy.find_reloadables().size() == 2 );
            WHEN( "the player triggers auto reload until the first revolver is full" ) {
                reload_a_revolver( dummy, *dummy.get_wielded_item(), *ammo );

                THEN( "the first (wielded) revolver is full" ) {
                    CHECK( dummy.get_wielded_item()->is_container_full() );
                }
                THEN( "one unloaded revolver remains" ) {
                    CHECK( dummy.find_reloadables().size() == 1 );
                }
                THEN( "no activity is generated" ) {
                    CHECK( !dummy.activity );
                }
                WHEN( "the player triggers auto reload until the second revolver is full" ) {
                    reload_a_revolver( dummy, *gun2, *ammo );

                    THEN( "the second revolver is full" ) {
                        CHECK( dummy.get_wielded_item()->is_container_full() );

                    }
                    THEN( "there are no more reloadables" ) {
                        for( const item_location &it : dummy.find_reloadables() ) {
                            CAPTURE( it.where() );
                        }
                        CHECK( dummy.find_reloadables().empty() );
                    }
                    THEN( "no activity is generated" ) {
                        CAPTURE( dummy.activity.id() );
                        CHECK( !dummy.activity );
                    }
                    WHEN( "the player triggers auto reload again with no reloadables" ) {
                        CAPTURE( dummy.get_wielded_item()->ammo_remaining( ) );
                        REQUIRE( dummy.find_reloadables().empty() );

                        g->reload_weapon( false );
                        THEN( "no activity is generated" ) {
                            CAPTURE( dummy.activity.id() );
                            CHECK( !dummy.activity );
                        }
                    }
                }
            }
        }
    }

    GIVEN( "a player wielding an unloaded gun, carrying an unloaded magazine, and carrying ammo for the magazine" ) {
        dummy.clear_worn();
        dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );
        item_location ammo = dummy.i_add( item( itype_9mm, calendar::turn_zero, 50 ) );
        const cata::value_ptr<islot_ammo> &ammo_type = ammo->type->ammo;
        REQUIRE( ammo_type );

        item_location mag = dummy.i_add( item( itype_glockmag, calendar::turn_zero, 0 ) );
        const cata::value_ptr<islot_magazine> &magazine_type = mag->type->magazine;
        REQUIRE( magazine_type );
        REQUIRE( magazine_type->type.count( ammo_type->type ) != 0 );
        REQUIRE( mag->ammo_remaining( ) == 0 );

        dummy.set_wielded_item( item( itype_glock_19, calendar::turn_zero, 0 ) );
        REQUIRE( dummy.get_wielded_item()->ammo_remaining( ) == 0 );

        WHEN( "the player triggers auto reload" ) {
            g->reload_weapon( false );
            REQUIRE( dummy.activity );
            process_activity( dummy );

            THEN( "the associated magazine is reloaded" ) {
                const std::vector<item *> mags = dummy.items_with( []( const item & it ) {
                    return it.typeId() == itype_glockmag;
                } );
                REQUIRE( mags.size() == 1 );
                REQUIRE( !mags.front()->empty() );
                CHECK( mags.front()->first_ammo().type == ammo->type );
            }
            WHEN( "the player triggers auto reload again" ) {
                g->reload_weapon( false );
                REQUIRE( dummy.activity );
                process_activity( dummy );

                THEN( "The magazine is loaded into the gun" ) {
                    CHECK( dummy.get_wielded_item()->ammo_remaining( ) > 0 );
                }
                WHEN( "the player triggers auto reload again" ) {
                    g->reload_weapon( false );
                    THEN( "No activity is generated" ) {
                        CHECK( !dummy.activity );
                    }
                }
            }
        }
        GIVEN( "the player also has an extended magazine" ) {
            item_location mag2 = dummy.i_add( item( itype_glockbigmag, calendar::turn_zero, 0 ) );
            const cata::value_ptr<islot_magazine> &magazine_type2 = mag2->type->magazine;
            REQUIRE( magazine_type2 );
            REQUIRE( magazine_type2->type.count( ammo_type->type ) != 0 );
            REQUIRE( mag2->ammo_remaining( ) == 0 );

            WHEN( "the player triggers auto reload" ) {
                g->reload_weapon( false );
                REQUIRE( dummy.activity );
                process_activity( dummy );

                THEN( "the associated magazine is reloaded" ) {
                    const std::vector<item *> mags = dummy.items_with( []( const item & it ) {
                        return it.typeId() == itype_glockmag;
                    } );
                    REQUIRE( mags.size() == 1 );
                    REQUIRE( !mags.front()->empty() );
                    CHECK( mags.front()->first_ammo().type == ammo->type );
                }
                WHEN( "the player triggers auto reload again" ) {
                    g->reload_weapon( false );
                    REQUIRE( dummy.activity );
                    process_activity( dummy );

                    THEN( "The magazine is loaded into the gun" ) {
                        CHECK( dummy.get_wielded_item()->ammo_remaining( ) > 0 );
                    }
                    WHEN( "the player triggers auto reload again" ) {
                        g->reload_weapon( false );
                        REQUIRE( dummy.activity );
                        process_activity( dummy );

                        THEN( "the second associated magazine is reloaded" ) {
                            const std::vector<item *> mags = dummy.items_with( []( const item & it ) {
                                return it.typeId() == itype_glockbigmag;
                            } );
                            REQUIRE( mags.size() == 1 );
                            REQUIRE( !mags.front()->empty() );
                            CHECK( mags.front()->first_ammo().type == ammo->type );
                        }
                        WHEN( "the player triggers auto reload again" ) {
                            g->reload_weapon( false );
                            THEN( "No activity is generated" ) {
                                CHECK( !dummy.activity );
                            }
                        }
                    }
                }
            }
        }
    }
}

// TODO: nested containers and frozen liquids.
TEST_CASE( "reload_liquid_container", "[reload],[liquid]" )
{
    Character &dummy = get_avatar();
    clear_avatar();
    clear_map();
    item backpack( itype_bigback );
    dummy.wear_item( backpack );
    item canteen( itype_2lcanteen );
    REQUIRE( dummy.wield( canteen ) ) ;

    item_location ammo_jug = dummy.i_add( item( itype_jug_plastic ) );
    ammo_jug->put_in( item( itype_water_clean, calendar::turn_zero, 2 ),
                      pocket_type::CONTAINER );
    units::volume ammo_volume = ammo_jug->total_contained_volume();

    SECTION( "reload liquid into empty container" ) {
        g->reload_wielded();
        REQUIRE( dummy.activity );
        process_activity( dummy );
        CHECK( dummy.get_wielded_item()->total_contained_volume() == ammo_volume );
        CHECK( ammo_jug->total_contained_volume() == units::volume() );
    }

    SECTION( "reload liquid into partially filled container with same type liquid" ) {
        item water_one( itype_water_clean, calendar::turn_zero, 1 );
        units::volume initial_volume = water_one.volume();
        dummy.get_wielded_item()->put_in( water_one, pocket_type::CONTAINER );
        g->reload_wielded();
        REQUIRE( dummy.activity );
        process_activity( dummy );
        CHECK( dummy.get_wielded_item()->total_contained_volume() == ammo_volume + initial_volume );
        CHECK( ammo_jug->total_contained_volume() == units::volume() );
    }

    SECTION( "reload liquid into partially filled container with different type liquid" ) {
        item milk_one( itype_milk, calendar::turn_zero, 1 );
        units::volume initial_volume = milk_one.volume();
        dummy.get_wielded_item()->put_in( milk_one, pocket_type::CONTAINER );
        g->reload_wielded();
        if( !!dummy.activity ) {
            process_activity( dummy );
        }
        CHECK( dummy.get_wielded_item()->total_contained_volume() == initial_volume );
        CHECK( ammo_jug->total_contained_volume() == ammo_volume );
    }

    SECTION( "reload liquid into container containing a non-liquid" ) {
        item pebble( itype_pebble, calendar::turn_zero, 1 );
        units::volume initial_volume = pebble.volume();
        dummy.get_wielded_item()->put_in( pebble, pocket_type::CONTAINER );
        g->reload_wielded();
        if( !!dummy.activity ) {
            process_activity( dummy );
        }
        CHECK( dummy.get_wielded_item()->total_contained_volume() == initial_volume );
        CHECK( ammo_jug->total_contained_volume() == ammo_volume );
    }

    SECTION( "reload liquid container with more liquid than it can hold" ) {
        ammo_jug->fill_with( item( itype_water_clean, calendar::turn_zero, 1 ) );
        ammo_volume = ammo_jug->total_contained_volume();
        g->reload_wielded();
        REQUIRE( dummy.activity );
        process_activity( dummy );
        CHECK( dummy.get_wielded_item()->get_total_capacity() ==
               dummy.get_wielded_item()->total_contained_volume() );
        CHECK( ammo_jug->total_contained_volume() +
               dummy.get_wielded_item()->total_contained_volume() == ammo_volume );
    }

    SECTION( "liquid reload from map" ) {
        const tripoint_bub_ms test_origin( 60, 60, 0 );
        map &here = get_map();
        dummy.setpos( here, test_origin );
        const tripoint_bub_ms near_point = test_origin + tripoint::east;

        SECTION( "liquid in container on floor" ) {
            ammo_jug.remove_item();
            ammo_jug = item_location( map_cursor( near_point ), &here.add_item( near_point,
                                      item( itype_bottle_plastic ) ) );
            ammo_jug->fill_with( item( itype_water_clean ) );
            ammo_volume = ammo_jug->total_contained_volume();
            g->reload_wielded();
            REQUIRE( dummy.activity );
            process_activity( dummy );
            CHECK( dummy.get_wielded_item()->total_contained_volume() == ammo_volume );
            CHECK( ammo_jug->total_contained_volume() == units::volume() );
        }

        SECTION( "liquid spill on floor" ) {
            REQUIRE( ammo_jug->spill_contents( near_point ) );
            g->reload_wielded();
            if( !!dummy.activity ) {
                process_activity( dummy );
            }
            CHECK( ammo_jug->total_contained_volume() == units::volume() );
            CHECK( dummy.get_wielded_item()->total_contained_volume() == units::volume() );
        }
    }
}
