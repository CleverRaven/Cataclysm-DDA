#include <cstdlib>
#include <map>
#include <utility>

#include "catch/catch.hpp"
#include "creature.h"
#include "monster.h"
#include "mtype.h"
#include "test_statistics.h"
#include "bodypart.h"

float expected_weights_base[][12] = { { 20, 0,   0,   0, 15, 15, 0, 0, 25, 25, 0, 0 },
    { 33.33, 2.33, 0.33, 0, 20, 20, 0, 0, 12, 12, 0, 0 },
    { 36.57, 5.71,   .57,  0, 22.86, 22.86, 0, 0, 5.71, 5.71, 0, 0 }
};

float expected_weights_max[][12] = { { 2000, 0,   0,   0, 1191.49, 1191.49, 0, 0, 2228.12, 2228.12, 0, 0 },
    { 3333, 1167.77, 65.84, 0, 1588.66, 1588.66, 0, 0, 1069.50, 1069.50, 0, 0 },
    { 3657, 2861.78,   113.73,  0, 1815.83, 1815.83, 0, 0, 508.904, 508.904, 0, 0 }
};

static void calculate_bodypart_distribution( const enum m_size asize, const enum m_size dsize,
        const int hit_roll, float ( &expected )[12] )
{
    INFO( "hit roll = " << hit_roll );
    std::map<body_part, int> selected_part_histogram = {
        { bp_torso, 0 }, { bp_head, 0 }, { bp_eyes, 0 }, { bp_mouth, 0 }, { bp_arm_l, 0 }, { bp_arm_r, 0 },
        { bp_hand_l, 0 }, { bp_hand_r, 0 }, { bp_leg_l, 0 }, { bp_leg_r, 0 }, { bp_foot_l, 0 }, { bp_foot_r, 0 }
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

    float total_weight = 0.0;
    for( float w : expected ) {
        total_weight += w;
    }

    for( auto weight : selected_part_histogram ) {
        INFO( body_part_name( convert_bp( weight.first ).id() ) );
        const double expected_proportion = expected[weight.first] / total_weight;
        CHECK_THAT( weight.second, IsBinomialObservation( num_tests, expected_proportion ) );
    }
}

TEST_CASE( "Check distribution of attacks to body parts for same sized opponents." )
{
    srand( 4242424242 );

    calculate_bodypart_distribution( MS_SMALL, MS_SMALL, 0, expected_weights_base[1] );
    calculate_bodypart_distribution( MS_SMALL, MS_SMALL, 1, expected_weights_base[1] );
    calculate_bodypart_distribution( MS_SMALL, MS_SMALL, 100, expected_weights_max[1] );
}

TEST_CASE( "Check distribution of attacks to body parts for smaller attacker." )
{
    srand( 4242424242 );

    calculate_bodypart_distribution( MS_SMALL, MS_MEDIUM, 0, expected_weights_base[0] );
    calculate_bodypart_distribution( MS_SMALL, MS_MEDIUM, 1, expected_weights_base[0] );
    calculate_bodypart_distribution( MS_SMALL, MS_MEDIUM, 100, expected_weights_max[0] );
}

TEST_CASE( "Check distribution of attacks to body parts for larger attacker." )
{
    srand( 4242424242 );

    calculate_bodypart_distribution( MS_MEDIUM, MS_SMALL, 0, expected_weights_base[2] );
    calculate_bodypart_distribution( MS_MEDIUM, MS_SMALL, 1, expected_weights_base[2] );
    calculate_bodypart_distribution( MS_MEDIUM, MS_SMALL, 100, expected_weights_max[2] );
}
