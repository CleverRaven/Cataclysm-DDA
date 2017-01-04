#include "catch/catch.hpp"

#include "game.h"
#include "itype.h"
#include "item.h"
#include "player.h"
#include "profession.h"
#include "scenario.h"
#include "mutation.h"
#include "string_id.h"

#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

std::vector<std::string> next_subset( const std::vector<std::string> &set )
{
    // Doing it this way conveniently returns a vector containing solely set[foo] before
    // it returns any other vectors with set[foo] in it
    static unsigned bitset = 0;
    std::vector<std::string> ret;

    ++bitset;
    // Check each bit position for a match
    for( unsigned i = 1; i < 1 << (unsigned)set.size(); i = i << 1 ) {
        if( i & bitset ) {
            // Find the index of the lowest set bit
            unsigned idx = 0;
            while( ( i & ( 1 << idx ) ) == 0 ) {
                idx++;
            }
            ret.push_back( set[idx] );
        }
    }
    return ret;
}

bool try_add_traits( const std::vector<std::string> &traits )
{
    // TODO: Similar to below
    for( const std::string &tr : traits ) {
        if( g->scen->forbidden_traits( tr ) || g->u.has_conflicting_trait( tr ) ) {
            return false;
        } else if( !g->scen->traitquery( tr ) && !mutation_branch::get( tr ).startingtrait ) {
            return false;
        } else if( !g->u.has_trait( tr ) ) {
            g->u.set_mutation( tr );
        }
    }
    return true;
}

bool need_to_check_scenario( const scenario &scen, const profession &p,
                             const std::vector<const scenario *> &special_scenarios )
{
    // TODO: Checking if a profession is allowed would be a lot easier without all these function calls-
    // Rebasing after the `Allow scenarios to blacklist professions` PR is merged will make it a bit easier,
    // and the rest can be refactored
    if( std::find( special_scenarios.begin(), special_scenarios.end(), &scen ) != special_scenarios.end() &&
        ( ( scen.profsize() == 0 && !p.has_flag( "SCEN_ONLY" ) ) || scen.profquery( p.ident() ) ) ) {
        return true;
    }

    if( !p.has_flag( "SCEN_ONLY" ) ) {
        return &scen == scenario::generic();
    }
    return scen.profsize() == 0 || scen.profquery( p.ident() );
}

TEST_CASE( "Ensure that starting items do not conflict with certain traits" ) {
    const std::vector<std::string> mutations = {
        "ANTIFRUIT",
        "ANTIJUNK",
        "ANTIWHEAT",
        "CANNIBAL",
        "CARNIVORE",
        "HERBIVORE",
        "LACTOSE",
        "MEATARIAN",
        "VEGETARIAN",
        "WOOLALLERGY"
    };


    /**
    * Scenarios that allow a mutation trait to be chosen must always be checked, for
    * every legal profession.
    */
    std::vector<const scenario *> special_scenarios;
    for( const scenario &scen : scenario::get_all() ) {
        for( const auto &mut : mutation_branch::get_all() ) {
            if( !mut.second.startingtrait && scen.traitquery( mut.first ) ) {
                special_scenarios.push_back( &scen );
                break;
            }
        }
    }

    struct failure {
        string_id<profession> prof;
        std::vector<std::string> mut;
        itype_id item_name;
        std::string reason;
    };
    std::vector<failure> failures;

    auto add_failure = [&]( const profession &prof, const std::vector<std::string> &traits,
                            const std::string &item_name, const std::string &reason ) {
        if( std::any_of( failures.begin(), failures.end(), [&item_name]( const failure &f ) {
            return f.item_name == item_name;
            } ) ) {
            return;
        }
        failures.push_back( failure{ prof.ident(), traits, item_name, reason } );
    };

    g->u = player();
    // You'd think that this hp stuff would be in the c'tor...
    g->u.recalc_hp();
    for( int i = 0; i < num_hp_parts; i++ ) {
        g->u.hp_cur[i] = g->u.hp_max[i];
    }

    // Which combinations need checked
    std::unordered_map<const scenario *, std::vector<const profession *>> scen_prof_combos;
    for( const profession &prof : profession::get_all() ) {
        for( const scenario &scen : scenario::get_all() ) {
            if( need_to_check_scenario( scen, prof, special_scenarios ) ) {
                scen_prof_combos[&scen].push_back( &prof );
            }
        }
    }

    std::vector<std::string> traits = next_subset( mutations );
    for( ; !traits.empty(); traits = next_subset( mutations ) ) {
        for( const auto &pair : scen_prof_combos ) {
            g->scen = pair.first;
            for( const profession *prof : pair.second ) {
                g->u.prof = prof;
                g->u.empty_traits();
                g->u.add_traits(); // Add mandatory scen/prof traits
                if( !try_add_traits( traits ) ) {
                    continue; // There's a trait conflict somewhere
                }

                std::list<item> items = prof->items( true, traits );
                const auto female = prof->items( false, traits );
                items.insert( items.begin(), female.begin(), female.end() );

                for( const item &it : items ) {
                    // Seeds don't count- they're for growing things, not eating
                    if( !it.is_seed() && it.is_food() && g->u.can_eat( it, false, false ) != EDIBLE ) {
                        add_failure( *prof, g->u.get_mutations(), it.type->get_id(), "Couldn't eat it" );
                    } else if( it.is_armor() && !g->u.can_wear( it, false ) ) {
                        add_failure( *prof, g->u.get_mutations(), it.type->get_id(), "Couldn't wear it" );
                    }
                }
            } // all profs
        } // all scens
    }

    for( const failure &f : failures ) {
        std::cout << f.prof.c_str() << " " << f.mut << " " << f.item_name << ": " << f.reason << "\n";
    }
    REQUIRE( failures.empty() );
}

