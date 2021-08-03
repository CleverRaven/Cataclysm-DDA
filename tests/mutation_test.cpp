#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "mutation.h"
#include "npc.h"
#include "player.h"
#include "player_helpers.h"
#include "type_id.h"

static std::string get_mutations_as_string( const player &p );

// Mutate player toward every mutation in a category (optionally including post-threshold mutations)
//
// Note: If a category has two mutually-exclusive mutations (like pretty/ugly for Lupine), the
// one they ultimately end up with depends on the order they were loaded from JSON
static void give_all_mutations( player &p, const mutation_category_trait &category,
                                const bool include_postthresh )
{
    p.set_body();
    const std::vector<trait_id> category_mutations = mutations_category[category.id];

    // Add the threshold mutation first
    if( include_postthresh && !category.threshold_mut.is_empty() ) {
        p.set_mutation( category.threshold_mut );
    }

    for( const trait_id &mut : category_mutations ) {
        const mutation_branch &mut_data = *mut;
        if( include_postthresh || ( !mut_data.threshold && mut_data.threshreq.empty() ) ) {
            // Try up to 10 times to mutate towards this trait
            int mutation_attempts = 10;
            while( mutation_attempts > 0 && p.mutation_ok( mut, false, false ) ) {
                INFO( "Current mutations: " << get_mutations_as_string( p ) );
                INFO( "Mutating towards " << mut.c_str() );
                if( !p.mutate_towards( mut ) ) {
                    --mutation_attempts;
                }
            }
        }
    }
}

// Return total strength of all mutation categories combined
static int get_total_category_strength( const player &p )
{
    int total = 0;
    for( const std::pair<const mutation_category_id, int> &cat : p.mutation_category_level ) {
        total += cat.second;
    }

    return total;
}

// Returns the list of mutations a player has as a string, for debugging
std::string get_mutations_as_string( const player &p )
{
    std::ostringstream s;
    for( trait_id &m : p.get_mutations() ) {
        s << static_cast<std::string>( m ) << " ";
    }
    return s.str();
}

// Mutation categories may share common mutations. As a character accumulates mutations within a
// category, the chance of breaching threshold for that category increases.
//
// For example the SMELLY mutation is common to FELINE / LUPINE / MOUSE. A character with only the
// SMELLY mutation would be equally strong in all three categories, with an equal chance to breach
// any of them.
//
// The UGLY mutation is common to the FELINE / LUPINE / RAPTOR categories. A character having both
// SMELLY and UGLY would have their FELINE and LUPINE categories strengthened (since they have two
// mutations in those categories), relative to the MOUSE and RAPTOR categories.
//
// Adding GROWL, which is shared by LUPINE / RAT / URSINE, should strengthen the character's LUPINE
// category further, and increase the chance to breach that category. If our character has all three
// mutations, their relative category strengths will look like this:
//
// RAT:    1  (growl)
// MOUSE:  1  (smelly)
// RAPTOR: 1  (ugly)
// URSINE: 1  (growl)
// FELINE: 2  (smelly, ugly)
// LUPINE: 3  (smelly, ugly, growl)
//
// This test illustrates and verifies the above scenario, using the same categories and mutations.
//
TEST_CASE( "mutation category strength based on current mutations", "[mutations][category]" )
{
    const mutation_category_id lupine( "LUPINE" );
    const mutation_category_id feline( "FELINE" );
    const mutation_category_id raptor( "RAPTOR" );
    const mutation_category_id ursine( "URSINE" );
    const mutation_category_id mouse( "MOUSE" );
    const mutation_category_id rat( "RAT" );

    npc dummy;

    // With no mutations, no category is the highest
    CHECK( dummy.get_highest_category().str().empty() );
    // All categories are at level 0
    CHECK( dummy.mutation_category_level[lupine] == 0 );
    CHECK( dummy.mutation_category_level[feline] == 0 );
    CHECK( dummy.mutation_category_level[raptor] == 0 );
    CHECK( dummy.mutation_category_level[ursine] == 0 );
    CHECK( dummy.mutation_category_level[mouse] == 0 );
    CHECK( dummy.mutation_category_level[rat] == 0 );

    // SMELLY mutation: Common to LUPINE, FELINE, and MOUSE
    const trait_id smelly( "SMELLY" );
    REQUIRE( dummy.mutate_towards( smelly ) );
    // No category should be highest
    CHECK( dummy.get_highest_category().str().empty() );
    // All levels should be equal
    CHECK( dummy.mutation_category_level[lupine] == dummy.mutation_category_level[feline] );
    CHECK( dummy.mutation_category_level[lupine] == dummy.mutation_category_level[mouse] );

    // UGLY mutation: Common to LUPINE, FELINE, and RAPTOR
    const trait_id ugly( "UGLY" );
    REQUIRE( dummy.mutate_towards( ugly ) );
    // Still no highest, since LUPINE and FELINE should be tied
    CHECK( dummy.get_highest_category().str().empty() );
    // LUPINE and FELINE should be equal level, and both stronger than MOUSE or RAPTOR
    CHECK( dummy.mutation_category_level[lupine] == dummy.mutation_category_level[feline] );
    CHECK( dummy.mutation_category_level[lupine] > dummy.mutation_category_level[mouse] );
    CHECK( dummy.mutation_category_level[lupine] > dummy.mutation_category_level[raptor] );

    // GROWL mutation: Common to LUPINE, RAT, and URSINE
    const trait_id growl( "GROWL" );
    REQUIRE( dummy.mutate_towards( growl ) );
    // LUPINE has the most mutations now, and should now be the strongest category
    CHECK( dummy.get_highest_category().str() == "LUPINE" );
    // LUPINE category level should be strictly higher than any other
    CHECK( dummy.mutation_category_level[lupine] > dummy.mutation_category_level[feline] );
    CHECK( dummy.mutation_category_level[lupine] > dummy.mutation_category_level[mouse] );
    CHECK( dummy.mutation_category_level[lupine] > dummy.mutation_category_level[raptor] );
    CHECK( dummy.mutation_category_level[lupine] > dummy.mutation_category_level[ursine] );
    CHECK( dummy.mutation_category_level[lupine] > dummy.mutation_category_level[rat] );
}

