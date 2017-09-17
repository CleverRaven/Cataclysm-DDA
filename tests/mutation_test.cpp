#include "catch/catch.hpp"

#include "mutation.h"

#include "game.h"
#include "player.h"

void clear_mutations( player &p )
{
    while ( !p.get_mutations().empty() ) {
        // Use the "real" way to remove mutations to ensure we don't get into a bad state
        p.remove_mutation( p.get_mutations()[0] );
    }
}

// Note: If a category has two mutually-exclusive mutations (like pretty/ugly for Lupine), the
// one they ultimately end up with depends on the order they were loaded from JSON
void give_all_mutations( player &p, std::string category, bool include_postthresh )
{
    const std::vector<trait_id> &mutations = p.get_mutations();
    const std::vector<trait_id> category_mutations = mutations_category["MUTCAT_" + category];

    // Add the threshold mutation first
    if (include_postthresh) {
        p.set_mutation(trait_id( "THRESH_" + category ));
    }

    for (auto &m : category_mutations) {
        const auto &mdata = m.obj();
        if ( include_postthresh || (!mdata.threshold && mdata.threshreq.empty()) ) {
            while ( p.mutation_ok( m, false, false ) ) {
                p.mutate_towards( m );
            }
        }
    }
}

int get_total_category_strength( player &p )
{
    int total = 0;
    for (auto &i : p.mutation_category_level) {
        total += i.second;
    }

    return total;
}


TEST_CASE( "Having all mutations give correct highest category" )
{
    player &dummy = g->u;

    for ( auto &cat : mutation_category_traits ) {
        auto cat_id = cat.second.category;
        
        SECTION( cat_id ) {
            GIVEN( "The player has all pre-threshold mutations for " + cat_id ) {
                clear_mutations( dummy );
                give_all_mutations( dummy, cat_id, false );

                THEN( cat_id + " is the strongest category" ) {
                    CHECK( dummy.get_highest_category() == "MUTCAT_" + cat_id );
                }
            }

            GIVEN( "The player has all mutations for " + cat_id ) {
                clear_mutations( dummy );
                give_all_mutations( dummy, cat_id, true );

                THEN( cat_id + " is the strongest category" ) {
                    CHECK( dummy.get_highest_category() == "MUTCAT_" + cat_id );
                }
            }
        }
    }
}
