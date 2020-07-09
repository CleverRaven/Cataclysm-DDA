#include <algorithm>
#include <sstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "catch/catch.hpp"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "player.h"
#include "string_id.h"
#include "type_id.h"

static const efftype_id effect_accumulated_mutagen( "accumulated_mutagen" );

std::string get_mutations_as_string( const player &p );

// Note: If a category has two mutually-exclusive mutations (like pretty/ugly for Lupine), the
// one they ultimately end up with depends on the order they were loaded from JSON
static void give_all_mutations( player &p, const mutation_category_trait &category,
                                const bool include_postthresh )
{
    const std::vector<trait_id> category_mutations = mutations_category[category.id];

    // Add the threshold mutation first
    if( include_postthresh && !category.threshold_mut.is_empty() ) {
        p.set_mutation( category.threshold_mut );
    }

    for( auto &m : category_mutations ) {
        const auto &mdata = m.obj();
        if( include_postthresh || ( !mdata.threshold && mdata.threshreq.empty() ) ) {
            int mutation_attempts = 10;
            while( mutation_attempts > 0 && p.mutation_ok( m, false, false ) ) {
                INFO( "Current mutations: " << get_mutations_as_string( p ) );
                INFO( "Mutating towards " << m.c_str() );
                if( !p.mutate_towards( m ) ) {
                    --mutation_attempts;
                }
                CHECK( mutation_attempts > 0 );
            }
        }
    }
}

static int get_total_category_strength( const player &p )
{
    int total = 0;
    for( auto &i : p.mutation_category_level ) {
        total += i.second;
    }

    return total;
}

// Returns the list of mutations a player has as a string, for debugging
std::string get_mutations_as_string( const player &p )
{
    std::ostringstream s;
    for( auto &m : p.get_mutations() ) {
        s << static_cast<std::string>( m ) << " ";
    }
    return s.str();
}

TEST_CASE( "Having all mutations give correct highest category", "[mutations]" )
{
    for( auto &cat : mutation_category_trait::get_all() ) {
        const auto &cur_cat = cat.second;
        const auto &cat_id = cur_cat.id;
        if( cat_id == "ANY" ) {
            continue;
        }

        GIVEN( "The player has all pre-threshold mutations for " + cat_id ) {
            npc dummy;
            give_all_mutations( dummy, cur_cat, false );

            THEN( cat_id + " is the strongest category" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( dummy.get_highest_category() == cat_id );
            }
        }

        GIVEN( "The player has all mutations for " + cat_id ) {
            npc dummy;
            give_all_mutations( dummy, cur_cat, true );

            THEN( cat_id + " is the strongest category" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( dummy.get_highest_category() == cat_id );
            }
        }
    }
}

TEST_CASE( "Having all pre-threshold mutations gives a sensible threshold breach chance",
           "[mutations]" )
{
    const float BREACH_CHANCE_MIN = 0.2f;
    const float BREACH_CHANCE_MAX = 0.4f;

    for( auto &cat : mutation_category_trait::get_all() ) {
        const auto &cur_cat = cat.second;
        const auto &cat_id = cur_cat.id;
        if( cat_id == "ANY" ) {
            continue;
        }

        GIVEN( "The player has all pre-threshold mutations for " + cat_id ) {
            npc dummy;
            give_all_mutations( dummy, cur_cat, false );

            const int category_strength = dummy.mutation_category_level[cat_id];
            const int total_strength = get_total_category_strength( dummy );
            float breach_chance = category_strength / static_cast<float>( total_strength );

            THEN( "Threshold breach chance is at least 0.2" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( breach_chance >= BREACH_CHANCE_MIN );
            }
            THEN( "Threshold breach chance is at most 0.4" ) {
                INFO( "MUTATIONS: " << get_mutations_as_string( dummy ) );
                CHECK( breach_chance <= BREACH_CHANCE_MAX );
            }
        }
    }
}

static float sum_without_category( const std::map<trait_id, float> &chances, std::string cat )
{
    float sum = 0.0f;
    for( const std::pair<trait_id, float> &c : chances ) {
        const auto &mut_categories = c.first->category;
        if( std::find( mut_categories.begin(), mut_categories.end(), cat ) == mut_categories.end() ) {
            sum += c.second;
        }
    }

    return sum;
}

TEST_CASE( "Gaining a mutation in category makes mutations from other categories less likely",
           "[mutations]" )
{
    for( auto &cat : mutation_category_trait::get_all() ) {
        const auto &cur_cat = cat.second;
        const auto &cat_id = cur_cat.id;
        if( cat_id == "ANY" ) {
            continue;
        }

        npc zero_mut_dummy;
        std::map<trait_id, float> chances_pre = zero_mut_dummy.mutation_chances();
        float sum_pre = sum_without_category( chances_pre, cat_id );
        for( const mutation_branch &mut : mutation_branch::get_all() ) {
            if( zero_mut_dummy.mutation_ok( mut.id, false, false ) ) {
                continue;
            }
            GIVEN( "The player gains a mutation " + mut.name() ) {
                npc dummy;
                dummy.mutate_towards( mut.id );
                THEN( "Sum of chances for mutations not of this category is lower than before" ) {
                    std::map<trait_id, float> chances_post = dummy.mutation_chances();
                    float sum_post = sum_without_category( chances_post, cat_id );
                    CHECK( sum_post < sum_pre );
                }
            }
        }
    }
}

TEST_CASE( "Mutating with full mutagen accumulation results in multiple mutations",
           "[mutations]" )
{
    REQUIRE( get_option<bool>( "BALANCED_MUTATIONS" ) );
    GIVEN( "Player with maximum intensity accumulated mutagen" ) {
        npc dummy;
        dummy.add_effect( effect_accumulated_mutagen, 30_days, num_bp, true );
        AND_GIVEN( "The player mutates" ) {
            dummy.mutate();
            THEN( "The player has >3 mutations" ) {
                CHECK( dummy.get_mutations().size() > 3 );
            }
        }
    }
}
