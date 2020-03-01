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

TEST_CASE( "eyedrops1", "[iuse][eyedrops]" )
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

TEST_CASE( "eyedrops2", "[iuse][eyedrops]" )
{
    avatar dummy;

    item &eyedrops = dummy.i_add( item( "saline", 0, item::default_charges_tag{} ) );
    int charges_before = eyedrops.charges;
    REQUIRE( charges_before > 0 );

    GIVEN( "player is boomered" ) {
        dummy.add_effect( efftype_id( "boomered" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "boomered" ) ) );

        WHEN( "they use eye drops" ) {
            dummy.invoke_item( &eyedrops );

            THEN( "one dose is depleted" ) {
                CHECK( eyedrops.charges == charges_before - 1 );

                AND_THEN( "it removes the boomered effect" ) {
                    CHECK_FALSE( dummy.has_effect( efftype_id( "boomered" ) ) );
                }
            }
        }
    }
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
    int charges_before = antifungal.charges;
    REQUIRE( charges_before > 0 );

    GIVEN( "player has a fungal infection" ) {
        dummy.add_effect( efftype_id( "fungus" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "fungus" ) ) );

        WHEN( "they take an antifungal drug" ) {
            dummy.invoke_item( &antifungal );

            THEN( "one dose is depleted" ) {
                CHECK( antifungal.charges == charges_before - 1 );

                AND_THEN( "it cures the fungal infection" ) {
                    CHECK_FALSE( dummy.has_effect( efftype_id( "fungus" ) ) );
                }
            }
        }
    }

    GIVEN( "player has fungal spores" ) {
        dummy.add_effect( efftype_id( "spores" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "spores" ) ) );

        WHEN( "they take an antifungal drug" ) {
            dummy.invoke_item( &antifungal );

            THEN( "one dose is depleted" ) {
                CHECK( antifungal.charges == charges_before - 1 );

                AND_THEN( "it has no effect on the spores" ) {
                    CHECK( dummy.has_effect( efftype_id( "spores" ) ) );
                }
            }
        }
    }
}

TEST_CASE( "antiparasitic", "[iuse][antiparasitic]" )
{
    avatar dummy;
    item &antiparasitic = dummy.i_add( item( "antiparasitic", 0, item::default_charges_tag{} ) );

    int charges_before = antiparasitic.charges;
    REQUIRE( charges_before > 0 );

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

            THEN( "one dose is depleted" ) {
                CHECK( antiparasitic.charges == charges_before - 1 );

                AND_THEN( "it cures all parasite infections" ) {
                    CHECK_FALSE( dummy.has_effect( efftype_id( "dermatik" ) ) );
                    CHECK_FALSE( dummy.has_effect( efftype_id( "tapeworm" ) ) );
                    CHECK_FALSE( dummy.has_effect( efftype_id( "bloodworms" ) ) );
                    CHECK_FALSE( dummy.has_effect( efftype_id( "brainworms" ) ) );
                    CHECK_FALSE( dummy.has_effect( efftype_id( "paincysts" ) ) );
                }
            }
        }
    }

    GIVEN( "player has a fungal infection" ) {
        dummy.add_effect( efftype_id( "fungus" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "fungus" ) ) );

        WHEN( "they use an antiparasitic drug" ) {
            dummy.invoke_item( &antiparasitic );

            THEN( "one dose is depleted" ) {
                CHECK( antiparasitic.charges == charges_before - 1 );

                AND_THEN( "it has no effect on the fungal infection" ) {
                    CHECK( dummy.has_effect( efftype_id( "fungus" ) ) );
                }
            }
        }
    }
}

TEST_CASE( "anticonvulsant", "[iuse][anticonvulsant]" )
{
    avatar dummy;
    item &anticonvulsant = dummy.i_add( item( "diazepam", 0, item::default_charges_tag{} ) );

    int charges_before = anticonvulsant.charges;
    REQUIRE( charges_before > 0 );

    GIVEN( "player has the shakes" ) {
        dummy.add_effect( efftype_id( "shakes" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "shakes" ) ) );

        WHEN( "they use an anticonvulsant drug" ) {
            dummy.invoke_item( &anticonvulsant );

            THEN( "one dose is depleted" ) {
                CHECK( anticonvulsant.charges == charges_before - 1 );

                AND_THEN( "it cures the shakes" ) {
                    CHECK_FALSE( dummy.has_effect( efftype_id( "shakes" ) ) );

                    AND_THEN( "it has side-effects" ) {
                        CHECK( dummy.has_effect( efftype_id( "valium" ) ) );
                        CHECK( dummy.has_effect( efftype_id( "high" ) ) );
                        CHECK( dummy.has_effect( efftype_id( "took_anticonvulsant_visible" ) ) );
                    }
                }
            }
        }
    }
}