// If character has all available mutations in a category (pre- or post-threshold), that should be
// their strongest/highest category. This test verifies that expectation for every category.
TEST_CASE( "Having all mutations give correct highest category", "[mutations][strongest]" )
{
    for( const std::pair<const mutation_category_id, mutation_category_trait> &cat :
         mutation_category_trait::get_all() ) {
        const mutation_category_trait &cur_cat = cat.second;
        const std::string cat_str = cur_cat.id.str();
        if( cat_str == "ANY" ) {
            continue;
        }
        // Unfinished mutation category.
        if( cur_cat.wip ) {
            continue;
        }

        GIVEN( "The player has all pre-threshold mutations for " + cat_str ) {
            npc dummy;
            give_all_mutations( dummy, cur_cat, false );

            THEN( cat_str + " is the strongest category" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( dummy.get_highest_category().str() == cat_str );
            }
        }

        GIVEN( "The player has all mutations for " + cat_str ) {
            npc dummy;
            give_all_mutations( dummy, cur_cat, true );

            THEN( cat_str + " is the strongest category" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( dummy.get_highest_category().str() == cat_str );
            }
        }
    }
}

// If character has all the pre-threshold mutations for a category, they should have a chance of
// breaching the threshold. The chance of breach is expected to be between 20% and 40%; this range
// is somewhat arbitrary, but the idea is that each category should have a distinctive collection of
// mutations, and categories should not be too similar or too different from each other.
//
// If a category breach chance falls below 20% (0.2), it suggests that category shares too many
// mutations with other categories. In other words, the category is not distinct enough from others.
//
// If a category breach chance goes above 40% (0.4), it suggests that category has too many special
// mutations of its own, and should have more mutations in common with other categories.  In other
// words, the category is too distinct from others.
//
// This test verifies the breach-chance expectation for all mutation categories.
TEST_CASE( "Having all pre-threshold mutations gives a sensible threshold breach chance",
           "[mutations][breach]" )
{
    const float BREACH_CHANCE_MIN = 0.2f;
    const float BREACH_CHANCE_MAX = 0.4f;

    for( const std::pair<mutation_category_id, mutation_category_trait> cat :
         mutation_category_trait::get_all() ) {
        const mutation_category_trait &cur_cat = cat.second;
        const mutation_category_id &cat_id = cur_cat.id;
        if( cat_id == mutation_category_id( "ANY" ) ) {
            continue;
        }
        // Unfinished mutation category.
        if( cur_cat.wip ) {
            continue;
        }

        GIVEN( "The player has all pre-threshold mutations for " + cat_id.str() ) {
            npc dummy;
            give_all_mutations( dummy, cur_cat, false );

            const int category_strength = dummy.mutation_category_level[cat_id];
            const int total_strength = get_total_category_strength( dummy );
            float breach_chance = category_strength / static_cast<float>( total_strength );

            THEN( "Threshold breach chance is between 0.2 and 0.4" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( breach_chance >= BREACH_CHANCE_MIN );
                CHECK( breach_chance <= BREACH_CHANCE_MAX );
            }
        }
    }
}

TEST_CASE( "Scout and Topographagnosia traits affect overmap sight range", "[mutations][overmap]" )
{
    Character &dummy = get_player_character();
    clear_avatar();

    WHEN( "character has Scout trait" ) {
        dummy.toggle_trait( trait_id( "EAGLEEYED" ) );
        THEN( "they have increased overmap sight range" ) {
            CHECK( dummy.mutation_value( "overmap_sight" ) == 5 );
        }
        // Regression test for #42853
        THEN( "the Self-Aware trait does not affect overmap sight range" ) {
            dummy.toggle_trait( trait_id( "SELFAWARE" ) );
            CHECK( dummy.mutation_value( "overmap_sight" ) == 5 );
        }
    }

    WHEN( "character has Topographagnosia trait" ) {
        dummy.toggle_trait( trait_id( "UNOBSERVANT" ) );
        THEN( "they have reduced overmap sight range" ) {
            CHECK( dummy.mutation_value( "overmap_sight" ) == -10 );
        }
        // Regression test for #42853
        THEN( "the Self-Aware trait does not affect overmap sight range" ) {
            dummy.toggle_trait( trait_id( "SELFAWARE" ) );
            CHECK( dummy.mutation_value( "overmap_sight" ) == -10 );
        }
    }
}

