#include "cata_catch.h"
#include "item.h"

#include <cmath>
#include <initializer_list>
#include <limits>
#include <memory>
#include <vector>

#include "avatar.h"
#include "avatar_action.h"
#include "calendar.h"
#include "enums.h"
#include "flag.h"
#include "game.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "itype.h"
#include "math_defines.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "test_data.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

static const flag_id json_flag_COLD( "COLD" );
static const flag_id json_flag_FILTHY( "FILTHY" );
static const flag_id json_flag_FIX_NEARSIGHT( "FIX_NEARSIGHT" );
static const flag_id json_flag_HOT( "HOT" );

static const item_category_id item_category_container( "container" );
static const item_category_id item_category_food( "food" );

static const itype_id itype_test_backpack( "test_backpack" );
static const itype_id itype_test_duffelbag( "test_duffelbag" );
static const itype_id itype_test_mp3( "test_mp3" );
static const itype_id itype_test_smart_phone( "test_smart_phone" );
static const itype_id itype_test_waterproof_bag( "test_waterproof_bag" );

static const json_character_flag json_flag_DEAF( "DEAF" );

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

    clear_avatar();
    item miner_hat( "miner_hat" );
    item ear_muffs( "attachable_ear_muffs" );
    REQUIRE( miner_hat.put_in( ear_muffs, item_pocket::pocket_type::CONTAINER ).success() );
    REQUIRE( !miner_hat.has_flag( json_flag_DEAF ) );
    guy.wear_item( miner_hat );
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

TEST_CASE( "item_single_type_contents", "[item]" )
{
    item walnut( "walnut" );
    item nail( "nail" );
    item bag( "bag_plastic" );
    REQUIRE( bag.get_category_of_contents().id == item_category_container );
    int const num = GENERATE( 1, 2 );
    bool ret = true;
    for( int i = 0; i < num; i++ ) {
        ret &= bag.put_in( walnut, item_pocket::pocket_type::CONTAINER ).success();
    }
    REQUIRE( ret );
    CAPTURE( num, bag.display_name() );
    CHECK( bag.get_category_of_contents() == *item_category_food );
    REQUIRE( nail.get_category_of_contents().id != walnut.get_category_of_contents().id );
    REQUIRE( bag.put_in( nail, item_pocket::pocket_type::CONTAINER ).success() );
    CHECK( bag.get_category_of_contents().id == item_category_container );
}
