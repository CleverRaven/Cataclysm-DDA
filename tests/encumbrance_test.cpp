#include "catch/catch.hpp"

#include <functional>
#include <list>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "character.h"
#include "item.h"
#include "itype.h"
#include "material.h"
#include "npc.h"
#include "options.h"
#include "player_helpers.h"
#include "ranged.h"
#include "type_id.h"

static void test_encumbrance_on(
    Character &p,
    const std::vector<item> &clothing,
    const std::string &body_part,
    int expected_encumbrance,
    const std::function<void( Character & )> &tweak_player = {}
)
{
    CAPTURE( body_part );
    p.set_body();
    p.clear_mutations();
    p.worn.clear();
    if( tweak_player ) {
        tweak_player( p );
    }
    for( const item &i : clothing ) {
        p.worn.push_back( i );
    }
    p.calc_encumbrance();
    encumbrance_data enc = p.get_part_encumbrance_data( bodypart_id( body_part ) );
    CHECK( enc.encumbrance == expected_encumbrance );
}

static void test_encumbrance_items(
    const std::vector<item> &clothing,
    const std::string &body_part,
    const int expected_encumbrance,
    const std::function<void( Character & )> &tweak_player = {}
)
{
    // Test NPC first because NPC code can accidentally end up using properties
    // of g->u, and such bugs are hidden if we test the other way around.
    SECTION( "testing on npc" ) {
        npc example_npc;
        test_encumbrance_on( example_npc, clothing, body_part, expected_encumbrance, tweak_player );
    }
    SECTION( "testing on player" ) {
        test_encumbrance_on( get_player_character(), clothing, body_part, expected_encumbrance,
                             tweak_player );
    }
}

static void test_encumbrance(
    const std::vector<std::string> &clothing_types,
    const std::string &body_part,
    const int expected_encumbrance
)
{
    CAPTURE( clothing_types );
    std::vector<item> clothing;
    for( const std::string &type : clothing_types ) {
        clothing.push_back( item( type ) );
    }
    test_encumbrance_items( clothing, body_part, expected_encumbrance );
}

// Make avatar take off everything and put on a single piece of clothing
static void wear_single_item( avatar &dummy, const item &clothing )
{
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) ) {}
    dummy.wear_item( clothing );

    // becaue dodging from encumbrance is cached and is only updates here.
    dummy.reset_bonuses();
    dummy.reset_stats();
}

struct add_trait {
    add_trait( const std::string &t ) : trait( t ) {}
    add_trait( const trait_id &t ) : trait( t ) {}

    void operator()( Character &p ) {
        p.toggle_trait( trait );
    }

    trait_id trait;
};

static constexpr int postman_shirt_e = 0;
static constexpr int longshirt_e = 3;
static constexpr int jacket_jean_e = 9;

TEST_CASE( "regular_clothing_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "postman_shirt" }, "torso", postman_shirt_e );
    test_encumbrance( { "longshirt" }, "torso", longshirt_e );
    test_encumbrance( { "jacket_jean" }, "torso", jacket_jean_e );
}

TEST_CASE( "separate_layer_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "longshirt", "jacket_jean" }, "torso", longshirt_e + jacket_jean_e );
}

TEST_CASE( "out_of_order_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "jacket_jean", "longshirt" }, "torso", longshirt_e * 2 + jacket_jean_e );
}

TEST_CASE( "same_layer_encumbrance", "[encumbrance]" )
{
    // When stacking within a layer, encumbrance for additional items is
    // counted twice
    test_encumbrance( { "longshirt", "longshirt" }, "torso", longshirt_e * 2 + longshirt_e );
    // ... with a minimum of 2
    test_encumbrance( { "postman_shirt", "postman_shirt" }, "torso", postman_shirt_e * 2 + 2 );
    // ... and a maximum of 10
    test_encumbrance( { "jacket_jean", "jacket_jean" }, "torso", jacket_jean_e * 2 + 10 );
}

TEST_CASE( "tiny_clothing", "[encumbrance]" )
{
    item i( "longshirt" );
    i.set_flag( "UNDERSIZE" );
    test_encumbrance_items( { i }, "torso", longshirt_e * 3 );
}

TEST_CASE( "tiny_character", "[encumbrance]" )
{
    item i( "longshirt" );
    SECTION( "regular shirt" ) {
        test_encumbrance_items( { i }, "torso", longshirt_e * 2, add_trait( "SMALL2" ) );
    }
    SECTION( "undersize shrt" ) {
        i.set_flag( "UNDERSIZE" );
        test_encumbrance_items( { i }, "torso", longshirt_e, add_trait( "SMALL2" ) );
    }
}

