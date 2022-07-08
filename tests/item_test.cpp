#include "cata_catch.h"
#include "item.h"

#include <cmath>
#include <initializer_list>
#include <limits>
#include <memory>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "enums.h"
#include "flag.h"
#include "game.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "itype.h"
#include "math_defines.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"


static const flag_id json_flag_COLD( "COLD" );
static const flag_id json_flag_FILTHY( "FILTHY" );
static const flag_id json_flag_HOT( "HOT" );

static const itype_id itype_test_backpack( "test_backpack" );
static const itype_id itype_test_duffelbag( "test_duffelbag" );
static const itype_id itype_test_mp3( "test_mp3" );
static const itype_id itype_test_smart_phone( "test_smart_phone" );
static const itype_id itype_test_waterproof_bag( "test_waterproof_bag" );

TEST_CASE( "item_volume", "[item]" )
{
    // Need to pick some item here which is count_by_charges and for which each
    // charge is at least 1_ml.  Battery works for now.
    item i( "battery", calendar::turn_zero, item::default_charges_tag() );
    REQUIRE( i.count_by_charges() );
    // Would be better with Catch2 generators
    const units::volume big_volume = units::from_milliliter( std::numeric_limits<int>::max() / 2 );
    for( units::volume v : {
             0_ml, 1_ml, i.volume(), big_volume
         } ) {
        INFO( "checking batteries that fit in " << v );
        const int charges_that_should_fit = i.charges_per_volume( v );
        i.charges = charges_that_should_fit;
        CHECK( i.volume() <= v ); // this many charges should fit
        i.charges++;
        CHECK( i.volume() > v ); // one more charge should not fit
    }
}

TEST_CASE( "simple_item_layers", "[item]" )
{
    CHECK( item( "arm_warmers" ).get_layer().front() == layer_level::SKINTIGHT );
    CHECK( item( "10gal_hat" ).get_layer().front() == layer_level::NORMAL );
    // intentionally no waist layer check since it is obsoleted
    CHECK( item( "armor_lightplate" ).get_layer().front() == layer_level::OUTER );
    CHECK( item( "legrig" ).get_layer().front() == layer_level::BELTED );
}

TEST_CASE( "gun_layer", "[item]" )
{
    item gun( "win70" );
    item mod( "shoulder_strap" );
    CHECK( gun.is_gunmod_compatible( mod ).success() );
    gun.put_in( mod, item_pocket::pocket_type::MOD );
    CHECK( gun.get_layer().front() == layer_level::BELTED );
}

TEST_CASE( "stacking_cash_cards", "[item]" )
{
    // Differently-charged cash cards should stack if neither is zero.
    item cash0( "cash_card", calendar::turn_zero );
    item cash1( "cash_card", calendar::turn_zero );
    item cash2( "cash_card", calendar::turn_zero );
    cash1.put_in( item( "money", calendar::turn_zero, 1 ), item_pocket::pocket_type::MAGAZINE );
    cash2.put_in( item( "money", calendar::turn_zero, 2 ), item_pocket::pocket_type::MAGAZINE );
    CHECK( !cash0.stacks_with( cash1 ) );
    //CHECK( cash1.stacks_with( cash2 ) ); Enable this once cash card stacking is brought back.
}

// second minute hour day week season year

