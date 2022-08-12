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
static const flag_id json_flag_FIX_NEARSIGHT( "FIX_NEARSIGHT" );
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
    itype_id( "bread" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wrapped_rad_badge" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "8mm_inc" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fire_drill" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meal_chitin_piece" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "blood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "curry_meat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "speargun" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "seasoning_salt" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "smart_phone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "delay_fuze" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "apple_canned" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_mech_recon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_nickel_powder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lc_steel_lump" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "v_table" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mc_steel_chunk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "drink_karsk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fish_bowl" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "leather_filing" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_sake_koji" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "artifact_slow_aura" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "coffee_raw" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_roach" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "v_rockwheel_item" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_knot" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "glassblowing_book" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sushi_veggyroll" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sugar_fried" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "donut_holes" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "family_cookbook" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "coffee_raw_kentucky" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bandages_makeshift" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "candy5" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "baton" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "homeopathic_pills" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tool_belt" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "marloss_berry" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_lab_security_drone_YM" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_disc" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "comb_pocket" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_flashlight" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "loom_frame" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "raw_curing_fatty_meat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_bird" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_napkin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "light_minus_battery_cell" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shot_beanbag" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "RPG-7_tbg7v" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_syringe" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "human_flesh" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "caramel_apple" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_generic_boy" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "milkshake_deluxe_choc" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "extinguishing_agent" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dayquil" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pine_tea" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pointy_stick" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soysauce" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "johnnycake" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plastic_knife" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "demihuman_blood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_gasbomb_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sandwich_reuben_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "book_anthology_an" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_patchskin1" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tailoring_pattern_set" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cell_phone_flashlight" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_bug_organs_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "q_staff" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "book_anthology_bk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "8mm_jhp" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "scorecard" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pepper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "budget_steel_lump" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "young_koji" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "k_gambeson_vest_xs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "candy2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_meat_scrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "acid_soaked_hide" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dragonskin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cuvettes" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mp3_on" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_cracklins" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_pheasant" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "venom_wasp" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_sphere" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "newest_newspaper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bandages_makeshift_boiled" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cooked_marrow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fruit_pancakes" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "folding_solar_panel_v2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_hacksaw" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tablesaw_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_mech_combat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "awl_bone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "beer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_hexamine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_raven" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sandwich_sauce_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_human_flesh" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "adobe_pallet_full" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "ruined_chunks" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "formic_acid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soaking_dandelion" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_smoked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bolt_simple_wood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shot_bird" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "vaccine_shot" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sugar" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "adderall" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fire_drill_large" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "milkshake_choc" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "blood_pancakes_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bow_sight" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_fireplace" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mycus_juice" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_rum" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cash_card" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mp3" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_security_black" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "punch_bone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manual_launcher" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "medium_surface_pseudo" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "freezer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bolt_explosive" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xanax" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hot_sauce" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "yeast" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "alloy_plate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "candy3" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "steel_lump" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "kevlar_chainmail_suit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bp_shot_scrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "a180mag4" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bolt_makeshift" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "liquid_soap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_aluminium_sulphate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "antibiotics" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dynamite" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_lab_security_drone_GR" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "parkour_practice" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_centipede" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "flatbread_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_roach_plague" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "butchery_rack_pseudo" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "birchbark" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "material_zincite" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_mantis" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "small_lcd_screen" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_child_calm" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "biscuit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "2h_flail_wood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_kevlar_chainmail_hauberk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "koji" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "light_minus_disposable_cell" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mobile_memory_card_encrypted" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fire_piston" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_c4_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "panacea" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "artifact_teleportitis_aura" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bandsaw_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_acetone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bp_shot_dragon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_medicalmut" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hotdogs_cooked_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manhole_cover" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "oatmeal_deluxe" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "demihuman_marrow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "drill_rock_primitive" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_industrial" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "drink_queenmary" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "planer_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sandwich_reuben" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "handrolled_cig" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "vortex" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wasp_sting" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_generic_girl" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pipe_tobacco" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_grouse" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "carbon_electrode" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hotdogs_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "candy" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fan" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "scrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "theater_props" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "poppysyrup" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "calcium_tablet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_lab_security_drone_BM2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_lab_security_drone_BS" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "book_anthology_sst" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_dragonfly" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "protein_drink" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "badge_cybercop" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_maintenance_yellow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_labchem" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_gambeson_sleeve" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_rod" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dieselpunk_tailor" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_mutagen_pink" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_artisan_member" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "large_surface_pseudo" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "deployment_bag" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lab_postit_blob" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_ferric_chloride" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "venom_bee" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_rifleturret" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "weeks_old_newspaper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "young_koji_done" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "adrenaline_injector" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "transponder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_weapon1" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_tacvest" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plastic_straw" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "k_gambeson_pants" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_stabbed" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "torch_done" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_patchskin2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_crescent" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soup_blood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tourniquet_lower" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "digging_stick" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bone_marrow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cable_speaker" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "conc_venom" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plastics_book" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_lamp" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "robofac_yrax_deactivation_manual" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cornbread" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hoboreel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "one_year_old_newspaper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "budget_steel_chunk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tanbark" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dog_whistle_wood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tailor_japanese" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dandelionburdock_tea" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "craft" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broom" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "arm" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_appliance_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "light_atomic_battery_cell" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "frozen_lemonade" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "glowstick_dead" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "rmi2_corpse" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "milk_fortified" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "coin_gold" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cable_xlr" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "prophylactic_antivenom" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mannequin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soy_wheat_dough_done" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "towel_hanger" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_maintenance_green" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_gin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_pointy_stick" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tobacco" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cutting_board" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_hb_beer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_gunned" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_railgun" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lab_postit_tech" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tactical_shotshell_pouch" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_caseless" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mortar_lime" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "nectar" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "necronomicon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pizza_supreme" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "66mm_HEAT" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_thermite" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "motorcycle_headlight" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "altered_rubicks_cube" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hi_q_shatter" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_mole_cricket" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meal_bone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_generic_gunned" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bellywrap_leather" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "survivor_scope" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "triffid_fungicide" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pine_resin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "folding_solar_panel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "badge_swat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "can_peach" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bowling_pin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_firestarter" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_stag_beetle" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "rehydration_drink" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bee_sting" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_cracklins" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "iotv_shoulder_plate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "permanent_marker" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "kilt_leather" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "flu_shot" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "k_gambeson_xs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "arcfurnace" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "maple_sap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "a180mag3" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_washing_soda" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "peanutbutter_imitation" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "casting_mold" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "book_anthology_htloop" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "arrow_heavy_fire_hardened_fletched" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dry_beans" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "button_bronze" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bacon_uncut" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tÖttchen" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bp_shot_00" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "clogs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "feces_cow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hand_crank_charger" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dynamite_act" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bonemeal_tablet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xs_gambeson_sleeve" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "circuit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "blood_pancakes" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gambeson_sleeve" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "rolling_pin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "scrambledeggsandbrain" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sewage" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_load_bearing_vest" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sauerkraut_onions" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cow_candy" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soggy_hardtack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "antarvasa" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "needle_bone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "concrete" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "toolset" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "rigid_plastic_sheet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_formaldehyde" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sandwich_deluxe" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "robofac_test_data" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "snail_garden" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "irradiated_cranberries" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hallula" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_chloroform" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soaked_dandelion" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "coffee_syrup" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_strider" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_lab_security_drone_GM" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fertilizer_liquid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "neccowafers" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "acid_spit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_painful" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "offal" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "strong_antibiotic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gelatin_powder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mc_steel_lump" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dogfood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bullwhip" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "book_nonf_hard_homemk_grtrms" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chunk_sulfur" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_fauxfur" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "water" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "freeze_dried_meal_hydrated" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "evaporator_coil" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "resin_chunk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "scrap_aluminum" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hops" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "crackpipe" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pumpkin_yeast_bread" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bandages" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shot_flechette" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dried_salad" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_geiger" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "8mm_caseless" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sandwich_sauce" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lung_provence" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "portal" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dandelion_tea" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_security_yellow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "scots_cookbook" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "primitive_hammer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "water_mineral" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_beverly_shear" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "badge_detective" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bamboo_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "throwing_stick" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cookbook_sushi" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "oil_press_mounted" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_kevlar_chainmail_suit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_military" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "willowbark" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "young_yeast" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_boltcutter_elec" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bo" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "months_old_newspaper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_manhack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "slime_scrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bagh_nakha" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_bark" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_transport_1" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tacvest" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chilidogs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "coin_half_dollar" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "glowstick" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_patchskin3" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_augs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_bark2_a" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_lab_elec" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pumpkin_muffins" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_soporific" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_smart_phone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "iotv_groin_plate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "coin_silver" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_tube" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "codeine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "textbook_extraction" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_crystal" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_vinegar" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_scrap_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "salt" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_human_blood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tactical_grenade_pouch" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hc_steel_chunk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "marloss_gel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "inj_iron" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "badge_foodkid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gelatin_extracted" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "nyquil" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "washboard" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "nitrox" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "steel_ballistic_plate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wood_plate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "nailgun" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "note_mutant_alpha_boss" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "prozac" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cocaine_topical" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_climate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "feces_dog" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "butchery_tree_pseudo" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "folding_poncho" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tortilla_corn" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_gasbomb_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meal_bone_tainted" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hackerman_computer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tarp_raincatcher" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "microphone_xlr_generic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "load_bearing_vest_sling" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fish_bait" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gasdiscount_gold" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "reloaded_shot_flechette" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "primitive_knife" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_pyramid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_blood_anal" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "w_table" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_k_gambeson_loose" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mre_accessory" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "textbook_computer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "candy3gator" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "kevlar_chainmail_suit_xs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bolt_simple_small_game" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "raw_bamboo" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "arrow_fire_hardened_fletched" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_human_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plastic_fork" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_lady_bug" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_whiskey" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "human_meat_scrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manual_bashing" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "nic_gum" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_power_storage_mkII" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "saline" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_antimony_trichloride" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hammer_sledge_heavy" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_meat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plutonium" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_lab_security_drone_GM" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_meat_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_DMSO" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_lab_security_drone_BM" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hard_plate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "waterskin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "joint_roach" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sandwich_deluxe_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "base_toiletries" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_locust" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "glass_tinted" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "stomach_sealed" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sake_koji_rice" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "e_scrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "staff_sling" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "heavy_load_bearing_vest_breacher" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "usb_drive_nano" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_hydrogen_peroxide" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_animal" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "impact_fuze" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "board_trap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chestpouch_leather" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tourniquet_upper_XS" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "disinfectant_makeshift" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "choco_coffee_beans" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "boiled_egg" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sourdough_young_uncovered" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fishing_rod_basic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mre_dessert" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "reference_cooking" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hairbrush" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "marloss_seed" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_meat_scrap_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sandwich_t" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_human_marrow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gasdiscount_silver" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mitresaw_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soup_blood_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dynamite_bomb" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_oldwoman_jewelry" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_waterskin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_tazer_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_atomic_battery" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gloves_medical" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "stab_plate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gum" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "smartphone_music" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "c4" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recon_mech_laser_single" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_mutant_tainted" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bp_shot_slug" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gatling_mech_laser_single" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chilly-p" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "survnote" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "book_anthology_cw" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_burdock_wine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "firecracker_pack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "duct_tape" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_meteorologist" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_arc_furnace" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "slingshot" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_boltcutter" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "crane_pseudo_item" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wax" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hydraulic_press_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cig" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dynamite_bomb_act" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "talking_doll" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gas_mask_pouch" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_maintenance_blue" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_visitor_1" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "splatter_guard" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_ups" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cell_phone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "water_mill" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "book_anthology_sunvault" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "water_wheel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mixer_music" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bleach" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "36navy" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "diazepam" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sandwich_t_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "usb_drive" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "escape_stone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "water_clean" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mustard_powder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gambeson_hood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "v_planter_item_advanced" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "milk_evap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_mutagen_green" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "almond" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_water_wheel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_clay_kiln" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "red_phosphorous" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "belt40mm" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_lab_security_drone_BS" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "alien_pod_resin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "light_minus_atomic_battery_cell" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xs_gambeson_hood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "torch_lit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "engineering_robotics_kit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_grenade_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hd_tow_cable" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "material_rhodonite" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "textbook_atomic_lab" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "human_cracklins" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "acid_spray" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "protein_powder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bio_scalpel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "8mm_bootleg" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hellfire_stew" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "arrow_wood_heavy" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_fish" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "large_stomach_sealed" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "book_nonf_hard_dodge_tlwd" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "joint" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_mutant_tainted_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "arrow_small_game_fletched" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_arcfurnace" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_tire_changer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_lab_cvd" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "ch_steel_lump" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "blt_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_slaked_lime" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "drill_press_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "thermal_suit_on" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "headphones_circumaural" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "milkshake_fastfood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "steel_chunk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "drink_gunfire" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_spiral" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "conc_paralytic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soft_adobe_brick" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "spider_egg" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "radio_car" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "seed_sugar_beet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manual_stabbing" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "survival_marker" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "iodine_crystal" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sheet_metal_lit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gasfilter_s" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_weight" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "polycarbonate_sheet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_jelly" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_hydrogen_peroxide_conc" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sling" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_chitin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "protein_shake" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bacon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soap_holder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "petrified_eye" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_weapon2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_hb_seltzer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_mech_lifter" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bandages_makeshift_bleached" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sig_mcx_rattler_sbr" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soy_wheat_dough" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "billet_wood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_salted" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sourdough_starter_uncovered" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_drop_hammer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_oven" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fetid_goop" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_bird_unfert" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tinder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "rehydrated_meat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_teardrop" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "carding_paddles" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hickory_nut" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "milkshake" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lye_potassium" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chainmail_junk_feet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manual_knives" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "melatonin_tablet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plant_sac" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "doublespeargun" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fridge_test" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "glass_shiv" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_liquid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_watch" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bond_410" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gasfilter_m" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "iodine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "torch" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hardtack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "water_purifier_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "textbook_arthropod" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "vitamins" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_match_head_powder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_salad" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wood_panel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "unfinished_charcoal" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "con_milk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "venom_paralytic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "adobe_brick" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wrapper_foil" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "offal_pickled" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "minispeargun" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "oil_lamp_clay" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cigar_butt" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "acetic_anhydride" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "licorice" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "melting_point" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_razor" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wash_whiskey" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "coffee_bean" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "nailbat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "thorazine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "grenadine_syrup" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "protein_smoothie" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "material_sand" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gummy_vitamins" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "quiver_birchbark" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_calcium_chloride" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_flashbang_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cheeseburger" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "k_gambeson_vest_xs_loose" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tramadol" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_firefly" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "helmet_eod" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "car_wide_headlight" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hc_steel_lump" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "carpentry_book" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_medical_red" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hickory_root" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "leg" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sheet_metal_small" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "milkshake_deluxe" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mobile_memory_card" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hotdogs_newyork" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_sulphuric_acid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_bark2_b" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "confit_meat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "textbook_tailor" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_k_gambeson_vest_loose" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_chitin3" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pointy_stick_long" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_phenol" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "ant_egg" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_wasp" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "empty_corn_cob" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "combination_gun_shotgun" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mask_dust" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "small_relic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lab_postit_bio" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tanned_hide" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "reloaded_shot_00" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_mp3" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "FlatCoin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "radio_car_on" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pike_wood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "badge_doctor" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_blood_filter" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "button_wood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plastic_mandible_guard" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cracklins" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_canned" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shot_00" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pistachio" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "nachosvc" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mortar_build" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "inj_vitb" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "spinwheelitem" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "clay_lump" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manual_cutting" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pepto" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_half_beheaded" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "honey_ant" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plut_slurry" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "toolset_extended" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "flintlock_ammo" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_EMP_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "animal_blood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shot_slug" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "1st_aid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "migo_bio_gun" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "spear_wood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "knitting_needles" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cannonball_4lb" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "butter_churn" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_weldtank" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "8mm_civilian" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_glycerol" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_k_gambeson_pants" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_router_table" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_gum" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bread_garlic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "feces_manure" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "ammo_satchel_leather" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "alder_bark" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_maiar" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "disassembly" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "toothbrush_electric" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_oxytorch" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_power_storage" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "catan" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "reference_firstaid1" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_alarm" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_watertight_open_sealed_multipocket_container_2x250ml" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tourniquet_lower_XL" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_woodstove" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "nicotine_liquid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_bloody" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "ecig" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sourdough_split_uncovered" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manual_dodge" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "blacksmith_prototypes" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_nitric_acid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lentils_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "halligan" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "120mm_HEAT" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_acetic_acid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fun_survival" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_vodka" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "44army" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_solid_1ml" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_bark2_c" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bag_body_bag" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lsd" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "k_gambeson_vest_loose" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_mutagen_cyan" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_milling_item" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_plantskin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cookies_egg" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "flesh_golem_heart" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hotdogs_newyork_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mycus_fruit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tourniquet_upper_XL" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "k_gambeson_loose" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "makeshift_crutches" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_textbook_fabrication" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_crows_m240" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pecan" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_marloss_wine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "heatpack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "oxycodone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "scots_tailor" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_smoke_plume" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "syrup" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "foodplace_food" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_dandelion_wine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "badge_deputy" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "offal_canned" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "radiocontrol" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "exodii_ingot_tin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_pickled" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cigar" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lab_postit_migo" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plastic_spoon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "jabberwock_heart" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cup_foil" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_bullwhip" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "amplifier_head" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sarcophagus_access_code" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_goose_canadian" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_blood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "8mm_fmj" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chestnut" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "flintlock_shot" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mobile_memory_card_used" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xedra_microphone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "flatbread" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "roasted_coffee_bean" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "10lbronze" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_k_gambeson_vest" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "poppy_bud" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "webbasics_computer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_liquid_1ml" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mobile_memory_card_science" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "compositecrossbow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "toothbrush_plain" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "solar_panel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "homemade_burrito" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_cockatrice" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_lab_security_drone_BM" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dog_whistle" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "training_dummy_light" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_armor_arms" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_manhack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "router_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "heavy_load_bearing_vest_sling" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_beads" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cable_instrument" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_flashbang_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recon_mech_laser" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "jabberwock_heart_desiccated" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "demihuman_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "needle_wood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hazelnut" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "knuckle_nail" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "canister_goo" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_power_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_lab_security_drone_YM" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "weak_antibiotic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "heavy_steel_ballistic_plate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cudgel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_gas" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "choc_pancakes" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bellywrap_fur" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "drink_bloody_mary_blood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sandbag" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bolt_crude" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sweet_water" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hammer_sledge_short" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "leathersandals" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "thermal_suit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "flavored_bonemeal_tablet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wood_sheet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "10liron" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "jointer_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "demihuman_meat_scrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chaw" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "solar_panel_v2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "moisturizer" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "boulder_anvil" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "8mm_hvp" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shot_dragon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "load_bearing_vest" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "deluxe_beansnrice" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "badge_marshal" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_lab_security_drone_GR" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_generic_human" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "drivebelt_makeshift" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "loincloth_leather" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "many_years_old_newspaper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_human_cracklins" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shillelagh" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "ammonia_liquid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tainted_marrow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "atomic_coffeepot" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "human_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_armor_legs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "5liron" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shed_stick" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "glasses_monocle" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_tazer_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "molasses" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_antimateriel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_pine_wine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "human_marrow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_lab_security_drone_BM2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "book_anthology_moataatgb" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "eyedrops" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "motor_micro" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tallow_tainted" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hi_q_distillate" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lens" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_tattoo_led" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "rosin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "qt_steel_chunk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "migo_bio_tech" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "material_cement" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "textbook_toxicology" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_chicken" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gasdiscount_platinum" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_fatty_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_bug_organs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "telepad" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_turpentine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "reloaded_410shot_000" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "weed" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manual_melee" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "waterskin3" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fuse" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "k_gambeson_vest" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "stuffed_clams" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_scrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "puller" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "testcomest" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "acid_sniper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "deluxe_eggs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "reloaded_shot_dragon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_trickle" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "caff_gum" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "altered_stopwatch" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "artificial_sweetener" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cattlefodder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cig_butt" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "blt" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "brew_mycus_wine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "integrated_chitin2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chicory_coffee" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pickled_egg" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_power_armor_interface" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "juice_pulp" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "peanutbutter" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_char_smoker" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_stove" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "ch_steel_chunk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lens_small" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bp_shot_flechette" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "peanut" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "royal_jelly" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sheet_leather_patchwork" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cattail_jelly" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_char_kiln" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manual_swords" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "salt_water" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_k_gambeson" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "manual_rifle" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "seasoning_italian" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "crossbow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "qt_steel_lump" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_antlion" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_generic_male" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "5llead" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cookies" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_butcher_rack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "spatula" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "RAM" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wastebread" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_robofac_armor_rig" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "story_book" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_heatsink" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "offal_cooked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "radio_repeater_mod" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "air_compressor_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lc_steel_chunk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "guidebook" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gatling_mech_laser" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "protein_shake_fortified" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "410shot_000" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "gasfilter_l" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_c4_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_snake" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_rocket_fuel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shot_scrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_bitter_almond" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "press" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "leech_flower" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "curry_powder" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "beartrap" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "candy4" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "unfinished_cac2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "arrow_plastic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lab_postit_portal" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "reloaded_shot_slug" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "broken_grenade_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shot_he" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tindalos_whistle" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "ammonia_hydroxide" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "XL_holster" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xs_tacvest" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_grasshopper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "xl_gambeson_hood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_child_gunned" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plastic_spoon_kids" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "shoes_birchbark" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "toothbrush_dirty" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_hacksaw_elec" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "signal_flare" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "water_sewage" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pouch_autoclave" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mutant_marrow" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "reference_firstaid2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "glowstick_lit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lye" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "walnut" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "anvil" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chaps_leather" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hardtack_pudding" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_serum" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fetus" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "makeshift_hose" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cheeseburger_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "hi_q_wax" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "billet_bone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tourniquet_upper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "10llead" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "architect_cube" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "firehelmet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "smart_phone_flashlight" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wild_herbs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_burrowing" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bread_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "id_science_security_magenta" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "base_plastic_silverware" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chopsticks" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mannequin_decoy" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_generic_female" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "demihuman_cracklins" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bp_shot_bird" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_hallu_nutmeg" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "years_old_newspaper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "plut_slurry_dense" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_shotgun" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "creepy_doll" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "condensor_coil" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "waterskin2" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "disinfectant" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "helmet_eod_on" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "oil_lamp_clay_on" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "AID_bio_magnet" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "heavy_lathe_tool" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_halved_upper" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cranberries" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "robofac_armor_rig" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sweet_milk_fortified" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_muriatic_acid" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "pinecone" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "reloaded_shot_bird" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "purifier_smart_shot" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "alcohol_wipes" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "colander_steel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "porkbelly" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "huge_atomic_battery_cell" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "airhorn" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "spread_peanutbutter" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "kevlar_chainmail_hauberk_xs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "corpse_scorched" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mortar_adobe" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chilidogs_wheat_free" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "deviled_kidney" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_lady_bug_giant" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "v_plow_item" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cinnamon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_mutant_tainted_smoked" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_power_lathe" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "scutcheritem" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wallet_travel" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "demihuman_flesh" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "barb_paralysis" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "quiver_large_birchbark" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "arrow_exploding" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fried_brain" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "chem_baking_soda" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "ph_meter" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "data_card" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_mil_augs" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soap_flakes" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fridge" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "recipe_creepy" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "soapy_water" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "camera_control" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "material_quicklime" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "kevlar_chainmail_hauberk" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "art_urchin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_fried" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "k_gambeson" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "test_waterproof_bag" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fake_goggles" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "leather" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "c4armed" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "camphine" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "bot_EMP_hack" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "magical_throwing_knife_cut" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "rune_biomancer_weapon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "fireproof_mortar" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "animist_spirits" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "q_staff_plus_one" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mtorch_everburning" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "tainted_blood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "techno_basic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sledge_heavy_plus_one" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wizard_advanced" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "egg_owlbear" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "summon_scroll_smudged" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wolfshead_cufflinks" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "magic_armormaking" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "black_dragons" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mtorch_everburning_lit" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "dragon_blood" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "black_dragons_historical" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "necro_basic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "magical_throwing_knife" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "magical_throwing_knife_cold" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "magical_throwing_knife_fire" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "magical_throwing_knife_biological" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "meat_dragon" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "metal_legends" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "flamesword" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "purified_meat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "sledge_heavy_plus_two" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lizardfolk_javelin" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cooking_poison" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "wizard_beginner" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "alchemy_basic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "mkey_opening" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "enchantment_basic" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "cudgel_plus_one" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "magical_throwing_knife_pure_special" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "impure_meat" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "lizardfolk_club" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "adrenal_gland_large" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "q_staff_plus_two" ), // NOLINT(cata-static-string_id-constants)
    itype_id( "rocuronium" ) // NOLINT(cata-static-string_id-constants)
};

