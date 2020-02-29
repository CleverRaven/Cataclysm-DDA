#include <climits>
#include <list>
#include <memory>

#include "avatar.h"
#include "cata_string_consts.h" // for flag_WET
#include "catch/catch.hpp"
#include "game.h"
#include "monster.h"
#include "morale_types.h" // For MORALE_WET
#include "mtype.h"
#include "player.h"
#include "bodypart.h"
#include "calendar.h"
#include "inventory.h"
#include "item.h"
#include "itype.h" // for efftype_id
#include "string_id.h"
#include "type_id.h"
#include "point.h"

static player &get_sanitized_player( )
{
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );
    dummy.inv.clear();
    dummy.remove_weapon();

    return dummy;
}

TEST_CASE( "eyedrops", "[iuse][eyedrops]" )
{
    player &dummy = get_sanitized_player();

    item &test_item = dummy.i_add( item( "saline", 0, item::default_charges_tag{} ) );

    REQUIRE( test_item.charges == 5 );

    dummy.add_env_effect( efftype_id( "boomered" ), bp_eyes, 3, 12_turns );

    item_location loc = item_location( dummy, &test_item );
    REQUIRE( loc );
    int test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos != INT_MIN );

    dummy.consume( loc );

    test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos != INT_MIN );
    REQUIRE( test_item.charges == 4 );
    REQUIRE( !dummy.has_effect( efftype_id( "boomered" ) ) );

    dummy.consume( loc );
    dummy.consume( loc );
    dummy.consume( loc );
    dummy.consume( loc );

    test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos == INT_MIN );
}

static monster *find_adjacent_monster( const tripoint &pos )
{
    tripoint target = pos;
    for( target.x = pos.x - 1; target.x <= pos.x + 1; target.x++ ) {
        for( target.y = pos.y - 1; target.y <= pos.y + 1; target.y++ ) {
            if( target == pos ) {
                continue;
            }
            if( monster *const candidate = g->critter_at<monster>( target ) ) {
                return candidate;
            }
        }
    }
    return nullptr;
}

TEST_CASE( "manhack", "[iuse][manhack]" )
{
    player &dummy = get_sanitized_player();

    g->clear_zombies();
    item &test_item = dummy.i_add( item( "bot_manhack", 0, item::default_charges_tag{} ) );

    int test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos != INT_MIN );

    monster *new_manhack = find_adjacent_monster( dummy.pos() );
    REQUIRE( new_manhack == nullptr );

    dummy.invoke_item( &test_item );

    test_item_pos = dummy.inv.position_by_item( &test_item );
    REQUIRE( test_item_pos == INT_MIN );

    new_manhack = find_adjacent_monster( dummy.pos() );
    REQUIRE( new_manhack != nullptr );
    REQUIRE( new_manhack->type->id == mtype_id( "mon_manhack" ) );
    g->clear_zombies();
}

TEST_CASE( "antifungal", "[iuse][antifungal]" )
{
    avatar dummy;
    item &antifungal = dummy.i_add( item( "antifungal", 0, item::default_charges_tag{} ) );

    GIVEN( "player has a fungal infection" ) {
        dummy.add_effect( efftype_id( "fungus" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "fungus" ) ) );

        WHEN( "they take an antifungal drug" ) {
            dummy.invoke_item( &antifungal );

            THEN( "it cures the fungal infection" ) {
                CHECK_FALSE( dummy.has_effect( efftype_id( "fungus" ) ) );
            }
        }
    }

    GIVEN( "player has fungal spores" ) {
        dummy.add_effect( efftype_id( "spores" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "spores" ) ) );

        WHEN( "they take an antifungal drug" ) {
            dummy.invoke_item( &antifungal );

            THEN( "it has no effect on the spores" ) {
                CHECK( dummy.has_effect( efftype_id( "spores" ) ) );
            }
        }
    }
}

TEST_CASE( "antiparasitic", "[iuse][antiparasitic]" )
{
    avatar dummy;
    item &antiparasitic = dummy.i_add( item( "antiparasitic", 0, item::default_charges_tag{} ) );

    GIVEN( "player has parasite infections" ) {
        dummy.add_effect( efftype_id( "dermatik" ), 1_hours );
        dummy.add_effect( efftype_id( "tapeworm" ), 1_hours );
        dummy.add_effect( efftype_id( "bloodworms" ), 1_hours );
        dummy.add_effect( efftype_id( "brainworms" ), 1_hours );
        dummy.add_effect( efftype_id( "paincysts" ), 1_hours );

        REQUIRE( dummy.has_effect( efftype_id( "dermatik" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "tapeworm" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "bloodworms" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "brainworms" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "paincysts" ) ) );

        WHEN( "they use an antiparasitic drug" ) {
            dummy.invoke_item( &antiparasitic );

            THEN( "it cures all parasite infections" ) {
                CHECK_FALSE( dummy.has_effect( efftype_id( "dermatik" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "tapeworm" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "bloodworms" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "brainworms" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "paincysts" ) ) );
            }
        }
    }

    GIVEN( "player has a fungal infection" ) {
        dummy.add_effect( efftype_id( "fungus" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "fungus" ) ) );

        WHEN( "they use an antiparasitic drug" ) {
            dummy.invoke_item( &antiparasitic );

            THEN( "it has no effect on the fungal infection" ) {
                CHECK( dummy.has_effect( efftype_id( "fungus" ) ) );
            }
        }
    }
}

