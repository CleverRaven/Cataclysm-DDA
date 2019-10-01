#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <functional>
#include <list>
#include <memory>
#include <utility>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "profession.h"
#include "scenario.h"
#include "string_id.h"
#include "optional.h"
#include "pldata.h"
#include "ret_val.h"
#include "type_id.h"

static std::ostream &operator<<( std::ostream &s, const std::vector<trait_id> &v )
{
    for( const auto &e : v ) {
        s << e.c_str() << " ";
    }
    return s;
}

static std::vector<trait_id> next_subset( const std::vector<trait_id> &set )
{
    // Doing it this way conveniently returns a vector containing solely set[foo] before
    // it returns any other vectors with set[foo] in it
    static unsigned bitset = 0;
    std::vector<trait_id> ret;

    ++bitset;
    // Check each bit position for a match
    for( size_t idx = 0; idx < set.size(); idx++ ) {
        if( bitset & ( 1 << idx ) ) {
            ret.push_back( set[idx] );
        }
    }
    return ret;
}

static bool try_set_traits( const std::vector<trait_id> &traits )
{
    g->u.empty_traits();
    g->u.add_traits(); // mandatory prof/scen traits
    for( const trait_id &tr : traits ) {
        if( g->u.has_conflicting_trait( tr ) || !g->scen->traitquery( tr ) ) {
            return false;
        } else if( !g->u.has_trait( tr ) ) {
            g->u.set_mutation( tr );
        }
    }
    return true;
}

static avatar get_sanitized_player()
{
    // You'd think that this hp stuff would be in the c'tor...
    avatar ret = avatar();
    ret.recalc_hp();
    for( int i = 0; i < num_hp_parts; i++ ) {
        ret.hp_cur[i] = ret.hp_max[i];
    }
    // Set these insanely high so can_eat doesn't return TOO_FULL
    ret.set_hunger( 10000 );
    ret.set_thirst( 10000 );
    return ret;
}

struct failure {
    string_id<profession> prof;
    std::vector<trait_id> mut;
    itype_id item_name;
    std::string reason;
};

namespace std
{
template<>
struct less<failure> {
    bool operator()( const failure &lhs, const failure &rhs ) const {
        return lhs.prof < rhs.prof;
    }
};
} // namespace std

// TODO: According to profiling (interrupt, backtrace, wait a few seconds, repeat) with a sample
// size of 20, 70% of the time is due to the call to Character::set_mutation in try_set_traits.
// When the mutation stuff isn't commented out, the test takes 110 minutes (not a typo)!

TEST_CASE( "starting_items" )
{
    // Every starting trait that interferes with food/clothing
    const std::vector<trait_id> mutations = {
        trait_id( "ANTIFRUIT" ),
        trait_id( "ANTIJUNK" ),
        trait_id( "ANTIWHEAT" ),
        //trait_id( "ARM_TENTACLES" ),
        //trait_id( "BEAK" ),
        trait_id( "CANNIBAL" ),
        //trait_id( "CARNIVORE" ),
        //trait_id( "HERBIVORE" ),
        //trait_id( "HOOVES" ),
        trait_id( "LACTOSE" ),
        //trait_id( "LEG_TENTACLES" ),
        trait_id( "MEATARIAN" ),
        //trait_id( "RAP_TALONS" ),
        //trait_id( "TAIL_FLUFFY" ),
        //trait_id( "TAIL_LONG" ),
        trait_id( "VEGETARIAN" ),
        trait_id( "WOOLALLERGY" )
    };
    // Prof/scen combinations that need to be checked.
    std::unordered_map<const scenario *, std::vector<string_id<profession>>> scen_prof_combos;
    for( const auto &id : scenario::generic()->permitted_professions() ) {
        scen_prof_combos[scenario::generic()].push_back( id );
    }

    std::set<failure> failures;

    g->u = get_sanitized_player();
    // Avoid false positives from ingredients like salt and cornmeal.
    const avatar control = get_sanitized_player();

    std::vector<trait_id> traits = next_subset( mutations );
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
                        const bool is_food =  !it.is_seed() && it.is_food() &&
                                              !g->u.can_eat( it ).success() && control.can_eat( it ).success();
                        const bool is_armor = it.is_armor() && !g->u.wear_item( it, false );
                        // Seeds don't count- they're for growing things, not eating
                        if( is_food || is_armor ) {
                            failures.insert( failure{ prof->ident(), g->u.get_mutations(), it.typeId(), is_food ? "Couldn't eat it" : "Couldn't wear it." } );
                        }

                        const bool is_holster = it.is_armor() && it.type->get_use( "holster" );
                        if( is_holster ) {
                            const item &holstered_it = it.get_contained();
                            const bool empty_holster = holstered_it.is_null();
                            if( !empty_holster && !it.can_holster( holstered_it, true ) ) {
                                failures.insert( failure{ prof->ident(), g->u.get_mutations(), it.typeId(), "Couldn't put item back to holster" } );
                            }
                        }
                    }
                } // all genders
            } // all profs
        } // all scens
    }
    std::stringstream failure_messages;
    for( const failure &f : failures ) {
        failure_messages << f.prof.c_str() << " " << f.mut <<
                         " " << f.item_name << ": " << f.reason << "\n";
    }
    INFO( failure_messages.str() );
    REQUIRE( failures.empty() );
}