TEST_CASE( "stacking_over_time", "[item]" )
{
    item A( "neccowafers" );
    item B( "neccowafers" );

    GIVEN( "Two items with the same birthday" ) {
        REQUIRE( A.stacks_with( B ) );
        WHEN( "the items are aged different numbers of seconds" ) {
            A.mod_rot( A.type->comestible->spoils - 1_turns );
            B.mod_rot( B.type->comestible->spoils - 3_turns );
            THEN( "they don't stack" ) {
                CHECK( !A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged the same to the minute but different numbers of seconds" ) {
            A.mod_rot( A.type->comestible->spoils - 5_minutes );
            B.mod_rot( B.type->comestible->spoils - 5_minutes );
            B.mod_rot( -5_turns );
            THEN( "they stack" ) {
                CHECK( A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged a few seconds different but different minutes" ) {
            A.mod_rot( A.type->comestible->spoils - 5_minutes );
            B.mod_rot( B.type->comestible->spoils - 5_minutes );
            B.mod_rot( 5_turns );
            THEN( "they don't stack" ) {
                CHECK( !A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged the same to the hour but different numbers of minutes" ) {
            A.mod_rot( A.type->comestible->spoils - 5_hours );
            B.mod_rot( B.type->comestible->spoils - 5_hours );
            B.mod_rot( -5_minutes );
            THEN( "they stack" ) {
                CHECK( A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged a few seconds different but different hours" ) {
            A.mod_rot( A.type->comestible->spoils - 5_hours );
            B.mod_rot( B.type->comestible->spoils - 5_hours );
            B.mod_rot( 5_turns );
            THEN( "they don't stack" ) {
                CHECK( !A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged the same to the day but different numbers of seconds" ) {
            A.mod_rot( A.type->comestible->spoils - 3_days );
            B.mod_rot( B.type->comestible->spoils - 3_days );
            B.mod_rot( -5_turns );
            THEN( "they stack" ) {
                CHECK( A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged a few seconds different but different days" ) {
            A.mod_rot( A.type->comestible->spoils - 3_days );
            B.mod_rot( B.type->comestible->spoils - 3_days );
            B.mod_rot( 5_turns );
            THEN( "they don't stack" ) {
                CHECK( !A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged the same to the week but different numbers of seconds" ) {
            A.mod_rot( A.type->comestible->spoils - 7_days );
            B.mod_rot( B.type->comestible->spoils - 7_days );
            B.mod_rot( -5_turns );
            THEN( "they stack" ) {
                CHECK( A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged a few seconds different but different weeks" ) {
            A.mod_rot( A.type->comestible->spoils - 7_days );
            B.mod_rot( B.type->comestible->spoils - 7_days );
            B.mod_rot( 5_turns );
            THEN( "they don't stack" ) {
                CHECK( !A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged the same to the season but different numbers of seconds" ) {
            A.mod_rot( A.type->comestible->spoils - calendar::season_length() );
            B.mod_rot( B.type->comestible->spoils - calendar::season_length() );
            B.mod_rot( -5_turns );
            THEN( "they stack" ) {
                CHECK( A.stacks_with( B ) );
            }
        }
        WHEN( "the items are aged a few seconds different but different seasons" ) {
            A.mod_rot( A.type->comestible->spoils - calendar::season_length() );
            B.mod_rot( B.type->comestible->spoils - calendar::season_length() );
            B.mod_rot( 5_turns );
            THEN( "they don't stack" ) {
                CHECK( !A.stacks_with( B ) );
            }
        }
    }
}

TEST_CASE( "liquids at different temperatures", "[item][temperature][stack][combine]" )
{
    item liquid_hot( "test_liquid" );
    item liquid_cold( "test_liquid" );
    item liquid_filthy( "test_liquid" );

    // heat_up/cold_up sets temperature of item and corresponding HOT/COLD flags
    liquid_hot.heat_up(); // 60 C (333.15 K)
    liquid_cold.cold_up(); // 3 C (276.15 K)
    liquid_filthy.cold_up(); // 3 C (276.15 K)
    liquid_filthy.set_flag( json_flag_FILTHY );

    REQUIRE( units::to_kelvin( liquid_hot.temperature ) == Approx( 333.15 ) );
    REQUIRE( units::to_kelvin( liquid_cold.temperature ) == Approx( 276.15 ) );
    REQUIRE( liquid_hot.has_flag( json_flag_HOT ) );
    REQUIRE( liquid_cold.has_flag( json_flag_COLD ) );

    SECTION( "liquids at the same temperature can stack together" ) {
        CHECK( liquid_cold.stacks_with( liquid_cold ) );
        CHECK( liquid_hot.stacks_with( liquid_hot ) );
    }

    SECTION( "liquids at different temperature do not stack" ) {
        // Items with different flags do not stack
        CHECK_FALSE( liquid_cold.stacks_with( liquid_hot ) );
        CHECK_FALSE( liquid_hot.stacks_with( liquid_cold ) );
    }

    SECTION( "liquids at different temperature can be combined" ) {
        CHECK( liquid_cold.can_combine( liquid_hot ) );
        CHECK( liquid_hot.can_combine( liquid_cold ) );
    }

    SECTION( "liquids with different flags can not be combined" ) {
        CHECK( !liquid_cold.can_combine( liquid_filthy ) );
        CHECK( !liquid_filthy.can_combine( liquid_cold ) );
    }
}

static void assert_minimum_length_to_volume_ratio( const item &target )
{
    if( target.type->get_id().is_null() ) {
        return;
    }
    CAPTURE( target.type->get_id() );
    CAPTURE( target.volume() );
    CAPTURE( target.base_volume() );
    CAPTURE( target.type->volume );
    if( target.made_of( phase_id::LIQUID ) || target.is_soft() ) {
        CHECK( target.length() == 0_mm );
        return;
    }
    if( target.volume() == 0_ml ) {
        CHECK( target.length() == -1_mm );
        return;
    }
    if( target.volume() == 1_ml ) {
        CHECK( target.length() >= 0_mm );
        return;
    }
    // Minimum possible length is if the item is a sphere.
    const float minimal_diameter = std::cbrt( ( 3.0 * units::to_milliliter( target.base_volume() ) ) /
                                   ( 4.0 * M_PI ) );
    CHECK( units::to_millimeter( target.length() ) >= minimal_diameter * 10.0 );
}

TEST_CASE( "item length sanity check", "[item]" )
{
    for( const itype *type : item_controller->all() ) {
        const item sample( type, calendar::turn_zero, item::solitary_tag {} );
        assert_minimum_length_to_volume_ratio( sample );
    }
}

TEST_CASE( "corpse length sanity check", "[item]" )
{
    for( const mtype &type : MonsterGenerator::generator().get_all_mtypes() ) {
        const item sample = item::make_corpse( type.id );
        assert_minimum_length_to_volume_ratio( sample );
    }
}

static void check_spawning_in_container( const std::string &item_type )
{
    item test_item{ itype_id( item_type ) };
    REQUIRE( test_item.type->default_container );
    item container_item = test_item.in_its_container( 1 );
    CHECK( container_item.typeId() == *test_item.type->default_container );
    if( container_item.is_container() ) {
        CHECK( container_item.has_item_with( [&test_item]( const item & it ) {
            return it.typeId() == test_item.typeId();
        } ) );
    } else if( test_item.is_software() ) {
        REQUIRE( container_item.is_software_storage() );
        const std::vector<const item *> softwares = container_item.softwares();
        CHECK( !softwares.empty() );
        for( const item *itm : softwares ) {
            CHECK( itm->typeId() == test_item.typeId() );
        }
    } else {
        FAIL( "Not container or software storage." );
    }
}

TEST_CASE( "items spawn in their default containers", "[item]" )
{
    check_spawning_in_container( "water" );
    check_spawning_in_container( "gunpowder" );
    check_spawning_in_container( "nitrox" );
    check_spawning_in_container( "ammonia_hydroxide" );
    check_spawning_in_container( "detergent" );
    check_spawning_in_container( "pale_ale" );
    check_spawning_in_container( "single_malt_whiskey" );
    check_spawning_in_container( "rocuronium" );
    check_spawning_in_container( "chem_muriatic_acid" );
    check_spawning_in_container( "chem_black_powder" );
    check_spawning_in_container( "software_useless" );
}

TEST_CASE( "item variables round-trip accurately", "[item]" )
{
    item i( "water" );
    i.set_var( "A", 17 );
    CHECK( i.get_var( "A", 0 ) == 17 );
    i.set_var( "B", 0.125 );
    CHECK( i.get_var( "B", 0.0 ) == 0.125 );
    i.set_var( "C", tripoint( 2, 3, 4 ) );
    CHECK( i.get_var( "C", tripoint() ) == tripoint( 2, 3, 4 ) );
}

TEST_CASE( "water affect items while swimming check", "[item][water][swimming]" )
{
    avatar &guy = get_avatar();
    clear_avatar();

    GIVEN( "an item with flag WATER_DISSOLVE" ) {

        REQUIRE( item( "aspirin" ).has_flag( flag_WATER_DISSOLVE ) );

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.worn.clear();

            item aspirin( "aspirin" );

            REQUIRE( guy.wield( aspirin ) );

            THEN( "should dissolve in water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_WATER_DISSOLVE ) );
            }
        }

        WHEN( "item in backpack" ) {
            guy.unwield();
            guy.worn.clear();

            item aspirin( "aspirin" );
            item backpack( "backpack" );

            backpack.put_in( aspirin, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wear_item( backpack ) );

            THEN( "should dissolve in water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_WATER_DISSOLVE ) );
            }
        }

        WHEN( "item in small plastic bottle" ) {
            guy.unwield();
            guy.worn.clear();

            item aspirin( "aspirin" );
            item bottle_small( "bottle_plastic_small" );

            bottle_small.put_in( aspirin, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( bottle_small ) );

            THEN( "should not dissolve in water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_WATER_DISSOLVE ) );
            }
        }

        WHEN( "item in backpack inside duffel bag" ) {
            guy.unwield();
            guy.worn.clear();

            item aspirin( "aspirin" );
            item backpack( "backpack" );
            item duffelbag( "duffelbag" );

            backpack.put_in( aspirin, item_pocket::pocket_type::CONTAINER );
            duffelbag.put_in( backpack, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wear_item( duffelbag ) );

            THEN( "should dissolve in water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_WATER_DISSOLVE ) );
            }
        }

        WHEN( "item in backpack inside body bag" ) {
            guy.unwield();
            guy.worn.clear();

            item aspirin( "aspirin" );
            item backpack( "backpack" );
            item body_bag( "test_waterproof_bag" );

            backpack.put_in( aspirin, item_pocket::pocket_type::CONTAINER );
            body_bag.put_in( backpack, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( body_bag ) );

            THEN( "should not dissolve in water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_WATER_DISSOLVE ) );
            }
        }
    }

    GIVEN( "an item with flag WATER_BREAK" ) {

        REQUIRE( itype_test_smart_phone->has_flag( flag_WATER_BREAK ) );

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.worn.clear();

            item smart_phone( itype_test_smart_phone );

            REQUIRE( guy.wield( smart_phone ) );

            THEN( "should be broken by water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_ITEM_BROKEN ) );
            }
        }

        WHEN( "item in backpack" ) {
            guy.unwield();
            guy.worn.clear();

            item smart_phone( itype_test_smart_phone );
            item backpack( itype_test_backpack );

            backpack.put_in( smart_phone, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( backpack ) );

            THEN( "should be broken by water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_ITEM_BROKEN ) );
            }
        }

        WHEN( "item in body bag" ) {
            guy.unwield();
            guy.worn.clear();

            item smart_phone( itype_test_smart_phone );
            item body_bag( "test_waterproof_bag" );

            body_bag.put_in( smart_phone, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( body_bag ) );

            THEN( "should not be broken by water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_ITEM_BROKEN ) );
            }
        }

        WHEN( "item in backpack inside duffel bag" ) {
            guy.unwield();
            guy.worn.clear();

            item smart_phone( itype_test_smart_phone );
            item backpack( itype_test_backpack );
            item duffelbag( itype_test_duffelbag );

            backpack.put_in( smart_phone, item_pocket::pocket_type::CONTAINER );
            duffelbag.put_in( backpack, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( duffelbag ) );

            THEN( "should be broken by water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_ITEM_BROKEN ) );
            }
        }

        WHEN( "item in backpack inside body bag" ) {
            guy.unwield();
            guy.worn.clear();

            item smart_phone( itype_test_smart_phone );
            item backpack( itype_test_backpack );
            item body_bag( itype_test_waterproof_bag );

            backpack.put_in( smart_phone, item_pocket::pocket_type::CONTAINER );
            body_bag.put_in( backpack, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( body_bag ) );

            THEN( "should not be broken by water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_ITEM_BROKEN ) );
            }
        }
    }

    GIVEN( "an item with flag WATER_BREAK_ACTIVE" ) {

        REQUIRE( itype_test_mp3->has_flag( flag_WATER_BREAK_ACTIVE ) );

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.worn.clear();

            item mp3( itype_test_mp3 );

            REQUIRE( guy.wield( mp3 ) );

            THEN( "should get wet from water" ) {
                g->water_affect_items( guy );
                CHECK( guy.get_wielded_item()->wetness > 0 );
            }
        }

        WHEN( "item in backpack" ) {
            guy.unwield();
            guy.worn.clear();

            item mp3( itype_test_mp3 );
            item backpack( itype_test_backpack );

            backpack.put_in( mp3, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( backpack ) );

            THEN( "should get wet from water" ) {
                g->water_affect_items( guy );
                const item *test_item = guy.get_wielded_item()->all_items_top().front();
                REQUIRE( test_item->typeId() == itype_test_mp3 );
                CHECK( test_item->wetness > 0 );
            }
        }

        WHEN( "item in body bag" ) {
            guy.unwield();
            guy.worn.clear();

            item mp3( itype_test_mp3 );
            item body_bag( itype_test_waterproof_bag );

            body_bag.put_in( mp3, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( body_bag ) );

            THEN( "should not be broken by water" ) {
                g->water_affect_items( guy );
                const item *test_item = guy.get_wielded_item()->all_items_top().front();
                REQUIRE( test_item->typeId() == itype_test_mp3 );
                CHECK( test_item->wetness == 0 );
            }
        }

        WHEN( "item in backpack inside duffel bag" ) {
            guy.unwield();
            guy.worn.clear();

            item mp3( itype_test_mp3 );
            item backpack( itype_test_backpack );
            item duffelbag( itype_test_duffelbag );

            backpack.put_in( mp3, item_pocket::pocket_type::CONTAINER );
            duffelbag.put_in( backpack, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( duffelbag ) );

            THEN( "should get wet from water" ) {
                g->water_affect_items( guy );
                const item *test_item = guy.get_wielded_item()->all_items_top().front()->all_items_top().front();
                REQUIRE( test_item->typeId() == itype_test_mp3 );
                CHECK( test_item->wetness > 0 );
            }
        }

        WHEN( "item in backpack inside body bag" ) {
            guy.unwield();
            guy.worn.clear();

            item mp3( itype_test_mp3 );
            item backpack( itype_test_backpack );
            item body_bag( itype_test_waterproof_bag );

            backpack.put_in( mp3, item_pocket::pocket_type::CONTAINER );
            body_bag.put_in( backpack, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( body_bag ) );

            THEN( "should not be broken by water" ) {
                g->water_affect_items( guy );
                const item *test_item = guy.get_wielded_item()->all_items_top().front()->all_items_top().front();
                REQUIRE( test_item->typeId() == itype_test_mp3 );
                CHECK( test_item->wetness == 0 );
            }
        }

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.worn.clear();

            item mp3( itype_test_mp3 );

            REQUIRE( guy.wield( mp3 ) );

            THEN( "should be wet for around 8664 seconds" ) {
                g->water_affect_items( guy );
                CHECK( guy.get_wielded_item()->wetness == Approx( 8664 ).margin( 20 ) );
            }
        }

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.worn.clear();

            item mp3( itype_test_mp3 );

            REQUIRE( guy.wield( mp3 ) );

            THEN( "gets wet five times in a row " ) {
                g->water_affect_items( guy );
                g->water_affect_items( guy );
                g->water_affect_items( guy );
                g->water_affect_items( guy );
                g->water_affect_items( guy );
                AND_THEN( "should be wet for around 43320 seconds" ) {
                    CHECK( guy.get_wielded_item()->wetness == Approx( 43320 ).margin( 100 ) );
                }
            }
        }
    }

    GIVEN( "a towel" ) {

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.worn.clear();

            item towel( "towel" );

            REQUIRE( guy.wield( towel ) );

            THEN( "should get wet in water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_WET ) );
            }
        }

        WHEN( "wearing item" ) {
            guy.unwield();
            guy.worn.clear();

            item towel( "towel" );

            REQUIRE( guy.wear_item( towel ) );

            THEN( "should get wet in water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_WET ) );
            }
        }

        WHEN( "inside a backpack" ) {
            guy.unwield();
            guy.worn.clear();

            item towel( "towel" );
            item backpack( "backpack" );

            backpack.put_in( towel, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( backpack ) );

            THEN( "should get wet in water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_WET ) );
            }
        }

        WHEN( "inside a body bag" ) {
            guy.unwield();
            guy.worn.clear();

            item towel( "towel" );
            item body_bag( "test_waterproof_bag" );

            body_bag.put_in( towel, item_pocket::pocket_type::CONTAINER );

            REQUIRE( guy.wield( body_bag ) );

            THEN( "should not get wet in water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_WET ) );
            }
        }
    }
}

static const std::set<itype_id> known_bad = {
    itype_id( "engine_block_small" ),
    itype_id( "bread" ),
    itype_id( "wrapped_rad_badge" ),
    itype_id( "8mm_inc" ),
    itype_id( "fire_drill" ),
    itype_id( "meal_chitin_piece" ),
    itype_id( "blood" ),
    itype_id( "curry_meat" ),
    itype_id( "speargun" ),
    itype_id( "1cyl_combustion_large" ),
    itype_id( "seasoning_salt" ),
    itype_id( "smart_phone" ),
    itype_id( "delay_fuze" ),
    itype_id( "apple_canned" ),
    itype_id( "broken_mech_recon" ),
    itype_id( "chem_nickel_powder" ),
    itype_id( "lc_steel_lump" ),
    itype_id( "v_table" ),
    itype_id( "mc_steel_chunk" ),
    itype_id( "drink_karsk" ),
    itype_id( "fish_bowl" ),
    itype_id( "leather_filing" ),
    itype_id( "brew_sake_koji" ),
    itype_id( "artifact_slow_aura" ),
    itype_id( "coffee_raw" ),
    itype_id( "egg_roach" ),
    itype_id( "v_rockwheel_item" ),
    itype_id( "art_knot" ),
    itype_id( "glassblowing_book" ),
    itype_id( "sushi_veggyroll" ),
    itype_id( "sugar_fried" ),
    itype_id( "donut_holes" ),
    itype_id( "v2_combustion" ),
    itype_id( "family_cookbook" ),
    itype_id( "coffee_raw_kentucky" ),
    itype_id( "bandages_makeshift" ),
    itype_id( "candy5" ),
    itype_id( "baton" ),
    itype_id( "homeopathic_pills" ),
    itype_id( "tool_belt" ),
    itype_id( "marloss_berry" ),
    itype_id( "bot_lab_security_drone_YM" ),
    itype_id( "art_disc" ),
    itype_id( "comb_pocket" ),
    itype_id( "AID_bio_flashlight" ),
    itype_id( "loom_frame" ),
    itype_id( "raw_curing_fatty_meat" ),
    itype_id( "egg_bird" ),
    itype_id( "art_napkin" ),
    itype_id( "light_minus_battery_cell" ),
    itype_id( "shot_beanbag" ),
    itype_id( "RPG-7_tbg7v" ),
    itype_id( "AID_bio_syringe" ),
    itype_id( "human_flesh" ),
    itype_id( "caramel_apple" ),
    itype_id( "corpse_generic_boy" ),
    itype_id( "milkshake_deluxe_choc" ),
    itype_id( "extinguishing_agent" ),
    itype_id( "dayquil" ),
    itype_id( "pine_tea" ),
    itype_id( "pointy_stick" ),
    itype_id( "soysauce" ),
    itype_id( "johnnycake" ),
    itype_id( "plastic_knife" ),
    itype_id( "demihuman_blood" ),
    itype_id( "bot_gasbomb_hack" ),
    itype_id( "sandwich_reuben_wheat_free" ),
    itype_id( "book_anthology_an" ),
    itype_id( "integrated_patchskin1" ),
    itype_id( "tailoring_pattern_set" ),
    itype_id( "cell_phone_flashlight" ),
    itype_id( "mutant_bug_organs_cooked" ),
    itype_id( "q_staff" ),
    itype_id( "book_anthology_bk" ),
    itype_id( "8mm_jhp" ),
    itype_id( "scorecard" ),
    itype_id( "pepper" ),
    itype_id( "budget_steel_lump" ),
    itype_id( "young_koji" ),
    itype_id( "k_gambeson_vest_xs" ),
    itype_id( "candy2" ),
    itype_id( "mutant_meat_scrap" ),
    itype_id( "acid_soaked_hide" ),
    itype_id( "dragonskin" ),
    itype_id( "cuvettes" ),
    itype_id( "mp3_on" ),
    itype_id( "mutant_cracklins" ),
    itype_id( "egg_pheasant" ),
    itype_id( "venom_wasp" ),
    itype_id( "art_sphere" ),
    itype_id( "id_science" ),
    itype_id( "newest_newspaper" ),
    itype_id( "bandages_makeshift_boiled" ),
    itype_id( "cooked_marrow" ),
    itype_id( "fruit_pancakes" ),
    itype_id( "folding_solar_panel_v2" ),
    itype_id( "test_hacksaw" ),
    itype_id( "tablesaw_tool" ),
    itype_id( "broken_mech_combat" ),
    itype_id( "awl_bone" ),
    itype_id( "beer" ),
    itype_id( "chem_hexamine" ),
    itype_id( "egg_raven" ),
    itype_id( "sandwich_sauce_wheat_free" ),
    itype_id( "mutant_human_flesh" ),
    itype_id( "adobe_pallet_full" ),
    itype_id( "ruined_chunks" ),
    itype_id( "formic_acid" ),
    itype_id( "soaking_dandelion" ),
    itype_id( "meat_smoked" ),
    itype_id( "bolt_simple_wood" ),
    itype_id( "shot_bird" ),
    itype_id( "vaccine_shot" ),
    itype_id( "sugar" ),
    itype_id( "adderall" ),
    itype_id( "fire_drill_large" ),
    itype_id( "milkshake_choc" ),
    itype_id( "blood_pancakes_wheat_free" ),
    itype_id( "bow_sight" ),
    itype_id( "fake_fireplace" ),
    itype_id( "mycus_juice" ),
    itype_id( "brew_rum" ),
    itype_id( "cash_card" ),
    itype_id( "mp3" ),
    itype_id( "id_science_security_black" ),
    itype_id( "punch_bone" ),
    itype_id( "manual_launcher" ),
    itype_id( "medium_surface_pseudo" ),
    itype_id( "freezer" ),
    itype_id( "bolt_explosive" ),
    itype_id( "xanax" ),
    itype_id( "hot_sauce" ),
    itype_id( "yeast" ),
    itype_id( "alloy_plate" ),
    itype_id( "candy3" ),
    itype_id( "steel_lump" ),
    itype_id( "kevlar_chainmail_suit" ),
    itype_id( "bp_shot_scrap" ),
    itype_id( "a180mag4" ),
    itype_id( "bolt_makeshift" ),
    itype_id( "liquid_soap" ),
    itype_id( "chem_aluminium_sulphate" ),
    itype_id( "antibiotics" ),
    itype_id( "dynamite" ),
    itype_id( "i4_combustion" ),
    itype_id( "bot_lab_security_drone_GR" ),
    itype_id( "parkour_practice" ),
    itype_id( "egg_centipede" ),
    itype_id( "flatbread_wheat_free" ),
    itype_id( "egg_roach_plague" ),
    itype_id( "butchery_rack_pseudo" ),
    itype_id( "birchbark" ),
    itype_id( "material_zincite" ),
    itype_id( "egg_mantis" ),
    itype_id( "small_lcd_screen" ),
    itype_id( "corpse_child_calm" ),
    itype_id( "biscuit" ),
    itype_id( "2h_flail_wood" ),
    itype_id( "xl_kevlar_chainmail_hauberk" ),
    itype_id( "koji" ),
    itype_id( "light_minus_disposable_cell" ),
    itype_id( "mobile_memory_card_encrypted" ),
    itype_id( "fire_piston" ),
    itype_id( "broken_c4_hack" ),
    itype_id( "panacea" ),
    itype_id( "artifact_teleportitis_aura" ),
    itype_id( "bandsaw_tool" ),
    itype_id( "chem_acetone" ),
    itype_id( "bp_shot_dragon" ),
    itype_id( "recipe_medicalmut" ),
    itype_id( "hotdogs_cooked_wheat_free" ),
    itype_id( "manhole_cover" ),
    itype_id( "oatmeal_deluxe" ),
    itype_id( "demihuman_marrow" ),
    itype_id( "drill_rock_primitive" ),
    itype_id( "id_industrial" ),
    itype_id( "drink_queenmary" ),
    itype_id( "planer_tool" ),
    itype_id( "sandwich_reuben" ),
    itype_id( "handrolled_cig" ),
    itype_id( "vortex" ),
    itype_id( "wasp_sting" ),
    itype_id( "corpse_generic_girl" ),
    itype_id( "pipe_tobacco" ),
    itype_id( "egg_grouse" ),
    itype_id( "carbon_electrode" ),
    itype_id( "hotdogs_cooked" ),
    itype_id( "candy" ),
    itype_id( "fan" ),
    itype_id( "scrap" ),
    itype_id( "theater_props" ),
    itype_id( "poppysyrup" ),
    itype_id( "calcium_tablet" ),
    itype_id( "broken_lab_security_drone_BM2" ),
    itype_id( "bot_lab_security_drone_BS" ),
    itype_id( "book_anthology_sst" ),
    itype_id( "egg_dragonfly" ),
    itype_id( "protein_drink" ),
    itype_id( "badge_cybercop" ),
    itype_id( "id_science_maintenance_yellow" ),
    itype_id( "recipe_labchem" ),
    itype_id( "xl_gambeson_sleeve" ),
    itype_id( "art_rod" ),
    itype_id( "dieselpunk_tailor" ),
    itype_id( "id_science_mutagen_pink" ),
    itype_id( "id_artisan_member" ),
    itype_id( "large_surface_pseudo" ),
    itype_id( "deployment_bag" ),
    itype_id( "lab_postit_blob" ),
    itype_id( "chem_ferric_chloride" ),
    itype_id( "venom_bee" ),
    itype_id( "bot_rifleturret" ),
    itype_id( "weeks_old_newspaper" ),
    itype_id( "young_koji_done" ),
    itype_id( "adrenaline_injector" ),
    itype_id( "transponder" ),
    itype_id( "test_weapon1" ),
    itype_id( "xl_tacvest" ),
    itype_id( "plastic_straw" ),
    itype_id( "k_gambeson_pants" ),
    itype_id( "corpse_stabbed" ),
    itype_id( "torch_done" ),
    itype_id( "integrated_patchskin2" ),
    itype_id( "art_crescent" ),
    itype_id( "soup_blood" ),
    itype_id( "tourniquet_lower" ),
    itype_id( "digging_stick" ),
    itype_id( "bone_marrow" ),
    itype_id( "cable_speaker" ),
    itype_id( "conc_venom" ),
    itype_id( "plastics_book" ),
    itype_id( "art_lamp" ),
    itype_id( "robofac_yrax_deactivation_manual" ),
    itype_id( "cornbread" ),
    itype_id( "hoboreel" ),
    itype_id( "one_year_old_newspaper" ),
    itype_id( "budget_steel_chunk" ),
    itype_id( "tanbark" ),
    itype_id( "dog_whistle_wood" ),
    itype_id( "tailor_japanese" ),
    itype_id( "dandelionburdock_tea" ),
    itype_id( "craft" ),
    itype_id( "broom" ),
    itype_id( "arm" ),
    itype_id( "v6_diesel" ),
    itype_id( "fake_appliance_tool" ),
    itype_id( "light_atomic_battery_cell" ),
    itype_id( "frozen_lemonade" ),
    itype_id( "glowstick_dead" ),
    itype_id( "rmi2_corpse" ),
    itype_id( "milk_fortified" ),
    itype_id( "coin_gold" ),
    itype_id( "cable_xlr" ),
    itype_id( "prophylactic_antivenom" ),
    itype_id( "mannequin" ),
    itype_id( "soy_wheat_dough_done" ),
    itype_id( "towel_hanger" ),
    itype_id( "id_science_maintenance_green" ),
    itype_id( "brew_gin" ),
    itype_id( "belt308" ),
    itype_id( "test_pointy_stick" ),
    itype_id( "tobacco" ),
    itype_id( "cutting_board" ),
    itype_id( "brew_hb_beer" ),
    itype_id( "corpse_gunned" ),
    itype_id( "AID_bio_railgun" ),
    itype_id( "lab_postit_tech" ),
    itype_id( "tactical_shotshell_pouch" ),
    itype_id( "recipe_caseless" ),
    itype_id( "mortar_lime" ),
    itype_id( "nectar" ),
    itype_id( "necronomicon" ),
    itype_id( "pizza_supreme" ),
    itype_id( "66mm_HEAT" ),
    itype_id( "chem_thermite" ),
    itype_id( "motorcycle_headlight" ),
    itype_id( "altered_rubicks_cube" ),
    itype_id( "hi_q_shatter" ),
    itype_id( "egg_mole_cricket" ),
    itype_id( "meal_bone" ),
    itype_id( "corpse_generic_gunned" ),
    itype_id( "bellywrap_leather" ),
    itype_id( "survivor_scope" ),
    itype_id( "triffid_fungicide" ),
    itype_id( "pine_resin" ),
    itype_id( "folding_solar_panel" ),
    itype_id( "badge_swat" ),
    itype_id( "can_peach" ),
    itype_id( "bowling_pin" ),
    itype_id( "fake_firestarter" ),
    itype_id( "egg_stag_beetle" ),
    itype_id( "rehydration_drink" ),
    itype_id( "bee_sting" ),
    itype_id( "test_cracklins" ),
    itype_id( "iotv_shoulder_plate" ),
    itype_id( "permanent_marker" ),
    itype_id( "kilt_leather" ),
    itype_id( "flu_shot" ),
    itype_id( "k_gambeson_xs" ),
    itype_id( "arcfurnace" ),
    itype_id( "maple_sap" ),
    itype_id( "a180mag3" ),
    itype_id( "chem_washing_soda" ),
    itype_id( "peanutbutter_imitation" ),
    itype_id( "casting_mold" ),
    itype_id( "book_anthology_htloop" ),
    itype_id( "arrow_heavy_fire_hardened_fletched" ),
    itype_id( "dry_beans" ),
    itype_id( "button_bronze" ),
    itype_id( "bacon_uncut" ),
    itype_id( "tÖttchen" ),
    itype_id( "bp_shot_00" ),
    itype_id( "clogs" ),
    itype_id( "feces_cow" ),
    itype_id( "hand_crank_charger" ),
    itype_id( "dynamite_act" ),
    itype_id( "bonemeal_tablet" ),
    itype_id( "xs_gambeson_sleeve" ),
    itype_id( "circuit" ),
    itype_id( "blood_pancakes" ),
    itype_id( "gambeson_sleeve" ),
    itype_id( "rolling_pin" ),
    itype_id( "scrambledeggsandbrain" ),
    itype_id( "sewage" ),
    itype_id( "test_load_bearing_vest" ),
    itype_id( "sauerkraut_onions" ),
    itype_id( "cow_candy" ),
    itype_id( "soggy_hardtack" ),
    itype_id( "antarvasa" ),
    itype_id( "needle_bone" ),
    itype_id( "concrete" ),
    itype_id( "v12_combustion" ),
    itype_id( "toolset" ),
    itype_id( "rigid_plastic_sheet" ),
    itype_id( "chem_formaldehyde" ),
    itype_id( "sandwich_deluxe" ),
    itype_id( "robofac_test_data" ),
    itype_id( "snail_garden" ),
    itype_id( "irradiated_cranberries" ),
    itype_id( "hallula" ),
    itype_id( "chem_chloroform" ),
    itype_id( "soaked_dandelion" ),
    itype_id( "coffee_syrup" ),
    itype_id( "egg_strider" ),
    itype_id( "bot_lab_security_drone_GM" ),
    itype_id( "fertilizer_liquid" ),
    itype_id( "neccowafers" ),
    itype_id( "acid_spit" ),
    itype_id( "corpse_painful" ),
    itype_id( "offal" ),
    itype_id( "strong_antibiotic" ),
    itype_id( "gelatin_powder" ),
    itype_id( "mc_steel_lump" ),
    itype_id( "dogfood" ),
    itype_id( "bullwhip" ),
    itype_id( "v8_combustion" ),
    itype_id( "book_nonf_hard_homemk_grtrms" ),
    itype_id( "chunk_sulfur" ),
    itype_id( "recipe_fauxfur" ),
    itype_id( "water" ),
    itype_id( "freeze_dried_meal_hydrated" ),
    itype_id( "evaporator_coil" ),
    itype_id( "resin_chunk" ),
    itype_id( "scrap_aluminum" ),
    itype_id( "hops" ),
    itype_id( "crackpipe" ),
    itype_id( "pumpkin_yeast_bread" ),
    itype_id( "bandages" ),
    itype_id( "shot_flechette" ),
    itype_id( "dried_salad" ),
    itype_id( "AID_bio_geiger" ),
    itype_id( "8mm_caseless" ),
    itype_id( "sandwich_sauce" ),
    itype_id( "lung_provence" ),
    itype_id( "portal" ),
    itype_id( "dandelion_tea" ),
    itype_id( "id_science_security_yellow" ),
    itype_id( "scots_cookbook" ),
    itype_id( "primitive_hammer" ),
    itype_id( "water_mineral" ),
    itype_id( "fake_beverly_shear" ),
    itype_id( "badge_detective" ),
    itype_id( "bamboo_cooked" ),
    itype_id( "throwing_stick" ),
    itype_id( "cookbook_sushi" ),
    itype_id( "oil_press_mounted" ),
    itype_id( "xl_kevlar_chainmail_suit" ),
    itype_id( "id_military" ),
    itype_id( "willowbark" ),
    itype_id( "soap" ),
    itype_id( "young_yeast" ),
    itype_id( "test_boltcutter_elec" ),
    itype_id( "bo" ),
    itype_id( "months_old_newspaper" ),
    itype_id( "broken_manhack" ),
    itype_id( "slime_scrap" ),
    itype_id( "bagh_nakha" ),
    itype_id( "integrated_bark" ),
    itype_id( "id_science_transport_1" ),
    itype_id( "tacvest" ),
    itype_id( "chilidogs" ),
    itype_id( "coin_half_dollar" ),
    itype_id( "glowstick" ),
    itype_id( "integrated_patchskin3" ),
    itype_id( "recipe_augs" ),
    itype_id( "integrated_bark2_a" ),
    itype_id( "recipe_lab_elec" ),
    itype_id( "pumpkin_muffins" ),
    itype_id( "AID_bio_soporific" ),
    itype_id( "american_flag" ),
    itype_id( "test_smart_phone" ),
    itype_id( "iotv_groin_plate" ),
    itype_id( "coin_silver" ),
    itype_id( "test_tube" ),
    itype_id( "codeine" ),
    itype_id( "textbook_extraction" ),
    itype_id( "art_crystal" ),
    itype_id( "brew_vinegar" ),
    itype_id( "meat_scrap_cooked" ),
    itype_id( "salt" ),
    itype_id( "mutant_human_blood" ),
    itype_id( "tactical_grenade_pouch" ),
    itype_id( "hc_steel_chunk" ),
    itype_id( "marloss_gel" ),
    itype_id( "inj_iron" ),
    itype_id( "badge_foodkid" ),
    itype_id( "gelatin_extracted" ),
    itype_id( "nyquil" ),
    itype_id( "washboard" ),
    itype_id( "nitrox" ),
    itype_id( "steel_ballistic_plate" ),
    itype_id( "wood_plate" ),
    itype_id( "nailgun" ),
    itype_id( "note_mutant_alpha_boss" ),
    itype_id( "prozac" ),
    itype_id( "cocaine_topical" ),
    itype_id( "AID_bio_climate" ),
    itype_id( "feces_dog" ),
    itype_id( "butchery_tree_pseudo" ),
    itype_id( "folding_poncho" ),
    itype_id( "tortilla_corn" ),
    itype_id( "broken_gasbomb_hack" ),
    itype_id( "meal_bone_tainted" ),
    itype_id( "hackerman_computer" ),
    itype_id( "tarp_raincatcher" ),
    itype_id( "microphone_xlr_generic" ),
    itype_id( "load_bearing_vest_sling" ),
    itype_id( "fish_bait" ),
    itype_id( "gasdiscount_gold" ),
    itype_id( "reloaded_shot_flechette" ),
    itype_id( "primitive_knife" ),
    itype_id( "art_pyramid" ),
    itype_id( "AID_bio_blood_anal" ),
    itype_id( "w_table" ),
    itype_id( "xl_k_gambeson_loose" ),
    itype_id( "mre_accessory" ),
    itype_id( "textbook_computer" ),
    itype_id( "candy3gator" ),
    itype_id( "kevlar_chainmail_suit_xs" ),
    itype_id( "bolt_simple_small_game" ),
    itype_id( "raw_bamboo" ),
    itype_id( "arrow_fire_hardened_fletched" ),
    itype_id( "mutant_human_cooked" ),
    itype_id( "plastic_fork" ),
    itype_id( "egg_lady_bug" ),
    itype_id( "brew_whiskey" ),
    itype_id( "human_meat_scrap" ),
    itype_id( "manual_bashing" ),
    itype_id( "nic_gum" ),
    itype_id( "AID_bio_power_storage_mkII" ),
    itype_id( "saline" ),
    itype_id( "chem_antimony_trichloride" ),
    itype_id( "hammer_sledge_heavy" ),
    itype_id( "mutant_meat" ),
    itype_id( "plutonium" ),
    itype_id( "broken_lab_security_drone_GM" ),
    itype_id( "mutant_meat_cooked" ),
    itype_id( "chem_DMSO" ),
    itype_id( "broken_lab_security_drone_BM" ),
    itype_id( "hard_plate" ),
    itype_id( "waterskin" ),
    itype_id( "joint_roach" ),
    itype_id( "sandwich_deluxe_wheat_free" ),
    itype_id( "base_toiletries" ),
    itype_id( "egg_locust" ),
    itype_id( "glass_tinted" ),
    itype_id( "stomach_sealed" ),
    itype_id( "sake_koji_rice" ),
    itype_id( "e_scrap" ),
    itype_id( "staff_sling" ),
    itype_id( "heavy_load_bearing_vest_breacher" ),
    itype_id( "usb_drive_nano" ),
    itype_id( "chem_hydrogen_peroxide" ),
    itype_id( "recipe_animal" ),
    itype_id( "impact_fuze" ),
    itype_id( "board_trap" ),
    itype_id( "chestpouch_leather" ),
    itype_id( "tourniquet_upper_XS" ),
    itype_id( "disinfectant_makeshift" ),
    itype_id( "choco_coffee_beans" ),
    itype_id( "boiled_egg" ),
    itype_id( "sourdough_young_uncovered" ),
    itype_id( "fishing_rod_basic" ),
    itype_id( "mre_dessert" ),
    itype_id( "reference_cooking" ),
    itype_id( "hairbrush" ),
    itype_id( "marloss_seed" ),
    itype_id( "mutant_meat_scrap_cooked" ),
    itype_id( "sandwich_t" ),
    itype_id( "mutant_human_marrow" ),
    itype_id( "alloy_sheet" ),
    itype_id( "gasdiscount_silver" ),
    itype_id( "mitresaw_tool" ),
    itype_id( "soup_blood_wheat_free" ),
    itype_id( "dynamite_bomb" ),
    itype_id( "corpse_oldwoman_jewelry" ),
    itype_id( "test_waterskin" ),
    itype_id( "bot_tazer_hack" ),
    itype_id( "recipe_atomic_battery" ),
    itype_id( "gloves_medical" ),
    itype_id( "stab_plate" ),
    itype_id( "gum" ),
    itype_id( "smartphone_music" ),
    itype_id( "c4" ),
    itype_id( "recon_mech_laser_single" ),
    itype_id( "meat_mutant_tainted" ),
    itype_id( "bp_shot_slug" ),
    itype_id( "gatling_mech_laser_single" ),
    itype_id( "chilly-p" ),
    itype_id( "survnote" ),
    itype_id( "book_anthology_cw" ),
    itype_id( "brew_burdock_wine" ),
    itype_id( "firecracker_pack" ),
    itype_id( "duct_tape" ),
    itype_id( "AID_bio_meteorologist" ),
    itype_id( "fake_arc_furnace" ),
    itype_id( "slingshot" ),
    itype_id( "test_boltcutter" ),
    itype_id( "crane_pseudo_item" ),
    itype_id( "wax" ),
    itype_id( "hydraulic_press_tool" ),
    itype_id( "cig" ),
    itype_id( "dynamite_bomb_act" ),
    itype_id( "talking_doll" ),
    itype_id( "gas_mask_pouch" ),
    itype_id( "id_science_maintenance_blue" ),
    itype_id( "id_science_visitor_1" ),
    itype_id( "splatter_guard" ),
    itype_id( "AID_bio_ups" ),
    itype_id( "cell_phone" ),
    itype_id( "water_mill" ),
    itype_id( "book_anthology_sunvault" ),
    itype_id( "water_wheel" ),
    itype_id( "mixer_music" ),
    itype_id( "bleach" ),
    itype_id( "36navy" ),
    itype_id( "diazepam" ),
    itype_id( "sandwich_t_wheat_free" ),
    itype_id( "usb_drive" ),
    itype_id( "escape_stone" ),
    itype_id( "water_clean" ),
    itype_id( "mustard_powder" ),
    itype_id( "gambeson_hood" ),
    itype_id( "v_planter_item_advanced" ),
    itype_id( "milk_evap" ),
    itype_id( "id_science_mutagen_green" ),
    itype_id( "almond" ),
    itype_id( "xl_water_wheel" ),
    itype_id( "fake_clay_kiln" ),
    itype_id( "red_phosphorous" ),
    itype_id( "belt40mm" ),
    itype_id( "broken_lab_security_drone_BS" ),
    itype_id( "alien_pod_resin" ),
    itype_id( "light_minus_atomic_battery_cell" ),
    itype_id( "xs_gambeson_hood" ),
    itype_id( "torch_lit" ),
    itype_id( "engineering_robotics_kit" ),
    itype_id( "bot_grenade_hack" ),
    itype_id( "hd_tow_cable" ),
    itype_id( "material_rhodonite" ),
    itype_id( "textbook_atomic_lab" ),
    itype_id( "human_cracklins" ),
    itype_id( "acid_spray" ),
    itype_id( "protein_powder" ),
    itype_id( "bio_scalpel" ),
    itype_id( "8mm_bootleg" ),
    itype_id( "hellfire_stew" ),
    itype_id( "arrow_wood_heavy" ),
    itype_id( "egg_fish" ),
    itype_id( "large_stomach_sealed" ),
    itype_id( "book_nonf_hard_dodge_tlwd" ),
    itype_id( "joint" ),
    itype_id( "meat_mutant_tainted_cooked" ),
    itype_id( "arrow_small_game_fletched" ),
    itype_id( "fake_arcfurnace" ),
    itype_id( "fake_tire_changer" ),
    itype_id( "recipe_lab_cvd" ),
    itype_id( "ch_steel_lump" ),
    itype_id( "blt_wheat_free" ),
    itype_id( "chem_slaked_lime" ),
    itype_id( "drill_press_tool" ),
    itype_id( "thermal_suit_on" ),
    itype_id( "headphones_circumaural" ),
    itype_id( "milkshake_fastfood" ),
    itype_id( "steel_chunk" ),
    itype_id( "drink_gunfire" ),
    itype_id( "art_spiral" ),
    itype_id( "conc_paralytic" ),
    itype_id( "soft_adobe_brick" ),
    itype_id( "spider_egg" ),
    itype_id( "radio_car" ),
    itype_id( "seed_sugar_beet" ),
    itype_id( "manual_stabbing" ),
    itype_id( "survival_marker" ),
    itype_id( "iodine_crystal" ),
    itype_id( "sheet_metal_lit" ),
    itype_id( "gasfilter_s" ),
    itype_id( "AID_bio_weight" ),
    itype_id( "belt50" ),
    itype_id( "polycarbonate_sheet" ),
    itype_id( "art_jelly" ),
    itype_id( "chem_hydrogen_peroxide_conc" ),
    itype_id( "sling" ),
    itype_id( "integrated_chitin" ),
    itype_id( "protein_shake" ),
    itype_id( "bacon" ),
    itype_id( "soap_holder" ),
    itype_id( "petrified_eye" ),
    itype_id( "test_weapon2" ),
    itype_id( "brew_hb_seltzer" ),
    itype_id( "broken_mech_lifter" ),
    itype_id( "bandages_makeshift_bleached" ),
    itype_id( "sig_mcx_rattler_sbr" ),
    itype_id( "soy_wheat_dough" ),
    itype_id( "billet_wood" ),
    itype_id( "meat_salted" ),
    itype_id( "sourdough_starter_uncovered" ),
    itype_id( "fake_drop_hammer" ),
    itype_id( "fake_oven" ),
    itype_id( "fetid_goop" ),
    itype_id( "egg_bird_unfert" ),
    itype_id( "tinder" ),
    itype_id( "engine_block_massive" ),
    itype_id( "rehydrated_meat" ),
    itype_id( "art_teardrop" ),
    itype_id( "carding_paddles" ),
    itype_id( "hickory_nut" ),
    itype_id( "meat" ),
    itype_id( "milkshake" ),
    itype_id( "lye_potassium" ),
    itype_id( "v12_diesel" ),
    itype_id( "chainmail_junk_feet" ),
    itype_id( "manual_knives" ),
    itype_id( "melatonin_tablet" ),
    itype_id( "plant_sac" ),
    itype_id( "doublespeargun" ),
    itype_id( "fridge_test" ),
    itype_id( "glass_shiv" ),
    itype_id( "test_liquid" ),
    itype_id( "AID_bio_watch" ),
    itype_id( "bond_410" ),
    itype_id( "gasfilter_m" ),
    itype_id( "iodine" ),
    itype_id( "torch" ),
    itype_id( "hardtack" ),
    itype_id( "water_purifier_tool" ),
    itype_id( "textbook_arthropod" ),
    itype_id( "vitamins" ),
    itype_id( "chem_match_head_powder" ),
    itype_id( "egg_salad" ),
    itype_id( "wood_panel" ),
    itype_id( "unfinished_charcoal" ),
    itype_id( "con_milk" ),
    itype_id( "venom_paralytic" ),
    itype_id( "adobe_brick" ),
    itype_id( "wrapper_foil" ),
    itype_id( "offal_pickled" ),
    itype_id( "minispeargun" ),
    itype_id( "oil_lamp_clay" ),
    itype_id( "cigar_butt" ),
    itype_id( "acetic_anhydride" ),
    itype_id( "licorice" ),
    itype_id( "melting_point" ),
    itype_id( "fake_razor" ),
    itype_id( "wash_whiskey" ),
    itype_id( "coffee_bean" ),
    itype_id( "nailbat" ),
    itype_id( "thorazine" ),
    itype_id( "grenadine_syrup" ),
    itype_id( "protein_smoothie" ),
    itype_id( "material_sand" ),
    itype_id( "gummy_vitamins" ),
    itype_id( "quiver_birchbark" ),
    itype_id( "material_limestone" ),
    itype_id( "chem_calcium_chloride" ),
    itype_id( "broken_flashbang_hack" ),
    itype_id( "cheeseburger" ),
    itype_id( "k_gambeson_vest_xs_loose" ),
    itype_id( "tramadol" ),
    itype_id( "egg_firefly" ),
    itype_id( "helmet_eod" ),
    itype_id( "car_wide_headlight" ),
    itype_id( "hc_steel_lump" ),
    itype_id( "carpentry_book" ),
    itype_id( "id_science_medical_red" ),
    itype_id( "hickory_root" ),
    itype_id( "leg" ),
    itype_id( "sheet_metal_small" ),
    itype_id( "milkshake_deluxe" ),
    itype_id( "mobile_memory_card" ),
    itype_id( "hotdogs_newyork" ),
    itype_id( "chem_sulphuric_acid" ),
    itype_id( "integrated_bark2_b" ),
    itype_id( "confit_meat" ),
    itype_id( "textbook_tailor" ),
    itype_id( "xl_k_gambeson_vest_loose" ),
    itype_id( "integrated_chitin3" ),
    itype_id( "pointy_stick_long" ),
    itype_id( "chem_phenol" ),
    itype_id( "ant_egg" ),
    itype_id( "egg_wasp" ),
    itype_id( "empty_corn_cob" ),
    itype_id( "combination_gun_shotgun" ),
    itype_id( "mask_dust" ),
    itype_id( "small_relic" ),
    itype_id( "engine_block_medium" ),
    itype_id( "lab_postit_bio" ),
    itype_id( "tanned_hide" ),
    itype_id( "reloaded_shot_00" ),
    itype_id( "test_mp3" ),
    itype_id( "FlatCoin" ),
    itype_id( "large_turbine_engine" ),
    itype_id( "radio_car_on" ),
    itype_id( "pike_wood" ),
    itype_id( "i6_diesel" ),
    itype_id( "badge_doctor" ),
    itype_id( "1cyl_combustion_small" ),
    itype_id( "AID_bio_blood_filter" ),
    itype_id( "button_wood" ),
    itype_id( "plastic_mandible_guard" ),
    itype_id( "cracklins" ),
    itype_id( "meat_canned" ),
    itype_id( "shot_00" ),
    itype_id( "pistachio" ),
    itype_id( "nachosvc" ),
    itype_id( "v6_combustion" ),
    itype_id( "mortar_build" ),
    itype_id( "inj_vitb" ),
    itype_id( "spinwheelitem" ),
    itype_id( "clay_lump" ),
    itype_id( "manual_cutting" ),
    itype_id( "pepto" ),
    itype_id( "corpse_half_beheaded" ),
    itype_id( "honey_ant" ),
    itype_id( "plut_slurry" ),
    itype_id( "toolset_extended" ),
    itype_id( "flintlock_ammo" ),
    itype_id( "broken_EMP_hack" ),
    itype_id( "animal_blood" ),
    itype_id( "shot_slug" ),
    itype_id( "1st_aid" ),
    itype_id( "migo_bio_gun" ),
    itype_id( "spear_wood" ),
    itype_id( "knitting_needles" ),
    itype_id( "cannonball_4lb" ),
    itype_id( "butter_churn" ),
    itype_id( "test_weldtank" ),
    itype_id( "8mm_civilian" ),
    itype_id( "chem_glycerol" ),
    itype_id( "xl_k_gambeson_pants" ),
    itype_id( "fake_router_table" ),
    itype_id( "test_gum" ),
    itype_id( "bread_garlic" ),
    itype_id( "feces_manure" ),
    itype_id( "ammo_satchel_leather" ),
    itype_id( "alder_bark" ),
    itype_id( "recipe_maiar" ),
    itype_id( "disassembly" ),
    itype_id( "toothbrush_electric" ),
    itype_id( "test_oxytorch" ),
    itype_id( "AID_bio_power_storage" ),
    itype_id( "catan" ),
    itype_id( "reference_firstaid1" ),
    itype_id( "AID_bio_alarm" ),
    itype_id( "test_watertight_open_sealed_multipocket_container_2x250ml" ),
    itype_id( "tourniquet_lower_XL" ),
    itype_id( "fake_woodstove" ),
    itype_id( "nicotine_liquid" ),
    itype_id( "corpse_bloody" ),
    itype_id( "ecig" ),
    itype_id( "sourdough_split_uncovered" ),
    itype_id( "manual_dodge" ),
    itype_id( "blacksmith_prototypes" ),
    itype_id( "chem_nitric_acid" ),
    itype_id( "lentils_cooked" ),
    itype_id( "halligan" ),
    itype_id( "120mm_HEAT" ),
    itype_id( "chem_acetic_acid" ),
    itype_id( "fun_survival" ),
    itype_id( "brew_vodka" ),
    itype_id( "44army" ),
    itype_id( "test_solid_1ml" ),
    itype_id( "integrated_bark2_c" ),
    itype_id( "bag_body_bag" ),
    itype_id( "lsd" ),
    itype_id( "k_gambeson_vest_loose" ),
    itype_id( "id_science_mutagen_cyan" ),
    itype_id( "fake_milling_item" ),
    itype_id( "integrated_plantskin" ),
    itype_id( "cookies_egg" ),
    itype_id( "flesh_golem_heart" ),
    itype_id( "hotdogs_newyork_wheat_free" ),
    itype_id( "mycus_fruit" ),
    itype_id( "tourniquet_upper_XL" ),
    itype_id( "k_gambeson_loose" ),
    itype_id( "makeshift_crutches" ),
    itype_id( "test_textbook_fabrication" ),
    itype_id( "bot_crows_m240" ),
    itype_id( "pecan" ),
    itype_id( "brew_marloss_wine" ),
    itype_id( "heatpack" ),
    itype_id( "oxycodone" ),
    itype_id( "scots_tailor" ),
    itype_id( "fake_smoke_plume" ),
    itype_id( "syrup" ),
    itype_id( "foodplace_food" ),
    itype_id( "brew_dandelion_wine" ),
    itype_id( "badge_deputy" ),
    itype_id( "offal_canned" ),
    itype_id( "radiocontrol" ),
    itype_id( "exodii_ingot_tin" ),
    itype_id( "meat_pickled" ),
    itype_id( "cigar" ),
    itype_id( "lab_postit_migo" ),
    itype_id( "plastic_spoon" ),
    itype_id( "jabberwock_heart" ),
    itype_id( "cup_foil" ),
    itype_id( "test_bullwhip" ),
    itype_id( "amplifier_head" ),
    itype_id( "sarcophagus_access_code" ),
    itype_id( "egg_goose_canadian" ),
    itype_id( "mutant_blood" ),
    itype_id( "8mm_fmj" ),
    itype_id( "chestnut" ),
    itype_id( "flintlock_shot" ),
    itype_id( "mobile_memory_card_used" ),
    itype_id( "xedra_microphone" ),
    itype_id( "5lgold" ),
    itype_id( "v8_diesel" ),
    itype_id( "flatbread" ),
    itype_id( "roasted_coffee_bean" ),
    itype_id( "10lbronze" ),
    itype_id( "xl_k_gambeson_vest" ),
    itype_id( "poppy_bud" ),
    itype_id( "webbasics_computer" ),
    itype_id( "test_liquid_1ml" ),
    itype_id( "mobile_memory_card_science" ),
    itype_id( "compositecrossbow" ),
    itype_id( "toothbrush_plain" ),
    itype_id( "solar_panel" ),
    itype_id( "homemade_burrito" ),
    itype_id( "egg_cockatrice" ),
    itype_id( "bot_lab_security_drone_BM" ),
    itype_id( "dog_whistle" ),
    itype_id( "training_dummy_light" ),
    itype_id( "AID_bio_armor_arms" ),
    itype_id( "bot_manhack" ),
    itype_id( "router_tool" ),
    itype_id( "heavy_load_bearing_vest_sling" ),
    itype_id( "art_beads" ),
    itype_id( "cable_instrument" ),
    itype_id( "bot_flashbang_hack" ),
    itype_id( "recon_mech_laser" ),
    itype_id( "jabberwock_heart_desiccated" ),
    itype_id( "demihuman_cooked" ),
    itype_id( "needle_wood" ),
    itype_id( "hazelnut" ),
    itype_id( "knuckle_nail" ),
    itype_id( "canister_goo" ),
    itype_id( "fake_power_tool" ),
    itype_id( "broken_lab_security_drone_YM" ),
    itype_id( "weak_antibiotic" ),
    itype_id( "heavy_steel_ballistic_plate" ),
    itype_id( "cudgel" ),
    itype_id( "test_gas" ),
    itype_id( "choc_pancakes" ),
    itype_id( "bellywrap_fur" ),
    itype_id( "drink_bloody_mary_blood" ),
    itype_id( "sandbag" ),
    itype_id( "bolt_crude" ),
    itype_id( "sweet_water" ),
    itype_id( "hammer_sledge_short" ),
    itype_id( "leathersandals" ),
    itype_id( "thermal_suit" ),
    itype_id( "flavored_bonemeal_tablet" ),
    itype_id( "wood_sheet" ),
    itype_id( "10liron" ),
    itype_id( "jointer_tool" ),
    itype_id( "demihuman_meat_scrap" ),
    itype_id( "chaw" ),
    itype_id( "solar_panel_v2" ),
    itype_id( "moisturizer" ),
    itype_id( "boulder_anvil" ),
    itype_id( "8mm_hvp" ),
    itype_id( "shot_dragon" ),
    itype_id( "load_bearing_vest" ),
    itype_id( "small_turbine_engine" ),
    itype_id( "deluxe_beansnrice" ),
    itype_id( "badge_marshal" ),
    itype_id( "broken_lab_security_drone_GR" ),
    itype_id( "corpse_generic_human" ),
    itype_id( "drivebelt_makeshift" ),
    itype_id( "loincloth_leather" ),
    itype_id( "many_years_old_newspaper" ),
    itype_id( "mutant_human_cracklins" ),
    itype_id( "shillelagh" ),
    itype_id( "ammonia_liquid" ),
    itype_id( "tainted_marrow" ),
    itype_id( "atomic_coffeepot" ),
    itype_id( "human_cooked" ),
    itype_id( "AID_bio_armor_legs" ),
    itype_id( "5liron" ),
    itype_id( "shed_stick" ),
    itype_id( "glasses_monocle" ),
    itype_id( "broken_tazer_hack" ),
    itype_id( "molasses" ),
    itype_id( "bot_antimateriel" ),
    itype_id( "brew_pine_wine" ),
    itype_id( "human_marrow" ),
    itype_id( "bot_lab_security_drone_BM2" ),
    itype_id( "book_anthology_moataatgb" ),
    itype_id( "eyedrops" ),
    itype_id( "motor_micro" ),
    itype_id( "tallow_tainted" ),
    itype_id( "hi_q_distillate" ),
    itype_id( "lens" ),
    itype_id( "AID_bio_tattoo_led" ),
    itype_id( "rosin" ),
    itype_id( "qt_steel_chunk" ),
    itype_id( "migo_bio_tech" ),
    itype_id( "material_cement" ),
    itype_id( "textbook_toxicology" ),
    itype_id( "egg_chicken" ),
    itype_id( "gasdiscount_platinum" ),
    itype_id( "aero_engine_light" ),
    itype_id( "meat_fatty_cooked" ),
    itype_id( "mutant_bug_organs" ),
    itype_id( "telepad" ),
    itype_id( "chem_turpentine" ),
    itype_id( "reloaded_410shot_000" ),
    itype_id( "weed" ),
    itype_id( "manual_melee" ),
    itype_id( "waterskin3" ),
    itype_id( "fuse" ),
    itype_id( "k_gambeson_vest" ),
    itype_id( "stuffed_clams" ),
    itype_id( "meat_scrap" ),
    itype_id( "puller" ),
    itype_id( "testcomest" ),
    itype_id( "acid_sniper" ),
    itype_id( "deluxe_eggs" ),
    itype_id( "reloaded_shot_dragon" ),
    itype_id( "AID_bio_trickle" ),
    itype_id( "caff_gum" ),
    itype_id( "altered_stopwatch" ),
    itype_id( "artificial_sweetener" ),
    itype_id( "cattlefodder" ),
    itype_id( "cig_butt" ),
    itype_id( "blt" ),
    itype_id( "brew_mycus_wine" ),
    itype_id( "integrated_chitin2" ),
    itype_id( "chicory_coffee" ),
    itype_id( "pickled_egg" ),
    itype_id( "AID_bio_power_armor_interface" ),
    itype_id( "juice_pulp" ),
    itype_id( "peanutbutter" ),
    itype_id( "fake_char_smoker" ),
    itype_id( "fake_stove" ),
    itype_id( "ch_steel_chunk" ),
    itype_id( "lens_small" ),
    itype_id( "bp_shot_flechette" ),
    itype_id( "peanut" ),
    itype_id( "royal_jelly" ),
    itype_id( "sheet_leather_patchwork" ),
    itype_id( "cattail_jelly" ),
    itype_id( "fake_char_kiln" ),
    itype_id( "manual_swords" ),
    itype_id( "salt_water" ),
    itype_id( "xl_k_gambeson" ),
    itype_id( "manual_rifle" ),
    itype_id( "meat_cooked" ),
    itype_id( "seasoning_italian" ),
    itype_id( "crossbow" ),
    itype_id( "qt_steel_lump" ),
    itype_id( "belt223" ),
    itype_id( "egg_antlion" ),
    itype_id( "corpse_generic_male" ),
    itype_id( "5llead" ),
    itype_id( "cookies" ),
    itype_id( "fake_butcher_rack" ),
    itype_id( "spatula" ),
    itype_id( "RAM" ),
    itype_id( "wastebread" ),
    itype_id( "test_robofac_armor_rig" ),
    itype_id( "story_book" ),
    itype_id( "AID_bio_heatsink" ),
    itype_id( "offal_cooked" ),
    itype_id( "radio_repeater_mod" ),
    itype_id( "air_compressor_tool" ),
    itype_id( "lc_steel_chunk" ),
    itype_id( "guidebook" ),
    itype_id( "engine_block_tiny" ),
    itype_id( "engine_block_large" ),
    itype_id( "gatling_mech_laser" ),
    itype_id( "protein_shake_fortified" ),
    itype_id( "410shot_000" ),
    itype_id( "gasfilter_l" ),
    itype_id( "bot_c4_hack" ),
    itype_id( "art_snake" ),
    itype_id( "chem_rocket_fuel" ),
    itype_id( "shot_scrap" ),
    itype_id( "test_bitter_almond" ),
    itype_id( "press" ),
    itype_id( "leech_flower" ),
    itype_id( "curry_powder" ),
    itype_id( "beartrap" ),
    itype_id( "candy4" ),
    itype_id( "unfinished_cac2" ),
    itype_id( "arrow_plastic" ),
    itype_id( "lab_postit_portal" ),
    itype_id( "reloaded_shot_slug" ),
    itype_id( "broken_grenade_hack" ),
    itype_id( "shot_he" ),
    itype_id( "tindalos_whistle" ),
    itype_id( "ammonia_hydroxide" ),
    itype_id( "XL_holster" ),
    itype_id( "xs_tacvest" ),
    itype_id( "bat" ),
    itype_id( "egg_grasshopper" ),
    itype_id( "xl_gambeson_hood" ),
    itype_id( "corpse_child_gunned" ),
    itype_id( "plastic_spoon_kids" ),
    itype_id( "shoes_birchbark" ),
    itype_id( "toothbrush_dirty" ),
    itype_id( "test_hacksaw_elec" ),
    itype_id( "signal_flare" ),
    itype_id( "water_sewage" ),
    itype_id( "pouch_autoclave" ),
    itype_id( "mutant_marrow" ),
    itype_id( "reference_firstaid2" ),
    itype_id( "glowstick_lit" ),
    itype_id( "lye" ),
    itype_id( "walnut" ),
    itype_id( "anvil" ),
    itype_id( "chaps_leather" ),
    itype_id( "hardtack_pudding" ),
    itype_id( "recipe_serum" ),
    itype_id( "fetus" ),
    itype_id( "makeshift_hose" ),
    itype_id( "cheeseburger_wheat_free" ),
    itype_id( "hi_q_wax" ),
    itype_id( "billet_bone" ),
    itype_id( "tourniquet_upper" ),
    itype_id( "10llead" ),
    itype_id( "architect_cube" ),
    itype_id( "firehelmet" ),
    itype_id( "smart_phone_flashlight" ),
    itype_id( "wild_herbs" ),
    itype_id( "fake_burrowing" ),
    itype_id( "bread_wheat_free" ),
    itype_id( "id_science_security_magenta" ),
    itype_id( "base_plastic_silverware" ),
    itype_id( "chopsticks" ),
    itype_id( "mannequin_decoy" ),
    itype_id( "corpse_generic_female" ),
    itype_id( "demihuman_cracklins" ),
    itype_id( "bp_shot_bird" ),
    itype_id( "test_hallu_nutmeg" ),
    itype_id( "years_old_newspaper" ),
    itype_id( "plut_slurry_dense" ),
    itype_id( "AID_bio_shotgun" ),
    itype_id( "creepy_doll" ),
    itype_id( "condensor_coil" ),
    itype_id( "10lgold" ),
    itype_id( "waterskin2" ),
    itype_id( "disinfectant" ),
    itype_id( "helmet_eod_on" ),
    itype_id( "oil_lamp_clay_on" ),
    itype_id( "AID_bio_magnet" ),
    itype_id( "heavy_lathe_tool" ),
    itype_id( "corpse_halved_upper" ),
    itype_id( "cranberries" ),
    itype_id( "robofac_armor_rig" ),
    itype_id( "sweet_milk_fortified" ),
    itype_id( "chem_muriatic_acid" ),
    itype_id( "pinecone" ),
    itype_id( "reloaded_shot_bird" ),
    itype_id( "purifier_smart_shot" ),
    itype_id( "alcohol_wipes" ),
    itype_id( "colander_steel" ),
    itype_id( "porkbelly" ),
    itype_id( "huge_atomic_battery_cell" ),
    itype_id( "1cyl_combustion" ),
    itype_id( "airhorn" ),
    itype_id( "spread_peanutbutter" ),
    itype_id( "kevlar_chainmail_hauberk_xs" ),
    itype_id( "corpse_scorched" ),
    itype_id( "mortar_adobe" ),
    itype_id( "chilidogs_wheat_free" ),
    itype_id( "deviled_kidney" ),
    itype_id( "egg_lady_bug_giant" ),
    itype_id( "v_plow_item" ),
    itype_id( "cinnamon" ),
    itype_id( "meat_mutant_tainted_smoked" ),
    itype_id( "fake_power_lathe" ),
    itype_id( "scutcheritem" ),
    itype_id( "wallet_travel" ),
    itype_id( "demihuman_flesh" ),
    itype_id( "barb_paralysis" ),
    itype_id( "quiver_large_birchbark" ),
    itype_id( "arrow_exploding" ),
    itype_id( "fried_brain" ),
    itype_id( "chem_baking_soda" ),
    itype_id( "ph_meter" ),
    itype_id( "data_card" ),
    itype_id( "recipe_mil_augs" ),
    itype_id( "soap_flakes" ),
    itype_id( "fridge" ),
    itype_id( "recipe_creepy" ),
    itype_id( "soapy_water" ),
    itype_id( "camera_control" ),
    itype_id( "material_quicklime" ),
    itype_id( "kevlar_chainmail_hauberk" ),
    itype_id( "art_urchin" ),
    itype_id( "meat_fried" ),
    itype_id( "k_gambeson" ),
    itype_id( "test_waterproof_bag" ),
    itype_id( "fake_goggles" ),
    itype_id( "leather" ),
    itype_id( "c4armed" ),
    itype_id( "camphine" ),
    itype_id( "bot_EMP_hack" ),
    itype_id( "rocuronium" )
};

static bool assert_maximum_density_for_material( const item &target )
{
    if( to_milliliter( target.volume() ) == 0 ) {
        return false;
    }
    const std::map<material_id, int> mats = target.made_of();
    if( !mats.empty() && known_bad.count( target.typeId() ) == 0 ) {

        double item_density = static_cast<double>( to_gram( target.weight() ) ) / static_cast<double>
                              ( to_milliliter( target.volume() ) );
        double max_density = 0;
        for( const auto &m : mats ) {
            // this test will NOT pass right now so for now check but allow failing
            max_density += m.first.obj().density() * m.second / target.type->mat_portion_total;
        }
        INFO( target.type_name() );
        CHECK( item_density <= max_density );

        return item_density > max_density;
    }

    // fallback return
    return false;
}

TEST_CASE( "item_material_density_sanity_check", "[item]" )
{
    // randomize items so you get varied failures when testing densities
    std::vector<const itype *> all_items = item_controller->all();
    std::shuffle( std::begin( all_items ), std::end( all_items ), rng_get_engine() );

    // only allow so many failures before stopping
    int number_of_failures = 0;

    for( const itype *type : all_items ) {
        const item sample( type, calendar::turn_zero, item::solitary_tag{} );
        if( assert_maximum_density_for_material( sample ) ) {
            number_of_failures++;
            if( number_of_failures > 20 ) {
                break;
            }
        }
    }
}

TEST_CASE( "armor_entry_consolidate_check", "[item][armor]" )
{
    item test_consolidate( "test_consolidate" );

    //check this item has a single armor entry, not 3 like is written in the json explicitly

    CHECK( test_consolidate.find_armor_data()->sub_data.size() == 1 );
}

TEST_CASE( "rigid_armor_compliance", "[item][armor]" )
{
    avatar &guy = get_avatar();
    clear_avatar();
    // check if you can swap a rigid armor
    item test_armguard( "test_armguard" );
    REQUIRE( guy.wield( test_armguard ) );

    REQUIRE( guy.wear( guy.used_weapon(), false ) );

    CHECK( guy.worn.top_items_loc( guy ).front().get_item()->get_side() == side::LEFT );

    guy.change_side( *guy.worn.top_items_loc( guy ).front().get_item() );

    CHECK( guy.worn.top_items_loc( guy ).front().get_item()->get_side() == side::RIGHT );


    // check if you can't wear 3 rigid armors
    clear_avatar();

    item first_test_armguard( "test_armguard" );
    REQUIRE( guy.wield( first_test_armguard ) );
    REQUIRE( guy.wear( guy.used_weapon(), false ) );

    item second_test_armguard( "test_armguard" );
    REQUIRE( guy.wield( second_test_armguard ) );
    REQUIRE( guy.wear( guy.used_weapon(), false ) );

    item third_test_armguard( "test_armguard" );
    REQUIRE( guy.wield( third_test_armguard ) );
    REQUIRE( !guy.wear( guy.used_weapon(), false ) );
}

TEST_CASE( "rigid_splint_compliance", "[item][armor]" )
{
    avatar &guy = get_avatar();
    clear_avatar();

    item test_armguard( "test_armguard" );
    item second_test_armguard( "test_armguard" );
    item splint( "arm_splint" );
    item second_splint( "arm_splint" );
    item third_splint( "arm_splint" );

    // check if you can wear a splint
    clear_avatar();

    guy.set_part_hp_cur( bodypart_id( "arm_r" ), 0 );
    REQUIRE( guy.wield( splint ) );
    REQUIRE( guy.wear( guy.used_weapon(), false ) );

    // check if you cannot wear a splint if something rigid is on that arm
    clear_avatar();

    REQUIRE( guy.wield( test_armguard ) );
    guy.set_part_hp_cur( bodypart_id( "arm_r" ), 0 );
    REQUIRE( guy.wear( guy.used_weapon(), false ) );

    CHECK( guy.worn.top_items_loc( guy ).front().get_item()->get_side() == side::LEFT );
    // swap side to the broken arm side
    guy.change_side( *guy.worn.top_items_loc( guy ).front().get_item() );
    // should fail to wear
    REQUIRE( guy.wield( second_splint ) );
    REQUIRE( !guy.wear( guy.used_weapon(), false ) );

    // check if you can wear a splint if nothing rigid is on that arm
    clear_avatar();

    REQUIRE( guy.wield( second_test_armguard ) );
    guy.set_part_hp_cur( bodypart_id( "arm_r" ), 0 );
    REQUIRE( guy.wear( guy.used_weapon(), false ) );

    CHECK( guy.worn.top_items_loc( guy ).front().get_item()->get_side() == side::LEFT );

    // should be able to wear the arm is open
    REQUIRE( guy.wield( third_splint ) );
    REQUIRE( guy.wear( guy.used_weapon(), false ) );
}