static float max_density_for_mats( const std::map<material_id, int> &mats, float total_size )
{
    REQUIRE( !mats.empty() );

    float max_density = 0.f;

    for( const auto &m : mats ) {
        max_density += m.first->density() * ( m.second  / total_size );
    }

    return max_density;
}

static float item_density( const item &target )
{
    return static_cast<float>( to_gram( target.weight() ) ) / static_cast<float>( to_milliliter(
                target.volume() ) );
}

static bool assert_maximum_density_for_material( const item &target )
{
    if( to_milliliter( target.volume() ) == 0 ) {
        return false;
    }
    const std::map<material_id, int> &mats = target.made_of();
    if( !mats.empty() && known_bad.count( target.typeId() ) == 0 ) {
        const float max_density = max_density_for_mats( mats, target.type->mat_portion_total );
        INFO( target.typeId() );
        CHECK( item_density( target ) <= max_density );

        return item_density( target ) > max_density;
    }

    // fallback return
    return false;
}

TEST_CASE( "item_material_density_sanity_check", "[item]" )
{
    std::vector<const itype *> all_items = item_controller->all();

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

TEST_CASE( "item_material_density_blacklist_is_pruned", "[item][!mayfail]" )
{
    for( const itype_id &bad : known_bad ) {
        if( !bad.is_valid() ) {
            continue;
        }
        const item target( bad, calendar::turn_zero, item::solitary_tag{} );
        if( to_milliliter( target.volume() ) == 0 ) {
            continue;
        }
        const std::map<material_id, int> &mats = target.made_of();
        if( !mats.empty() ) {
            INFO( bad.str() );
            const float max_density = max_density_for_mats( mats, bad->mat_portion_total );
            // Failing? Just remove the relevant items from the known_bad list above
            CHECK( item_density( target ) > max_density );
        }
    }
}

TEST_CASE( "armor_entry_consolidate_check", "[item][armor]" )
{
    item test_consolidate( "test_consolidate" );

    //check this item has a single armor entry, not 3 like is written in the json explicitly

    CHECK( test_consolidate.find_armor_data()->sub_data.size() == 1 );
}

TEST_CASE( "module_inheritance", "[item][armor]" )
{
    avatar &guy = get_avatar();
    clear_avatar();
    guy.set_body();
    guy.clear_mutations();
    guy.worn.clear();

    item test_exo( "test_modular_exosuit" );
    item test_module( "test_exo_lense_module" );

    test_exo.force_insert_item( test_module, item_pocket::pocket_type::CONTAINER );

    guy.worn.wear_item( guy, test_exo, false, false, false );

    CHECK( guy.worn.worn_with_flag( json_flag_FIX_NEARSIGHT ) );
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