// test the `iuse::oxygen_bottle` function
TEST_CASE( "oxygen tank", "[iuse][oxygen_bottle]" )
{
    avatar dummy;
    item &oxygen = dummy.i_add( item( "oxygen_tank", 0, item::default_charges_tag{} ) );

    int charges_before = oxygen.charges;
    REQUIRE( charges_before > 0 );

    // Ensure baseline painkiller value to measure painkiller effects
    dummy.set_painkiller( 0 );
    REQUIRE( dummy.get_painkiller() == 0 );

    GIVEN( "player is suffering from smoke inhalation" ) {
        dummy.add_effect( efftype_id( "smoke" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "smoke" ) ) );

        THEN( "a dose of oxygen relieves the smoke inhalation" ) {
            dummy.invoke_item( &oxygen );
            CHECK( oxygen.charges == charges_before - 1 );
            CHECK_FALSE( dummy.has_effect( efftype_id( "smoke" ) ) );

            AND_THEN( "it acts as a mild painkiller" ) {
                CHECK( dummy.get_painkiller() == 2 );
            }
        }
    }

    GIVEN( "player is suffering from tear gas" ) {
        dummy.add_effect( efftype_id( "teargas" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "teargas" ) ) );

        THEN( "a dose of oxygen relieves the effects of tear gas" ) {
            dummy.invoke_item( &oxygen );
            CHECK( oxygen.charges == charges_before - 1 );
            CHECK_FALSE( dummy.has_effect( efftype_id( "teargas" ) ) );

            AND_THEN( "it acts as a mild painkiller" ) {
                CHECK( dummy.get_painkiller() == 2 );
            }
        }
    }

    GIVEN( "player is suffering from asthma" ) {
        dummy.add_effect( efftype_id( "asthma" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "asthma" ) ) );

        THEN( "a dose of oxygen relieves the effects of asthma" ) {
            dummy.invoke_item( &oxygen );
            CHECK( oxygen.charges == charges_before - 1 );
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

            THEN( "a dose of oxygen is stimulating" ) {
                dummy.invoke_item( &oxygen );
                CHECK( oxygen.charges == charges_before - 1 );
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

            THEN( "a dose of oxygen has no additional stimulation effects" ) {
                dummy.invoke_item( &oxygen );
                CHECK( oxygen.charges == charges_before - 1 );
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

    SECTION( "atomic caffeine greatly reduces fatigue, but may also irradiate you" ) {
        item &atomic_coffee = dummy.i_add( item( "atomic_coffee", 0, item::default_charges_tag{} ) );
        dummy.invoke_item( &atomic_coffee );
        CHECK( dummy.get_fatigue() == fatigue_before - 12 * atomic_coffee.get_comestible()->stim );
        CHECK( dummy.get_stim() == 0 );
        // irradiation is random, between 0 and 8
        CHECK( dummy.get_rad() >= 0 );
        CHECK( dummy.get_rad() <= 8 );
    }
}

TEST_CASE( "towel", "[iuse][towel]" )
{
    avatar dummy;

    // Dry and wet towels
    item &towel_dry = dummy.i_add( item( "towel", 0, item::default_charges_tag{} ) );
    REQUIRE_FALSE( towel_dry.has_flag( flag_WET ) );
    item &towel_wet = dummy.i_add( item( "towel_wet", 0, item::default_charges_tag{} ) );
    REQUIRE( towel_wet.has_flag( flag_WET ) );

    GIVEN( "player is wet" ) {
        dummy.drench( 100, { bp_torso }, false );
        REQUIRE( dummy.body_wetness[bp_torso] > 0 );

        WHEN( "they use a wet towel" ) {
            dummy.invoke_item( &towel_wet );

            THEN( "it does not dry them off" ) {
                CHECK( dummy.body_wetness[bp_torso] > 0 );
            }
        }

        /* FIXME
         * This fails, not sure why...
         *
        WHEN( "they use a dry towel" ) {
            dummy.invoke_item( &towel_dry );

            THEN( "it dries them off" ) {
                CHECK( dummy.body_wetness[bp_torso] == 0 );
            }
        }
        */
    }

    GIVEN( "player has poor morale due to being wet" ) {
        dummy.add_morale( MORALE_WET, -10, -10, 1_hours, 1_hours );
        REQUIRE( dummy.has_morale( MORALE_WET ) == -10 );

        WHEN( "they use a wet towel" ) {
            dummy.invoke_item( &towel_wet );

            THEN( "it does not improve their morale" ) {
                CHECK( dummy.has_morale( MORALE_WET ) == -10 );
            }
        }

        WHEN( "they use a dry towel" ) {
            dummy.invoke_item( &towel_dry );

            THEN( "it improves their morale" ) {
                CHECK( dummy.has_morale( MORALE_WET ) == 0 );
            }
        }
    }

    GIVEN( "player is slimed, boomered, and glowing" ) {
        dummy.add_effect( efftype_id( "slimed" ), 1_hours );
        dummy.add_effect( efftype_id( "boomered" ), 1_hours );
        dummy.add_effect( efftype_id( "glowing" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "slimed" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "boomered" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "glowing" ) ) );

        WHEN( "they use a dry towel once" ) {
            dummy.invoke_item( &towel_dry );

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

        WHEN( "they use a dry towel" ) {
            dummy.invoke_item( &towel_dry );

            THEN( "it removes the boomered effect, but not the wetness" ) {
                CHECK_FALSE( dummy.has_effect( efftype_id( "boomered" ) ) );
                CHECK( abs( dummy.has_morale( MORALE_WET ) ) );
            }
        }
    }
}

TEST_CASE( "thorazine", "[iuse][thorazine]" )
{
    avatar dummy;
    dummy.set_fatigue( 0 );
    item &thorazine = dummy.i_add( item( "thorazine", 0, item::default_charges_tag{} ) );
    int charges_before = thorazine.charges;
    REQUIRE( charges_before >= 2 );

    GIVEN( "player has hallucination, visuals, and high effects" ) {
        dummy.add_effect( efftype_id( "hallu" ), 1_hours );
        dummy.add_effect( efftype_id( "visuals" ), 1_hours );
        dummy.add_effect( efftype_id( "high" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "hallu" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "visuals" ) ) );
        REQUIRE( dummy.has_effect( efftype_id( "high" ) ) );

        WHEN( "they take some thorazine" ) {
            dummy.invoke_item( &thorazine );

            THEN( "it relieves all those effects with a single dose" ) {
                CHECK( thorazine.charges == charges_before - 1 );
                REQUIRE_FALSE( dummy.has_effect( efftype_id( "hallu" ) ) );
                REQUIRE_FALSE( dummy.has_effect( efftype_id( "visuals" ) ) );
                REQUIRE_FALSE( dummy.has_effect( efftype_id( "high" ) ) );

                AND_THEN( "it causes some fatigue" ) {
                    CHECK( dummy.get_fatigue() >= 5 );
                }
            }
        }
    }

    GIVEN( "player has already taken some thorazine" ) {
        dummy.invoke_item( &thorazine );
        REQUIRE( thorazine.charges == charges_before - 1 );
        REQUIRE( dummy.has_effect( efftype_id( "took_thorazine" ) ) );

        WHEN( "they take more thorazine" ) {
            dummy.invoke_item( &thorazine );

            THEN( "it only causes more fatigue" ) {
                CHECK( thorazine.charges == charges_before - 2 );
                CHECK( dummy.get_fatigue() >= 20 );
            }
        }
    }
}

TEST_CASE( "prozac", "[iuse][prozac]" )
{
    avatar dummy;
    item &prozac = dummy.i_add( item( "prozac", 0, item::default_charges_tag{} ) );

    SECTION( "prozac gives prozac and visible prozac effect" ) {
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "took_prozac" ) ) );
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "took_prozac_visible" ) ) );

        dummy.invoke_item( &prozac );
        CHECK( dummy.has_effect( efftype_id( "took_prozac" ) ) );
        CHECK( dummy.has_effect( efftype_id( "took_prozac_visible" ) ) );
    }

    SECTION( "taking prozac twice gives a stimulant effect" ) {
        dummy.set_stim( 0 );

        dummy.invoke_item( &prozac );
        CHECK( dummy.get_stim() == 0 );
        dummy.invoke_item( &prozac );
        CHECK( dummy.get_stim() > 0 );
    }
}

