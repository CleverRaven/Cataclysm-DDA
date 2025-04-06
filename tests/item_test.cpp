#include <array>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "avatar_action.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_attire.h"
#include "coordinates.h"
#include "enums.h"
#include "flag.h"
#include "game.h"
#include "item.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_location.h"
#include "itype.h"
#include "material.h"
#include "math_defines.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "subbodypart.h"
#include "test_data.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const flag_id json_flag_COLD( "COLD" );
static const flag_id json_flag_FILTHY( "FILTHY" );
static const flag_id json_flag_FIX_NEARSIGHT( "FIX_NEARSIGHT" );
static const flag_id json_flag_HOT( "HOT" );

static const item_category_id item_category_clothing( "clothing" );
static const item_category_id item_category_container( "container" );
static const item_category_id item_category_food( "food" );
static const item_category_id item_category_guns( "guns" );
static const item_category_id item_category_spare_parts( "spare_parts" );
static const item_category_id item_category_tools( "tools" );

static const itype_id itype_10gal_hat( "10gal_hat" );
static const itype_id itype_ammonia_hydroxide( "ammonia_hydroxide" );
static const itype_id itype_arm_splint( "arm_splint" );
static const itype_id itype_arm_warmers( "arm_warmers" );
static const itype_id itype_armor_mc_lightplate( "armor_mc_lightplate" );
static const itype_id itype_aspirin( "aspirin" );
static const itype_id itype_attachable_ear_muffs( "attachable_ear_muffs" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_bag_plastic( "bag_plastic" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_bottle_plastic_small( "bottle_plastic_small" );
static const itype_id itype_butter( "butter" );
static const itype_id itype_cash_card( "cash_card" );
static const itype_id itype_chem_black_powder( "chem_black_powder" );
static const itype_id itype_chem_muriatic_acid( "chem_muriatic_acid" );
static const itype_id itype_detergent( "detergent" );
static const itype_id itype_duffelbag( "duffelbag" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_hat_hard( "hat_hard" );
static const itype_id itype_jeans( "jeans" );
static const itype_id itype_legrig( "legrig" );
static const itype_id itype_money( "money" );
static const itype_id itype_neccowafers( "neccowafers" );
static const itype_id itype_pale_ale( "pale_ale" );
static const itype_id itype_rocuronium( "rocuronium" );
static const itype_id itype_shoulder_strap( "shoulder_strap" );
static const itype_id itype_single_malt_whiskey( "single_malt_whiskey" );
static const itype_id itype_software_hacking( "software_hacking" );
static const itype_id itype_test_armguard( "test_armguard" );
static const itype_id itype_test_backpack( "test_backpack" );
static const itype_id itype_test_consolidate( "test_consolidate" );
static const itype_id itype_test_duffelbag( "test_duffelbag" );
static const itype_id itype_test_exo_lense_module( "test_exo_lense_module" );
static const itype_id itype_test_liquid( "test_liquid" );
static const itype_id itype_test_modular_exosuit( "test_modular_exosuit" );
static const itype_id itype_test_mp3( "test_mp3" );
static const itype_id itype_test_rock( "test_rock" );
static const itype_id itype_test_smart_phone( "test_smart_phone" );
static const itype_id itype_test_waterproof_bag( "test_waterproof_bag" );
static const itype_id itype_towel( "towel" );
static const itype_id itype_usb_drive( "usb_drive" );
static const itype_id itype_walnut( "walnut" );
static const itype_id itype_water( "water" );
static const itype_id itype_win70( "win70" );
static const itype_id itype_wrapper( "wrapper" );

static const json_character_flag json_flag_DEAF( "DEAF" );

TEST_CASE( "item_volume", "[item]" )
{
    // Need to pick some item here which is count_by_charges and for which each
    // charge is at least 1_ml.  Battery works for now.
    item i( itype_battery, calendar::turn_zero, item::default_charges_tag() );
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
    CHECK( item( itype_arm_warmers ).get_layer().front() == layer_level::SKINTIGHT );
    CHECK( item( itype_10gal_hat ).get_layer().front() == layer_level::NORMAL );
    // intentionally no waist layer check since it is obsoleted
    CHECK( item( itype_armor_mc_lightplate ).get_layer().front() == layer_level::OUTER );
    CHECK( item( itype_legrig ).get_layer().front() == layer_level::BELTED );
}

TEST_CASE( "gun_layer", "[item]" )
{
    item gun( itype_win70 );
    item mod( itype_shoulder_strap );
    CHECK( gun.is_gunmod_compatible( mod ).success() );
    gun.put_in( mod, pocket_type::MOD );
    CHECK( gun.get_layer().front() == layer_level::BELTED );
    CHECK( gun.get_category_of_contents().id == item_category_guns );
}

TEST_CASE( "stacking_cash_cards", "[item]" )
{
    // Differently-charged cash cards should stack if neither is zero.
    item cash0( itype_cash_card, calendar::turn_zero );
    item cash1( itype_cash_card, calendar::turn_zero );
    item cash2( itype_cash_card, calendar::turn_zero );
    cash1.put_in( item( itype_money, calendar::turn_zero, 1 ), pocket_type::MAGAZINE );
    cash2.put_in( item( itype_money, calendar::turn_zero, 2 ), pocket_type::MAGAZINE );
    CHECK( !cash0.stacks_with( cash1 ) );
    //CHECK( cash1.stacks_with( cash2 ) ); Enable this once cash card stacking is brought back.
}

// second minute hour day week season year

TEST_CASE( "stacking_over_time", "[item]" )
{
    item A( itype_neccowafers );
    item B( itype_neccowafers );

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

TEST_CASE( "liquids_at_different_temperatures", "[item][temperature][stack][combine]" )
{
    item liquid_hot( itype_test_liquid );
    item liquid_cold( itype_test_liquid );
    item liquid_filthy( itype_test_liquid );

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

TEST_CASE( "item_length_sanity_check", "[item]" )
{
    for( const itype *type : item_controller->all() ) {
        const item sample( type, calendar::turn_zero, item::solitary_tag {} );
        assert_minimum_length_to_volume_ratio( sample );
    }
}

TEST_CASE( "corpse_length_sanity_check", "[item]" )
{
    for( const mtype &type : MonsterGenerator::generator().get_all_mtypes() ) {
        const item sample = item::make_corpse( type.id );
        assert_minimum_length_to_volume_ratio( sample );
    }
}

static void check_spawning_in_container( const itype_id &item_type )
{
    item test_item( item_type );
    REQUIRE( test_item.type->default_container );
    item container_item = test_item.in_its_container( 1 );
    CHECK( container_item.typeId() == *test_item.type->default_container );
    if( container_item.is_container() ) {
        CHECK( container_item.has_item_with( [&test_item]( const item & it ) {
            return it.typeId() == test_item.typeId();
        } ) );
    } else if( test_item.is_software() ) {
        REQUIRE( container_item.is_estorage() );
        const std::vector<const item *> softwares = container_item.softwares();
        CHECK( !softwares.empty() );
        for( const item *itm : softwares ) {
            CHECK( itm->typeId() == test_item.typeId() );
        }
    } else {
        FAIL( "Not container or software storage." );
    }
}

TEST_CASE( "items_spawn_in_their_default_containers", "[item]" )
{
    check_spawning_in_container( itype_water );
    check_spawning_in_container( itype_ammonia_hydroxide );
    check_spawning_in_container( itype_detergent );
    check_spawning_in_container( itype_pale_ale );
    check_spawning_in_container( itype_single_malt_whiskey );
    check_spawning_in_container( itype_rocuronium );
    check_spawning_in_container( itype_chem_muriatic_acid );
    check_spawning_in_container( itype_chem_black_powder );
}

TEST_CASE( "item_variables_round-trip_accurately", "[item]" )
{
    item i( itype_water );
    i.set_var( "A", 17 );
    CHECK( i.get_var( "A", 0 ) == 17 );
    i.set_var( "B", 0.125 );
    CHECK( i.get_var( "B", 0.0 ) == 0.125 );
    i.set_var( "C", tripoint_abs_ms( 2, 3, 4 ) );
    CHECK( i.get_var( "C", tripoint_abs_ms::zero ) == tripoint_abs_ms( 2, 3, 4 ) );
}

TEST_CASE( "water_affect_items_while_swimming_check", "[item][water][swimming]" )
{
    avatar &guy = get_avatar();
    clear_avatar();

    GIVEN( "an item with flag WATER_DISSOLVE" ) {

        REQUIRE( item( itype_aspirin ).has_flag( flag_WATER_DISSOLVE ) );

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.clear_worn();

            item aspirin( itype_aspirin );

            REQUIRE( guy.wield( aspirin ) );

            THEN( "should dissolve in water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_WATER_DISSOLVE ) );
            }
        }

        WHEN( "item in backpack" ) {
            guy.unwield();
            guy.clear_worn();

            item aspirin( itype_aspirin );
            item backpack( itype_backpack );

            backpack.put_in( aspirin, pocket_type::CONTAINER );

            REQUIRE( guy.wear_item( backpack ) );

            THEN( "should dissolve in water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_WATER_DISSOLVE ) );
            }
        }

        WHEN( "item in small plastic bottle" ) {
            guy.unwield();
            guy.clear_worn();

            item aspirin( itype_aspirin );
            item bottle_small( itype_bottle_plastic_small );

            bottle_small.put_in( aspirin, pocket_type::CONTAINER );

            REQUIRE( guy.wield( bottle_small ) );

            THEN( "should not dissolve in water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_WATER_DISSOLVE ) );
            }
        }

        WHEN( "item in backpack inside duffel bag" ) {
            guy.unwield();
            guy.clear_worn();

            item aspirin( itype_aspirin );
            item backpack( itype_backpack );
            item duffelbag( itype_duffelbag );

            backpack.put_in( aspirin, pocket_type::CONTAINER );
            duffelbag.put_in( backpack, pocket_type::CONTAINER );

            REQUIRE( guy.wear_item( duffelbag ) );

            THEN( "should dissolve in water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_WATER_DISSOLVE ) );
            }
        }

        WHEN( "item in backpack inside body bag" ) {
            guy.unwield();
            guy.clear_worn();

            item aspirin( itype_aspirin );
            item backpack( itype_backpack );
            item body_bag( itype_test_waterproof_bag );

            backpack.put_in( aspirin, pocket_type::CONTAINER );
            body_bag.put_in( backpack, pocket_type::CONTAINER );

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
            guy.clear_worn();

            item smart_phone( itype_test_smart_phone );

            REQUIRE( guy.wield( smart_phone ) );
            item *test_item = guy.get_wielded_item().get_item();

            THEN( "should be broken by water" ) {
                g->water_affect_items( guy );

                CHECK_FALSE( test_item->faults.empty() );
                CHECK( test_item->is_broken() );
            }
        }

        WHEN( "item in backpack" ) {
            guy.unwield();
            guy.clear_worn();

            item smart_phone( itype_test_smart_phone );
            item backpack( itype_test_backpack );

            backpack.put_in( smart_phone, pocket_type::CONTAINER );

            REQUIRE( guy.wield( backpack ) );
            item *test_item = &guy.get_wielded_item()->only_item();

            THEN( "should be broken by water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( test_item->faults.empty() );
                CHECK( test_item->is_broken() );
            }
        }

        WHEN( "item in body bag" ) {
            guy.unwield();
            guy.clear_worn();

            item smart_phone( itype_test_smart_phone );
            item body_bag( itype_test_waterproof_bag );

            body_bag.put_in( smart_phone, pocket_type::CONTAINER );

            REQUIRE( guy.wield( body_bag ) );
            item *test_item = &guy.get_wielded_item()->only_item();

            THEN( "should not be broken by water" ) {
                g->water_affect_items( guy );
                CHECK( test_item->faults.empty() );
                CHECK_FALSE( test_item->is_broken() );
            }
        }

        WHEN( "item in backpack inside duffel bag" ) {
            guy.unwield();
            guy.clear_worn();

            item smart_phone( itype_test_smart_phone );
            item backpack( itype_test_backpack );
            item duffelbag( itype_test_duffelbag );

            backpack.put_in( smart_phone, pocket_type::CONTAINER );
            duffelbag.put_in( backpack, pocket_type::CONTAINER );

            REQUIRE( guy.wield( duffelbag ) );
            item *test_item = &guy.get_wielded_item()->only_item().only_item();

            THEN( "should be broken by water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( test_item->faults.empty() );
                CHECK( test_item->is_broken() );
            }
        }

        WHEN( "item in backpack inside body bag" ) {
            guy.unwield();
            guy.clear_worn();

            item smart_phone( itype_test_smart_phone );
            item backpack( itype_test_backpack );
            item body_bag( itype_test_waterproof_bag );

            backpack.put_in( smart_phone, pocket_type::CONTAINER );
            body_bag.put_in( backpack, pocket_type::CONTAINER );

            REQUIRE( guy.wield( body_bag ) );
            item *test_item = &guy.get_wielded_item()->only_item().only_item();

            THEN( "should not be broken by water" ) {
                g->water_affect_items( guy );
                CHECK( test_item->faults.empty() );
                CHECK_FALSE( test_item->is_broken() );
            }
        }
    }

    GIVEN( "an item with flag WATER_BREAK_ACTIVE" ) {

        REQUIRE( itype_test_mp3->has_flag( flag_WATER_BREAK_ACTIVE ) );

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.clear_worn();

            item mp3( itype_test_mp3 );

            REQUIRE( guy.wield( mp3 ) );

            THEN( "should get wet from water" ) {
                g->water_affect_items( guy );
                CHECK( guy.get_wielded_item()->wetness > 0 );
            }
        }

        WHEN( "item in backpack" ) {
            guy.unwield();
            guy.clear_worn();

            item mp3( itype_test_mp3 );
            item backpack( itype_test_backpack );

            backpack.put_in( mp3, pocket_type::CONTAINER );

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
            guy.clear_worn();

            item mp3( itype_test_mp3 );
            item body_bag( itype_test_waterproof_bag );

            body_bag.put_in( mp3, pocket_type::CONTAINER );

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
            guy.clear_worn();

            item mp3( itype_test_mp3 );
            item backpack( itype_test_backpack );
            item duffelbag( itype_test_duffelbag );

            backpack.put_in( mp3, pocket_type::CONTAINER );
            duffelbag.put_in( backpack, pocket_type::CONTAINER );

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
            guy.clear_worn();

            item mp3( itype_test_mp3 );
            item backpack( itype_test_backpack );
            item body_bag( itype_test_waterproof_bag );

            backpack.put_in( mp3, pocket_type::CONTAINER );
            body_bag.put_in( backpack, pocket_type::CONTAINER );

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
            guy.clear_worn();

            item mp3( itype_test_mp3 );

            REQUIRE( guy.wield( mp3 ) );

            THEN( "should be wet for around 9562 seconds" ) {
                g->water_affect_items( guy );
                CHECK( guy.get_wielded_item()->wetness == Approx( 9562 ).margin( 20 ) );
            }
        }

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.clear_worn();

            item mp3( itype_test_mp3 );

            REQUIRE( guy.wield( mp3 ) );

            THEN( "gets wet five times in a row " ) {
                g->water_affect_items( guy );
                g->water_affect_items( guy );
                g->water_affect_items( guy );
                g->water_affect_items( guy );
                g->water_affect_items( guy );
                AND_THEN( "should be wet for around 47810 seconds" ) {
                    CHECK( guy.get_wielded_item()->wetness == Approx( 47810 ).margin( 100 ) );
                }
            }
        }
    }

    GIVEN( "a towel" ) {

        WHEN( "item in hand" ) {
            guy.unwield();
            guy.clear_worn();

            item towel( itype_towel );

            REQUIRE( guy.wield( towel ) );

            THEN( "should get wet in water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_WET ) );
            }
        }

        WHEN( "wearing item" ) {
            guy.unwield();
            guy.clear_worn();

            item towel( itype_towel );

            REQUIRE( guy.wear_item( towel ) );

            THEN( "should get wet in water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_WET ) );
            }
        }

        WHEN( "inside a backpack" ) {
            guy.unwield();
            guy.clear_worn();

            item towel( itype_towel );
            item backpack( itype_backpack );

            backpack.put_in( towel, pocket_type::CONTAINER );

            REQUIRE( guy.wield( backpack ) );

            THEN( "should get wet in water" ) {
                g->water_affect_items( guy );
                CHECK( guy.has_item_with_flag( flag_WET ) );
            }
        }

        WHEN( "inside a body bag" ) {
            guy.unwield();
            guy.clear_worn();

            item towel( itype_towel );
            item body_bag( itype_test_waterproof_bag );

            body_bag.put_in( towel, pocket_type::CONTAINER );

            REQUIRE( guy.wield( body_bag ) );

            THEN( "should not get wet in water" ) {
                g->water_affect_items( guy );
                CHECK_FALSE( guy.has_item_with_flag( flag_WET ) );
            }
        }
    }
}


TEST_CASE( "item_new_to_hit_enforcement", "[item]" )
{
    std::vector<const itype *> all_items = item_controller->all();
    const std::set<itype_id> &blacklist = test_data::legacy_to_hit;
    std::string msg_enforce;
    std::string msg_prune;
    for( const itype *type : all_items ) {
        const bool on_blacklist = blacklist.find( type->get_id() ) != blacklist.end();
        if( type->using_legacy_to_hit ) {
            if( !on_blacklist ) {
                msg_enforce += msg_enforce.empty() ? string_format( "\n[\n  \"%s\"", type->get_id().str() ) :
                               string_format( ",\n  \"%s\"", type->get_id().str() );
            }
        } else if( on_blacklist ) {
            msg_prune += msg_prune.empty() ? string_format( "\n[\n  \"%s\"", type->get_id().str() ) :
                         string_format( ",\n  \"%s\"", type->get_id().str() );
        }
    }
    if( !msg_enforce.empty() ) {
        msg_enforce +=
            "\n]\nThe item(s) above use legacy to_hit, please change them to the newer object method (see /docs/design-balance-lore/GAME_BALANCE.md#to-hit-value) or remove the to_hit field if the item(s) aren't intended to be used as weapons.";
    }
    if( !msg_prune.empty() ) {
        msg_prune +=
            "\n]\nThe item(s) above should be removed from the blacklist at /data/mods/TEST_DATA/legacy_to_hit.json.";
    }
    CAPTURE( msg_enforce );
    REQUIRE( msg_enforce.empty() );
    CAPTURE( msg_prune );
    REQUIRE( msg_prune.empty() );
}

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
    if( !mats.empty() && test_data::known_bad.count( target.typeId() ) == 0 ) {
        const float max_density = max_density_for_mats( mats, target.type->mat_portion_total );
        CAPTURE( target.typeId(), target.weight(), target.volume() );
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

TEST_CASE( "item_material_density_blacklist_is_pruned", "[item]" )
{
    for( const itype_id &bad : test_data::known_bad ) {
        if( !bad.is_valid() ) {
            continue;
        }
        const item target( bad, calendar::turn_zero, item::solitary_tag{} );
        if( to_milliliter( target.volume() ) == 0 ) {
            continue;
        }
        const std::map<material_id, int> &mats = target.made_of();
        if( !mats.empty() ) {
            INFO( string_format( "%s had its density fixed, remove it from the list in data/mods/TEST_DATA/known_bad_density.json",
                                 bad.str() ) );
            const float max_density = max_density_for_mats( mats, bad->mat_portion_total );
            CHECK( item_density( target ) > max_density );
        }
    }
}

TEST_CASE( "armor_entry_consolidate_check", "[item][armor]" )
{
    item test_consolidate( itype_test_consolidate );

    //check this item has a single armor entry, not 3 like is written in the json explicitly

    CHECK( test_consolidate.find_armor_data()->sub_data.size() == 1 );
}

TEST_CASE( "module_inheritance", "[item][armor]" )
{
    avatar &guy = get_avatar();
    clear_avatar();
    guy.set_body();
    guy.clear_mutations();
    guy.clear_worn();

    item test_exo( itype_test_modular_exosuit );
    item test_module( itype_test_exo_lense_module );

    test_exo.force_insert_item( test_module, pocket_type::CONTAINER );

    guy.worn.wear_item( guy, test_exo, false, false, false );

    CHECK( guy.worn.worn_with_flag( json_flag_FIX_NEARSIGHT ) );

    clear_avatar();
    item hat_hard( itype_hat_hard );
    item ear_muffs( itype_attachable_ear_muffs );
    REQUIRE( hat_hard.put_in( ear_muffs, pocket_type::CONTAINER ).success() );
    REQUIRE( !hat_hard.has_flag( json_flag_DEAF ) );
    guy.wear_item( hat_hard );
    item_location worn_hat = guy.worn.top_items_loc( guy ).front();
    item_location worn_muffs( worn_hat, &worn_hat->only_item() );
    avatar_action::use_item( guy, worn_muffs, "transform" );
    CHECK( worn_hat->has_flag( json_flag_DEAF ) );
}

TEST_CASE( "rigid_armor_compliance", "[item][armor]" )
{
    avatar &guy = get_avatar();
    clear_avatar();
    // check if you can swap a rigid armor
    item test_armguard( itype_test_armguard );
    REQUIRE( guy.wield( test_armguard ) );

    REQUIRE( guy.wear( guy.used_weapon(), false ) );

    CHECK( guy.worn.top_items_loc( guy ).front().get_item()->get_side() == side::LEFT );

    guy.change_side( *guy.worn.top_items_loc( guy ).front().get_item() );

    CHECK( guy.worn.top_items_loc( guy ).front().get_item()->get_side() == side::RIGHT );

    // check if you can't wear 3 rigid armors
    clear_avatar();

    item first_test_armguard( itype_test_armguard );
    REQUIRE( guy.wield( first_test_armguard ) );
    REQUIRE( guy.wear( guy.used_weapon(), false ) );

    item second_test_armguard( itype_test_armguard );
    REQUIRE( guy.wield( second_test_armguard ) );
    REQUIRE( guy.wear( guy.used_weapon(), false ) );

    item third_test_armguard( itype_test_armguard );
    REQUIRE( guy.wield( third_test_armguard ) );
    REQUIRE( !guy.wear( guy.used_weapon(), false ) );
}

TEST_CASE( "rigid_splint_compliance", "[item][armor]" )
{
    avatar &guy = get_avatar();
    clear_avatar();

    item test_armguard( itype_test_armguard );
    item second_test_armguard( itype_test_armguard );
    item splint( itype_arm_splint );
    item second_splint( itype_arm_splint );
    item third_splint( itype_arm_splint );

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

TEST_CASE( "item_single_type_contents", "[item]" )
{
    item rock( itype_test_rock );
    std::array<std::string, 2> const variants = { "test_rock_blue", "test_rock_green" };
    item walnut( itype_walnut );
    item bag( itype_bag_plastic );
    REQUIRE( bag.get_category_of_contents().id == item_category_container );
    int const num = GENERATE( 1, 2 );
    bool ret = true;
    for( int i = 0; i < num; i++ ) {
        rock.set_itype_variant( variants[i] );
        ret &= bag.put_in( rock, pocket_type::CONTAINER ).success();
    }
    REQUIRE( ret );
    CAPTURE( num, bag.display_name() );
    CHECK( bag.get_category_of_contents() == *item_category_spare_parts );
    REQUIRE( walnut.get_category_of_contents().id != rock.get_category_of_contents().id );
    REQUIRE( bag.put_in( walnut, pocket_type::CONTAINER ).success() );
    if( num == 1 ) {
        // 1 rock and 1 walnut - nothing dominates
        CHECK( bag.get_category_of_contents().id == item_category_container );
    } else {
        // 2 rock and 1 walnuts - rocks dominate
        CHECK( bag.get_category_of_contents().id == item_category_spare_parts );
        REQUIRE( bag.put_in( walnut, pocket_type::CONTAINER ).success() );
        item hammer( itype_hammer );
        REQUIRE( hammer.get_category_of_contents().id != rock.get_category_of_contents().id );
        REQUIRE( hammer.get_category_of_contents().id != walnut.get_category_of_contents().id );
        REQUIRE( bag.put_in( hammer, pocket_type::CONTAINER ).success() );
        // no dominant category anymore - revert to container
        CHECK( bag.get_category_of_contents().id == item_category_container );
    }

    SECTION( "clothing" ) {
        item jeans( itype_jeans );
        REQUIRE( jeans.get_category_of_contents().id == item_category_clothing );
        REQUIRE( walnut.get_category_of_contents().id == item_category_food );
        REQUIRE( jeans.put_in( walnut, pocket_type::CONTAINER ).success() );
        CHECK( jeans.get_category_of_contents().id == item_category_clothing );
    }

    SECTION( "software" ) {
        item usb_drive( itype_usb_drive );
        item software_hacking( itype_software_hacking );
        REQUIRE( usb_drive.get_category_of_contents().id == item_category_tools );
        REQUIRE( usb_drive.put_in( software_hacking, pocket_type::E_FILE_STORAGE ).success() );
        CHECK( usb_drive.get_category_of_contents().id == item_category_tools );
    }
}

TEST_CASE( "item_nested_contents", "[item]" )
{
    item walnut( itype_walnut );
    item outer_bag( itype_bag_plastic );
    item inner_bag1( itype_bag_plastic );
    item inner_bag2( itype_bag_plastic );

    REQUIRE( inner_bag1.put_in( walnut, pocket_type::CONTAINER ).success() );
    REQUIRE( inner_bag1.put_in( walnut, pocket_type::CONTAINER ).success() );
    CHECK( inner_bag1.get_category_of_contents().id == item_category_food );

    REQUIRE( inner_bag2.put_in( walnut, pocket_type::CONTAINER ).success() );
    CHECK( inner_bag2.get_category_of_contents().id == item_category_food );

    REQUIRE( outer_bag.put_in( inner_bag1, pocket_type::CONTAINER ).success() );
    REQUIRE( outer_bag.put_in( inner_bag2, pocket_type::CONTAINER ).success() );
    CAPTURE( outer_bag.display_name() );
    // outer_bag
    //   inner_bag1
    //     walnut
    //     walnut
    //   inner_bag2
    //     walnut
    CHECK( outer_bag.get_category_of_contents().id == item_category_food );
}

TEST_CASE( "item_rotten_contents", "[item]" )
{
    item wrapper( itype_wrapper );
    REQUIRE( wrapper.get_category_of_contents().id == item_category_container );

    item butter_rotten( itype_butter );
    butter_rotten.set_relative_rot( 1.01 );
    REQUIRE( wrapper.put_in( butter_rotten, pocket_type::CONTAINER ).success() );
    REQUIRE( wrapper.put_in( butter_rotten, pocket_type::CONTAINER ).success() );
    CAPTURE( wrapper.display_name() );
    CHECK( wrapper.get_category_of_contents().id == item_category_food );

    item butter( itype_butter );
    butter.set_relative_rot( 0.5 );
    REQUIRE( wrapper.put_in( butter, pocket_type::CONTAINER ).success() );
    CAPTURE( wrapper.display_name() );
    // wrapper
    //   butter (rotten)
    //   butter (rotten)
    //   butter
    CHECK( wrapper.get_category_of_contents().id == item_category_food );
}