int throw_cost( const player &c, const item &to_throw );
int trap_base_detection_score( const Character &p );
TEST_CASE( "encumbrance_has_real_effects", "[encumbrance]" )
{
    avatar &dummy = get_avatar();
    clear_character( dummy );

    item chainmail_legs( "chainmail_legs" );
    REQUIRE( chainmail_legs.get_encumber( dummy, bodypart_id( "leg_l" ) ) == 20 );
    REQUIRE( chainmail_legs.get_encumber( dummy, bodypart_id( "leg_r" ) ) == 20 );
    item chainmail_arms( "chainmail_arms" );
    REQUIRE( chainmail_arms.get_encumber( dummy, bodypart_id( "arm_l" ) ) == 20 );
    REQUIRE( chainmail_arms.get_encumber( dummy, bodypart_id( "arm_r" ) ) == 20 );
    item chainmail_vest( "chainmail_vest" );
    REQUIRE( chainmail_vest.get_encumber( dummy, bodypart_id( "torso" ) ) == 20 );
    item mittens( "mittens" );
    REQUIRE( mittens.get_encumber( dummy, bodypart_id( "hand_l" ) ) == 80 );
    REQUIRE( mittens.get_encumber( dummy, bodypart_id( "hand_r" ) ) == 80 );
    item sleeping_bag( "sleeping_bag" );
    REQUIRE( sleeping_bag.get_encumber( dummy, bodypart_id( "torso" ) ) == 80 );


    SECTION( "throwing attack move cost increases" ) {
        item thrown( "throwing_stick" );
        REQUIRE( dummy.wield( thrown ) );
        const int unencumbered_throw_cost = throw_cost( dummy, thrown );

        std::pair<item, int>tests[] = {
            std::make_pair( chainmail_vest, 20 ),
            std::make_pair( mittens, 80 ),
            std::make_pair( chainmail_arms, 0 ),
            std::make_pair( chainmail_legs, 0 )
        };
        for( const auto &test : tests ) {
            wear_single_item( dummy, test.first );
            INFO( "Wearing " << test.first.type_name() );
            CHECK( throw_cost( dummy, thrown ) == unencumbered_throw_cost + test.second );
        }
    }

    SECTION( "melee attack move cost increases" ) {
        item melee( "q_staff" );
        REQUIRE( dummy.wield( melee ) );
        const int unencumbered_melee_cost = dummy.attack_speed( melee );

        std::pair<item, int>tests[] = {
            std::make_pair( chainmail_vest, 20 ),
            std::make_pair( mittens, 80 ),
            std::make_pair( chainmail_arms, 0 ),
            std::make_pair( chainmail_legs, 0 )
        };
        for( const auto &test : tests ) {
            wear_single_item( dummy, test.first );
            INFO( "Wearing " << test.first.type_name() );
            CHECK( dummy.attack_speed( melee ) == unencumbered_melee_cost + test.second );
        }
    }


    SECTION( "dodging is harder " ) {
        const float unencumbered_dodge = dummy.get_dodge();

        std::pair<item, int>tests[] = {
            std::make_pair( chainmail_legs, -2 ),
            std::make_pair( chainmail_vest, -2 ),
            std::make_pair( chainmail_arms, 0 ),
            std::make_pair( mittens, 0 ),
        };
        for( const auto &test : tests ) {
            wear_single_item( dummy, test.first );
            INFO( "Wearing " << test.first.type_name() );
            CHECK( dummy.get_dodge() == unencumbered_dodge + test.second );
        }
    }

    SECTION( "hitting in melee is harder - 1% per torso encumbrance " ) {
        // Unencumbered accuracy.
        // Using absolute values here instead of modifiers, because math on floats is imprecise and == test fails
        REQUIRE( dummy.get_melee_accuracy() == 2.0f );
        std::pair<item, float>tests[] = {
            std::make_pair( chainmail_vest, 1.6f ),
            std::make_pair( sleeping_bag, 0.5f ), // 80 encumbrance gives only 75% reduction because it's hardcapped
            std::make_pair( chainmail_arms, 2.0f ), // arms don't affect, surprisingly
        };
        for( const auto &test : tests ) {
            wear_single_item( dummy, test.first );
            INFO( "Wearing " << test.first.type_name() );
            CHECK( dummy.get_melee_accuracy() == test.second );
        }
    }

    SECTION( "eye encumbrance" ) {
        item sunglasses( "sunglasses" );
        REQUIRE( sunglasses.get_encumber( dummy, bodypart_id( "eyes" ) ) == 1 );
        item glasses_safety( "glasses_safety" );
        REQUIRE( glasses_safety.get_encumber( dummy, bodypart_id( "eyes" ) ) == 5 );
        item eclipse_glasses( "eclipse_glasses" );
        REQUIRE( eclipse_glasses.get_encumber( dummy, bodypart_id( "eyes" ) ) == 10 );
        item welding_mask( "welding_mask" );
        REQUIRE( welding_mask.get_encumber( dummy, bodypart_id( "eyes" ) ) == 60 );

        SECTION( "traps are less visible" ) {

            const float unencumbered_trap_detection_score = trap_base_detection_score( dummy );
            // For comparison, buried landmines vision score (that detection (score+rng-distance) is checked against) is 10.
            CHECK( unencumbered_trap_detection_score == 8 );
            std::pair<item, int >tests[] = {
                std::make_pair( sunglasses, 0 ),
                std::make_pair( glasses_safety, 0 ),
                std::make_pair( eclipse_glasses, -1 ),
                std::make_pair( welding_mask, -6 ),
            };
            for( const auto &test : tests ) {
                wear_single_item( dummy, test.first );
                INFO( "Wearing " << test.first.type_name() );
                CHECK( trap_base_detection_score( dummy ) == unencumbered_trap_detection_score + test.second );
            }
        }

        SECTION( "throwing dispersion is much higher" ) {
            item thrown( "throwing_stick" );
            REQUIRE( dummy.wield( thrown ) );

            // 2500 at the time of writing
            const float unencumbered_throwing_dispersion = dummy.throwing_dispersion( thrown, nullptr, false );
            // 8 perception means 8 is subtracted from effective encumbrance (before it's multiplied 10x)
            std::pair<item, int>tests[] = {
                std::make_pair( sunglasses, 0 ),
                std::make_pair( glasses_safety, 0 ),
                std::make_pair( eclipse_glasses, 20 ),
                std::make_pair( welding_mask, 520 ),
            };
            for( const auto &test : tests ) {
                wear_single_item( dummy, test.first );
                INFO( "Wearing " << test.first.type_name() );
                CHECK( dummy.throwing_dispersion( thrown, nullptr,
                                                  false ) == unencumbered_throwing_dispersion + test.second );
            }
        }

        SECTION( "guns dispersion is a tiny bit higher" ) {
            item gun( "glock_19" );
            REQUIRE( dummy.wield( gun ) );

            int sight_dispersion = gun.type->gun->sight_dispersion;
            // 44 at time of writing
            const float unencumbered_gun_dispersion = dummy.effective_dispersion( sight_dispersion );
            std::pair<item, int>tests[] = {
                std::make_pair( sunglasses, 0 ),
                std::make_pair( glasses_safety, 2 ),
                std::make_pair( eclipse_glasses, 5 ),
                std::make_pair( welding_mask, 30 ),
            };
            for( const auto &test : tests ) {
                wear_single_item( dummy, test.first );
                INFO( "Wearing " << test.first.type_name() );
                CHECK( dummy.effective_dispersion( sight_dispersion ) == unencumbered_gun_dispersion +
                       test.second );
            }
        }
    }

    SECTION( "mouth encumbrance" ) {
        item mask_guy_fawkes( "mask_guy_fawkes" );
        REQUIRE( mask_guy_fawkes.get_encumber( dummy, bodypart_id( "mouth" ) ) == 10 );
        item mask_filter( "mask_filter" );
        REQUIRE( mask_filter.get_encumber( dummy, bodypart_id( "mouth" ) ) == 20 );
        item mask_gas( "mask_gas" );
        REQUIRE( mask_gas.get_encumber( dummy, bodypart_id( "mouth" ) ) == 30 );
        item sleeping_bag( "sleeping_bag" );
        REQUIRE( sleeping_bag.get_encumber( dummy, bodypart_id( "mouth" ) ) == 80 );


        // 20 at the time of writing
        const float normal_regen_rate = get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" );
        const int turn_moves = to_moves<int>( 1_turns );
        std::pair<item, int>tests[] = {
            std::make_pair( mittens, 0 ),
            std::make_pair( mask_guy_fawkes, -2 ),
            std::make_pair( mask_filter, -4 ),
            std::make_pair( mask_gas, -6 ),
            std::make_pair( sleeping_bag, -16 ),
        };
        for( const auto &test : tests ) {
            wear_single_item( dummy, test.first );
            INFO( "Wearing " << test.first.type_name() );

            // Start at 10% stamina, then see how fast it replenishes
            dummy.set_stamina( dummy.get_stamina_max() / 10 );
            int before_stam = dummy.get_stamina();
            dummy.update_stamina( turn_moves );
            int after_stam = dummy.get_stamina();

            CHECK( after_stam - before_stam == ( normal_regen_rate + test.second ) * turn_moves );
        }
    }

}