TEST_CASE( "inhaler", "[iuse][inhaler]" )
{
    avatar dummy;
    item &inhaler = dummy.i_add( item( "inhaler", 0, item::default_charges_tag{} ) );

    GIVEN( "player is suffering from smoke inhalation" ) {
        dummy.add_effect( efftype_id( "smoke" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "smoke" ) ) );

        THEN( "inhaler relieves it" ) {
            dummy.invoke_item( &inhaler );
            CHECK_FALSE( dummy.has_effect( efftype_id( "smoke" ) ) );
        }
    }

    GIVEN( "player is suffering from asthma" ) {
        dummy.add_effect( efftype_id( "asthma" ), 1_hours );
        REQUIRE( dummy.has_effect( efftype_id( "asthma" ) ) );

        THEN( "inhaler relieves the effects of asthma" ) {
            dummy.invoke_item( &inhaler );
            CHECK_FALSE( dummy.has_effect( efftype_id( "asthma" ) ) );
        }
    }

    GIVEN( "player is not suffering from asthma" ) {
        REQUIRE_FALSE( dummy.has_effect( efftype_id( "asthma" ) ) );

        THEN( "inhaler reduces fatigue" ) {
            dummy.set_fatigue( 10 );
            dummy.invoke_item( &inhaler );
            CHECK( dummy.get_fatigue() < 10 );
        }
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

