#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "effect_on_condition.h"
#include "mutation.h"
#include "npc.h"
#include "player_helpers.h"
#include "type_id.h"

static const effect_on_condition_id effect_on_condition_changing_mutate2( "changing_mutate2" );

static const morale_type morale_perm_debug( "morale_perm_debug" );

static const mutation_category_id mutation_category_ALPHA( "ALPHA" );
static const mutation_category_id mutation_category_BIRD( "BIRD" );
static const mutation_category_id mutation_category_CHIMERA( "CHIMERA" );
static const mutation_category_id mutation_category_FELINE( "FELINE" );
static const mutation_category_id mutation_category_HUMAN( "HUMAN" );
static const mutation_category_id mutation_category_LUPINE( "LUPINE" );
static const mutation_category_id mutation_category_MOUSE( "MOUSE" );
static const mutation_category_id mutation_category_RAPTOR( "RAPTOR" );
static const mutation_category_id mutation_category_REMOVAL_TEST( "REMOVAL_TEST" );
static const mutation_category_id mutation_category_TROGLOBITE( "TROGLOBITE" );

static const trait_id trait_BEAK( "BEAK" );
static const trait_id trait_BEAK_PECK( "BEAK_PECK" );
static const trait_id trait_EAGLEEYED( "EAGLEEYED" );
static const trait_id trait_FELINE_EARS( "FELINE_EARS" );
static const trait_id trait_GOURMAND( "GOURMAND" );
static const trait_id trait_MYOPIC( "MYOPIC" );
static const trait_id trait_QUICK( "QUICK" );
static const trait_id trait_SMELLY( "SMELLY" );
static const trait_id trait_STR_ALPHA( "STR_ALPHA" );
static const trait_id trait_STR_UP( "STR_UP" );
static const trait_id trait_STR_UP_2( "STR_UP_2" );
static const trait_id trait_TEST_OVERMAP_SIGHT_5( "TEST_OVERMAP_SIGHT_5" );
static const trait_id trait_TEST_OVERMAP_SIGHT_MINUS_10( "TEST_OVERMAP_SIGHT_MINUS_10" );
static const trait_id trait_TEST_REMOVAL_0( "TEST_REMOVAL_0" );
static const trait_id trait_TEST_REMOVAL_1( "TEST_REMOVAL_1" );
static const trait_id trait_TEST_TRIGGER( "TEST_TRIGGER" );
static const trait_id trait_TEST_TRIGGER_2( "TEST_TRIGGER_2" );
static const trait_id trait_TEST_TRIGGER_2_active( "TEST_TRIGGER_2_active" );
static const trait_id trait_TEST_TRIGGER_active( "TEST_TRIGGER_active" );
static const trait_id trait_THRESH_ALPHA( "THRESH_ALPHA" );
static const trait_id trait_THRESH_BIRD( "THRESH_BIRD" );
static const trait_id trait_THRESH_CHIMERA( "THRESH_CHIMERA" );
static const trait_id trait_UGLY( "UGLY" );
static const trait_id trait_WINGS_BIRD( "WINGS_BIRD" );

static const vitamin_id vitamin_mutagen( "mutagen" );
static const vitamin_id vitamin_mutagen_human( "mutagen_human" );
static const vitamin_id vitamin_mutagen_test_removal( "mutagen_test_removal" );

static std::string get_mutations_as_string( const Character &you );

static void verify_mutation_flag( Character &you, const std::string &trait_name,
                                  const std::string &flag_name )
{
    clear_avatar();
    set_single_trait( you, trait_name );
    GIVEN( "trait: " + trait_name + ", flag: " + flag_name ) {
        CHECK( you.has_flag( flag_id( flag_name ) ) );
    }
}

static mutation_category_id get_highest_category( const Character &you )
{
    int iLevel = 0;
    mutation_category_id sMaxCat;

    for( const std::pair<const mutation_category_id, int> &elem : you.mutation_category_level ) {
        if( elem.second > iLevel ) {
            sMaxCat = elem.first;
            iLevel = elem.second;
        } else if( elem.second == iLevel ) {
            sMaxCat = mutation_category_id();  // no category on ties
        }
    }
    return sMaxCat;
}

