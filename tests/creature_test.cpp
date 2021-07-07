#include <map>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "cata_catch.h"
#include "character.h"
#include "creature.h"
#include "enum_traits.h"
#include "monster.h"
#include "mtype.h"
#include "rng.h"
#include "test_statistics.h"
#include "type_id.h"

static float expected_weights_base[][12] = { { 20, 0,   0,   0, 15, 15, 0, 0, 25, 25, 0, 0 },
    { 33.33, 2.33, 0.33, 0, 20, 20, 0, 0, 12, 12, 0, 0 },
    { 36.57, 5.71,   .57,  0, 22.86, 22.86, 0, 0, 5.71, 5.71, 0, 0 }
};

static float expected_weights_max[][12] = { { 2000, 0,   0,   0, 1191.49, 1191.49, 0, 0, 2228.12, 2228.12, 0, 0 },
    { 3333, 1167.77, 65.84, 0, 1588.66, 1588.66, 0, 0, 1069.50, 1069.50, 0, 0 },
    { 3657, 2861.78,   113.73,  0, 1815.83, 1815.83, 0, 0, 508.904, 508.904, 0, 0 }
};

static void calculate_bodypart_distribution( const creature_size asize, const creature_size dsize,
        const int hit_roll, float ( &expected )[12] )
{
    INFO( "hit roll = " << hit_roll );
    std::map<bodypart_id, int> selected_part_histogram = {
        { bodypart_id( "torso" ), 0 }, { bodypart_id( "head" ), 0 }, { bodypart_id( "eyes" ), 0 }, { bodypart_id( "mouth" ), 0 }, { bodypart_id( "arm_l" ), 0 }, { bodypart_id( "arm_r" ), 0 },
        { bodypart_id( "hand_l" ), 0 }, { bodypart_id( "hand_r" ), 0 }, { bodypart_id( "leg_l" ), 0 }, { bodypart_id( "leg_r" ), 0 }, { bodypart_id( "foot_l" ), 0 }, { bodypart_id( "foot_r" ), 0 }
    };

    mtype atype;
    atype.size = asize;
    monster attacker;
    attacker.type = &atype;
    mtype dtype;
    dtype.size = dsize;
    monster defender;
    defender.type = &dtype;

    const int num_tests = 15000;

    for( int i = 0; i < num_tests; ++i ) {
        selected_part_histogram[defender.select_body_part( &attacker, hit_roll )]++;
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

TEST_CASE( "Check distribution of attacks to body parts for same sized opponents." )
{
    rng_set_engine_seed( 4242424242 );

    calculate_bodypart_distribution( creature_size::small, creature_size::small, 0,
                                     expected_weights_base[1] );
    calculate_bodypart_distribution( creature_size::small, creature_size::small, 1,
                                     expected_weights_base[1] );
    calculate_bodypart_distribution( creature_size::small, creature_size::small, 100,
                                     expected_weights_max[1] );
}

TEST_CASE( "Check distribution of attacks to body parts for smaller attacker." )
{
    rng_set_engine_seed( 4242424242 );

    calculate_bodypart_distribution( creature_size::small, creature_size::medium, 0,
                                     expected_weights_base[0] );
    calculate_bodypart_distribution( creature_size::small, creature_size::medium, 1,
                                     expected_weights_base[0] );
    calculate_bodypart_distribution( creature_size::small, creature_size::medium, 100,
                                     expected_weights_max[0] );
}

TEST_CASE( "Check distribution of attacks to body parts for larger attacker." )
{
    rng_set_engine_seed( 4242424242 );

    calculate_bodypart_distribution( creature_size::medium, creature_size::small, 0,
                                     expected_weights_base[2] );
    calculate_bodypart_distribution( creature_size::medium, creature_size::small, 1,
                                     expected_weights_base[2] );
    calculate_bodypart_distribution( creature_size::medium, creature_size::small, 100,
                                     expected_weights_max[2] );
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
    CHECK( mtype_id( "mon_zombie" )->same_species( *mtype_id( "mon_zombie" ) ) );
    CHECK( mtype_id( "mon_zombie" )->same_species( *mtype_id( "mon_zombie_cop" ) ) );
    CHECK( mtype_id( "mon_zombie_cop" )->same_species( *mtype_id( "mon_zombie" ) ) );

    CHECK_FALSE( mtype_id( "mon_zombie" )->same_species( *mtype_id( "mon_fish_trout" ) ) );
    CHECK_FALSE( mtype_id( "mon_fish_trout" )->same_species( *mtype_id( "mon_zombie" ) ) );
}