TEST_CASE( "anticonvulsant", "[iuse][anticonvulsant]" )
{
    avatar dummy;
    item &anticonvulsant = dummy.i_add( item( "diazepam", 0, item::default_charges_tag{} ) );

    GIVEN( "player has the shakes" ) {
        dummy.add_effect( efftype_id( "shakes" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "shakes" ) ) );

        WHEN( "they use an anticonvulsant drug" ) {
            dummy.invoke_item( &anticonvulsant );

            THEN( "it cures the shakes" ) {
                CHECK_FALSE( dummy.has_effect( efftype_id( "shakes" ) ) );
            }

            AND_THEN( "it has side-effects" ) {
                CHECK( dummy.has_effect( efftype_id( "valium" ) ) );
                CHECK( dummy.has_effect( efftype_id( "high" ) ) );
                CHECK( dummy.has_effect( efftype_id( "took_anticonvulsant_visible" ) ) );
            }
        }
    }
}

// test the `iuse::oxygen_bottle` function
TEST_CASE( "oxygen tank", "[iuse][oxygen_bottle]" )
{
    avatar dummy;
    item &oxygen = dummy.i_add( item( "oxygen_tank", 0, item::default_charges_tag{} ) );

    // Ensure baseline painkiller value to measure painkiller effects
    dummy.set_painkiller( 0 );
    REQUIRE( dummy.get_painkiller() == 0 );

    GIVEN( "player is suffering from smoke inhalation" ) {
        dummy.add_effect( efftype_id( "smoke" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "smoke" ) ) );

        THEN( "oxygen relieves it" ) {
            dummy.invoke_item( &oxygen );
            CHECK_FALSE( dummy.has_effect( efftype_id( "smoke" ) ) );

            AND_THEN( "it acts as a mild painkiller" ) {
                CHECK( dummy.get_painkiller() == 2 );
            }
        }
    }

    GIVEN( "player is suffering from tear gas" ) {
        dummy.add_effect( efftype_id( "teargas" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "teargas" ) ) );

        THEN( "oxygen relieves the effects of tear gas" ) {
            dummy.invoke_item( &oxygen );
            CHECK_FALSE( dummy.has_effect( efftype_id( "teargas" ) ) );

            AND_THEN( "it acts as a mild painkiller" ) {
                CHECK( dummy.get_painkiller() == 2 );
            }
        }
    }

    GIVEN( "player is suffering from asthma" ) {
        dummy.add_effect( efftype_id( "asthma" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "asthma" ) ) );

        THEN( "oxygen relieves the effects of asthma" ) {
            dummy.invoke_item( &oxygen );
            CHECK_FALSE( dummy.has_effect( efftype_id( "asthma" ) ) );

            AND_THEN( "it acts as a mild painkiller" ) {
                CHECK( dummy.get_painkiller() == 2 );
            }
        }
    }

    GIVEN( "player has no ill effects for the oxygen to treat" ) {
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "smoke" ) ) );
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "teargas" ) ) );
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "asthma" ) ) );

        WHEN( "they are not already stimulated" ) {
            dummy.set_stim( 0 );
            REQUIRE( dummy.get_stim() == 0 );

            THEN( "oxygen is stimulating" ) {
                dummy.invoke_item( &oxygen );
                // values should match iuse function `oxygen_bottle`
                CHECK( dummy.get_stim() == 8 );

                AND_THEN( "it acts as a stronger painkiller" ) {
                    CHECK( dummy.get_painkiller() == 4 );
                }
            }
        }

        WHEN( "they are already quite stimulated" ) {
            // "quite stimulated" meaning the max-stimulation cutoff defined in
            // iuse function `oxygen_bottle`, which should match `max_stim` here:
            int max_stim = 16;

            dummy.set_stim( max_stim );
            REQUIRE( dummy.get_stim() == max_stim );

            THEN( "oxygen has no additional stimulation effects" ) {
                dummy.invoke_item( &oxygen );
                CHECK( dummy.get_stim() == max_stim );

                AND_THEN( "it acts as a mild painkiller" ) {
                    CHECK( dummy.get_painkiller() == 2 );
                }
            }
        }
    }
}