// Returns the list of mutations a player has as a string, for debugging
std::string get_mutations_as_string( const Character &you )
{
    std::ostringstream s;
    for( trait_id &m : you.get_mutations() ) {
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
// Adding GOURMAND, which is shared by LUPINE / MOUSE, should strengthen the character's LUPINE
// category further, and increase the chance to breach that category. If our character has all three
// mutations, their relative category strengths will look like this:
//
// RAPTOR: 1  (ugly)
// MOUSE:  2  (smelly. gourmand)
// FELINE: 2  (smelly, ugly)
// LUPINE: 3  (smelly, ugly, gourmand)
//
// This test illustrates and verifies the above scenario, using the same categories and mutations.
//
TEST_CASE( "mutation_category_strength_based_on_current_mutations", "[mutations][category]" )
{
    npc dummy;

    // With no mutations, no category is the highest
    CHECK( get_highest_category( dummy ).str().empty() );
    // All categories are at level 0
    CHECK( dummy.mutation_category_level[mutation_category_LUPINE] == 0 );
    CHECK( dummy.mutation_category_level[mutation_category_FELINE] == 0 );
    CHECK( dummy.mutation_category_level[mutation_category_RAPTOR] == 0 );
    CHECK( dummy.mutation_category_level[mutation_category_MOUSE] == 0 );

    // SMELLY mutation: Common to LUPINE, FELINE, and MOUSE
    REQUIRE( dummy.mutate_towards( trait_SMELLY ) );
    // No category should be highest
    CHECK( get_highest_category( dummy ).str().empty() );
    // All levels should be equal
    CHECK( dummy.mutation_category_level[mutation_category_LUPINE] ==
           dummy.mutation_category_level[mutation_category_FELINE] );
    CHECK( dummy.mutation_category_level[mutation_category_LUPINE] ==
           dummy.mutation_category_level[mutation_category_MOUSE] );

    // UGLY mutation: Common to LUPINE, FELINE, and RAPTOR
    REQUIRE( dummy.mutate_towards( trait_UGLY ) );
    // Still no highest, since LUPINE and FELINE should be tied
    CHECK( get_highest_category( dummy ).str().empty() );
    // LUPINE and FELINE should be equal level, and both stronger than MOUSE or RAPTOR
    CHECK( dummy.mutation_category_level[mutation_category_LUPINE] ==
           dummy.mutation_category_level[mutation_category_FELINE] );
    CHECK( dummy.mutation_category_level[mutation_category_LUPINE] >
           dummy.mutation_category_level[mutation_category_MOUSE] );
    CHECK( dummy.mutation_category_level[mutation_category_LUPINE] >
           dummy.mutation_category_level[mutation_category_RAPTOR] );

    // GROWL mutation: Common to LUPINE and MOUSE
    REQUIRE( dummy.mutate_towards( trait_GOURMAND ) );
    // LUPINE has the most mutations now, and should now be the strongest category
    CHECK( get_highest_category( dummy ).str() == "LUPINE" );
    // LUPINE category level should be strictly higher than any other
    CHECK( dummy.mutation_category_level[mutation_category_LUPINE] >
           dummy.mutation_category_level[mutation_category_FELINE] );
    CHECK( dummy.mutation_category_level[mutation_category_LUPINE] >
           dummy.mutation_category_level[mutation_category_MOUSE] );
    CHECK( dummy.mutation_category_level[mutation_category_LUPINE] >
           dummy.mutation_category_level[mutation_category_RAPTOR] );
}

// If character has all available mutations in a category (pre- or post-threshold), that should be
// their strongest/highest category. This test verifies that expectation for every category.
TEST_CASE( "Having_all_mutations_give_correct_highest_category", "[mutations][strongest]" )
{
    for( const std::pair<const mutation_category_id, mutation_category_trait> &cat :
         mutation_category_trait::get_all() ) {
        const mutation_category_trait &cur_cat = cat.second;
        const std::string cat_str = cur_cat.id.str();
        if( cat_str == "ANY" ) {
            continue;
        }
        // Unfinished mutation category.
        if( cur_cat.skip_test || cur_cat.wip ) {
            continue;
        }

        GIVEN( "The player has all pre-threshold mutations for " + cat_str ) {
            npc dummy;
            dummy.set_body();
            dummy.give_all_mutations( cur_cat, false );

            THEN( cat_str + " is the strongest category" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( get_highest_category( dummy ).str() == cat_str );
            }
        }

        GIVEN( "The player has all mutations for " + cat_str ) {
            npc dummy;
            dummy.set_body();
            dummy.give_all_mutations( cur_cat, true );

            THEN( cat_str + " is the strongest category" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( get_highest_category( dummy ).str() == cat_str );
            }
        }
    }
}

// If character has all the pre-threshold mutations for a category, they should have a chance of
// breaching the threshold on mutation. The chance of breach is expected to be above 55%
// given that the breach power is rolled out of 100.  In addition, a power below 30 is ignored.
//
// If a category breach power falls below 55, it suggests that category lacks enough pre-threshold mutations
// to comfortably cross the Threshold
//
// Alpha threshold is intentionally meant to be harder to breach, so the permitted range is 35-60
//
// When creating or editing a category, remember that 55 is a limit, not suggestion
// 65+ is the suggested range
//
// This test verifies the breach-power expectation for all mutation categories.
TEST_CASE( "Having_all_pre-threshold_mutations_gives_a_sensible_threshold_breach_power",
           "[mutations][breach]" )
{
    const int BREACH_POWER_MIN = 55;

    for( const std::pair<const mutation_category_id, mutation_category_trait> &cat :
         mutation_category_trait::get_all() ) {
        const mutation_category_trait &cur_cat = cat.second;
        const mutation_category_id &cat_id = cur_cat.id;
        if( cur_cat.threshold_mut.is_empty() ) {
            continue;
        }
        // Unfinished mutation category.
        if( cur_cat.skip_test || cur_cat.wip ) {
            continue;
        }

        GIVEN( "The player has all pre-threshold mutations for " + cat_id.str() ) {
            npc dummy;
            dummy.set_body();
            dummy.give_all_mutations( cur_cat, false );

            const int breach_chance = dummy.mutation_category_level[cat_id];
            if( cat_id == mutation_category_ALPHA ) {
                THEN( "Alpha Threshold breach power is between 35 and 60" ) {
                    INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                    CHECK( breach_chance >= 35 );
                    CHECK( breach_chance <= 60 );
                }
                continue;
            } else if( cat_id == mutation_category_CHIMERA ) {
                THEN( "Chimera Threshold breach power is 100+" ) {
                    INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                    CHECK( breach_chance >= 100 );
                }
                continue;
            }
            THEN( "Threshold breach power is 55+" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( breach_chance >= BREACH_POWER_MIN );
            }
        }
    }
}

TEST_CASE( "Mutation/starting_trait_interactions", "[mutations]" )
{
    Character &dummy = get_player_character();
    clear_avatar();

    SECTION( "Removal via purifier" ) {
        // Set up for purifier test
        dummy.my_traits.insert( trait_TEST_REMOVAL_0 );
        dummy.set_mutation( trait_TEST_REMOVAL_0 );
        dummy.my_traits.insert( trait_TEST_REMOVAL_1 );
        dummy.set_mutation( trait_TEST_REMOVAL_1 );
        dummy.vitamin_set( vitamin_mutagen_human, 1500 );
        dummy.vitamin_set( vitamin_mutagen, 1500 );

        // Check that everything works as it should
        CHECK( mutation_category_trait::get_category( mutation_category_HUMAN ).base_removal_chance == 0 );
        CHECK( dummy.has_trait( trait_TEST_REMOVAL_0 ) );
        CHECK( dummy.has_base_trait( trait_TEST_REMOVAL_0 ) );
        CHECK( dummy.has_trait( trait_TEST_REMOVAL_1 ) );
        CHECK( dummy.has_base_trait( trait_TEST_REMOVAL_1 ) );
        CHECK( dummy.vitamin_get( vitamin_mutagen_human ) == 1500 );
        CHECK( dummy.vitamin_get( vitamin_mutagen ) == 1500 );

        // Trigger a mutation
        dialogue newDialog( get_talker_for( dummy ), nullptr );
        effect_on_condition_changing_mutate2->activate( newDialog );

        // Mutation triggered as expected, traits not removed
        CHECK( dummy.vitamin_get( vitamin_mutagen ) < 1500 );
        CHECK( dummy.has_trait( trait_TEST_REMOVAL_0 ) );
        CHECK( dummy.has_base_trait( trait_TEST_REMOVAL_0 ) );
        CHECK( dummy.has_trait( trait_TEST_REMOVAL_1 ) );
        CHECK( dummy.has_base_trait( trait_TEST_REMOVAL_1 ) );
    }

    SECTION( "Removal via mutation" ) {
        clear_avatar();
        dummy.my_traits.insert( trait_TEST_REMOVAL_1 );
        dummy.set_mutation( trait_TEST_REMOVAL_1 );
        dummy.vitamin_set( vitamin_mutagen, 1500 );
        dummy.vitamin_set( vitamin_mutagen_test_removal, 1500 );

        CHECK( mutation_category_trait::get_category( mutation_category_REMOVAL_TEST ).base_removal_chance
               ==
               100 );
        CHECK( mutation_category_trait::get_category( mutation_category_REMOVAL_TEST ).base_removal_cost_mul
               ==
               2.0 );
        CHECK( dummy.has_trait( trait_TEST_REMOVAL_1 ) );
        CHECK( dummy.has_base_trait( trait_TEST_REMOVAL_1 ) );
        CHECK( dummy.vitamin_get( vitamin_mutagen_test_removal ) == 1500 );
        CHECK( dummy.vitamin_get( vitamin_mutagen ) == 1500 );

        // Trigger a mutation
        dialogue newDialog( get_talker_for( dummy ), nullptr );
        effect_on_condition_changing_mutate2->activate( newDialog );

        // Mutation triggered as expected, purifiable trait removed at the specified cost multiplier
        CHECK( dummy.vitamin_get( vitamin_mutagen ) < 1500 );
        CHECK( !dummy.has_trait( trait_TEST_REMOVAL_1 ) );
        CHECK( !dummy.has_base_trait( trait_TEST_REMOVAL_1 ) );
        CHECK( dummy.vitamin_get( vitamin_mutagen_test_removal ) == 1300 );
    }

    SECTION( "Non-purifiable traits are still protected" ) {
        clear_avatar();
        dummy.my_traits.insert( trait_TEST_REMOVAL_0 );
        dummy.set_mutation( trait_TEST_REMOVAL_0 );
        dummy.vitamin_set( vitamin_mutagen, 1500 );
        dummy.vitamin_set( vitamin_mutagen_test_removal, 1500 );

        CHECK( dummy.has_trait( trait_TEST_REMOVAL_0 ) );
        CHECK( dummy.has_base_trait( trait_TEST_REMOVAL_0 ) );
        CHECK( dummy.vitamin_get( vitamin_mutagen_test_removal ) == 1500 );
        CHECK( dummy.vitamin_get( vitamin_mutagen ) == 1500 );

        dialogue newDialog( get_talker_for( dummy ), nullptr );
        effect_on_condition_changing_mutate2->activate( newDialog );

        // Mutagen decremented but no trait gain, category vitamin level unchanged
        CHECK( dummy.vitamin_get( vitamin_mutagen ) < 1500 );
        CHECK( dummy.has_trait( trait_TEST_REMOVAL_0 ) );
        CHECK( dummy.has_base_trait( trait_TEST_REMOVAL_0 ) );
        CHECK( dummy.vitamin_get( vitamin_mutagen_test_removal ) == 1500 );

    }
}

TEST_CASE( "OVERMAP_SIGHT_enchantment_affect_overmap_sight_range", "[mutations][overmap]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    // 100.0f is light value equal to daylight

    WHEN( "character has no traits, that change overmap sight range" ) {
        THEN( "unchanged sight range" ) {
            CHECK( dummy.overmap_sight_range( 100.0f ) == 10.0 );
        }
    }

    WHEN( "character has TEST_OVERMAP_SIGHT_5" ) {
        dummy.toggle_trait( trait_TEST_OVERMAP_SIGHT_5 );
        THEN( "they have increased overmap sight range" ) {
            CHECK( dummy.overmap_sight_range( 100.0f ) == 15.0 );
        }
        // Regression test for #42853
        THEN( "having another trait does not cancel the TEST_OVERMAP_SIGHT_5 trait" ) {
            dummy.toggle_trait( trait_SMELLY );
            CHECK( dummy.overmap_sight_range( 100.0f ) == 15.0 );
        }
    }

    WHEN( "character has TEST_OVERMAP_SIGHT_MINUS_10 trait" ) {
        dummy.toggle_trait( trait_TEST_OVERMAP_SIGHT_MINUS_10 );
        THEN( "they have reduced overmap sight range" ) {
            CHECK( dummy.overmap_sight_range( 100.0f ) == 3.0 );
        }
        // Regression test for #42853
        THEN( "having another trait does not cancel the TEST_OVERMAP_SIGHT_MINUS_10 trait" ) {
            dummy.toggle_trait( trait_SMELLY );
            CHECK( dummy.overmap_sight_range( 100.0f ) == 3.0 );
        }
    }
}

static void check_test_mutation_is_triggered( const Character &dummy, bool trigger_on )
{
    if( trigger_on ) {
        THEN( "the mutation turns on" ) {
            CHECK( dummy.has_trait( trait_TEST_TRIGGER_active ) );
            CHECK( !dummy.has_trait( trait_TEST_TRIGGER ) );
        }
    } else {
        THEN( "the mutation turns off" ) {
            CHECK( !dummy.has_trait( trait_TEST_TRIGGER_active ) );
            CHECK( dummy.has_trait( trait_TEST_TRIGGER ) );
        }
    }
}

TEST_CASE( "The_various_type_of_triggers_work", "[mutations]" )
{
    Character &dummy = get_player_character();
    clear_avatar();

    WHEN( "character has OR test trigger mutation" ) {
        dummy.toggle_trait( trait_TEST_TRIGGER );

        WHEN( "character is happy" ) {
            dummy.add_morale( morale_perm_debug, 21 );
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
        dummy.toggle_trait( trait_TEST_TRIGGER_2 );

        WHEN( "it is the full moon but character is not in pain" ) {
            static const time_point full_moon = calendar::turn_zero + calendar::season_length() / 6;
            calendar::turn = full_moon;
            INFO( "MOON PHASE : " << io::enum_to_string<moon_phase>( get_moon_phase( calendar::turn ) ) );
            dummy.process_turn();

            THEN( "the mutation stays turned off" ) {
                CHECK( !dummy.has_trait( trait_TEST_TRIGGER_2_active ) );
                CHECK( dummy.has_trait( trait_TEST_TRIGGER_2 ) );
            }
        }

        WHEN( "character is in pain and it's the full moon" ) {
            dummy.set_pain( 120 );
            dummy.process_turn();

            THEN( "the mutation turns on" ) {
                CHECK( dummy.has_trait( trait_TEST_TRIGGER_2_active ) );
                CHECK( !dummy.has_trait( trait_TEST_TRIGGER_2 ) );
            }
        }

        WHEN( "character is no longer in pain" ) {
            dummy.set_pain( 0 );
            dummy.process_turn();

            THEN( "the mutation turns off" ) {
                CHECK( !dummy.has_trait( trait_TEST_TRIGGER_2_active ) );
                CHECK( dummy.has_trait( trait_TEST_TRIGGER_2 ) );
            }
        }
    }

}

TEST_CASE( "All_valid_mutations_can_be_purified", "[mutations][purifier]" )
{
    std::vector<trait_id> dummies;
    std::vector<trait_id> valid_traits;
    for( const mutation_branch &mbra : mutation_branch::get_all() ) {
        if( mbra.dummy ) {
            dummies.push_back( mbra.id );
        } else if( !mbra.debug && mbra.valid && mbra.purifiable && !mbra.category.empty() ) {
            valid_traits.push_back( mbra.id );
        }
    }
    REQUIRE( !dummies.empty() );
    for( const trait_id &checked : valid_traits ) {
        bool is_removable = false;
        GIVEN( "mutation of ID " + checked.str() + " is valid and removable" ) {
            THEN( "a dummy mutation should cancel it or have the same type" ) {
                for( const trait_id &dummy : dummies ) {
                    // First, check if the dummy mutation directly cancels us out
                    if( std::find( dummy->cancels.begin(), dummy->cancels.end(), checked ) != dummy->cancels.end() ) {
                        is_removable = true;
                        break;
                    }
                    // If it doesn't, then check to see if we have a conflicting type
                    for( const std::string &type : dummy->types ) {
                        if( checked->types.count( type ) != 0 ) {
                            is_removable = true;
                            break;
                        }
                    }
                }
                CHECK( is_removable );
            }
        }
    }
}

// Your odds of getting a good or bad mutation depend on what mutations you have and what tree you're in.
// You are given an integer that is first multiplied by 0.5 and then divided by the total non-bad mutations in the tree.
// For example if you mutate into Alpha, that tree has 21 mutations (as of 3/14/2024) that aren't bad.
// Troglobite has 28 mutations that aren't bad. (as of 3/4/2024).
// Thus, for all things equal, Alpha gets more bad mutations per good mutation than Trog,
// but since Trog gets more good muts total it has more chances to get bad mutations.
// The aforementioned "integer" is increased from 0 by 1 for each non-bad mutation you have.
// Mutations you have are counted as two if they don't belong to the tree you're mutating into.
// Starting traits are never counted, and bad mutations are never counted. Only "valid" (mutable) mutations count.
// This test case compares increasing instability in both Alpha and Trog as more mutations are added.
// Given that Alpha and Trog share some mutations but not all, they should either increase instability at the same or different rates.
// This test then also checks and makes sure that Alpha gets more bad mutations than Trog given the same total instability in each.
// After that it checks to make sure each tree gets more bad mutations than before after increasing effective instability by 4x.

TEST_CASE( "Chance_of_bad_mutations_vs_instability", "[mutations][instability]" )
{
    Character &dummy = get_player_character();
    clear_avatar();

    REQUIRE( dummy.get_instability_per_category( mutation_category_ALPHA ) == 0 );
    REQUIRE( dummy.get_instability_per_category( mutation_category_TROGLOBITE ) == 0 );


    WHEN( "character mutates Strong, a mutation belonging to both Alpha and Troglobite" ) {
        dummy.set_mutation( trait_STR_UP );
        THEN( "Both Alpha and Troglobite see their instability increase by 1" ) {
            CHECK( dummy.get_instability_per_category( mutation_category_ALPHA ) == 1 );
            CHECK( dummy.get_instability_per_category( mutation_category_TROGLOBITE ) == 1 );
        }
    }

    WHEN( "they then mutate Very Strong, which requires Strong and also belongs to both trees" ) {
        dummy.set_mutation( trait_STR_UP_2 );
        THEN( "Both Alpha and Troglobite see their instability increase by 1 more (2 total)" ) {
            CHECK( dummy.get_instability_per_category( mutation_category_ALPHA ) == 2 );
            CHECK( dummy.get_instability_per_category( mutation_category_TROGLOBITE ) == 2 );
        }
    }

    WHEN( "they then mutate Quick, which belongs to Troglobite but not Alpha" ) {
        dummy.set_mutation( trait_STR_UP_2 );
        dummy.set_mutation( trait_QUICK );
        THEN( "Alpha increases instability by 2 and Troglobite increases by 1" ) {
            CHECK( dummy.get_instability_per_category( mutation_category_ALPHA ) == 4 );
            CHECK( dummy.get_instability_per_category( mutation_category_TROGLOBITE ) == 3 );
        }
    }

    WHEN( "The character has Quick as a starting trait instead of a mutation" ) {
        dummy.set_mutation( trait_STR_UP_2 );
        dummy.toggle_trait( trait_QUICK );
        REQUIRE( dummy.has_trait( trait_QUICK ) );
        THEN( "Neither Alpha or Troglobite have their instability increased" ) {
            CHECK( dummy.get_instability_per_category( mutation_category_ALPHA ) == 2 );
            CHECK( dummy.get_instability_per_category( mutation_category_TROGLOBITE ) == 2 );
        }
    }

    WHEN( "The character mutates Near Sighted, which is a \"bad\" mutation" ) {
        dummy.set_mutation( trait_STR_UP_2 );
        dummy.set_mutation( trait_MYOPIC );
        THEN( "They do not gain instability since only 0+ point mutations increase that" ) {
            CHECK( dummy.get_instability_per_category( mutation_category_ALPHA ) == 2 );
            CHECK( dummy.get_instability_per_category( mutation_category_TROGLOBITE ) == 2 );
        }
    }

    const int tries = 10000;
    int trogBads = 0;
    int alphaBads = 0;

    clear_avatar();
    dummy.set_mutation( trait_STR_UP_2 );
    REQUIRE( dummy.get_instability_per_category( mutation_category_ALPHA ) == 2 );
    REQUIRE( dummy.get_instability_per_category( mutation_category_TROGLOBITE ) == 2 );

    // 10k trials, compare the number of successes of bad mutations.
    // As of 3/4/2024, Alpha has 21 non-bad mutations and Trog has 28.
    // Given that effective instability is currently 2, Alpha should see avg. 475 badmuts and trog 357 avg.
    // The odds of Trog rolling well enough to "win" are effectively zero if roll_bad_mutation() works correctly.
    // roll_bad_mutation does not actually give a bad mutation; it is called by the primary mutate function.
    for( int i = 0; i < tries; i++ ) {
        if( dummy.roll_bad_mutation( mutation_category_ALPHA ) ) {
            alphaBads++;
        }
        if( dummy.roll_bad_mutation( mutation_category_TROGLOBITE ) ) {
            trogBads++;
        }
    }

    WHEN( "Alpha and Troglobite both have the same level of instability" ) {
        THEN( "Alpha has fewer total mutations, so its odds of a bad mutation are higher than Trog's" ) {
            CHECK( alphaBads > trogBads );
        }
    }

    // Then, increase our instability and try again.
    dummy.set_mutation( trait_FELINE_EARS );
    dummy.set_mutation( trait_EAGLEEYED );
    dummy.set_mutation( trait_GOURMAND );
    REQUIRE( dummy.get_instability_per_category( mutation_category_ALPHA ) == 8 );
    REQUIRE( dummy.get_instability_per_category( mutation_category_TROGLOBITE ) == 8 );

    int trogBads2 = 0;
    int alphaBads2 = 0;

    for( int i = 0; i < tries; i++ ) {
        if( dummy.roll_bad_mutation( mutation_category_ALPHA ) ) {
            alphaBads2++;
        }
        if( dummy.roll_bad_mutation( mutation_category_TROGLOBITE ) ) {
            trogBads2++;
        }
    }

    WHEN( "The player has more mutation instability than before" ) {
        THEN( "They have a higher chance of getting bad mutations than before" ) {
            CHECK( alphaBads2 > alphaBads );
            CHECK( trogBads2 > trogBads );
        }
    }
    clear_avatar();

}

// Verify that flags linked to core mutations are still there.
// If has_trait( trait_XXX )-checks for a certain trait have been 'flagified' to check for
// has_flag( json_flag_XXX ) instead the check should be added here and it's recommended to
// reference to the PR that changes it.
TEST_CASE( "The_mutation_flags_are_associated_to_the_corresponding_base_mutations",
           "[mutations][flags]" )
{
    Character &dummy = get_player_character();

    // From Allow size flags for custom size changing mutations - PR #48850
    verify_mutation_flag( dummy, "LARGE", "LARGE" );
    verify_mutation_flag( dummy, "LARGE_OK", "LARGE" );
    verify_mutation_flag( dummy, "HUGE", "HUGE" );
    verify_mutation_flag( dummy, "HUGE_OK", "HUGE" );
    verify_mutation_flag( dummy, "SMALL", "SMALL" );
    verify_mutation_flag( dummy, "SMALL2", "TINY" );
    verify_mutation_flag( dummy, "SMALL_OK", "TINY" );

    // From Finetuning batrachian mutation (with intended sideeffects) - PR #59458
    verify_mutation_flag( dummy, "WEBBED", "WEBBED_HANDS" );
    verify_mutation_flag( dummy, "WEBBED_FEET", "WEBBED_FEET" );
    verify_mutation_flag( dummy, "SEESLEEP", "SEESLEEP" );

    // From Flagify COLDBLOOD mutations - PR #60052
    verify_mutation_flag( dummy, "COLDBLOOD", "COLDBLOOD" );
    verify_mutation_flag( dummy, "COLDBLOOD2", "COLDBLOOD2" );
    verify_mutation_flag( dummy, "COLDBLOOD3", "COLDBLOOD3" );
    verify_mutation_flag( dummy, "COLDBLOOD4", "ECTOTHERM" );
}

// Test that threshold substitutes work
// Tested using Chimera for the mainline use case
TEST_CASE( "Threshold_substitutions", "[mutations]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    // Check our assumptions
    const std::vector<trait_id> bird_subst = trait_THRESH_BIRD->threshold_substitutes;
    const std::vector<trait_id> alpha_subst = trait_THRESH_ALPHA->threshold_substitutes;
    REQUIRE( std::find( bird_subst.begin(), bird_subst.end(),
                        trait_THRESH_CHIMERA ) != bird_subst.end() );
    REQUIRE( std::find( alpha_subst.begin(), alpha_subst.end(),
                        trait_THRESH_CHIMERA ) == alpha_subst.end() );
    REQUIRE( !trait_BEAK_PECK->threshreq.empty() );
    REQUIRE( !trait_WINGS_BIRD->threshreq.empty() );
    REQUIRE( !trait_BEAK_PECK->strict_threshreq );
    REQUIRE( trait_WINGS_BIRD->strict_threshreq );

    // Character tries to gain post-thresh traits
    for( int i = 0; i < 10; i++ ) {
        dummy.mutate_towards( trait_BEAK_PECK, mutation_category_BIRD, nullptr, false, false );
        dummy.mutate_towards( trait_WINGS_BIRD, mutation_category_BIRD, nullptr, false, false );
        dummy.mutate_towards( trait_STR_ALPHA, mutation_category_ALPHA, nullptr, false, false );
    }
    // We didn't gain any, filled up on prereqs though
    CHECK( !dummy.has_trait( trait_BEAK_PECK ) );
    CHECK( !dummy.has_trait( trait_WINGS_BIRD ) );
    CHECK( !dummy.has_trait( trait_STR_ALPHA ) );
    CHECK( dummy.has_trait( trait_BEAK ) );
    // After gaining Chimera we can mutate a fancy beak, but no wings or Alpha traits
    dummy.set_mutation( trait_THRESH_CHIMERA );
    dummy.mutate_towards( trait_BEAK_PECK, mutation_category_BIRD, nullptr, false, false );
    dummy.mutate_towards( trait_WINGS_BIRD, mutation_category_BIRD, nullptr, false, false );
    dummy.mutate_towards( trait_STR_ALPHA, mutation_category_ALPHA, nullptr, false, false );
    CHECK( dummy.has_trait( trait_BEAK_PECK ) );
    CHECK( !dummy.has_trait( trait_WINGS_BIRD ) );
    CHECK( !dummy.has_trait( trait_STR_ALPHA ) );
}