static void check_test_mutation_is_triggered( const Character &dummy, bool trigger_on )
{
    if( trigger_on ) {
        THEN( "the mutation turns on" ) {
            CHECK( dummy.has_trait( trait_id( "TEST_TRIGGER_active" ) ) );
            CHECK( !dummy.has_trait( trait_id( "TEST_TRIGGER" ) ) );
        }
    } else {
        THEN( "the mutation turns off" ) {
            CHECK( !dummy.has_trait( trait_id( "TEST_TRIGGER_active" ) ) );
            CHECK( dummy.has_trait( trait_id( "TEST_TRIGGER" ) ) );
        }
    }
}


TEST_CASE( "The various type of triggers work", "[mutations]" )
{
    Character &dummy = get_player_character();
    clear_avatar();

    WHEN( "character has OR test trigger mutation" ) {
        dummy.toggle_trait( trait_id( "TEST_TRIGGER" ) );

        WHEN( "character is happy" ) {
            dummy.add_morale( morale_type( "morale_perm_debug" ), 21 );
            dummy.apply_persistent_morale();
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, true );
        }

        WHEN( "character is no longer happy" ) {
            dummy.clear_morale();
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, false );
        }

        WHEN( "it is the full moon" ) {
            static const time_point full_moon = calendar::turn_zero + calendar::season_length() / 6;
            calendar::turn = full_moon;
            INFO( "MOON PHASE : " << io::enum_to_string<moon_phase>( get_moon_phase( calendar::turn ) ) );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, true );
        }

        WHEN( "it's no longer the full moon" ) {
            calendar::turn = calendar::turn_zero;
            INFO( "MOON PHASE : " << io::enum_to_string<moon_phase>( get_moon_phase( calendar::turn ) ) );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, false );
        }

        WHEN( "character is very hungry" ) {
            dummy.set_hunger( 120 );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, true );
        }

        WHEN( "character is no longer very hungry" ) {
            dummy.set_hunger( 0 );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, false );
        }

        WHEN( "character is in pain" ) {
            dummy.set_pain( 120 );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, true );
        }

        WHEN( "character is no longer in pain" ) {
            dummy.set_pain( 0 );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, false );
        }

        WHEN( "character is thirsty" ) {
            dummy.set_thirst( 120 );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, true );
        }

        WHEN( "character is no longer thirsty" ) {
            dummy.set_thirst( 0 );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, false );
        }

        WHEN( "character is low on stamina" ) {
            dummy.set_stamina( 0 );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, true );
        }

        WHEN( "character is no longer low on stamina " ) {
            dummy.set_stamina( dummy.get_stamina_max() );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, false );
        }

        WHEN( "it's 2 am" ) {
            calendar::turn = calendar::turn_zero + 2_hours;
            INFO( "TIME OF DAY : " << to_string_time_of_day( calendar::turn ) );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, true );
        }

        WHEN( "it's no longer 2 am" ) {
            calendar::turn = calendar::turn_zero;
            INFO( "TIME OF DAY : " << to_string_time_of_day( calendar::turn ) );
            dummy.process_turn();
            check_test_mutation_is_triggered( dummy, false );
        }
    }

    clear_avatar();

    WHEN( "character has AND test trigger mutation" ) {
        dummy.toggle_trait( trait_id( "TEST_TRIGGER_2" ) );

        WHEN( "it is the full moon but character is not in pain" ) {
            static const time_point full_moon = calendar::turn_zero + calendar::season_length() / 6;
            calendar::turn = full_moon;
            INFO( "MOON PHASE : " << io::enum_to_string<moon_phase>( get_moon_phase( calendar::turn ) ) );
            dummy.process_turn();

            THEN( "the mutation stays turned off" ) {
                CHECK( !dummy.has_trait( trait_id( "TEST_TRIGGER_2_active" ) ) );
                CHECK( dummy.has_trait( trait_id( "TEST_TRIGGER_2" ) ) );
            }
        }

        WHEN( "character is in pain and it's the full moon" ) {
            dummy.set_pain( 120 );
            dummy.process_turn();

            THEN( "the mutation turns on" ) {
                CHECK( dummy.has_trait( trait_id( "TEST_TRIGGER_2_active" ) ) );
                CHECK( !dummy.has_trait( trait_id( "TEST_TRIGGER_2" ) ) );
            }
        }

        WHEN( "character is no longer in pain" ) {
            dummy.set_pain( 0 );
            dummy.process_turn();

            THEN( "the mutation turns off" ) {
                CHECK( !dummy.has_trait( trait_id( "TEST_TRIGGER_2_active" ) ) );
                CHECK( dummy.has_trait( trait_id( "TEST_TRIGGER_2" ) ) );
            }
        }
    }

}
