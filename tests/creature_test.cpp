#include "catch/catch.hpp"

#include "creature.h"
#include "monster.h"
#include "mtype.h"

float expected_weights_base[][12] = {{20, 0,   0,   0, 15, 15, 0, 0, 25, 25, 0, 0},
                                {33.33, 2.33, 0.33, 0, 20, 20, 0, 0, 12, 12, 0, 0},
                                {36.57, 5.71,   .57,  0, 22.86, 22.86, 0, 0, 5.71, 5.71, 0, 0}};

float expected_weights_max[][12] = {{2000, 0,   0,   0, 1191.49, 1191.49, 0, 0, 2228.12, 2228.12, 0, 0},
                                {3333, 1167.77, 65.84, 0, 1588.66, 1588.66, 0, 0, 1069.50, 1069.50, 0, 0},
                                {3657, 2861.78,   113.73,  0, 1815.83, 1815.83, 0, 0, 453.56, 453.56, 0, 0}};

void calculate_bodypart_distribution( monster &attacker, monster &defender, int hit_roll,
                                      float (&expected)[12]) {
    std::map<body_part, int> selected_part_histogram = {
        {bp_torso, 0}, {bp_head, 0}, {bp_eyes, 0}, {bp_mouth, 0}, {bp_arm_l, 0}, {bp_arm_r, 0},
        {bp_hand_l, 0}, {bp_hand_r, 0}, {bp_leg_l, 0}, {bp_leg_r, 0}, {bp_foot_l, 0}, {bp_foot_r, 0}
    };

    for( int i = 0; i < 15000; ++i) {
        selected_part_histogram[defender.select_body_part(&attacker, hit_roll)]++;
    }

    float total_weight = 0.0;
    for( float w : expected ) {
        total_weight += w;
    }

    for( auto weight : selected_part_histogram ) {
        CHECK( Approx(expected[weight.first] / total_weight).epsilon(0.01) ==
               weight.second / 15000.0 );
    }
}

TEST_CASE("Check distribution of attacks to body parts for same sized opponents.") {
    mtype smallmon;
    smallmon.size = MS_SMALL;
    monster attacker;
    attacker.type = &smallmon;
    monster defender;
    defender.type = &smallmon;

    srand(time(NULL));

    calculate_bodypart_distribution(attacker, defender, 0, expected_weights_base[1]);
    calculate_bodypart_distribution(attacker, defender, 1, expected_weights_base[1]);
    calculate_bodypart_distribution(attacker, defender, 100, expected_weights_max[1]);
}

TEST_CASE("Check distribution of attacks to body parts for smaller attacker.") {
    mtype smallmon;
    smallmon.size = MS_SMALL;
    mtype medmon;
    medmon.size = MS_MEDIUM;
    monster attacker;
    attacker.type = &smallmon;
    monster defender;
    defender.type = &medmon;

    srand(time(NULL));

    calculate_bodypart_distribution(attacker, defender, 0, expected_weights_base[0]);
    calculate_bodypart_distribution(attacker, defender, 1, expected_weights_base[0]);
    calculate_bodypart_distribution(attacker, defender, 100, expected_weights_max[0]);
}

TEST_CASE("Check distribution of attacks to body parts for larger attacker.") {
    mtype smallmon;
    smallmon.size = MS_SMALL;
    mtype medmon;
    medmon.size = MS_MEDIUM;
    monster attacker;
    attacker.type = &medmon;
    monster defender;
    defender.type = &smallmon;

    srand(time(NULL));

    calculate_bodypart_distribution(attacker, defender, 0, expected_weights_base[2]);
    calculate_bodypart_distribution(attacker, defender, 1, expected_weights_base[2]);
    calculate_bodypart_distribution(attacker, defender, 100, expected_weights_max[2]);
}
