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
#include "player_helpers.h"
#include "rng.h"
#include "test_statistics.h"
#include "type_id.h"

static const mtype_id mon_fish_trout( "mon_fish_trout" );
static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_cop( "mon_zombie_cop" );

static float expected_weights_base[][12] = { { 45, 0,   0,  0, 0, 0, 3, 3, 9, 9, 3, 3 },
    { 45, 6, 0.5, 0.5, 9, 9, 3, 3, 9, 9, 3, 3 }
};

static float expected_weights_max[][12] = { { 4500, 0, 0, 0, 0, 0, 475.47, 475.47, 802.12, 802.12, 119.44, 119.44 },
    { 4500, 3007.13, 99.77, 99.77, 714.9, 714.9, 475.46, 475.46, 802.12, 802.12, 119.43, 119.43 }
};
// Torso   head   eyes   mouth   arm   arm   hand   hand   leg   leg   foot   foot
//   45      6     0.5    0.5     9     9     3      3      9     9     3      3
//   1     1.35   1.15   1.15   0.95  0.95   1.1    1.1   0.975 0.975  0.8    0.8

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
    npc &defender = spawn_npc( point_zero, "thug" );
    clear_character( defender );
    defender.size_class = dsize;

    const int num_tests = 15000;

    for( int i = 0; i < num_tests; ++i ) {
        selected_part_histogram[defender.select_body_part( -1, -1, attacker.can_attack_high(),
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

TEST_CASE( "Check distribution of attacks to body parts for same sized opponents.", "[anatomy]" )
{
    rng_set_engine_seed( 4242424242 );

    calculate_bodypart_distribution( creature_size::medium, creature_size::medium, 0,
                                     expected_weights_base[1] );
    calculate_bodypart_distribution( creature_size::medium, creature_size::medium, 1,
                                     expected_weights_base[1] );
    calculate_bodypart_distribution( creature_size::medium, creature_size::medium, 100,
                                     expected_weights_max[1] );
}

TEST_CASE( "Check distribution of attacks to body parts for a small attacker.", "[anatomy]" )
{
    rng_set_engine_seed( 4242424242 );

    calculate_bodypart_distribution( creature_size::small, creature_size::medium, 0,
                                     expected_weights_base[0] );
    calculate_bodypart_distribution( creature_size::small, creature_size::medium, 1,
                                     expected_weights_base[0] );
    calculate_bodypart_distribution( creature_size::small, creature_size::medium, 100,
                                     expected_weights_max[0] );
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

    CHECK_FALSE( mon_zombie->same_species( *mon_fish_trout ) );
    CHECK_FALSE( mon_fish_trout->same_species( *mon_zombie ) );
}
