#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "creature.h"
#include "monster.h"
#include "mtype.h"

float expected_weights[][12] = {{55, 0,   0,   0, 18, 18, 0, 0, 28, 28, 0, 0},
			      {50, 3.5, 0.5, 0, 30, 30, 0, 0, 18, 18, 0, 0},
			      {55, 25,   5,  0, 28, 28, 0, 0, 10, 10, 0, 0}};

void calculate_bodypart_distribution( monster &attacker, monster &defender, int hit_roll,
				      float (&expected)[12]) {
    std::map<body_part, int> selected_part_histogram = {
      {bp_torso, 0}, {bp_head, 0}, {bp_eyes, 0}, {bp_mouth, 0}, {bp_arm_l, 0}, {bp_arm_r, 0},
      {bp_hand_l, 0}, {bp_hand_r, 0}, {bp_leg_l, 0}, {bp_leg_r, 0}, {bp_foot_l, 0}, {bp_foot_r, 0}
    };

    for( int i = 0; i < 150000; ++i) {
        selected_part_histogram[defender.select_body_part(&attacker, hit_roll)]++;
    }

    float total_weight = 0.0;
    for( float w : expected ) {
        total_weight += w;
    }
    
    printf("With hit roll = %d\n", hit_roll);
    for( auto weight : selected_part_histogram ) {
      CHECK( Approx(expected[weight.first] / total_weight).epsilon(0.01) == weight.second / 150000.0 );
      printf("Weight of %i is %i.\n", weight.first, weight.second);
    }
}

TEST_CASE("Check distribution of attacks to body parts.") {
    mtype smallmon;
    smallmon.size = MS_SMALL;
    monster attacker;
    attacker.type = &smallmon;
    monster defender;
    defender.type = &smallmon;

    srand(time(NULL));

    calculate_bodypart_distribution(attacker, defender, 0, expected_weights[1]);
    calculate_bodypart_distribution(attacker, defender, 1, expected_weights[1]);
    calculate_bodypart_distribution(attacker, defender, 100, expected_weights[1]);
}