// test the `iuse::caff` and `iuse::atomic_caff` functions
TEST_CASE( "caffeine and atomic caffeine", "[iuse][caff][atomic_caff]" )
{
    avatar dummy;

    // Baseline fatigue level before caffeinating
    int fatigue_before = 200;
    dummy.set_fatigue( fatigue_before );

    // No stimulants or radiation
    dummy.set_stim( 0 );
    dummy.set_rad( 0 );
    REQUIRE( dummy.get_stim() == 0 );
    REQUIRE( dummy.get_rad() == 0 );

    SECTION( "caffeine reduces fatigue, but does not give stimulant effect" ) {
        item &coffee = dummy.i_add( item( "coffee", 0, item::default_charges_tag{} ) );
        dummy.invoke_item( &coffee );
        CHECK( dummy.get_fatigue() == fatigue_before - 3 * coffee.get_comestible()->stim );
        CHECK( dummy.get_stim() == 0 );
    }

    SECTION( "atomic caffeine greatly reduces fatigue, but also irradiates you" ) {
        item &atomic_coffee = dummy.i_add( item( "atomic_coffee", 0, item::default_charges_tag{} ) );
        dummy.invoke_item( &atomic_coffee );
        CHECK( dummy.get_fatigue() == fatigue_before - 12 * atomic_coffee.get_comestible()->stim );
        CHECK( dummy.get_stim() == 0 );
        // irradiation is random, up to 8 per dose
        CHECK( dummy.get_rad() > 0 );
    }
}

TEST_CASE( "royal jelly", "[iuse][royal_jelly]" )
{
    avatar dummy;
    item &jelly = dummy.i_add( item( "royal_jelly", 0, item::default_charges_tag{} ) );

    SECTION( "royal jelly gives cure-all effect" ) {
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "cureall" ) ) );

        dummy.invoke_item( &jelly );
        CHECK( dummy.has_effect( efftype_id( "cureall" ) ) );
    }
}

TEST_CASE( "xanax", "[iuse][xanax]" )
{
    avatar dummy;
    item &xanax = dummy.i_add( item( "xanax", 0, item::default_charges_tag{} ) );

    SECTION( "xanax gives xanax and visible xanax effects" ) {
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "took_xanax" ) ) );
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "took_xanax_visible" ) ) );

        dummy.invoke_item( &xanax );
        CHECK( dummy.has_effect( efftype_id( "took_xanax" ) ) );
        CHECK( dummy.has_effect( efftype_id( "took_xanax_visible" ) ) );
    }
}

TEST_CASE( "towel", "[iuse][towel]" )
{
    avatar dummy;

    // Start with a dry towel
    item &towel = dummy.i_add( item( "towel", 0, item::default_charges_tag{} ) );
    REQUIRE_FALSE( towel.has_flag( flag_WET ) );

    GIVEN( "player is slimed, boomered, and glowing" ) {
        dummy.add_effect( efftype_id( "slimed" ), 1_hours );
        dummy.add_effect( efftype_id( "boomered" ), 1_hours );
        dummy.add_effect( efftype_id( "glowing" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "slimed" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "boomered" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "glowing" ) ) );

        WHEN( "they use a towel once" ) {
            dummy.invoke_item( &towel );

            THEN( "it removes all those effects at once" ) {
                CHECK_FALSE( dummy.has_effect( efftype_id( "slimed" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "boomered" ) ) );
                CHECK_FALSE( dummy.has_effect( efftype_id( "glowing" ) ) );
            }
        }
    }

    GIVEN( "player is boomered and wet" ) {
        dummy.add_effect( efftype_id( "boomered" ), 1_hours );
        dummy.add_morale( MORALE_WET, -10, -10, 1_hours, 1_hours );
        REQUIRE( abs( dummy.has_morale( MORALE_WET ) ) );

        WHEN( "they use a towel" ) {
            dummy.invoke_item( &towel );

            THEN( "it removes the boomered effect, but not the wetness" ) {
                CHECK_FALSE( dummy.has_effect( efftype_id( "boomered" ) ) );
                CHECK( abs( dummy.has_morale( MORALE_WET ) ) );
            }
        }
    }

    GIVEN( "towel is already wet" ) {
        item &towel_wet = dummy.i_add( item( "towel_wet", 0, item::default_charges_tag{} ) );
        REQUIRE( towel_wet.has_flag( flag_WET ) );

        AND_GIVEN( "player is wet" ) {
            dummy.add_morale( MORALE_WET, -10, -10, 1_hours, 1_hours );
            REQUIRE( abs( dummy.has_morale( MORALE_WET ) ) );

            WHEN( "they use the wet towel" ) {
                dummy.invoke_item( &towel_wet );

                THEN( "it does not dry them off" ) {
                    CHECK( abs( dummy.has_morale( MORALE_WET ) ) );
                }
            }
        }
    }
}

