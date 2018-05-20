#include "catch/catch.hpp"

#include "mutation.h"

#include "game.h"
#include "npc.h"
#include "player.h"

// Note: If a category has two mutually-exclusive mutations (like pretty/ugly for Lupine), the
// one they ultimately end up with depends on the order they were loaded from JSON
void give_all_mutations( player &p, const mutation_category_trait &category,
                         bool include_postthresh )
{
    const std::vector<trait_id> category_mutations = mutations_category[category.id];

    // Add the threshold mutation first
    if( include_postthresh && !category.threshold_mut.is_empty() ) {
        p.set_mutation( category.threshold_mut );
    }

    for( auto &m : category_mutations ) {
        const auto &mdata = m.obj();
        if( include_postthresh || ( !mdata.threshold && mdata.threshreq.empty() ) ) {
            while( p.mutation_ok( m, false, false ) ) {
                p.mutate_towards( m );
            }
        }
    }
}

int get_total_category_strength( const player &p )
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
        s << ( std::string ) m << " ";
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

            int category_strength = dummy.mutation_category_level[cat_id];
            int total_strength = get_total_category_strength( dummy );
            float breach_chance = category_strength / ( float ) total_strength;

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
