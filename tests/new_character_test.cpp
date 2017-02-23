#include "catch/catch.hpp"

#include "game.h"
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

static std::vector<std::string> next_subset( const std::vector<std::string> &set )
{
    // Doing it this way conveniently returns a vector containing solely set[foo] before
    // it returns any other vectors with set[foo] in it
    static unsigned bitset = 0;
    std::vector<std::string> ret;

    ++bitset;
    // Check each bit position for a match
    for( unsigned idx = 0; idx < set.size(); idx++ ) {
        if( bitset & ( 1 << idx ) ) {
            ret.push_back( set[idx] );
        }
    }
    return ret;
}

static bool try_set_traits( const std::vector<std::string> &traits )
{
    g->u.empty_traits();
    g->u.add_traits(); // mandatory prof/scen traits
    for( const std::string &tr : traits ) {
        if( g->u.has_conflicting_trait( tr ) || !g->scen->traitquery( tr ) ) {
            return false;
        } else if( !g->u.has_trait( tr ) ) {
            g->u.set_mutation( tr );
        }
    }
    return true;
}

static player get_sanitized_player()
{
    // You'd think that this hp stuff would be in the c'tor...
    player ret = player();
    ret.recalc_hp();
    for( int i = 0; i < num_hp_parts; i++ ) {
        ret.hp_cur[i] = ret.hp_max[i];
    }
    // Set these insanely high so can_eat doesn't return TOO_FULL
    ret.set_hunger( 10000 );
    ret.set_thirst( 10000 );
    return ret;
}

// TODO: According to profiling (interrupt, backtrace, wait a few seconds, repeat) with a sample
// size of 20, 70% of the time is due to the call to Character::set_mutation in try_set_traits.
// When the mutation stuff isn't commented out, the test takes 110 minutes (not a typo)!

TEST_CASE( "starting_items" ) {
    // Every starting trait that interferes with food/clothing
    const std::vector<std::string> mutations = {
        "ANTIFRUIT",
        "ANTIJUNK",
        "ANTIWHEAT",
        //"ARM_TENTACLES",
        //"BEAK",
        "CANNIBAL",
        //"CARNIVORE",
        //"HERBIVORE",
        //"HOOVES",
        "LACTOSE",
        //"LEG_TENTACLES",
        "MEATARIAN",
        //"RAP_TALONS",
        //"TAIL_FLUFFY",
        //"TAIL_LONG",
        "VEGETARIAN",
        "WOOLALLERGY"
    };
    // Prof/scen combinations that need to be checked.
    std::unordered_map<const scenario *, std::vector<string_id<profession>>> scen_prof_combos;
    for( const auto &id : scenario::generic()->permitted_professions() ) {
        scen_prof_combos[scenario::generic()].push_back( id );
    }
    /*for( const scenario &scen : scenario::get_all() ) {
        const bool special = std::any_of( mutation_branch::get_all().begin(), mutation_branch::get_all().end(),
            [&scen]( const std::pair<std::string, mutation_branch> &elem ) {
            return !elem.second.startingtrait && scen.traitquery( elem.first );
        } );
        if( !special && &scen != scenario::generic() ) {
            // The only scenarios that need checked are the ones that give access to mutation traits, and
            // the generic scenario
            continue;
        }
        for( const auto &id : scen.permitted_professions() ) {
            scen_prof_combos[&scen].push_back( id );
        }
    }*/

    struct failure {
        string_id<profession> prof;
        std::vector<std::string> mut;
        itype_id item_name;
        std::string reason;
    };
    std::vector<failure> failures;

    auto add_failure = [&]( const profession &prof, const std::vector<std::string> &traits,
                            const std::string &item_name, const std::string &reason ) {
        if( !std::any_of( failures.begin(), failures.end(), [&item_name]( const failure &f ) {
            return f.item_name == item_name;
            } ) ) {
            failures.push_back( failure{ prof.ident(), traits, item_name, reason } );
        }
    };

    g->u = get_sanitized_player();
    const player control = get_sanitized_player(); // Avoid false positives from ingredients like salt and cornmeal

    std::vector<std::string> traits = next_subset( mutations );
    for( ; !traits.empty(); traits = next_subset( mutations ) ) {
        for( const auto &pair : scen_prof_combos ) {
            g->scen = pair.first;
            for( const string_id<profession> &prof : pair.second ) {
                g->u.prof = &prof.obj();
                if( !try_set_traits( traits ) ) {
                    continue; // Trait conflict: this prof/scen/trait combo is impossible to attain
                }
                for( int i = 0; i < 2; i++ ) {
                    g->u.worn.clear();
                    g->u.reset_encumbrance();
                    g->u.male = i == 0;
                    std::list<item> items = prof->items( g->u.male, traits );
                    for( const item &it : items ) {
                        items.insert( items.begin(), it.contents.begin(), it.contents.end() );
                    }

                    for( const item &it : items ) {
                        // Seeds don't count- they're for growing things, not eating
                        if( !it.is_seed() && it.is_food() && g->u.can_eat( it, false, false ) != EDIBLE &&
                            control.can_eat( it, false, false ) == EDIBLE ) {
                            add_failure( *prof, g->u.get_mutations(), it.typeId(), "Couldn't eat it" );
                        } else if( it.is_armor() && !g->u.wear_item( it, false ) ) {
                            add_failure( *prof, g->u.get_mutations(), it.typeId(), "Couldn't wear it" );
                        }
                    }
                } // all genders
            } // all profs
        } // all scens
    }

    for( const failure &f : failures ) {
        std::cout << f.prof.c_str() << " " << f.mut << " " << f.item_name << ": " << f.reason << "\n";
    }
    REQUIRE( failures.empty() );
}

