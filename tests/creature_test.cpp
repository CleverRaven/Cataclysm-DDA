#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "enum_traits.h"
#include "mtype.h"
#include "npc.h"
#include "player_helpers.h"
#include "rng.h"
#include "test_statistics.h"
#include "type_id.h"

static const json_character_flag json_flag_LIMB_LOWER( "LIMB_LOWER" );
static const json_character_flag json_flag_LIMB_UPPER( "LIMB_UPPER" );

static const mtype_id mon_fish_rainbow_trout( "mon_fish_rainbow_trout" );
static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_cop( "mon_zombie_cop" );

static std::array<std::array<float, 12>, 2> expected_weights_base = { {
        { { 36, 0, 0, 0, 13, 13, 1.5, 1.5, 39, 39, 6, 6 } },
        { { 36, 4, 0.5, 0.5, 13, 13, 1.5, 1.5, 13, 13, 2, 2 } }
    }
};

static std::array<std::array<float, 12>, 2> expected_weights_max = { {
        { { 3600, 0, 0, 0, 1032.63, 1032.63, 237.74, 237.74, 2460.73, 2460.73, 238.86, 238.86 } },
        { { 3600, 1004.75, 99.77, 99.77, 1032.63, 1032.63, 237.74, 237.74, 820.24, 820.24, 79.62, 79.62 } }
    }
};
// Torso   head   eyes   mouth   arm   arm   hand   hand   leg   leg   foot   foot
//   36     4U    0.5U   0.5U    13    13    1.5   1.5     13L   13L   2L     2L
//   1      1.2   1.15   1.15   0.95  0.95    1.1   1.1    0.9   0.9   0.8    0.8

static void calculate_bodypart_distribution( const bool can_attack_high,
        const int hit_roll, const std::array<float, 12> &expected )
{
    INFO( "hit roll = " << hit_roll );
    std::map<bodypart_id, int> selected_part_histogram = {
        { bodypart_id( "torso" ), 0 }, { bodypart_id( "head" ), 0 }, { bodypart_id( "eyes" ), 0 }, { bodypart_id( "mouth" ), 0 }, { bodypart_id( "arm_l" ), 0 }, { bodypart_id( "arm_r" ), 0 },
        { bodypart_id( "hand_l" ), 0 }, { bodypart_id( "hand_r" ), 0 }, { bodypart_id( "leg_l" ), 0 }, { bodypart_id( "leg_r" ), 0 }, { bodypart_id( "foot_l" ), 0 }, { bodypart_id( "foot_r" ), 0 }
    };

    npc &defender = spawn_npc( point_bub_ms::zero, "thug" );
    clear_character( defender );
    REQUIRE( defender.has_bodypart_with_flag( json_flag_LIMB_LOWER ) );
    REQUIRE( defender.has_bodypart_with_flag( json_flag_LIMB_UPPER ) );

    const int num_tests = 15000;

    for( int i = 0; i < num_tests; ++i ) {
        selected_part_histogram[defender.select_body_part( -1, -1, can_attack_high,
                                                           hit_roll )]++;
    }

    float total_weight = 0.0f;
    for( float w : expected ) {
        total_weight += w;
    }

    int j = 0;
    for( std::pair<const bodypart_id, int> weight : selected_part_histogram ) {
        INFO( body_part_name( weight.first ) );
        const double expected_proportion = expected[j] / total_weight;
        CHECK_THAT( weight.second, IsBinomialObservation( num_tests, expected_proportion ) );
        j++;
    }
}

TEST_CASE( "Check_distribution_of_attacks_to_body_parts_for_attackers_who_can_not_attack_upper_limbs",
           "[anatomy]" )
{
    rng_set_engine_seed( 4242424242 );

    calculate_bodypart_distribution( false, 0,
                                     expected_weights_base[0] );
    calculate_bodypart_distribution( false, 1,
                                     expected_weights_base[0] );
    calculate_bodypart_distribution( false, 100,
                                     expected_weights_max[0] );
}

TEST_CASE( "Check_distribution_of_attacks_to_body_parts_for_attackers_who_can_attack_upper_limbs",
           "[anatomy]" )
{
    rng_set_engine_seed( 4242424242 );

    calculate_bodypart_distribution( true, 0,
                                     expected_weights_base[1] );
    calculate_bodypart_distribution( true, 1,
                                     expected_weights_base[1] );
    calculate_bodypart_distribution( true, 100,
                                     expected_weights_max[1] );
}

TEST_CASE( "body_part_sorting_all", "[bodypart]" )
{
    std::vector<bodypart_id> expected = {
        bodypart_id( "head" ), bodypart_id( "eyes" ), bodypart_id( "mouth" ),
        bodypart_id( "torso" ),
        bodypart_id( "arm_l" ), bodypart_id( "hand_l" ),
        bodypart_id( "arm_r" ), bodypart_id( "hand_r" ),
        bodypart_id( "leg_l" ), bodypart_id( "foot_l" ),
        bodypart_id( "leg_r" ), bodypart_id( "foot_r" )
    };
    std::vector<bodypart_id> observed =
        get_player_character().get_all_body_parts( get_body_part_flags::sorted );
    CHECK( observed == expected );
}

TEST_CASE( "body_part_sorting_main", "[bodypart]" )
{
    std::vector<bodypart_id> expected = {
        bodypart_id( "head" ), bodypart_id( "torso" ),
        bodypart_id( "arm_l" ), bodypart_id( "arm_r" ),
        bodypart_id( "leg_l" ), bodypart_id( "leg_r" )
    };
    std::vector<bodypart_id> observed =
        get_player_character().get_all_body_parts(
            get_body_part_flags::sorted | get_body_part_flags::only_main );
    CHECK( observed == expected );
}

TEST_CASE( "mtype_species_test", "[monster]" )
{
    CHECK( mon_zombie->same_species( *mon_zombie ) );
    CHECK( mon_zombie->same_species( *mon_zombie_cop ) );
    CHECK( mon_zombie_cop->same_species( *mon_zombie ) );

    CHECK_FALSE( mon_zombie->same_species( *mon_fish_rainbow_trout ) );
    CHECK_FALSE( mon_fish_rainbow_trout->same_species( *mon_zombie ) );
}
