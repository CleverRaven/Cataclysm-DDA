#include "catch/catch.hpp"

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "avatar.h"
#include "inventory.h"
#include "item.h"
#include "pimpl.h"
#include "profession.h"
#include "scenario.h"
#include "string_formatter.h"
#include "type_id.h"
#include "visitable.h"

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
    avatar &player_character = get_avatar();
    player_character.clear_mutations();
    player_character.add_traits(); // mandatory prof/scen traits
    for( const trait_id &tr : traits ) {
        if( player_character.has_conflicting_trait( tr ) ||
            !get_scenario()->traitquery( tr ) ) {
            return false;
        } else if( !player_character.has_trait( tr ) ) {
            player_character.set_mutation( tr );
        }
    }
    return true;
}

static avatar get_sanitized_player()
{
    // You'd think that this hp stuff would be in the c'tor...
    avatar ret = avatar();
    ret.set_body();
    ret.recalc_hp();

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

/**
 * Disabled temporarily because 3169 profession combinations do not work and need to be fixed in json
 */
TEST_CASE( "starting_items", "[slow]" )
{
    // Every starting trait that interferes with food/clothing
    const std::vector<trait_id> mutations = {
        trait_id( "ANTIFRUIT" ),
        trait_id( "ANTIJUNK" ),
        trait_id( "ANTIWHEAT" ),
        //trait_id( "ARM_TENTACLES" ),
        //trait_id( "BEAK" ),
        //trait_id( "CARNIVORE" ),
        //trait_id( "HERBIVORE" ),
        //trait_id( "HOOVES" ),
        trait_id( "LACTOSE" ),
        //trait_id( "LEG_TENTACLES" ),
        trait_id( "MEATARIAN" ),
        trait_id( "ASTHMA" ),
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

    avatar &player_character = get_avatar();
    player_character = get_sanitized_player();
    // Avoid false positives from ingredients like salt and cornmeal.
    const avatar control = get_sanitized_player();

    std::vector<trait_id> traits = next_subset( mutations );
    for( ; !traits.empty(); traits = next_subset( mutations ) ) {
        for( const auto &pair : scen_prof_combos ) {
            set_scenario( pair.first );
            for( const string_id<profession> &prof : pair.second ) {
                player_character.prof = &prof.obj();
                if( !try_set_traits( traits ) ) {
                    continue; // Trait conflict: this prof/scen/trait combo is impossible to attain
                }
                for( int i = 0; i < 2; i++ ) {
                    player_character.worn.clear();
                    player_character.remove_weapon();
                    player_character.inv->clear();
                    player_character.calc_encumbrance();
                    player_character.male = i == 0;

                    player_character.add_profession_items();
                    std::set<const item *> items_visited;
                    const auto visitable_counter = [&items_visited]( const item * it ) {
                        items_visited.emplace( it );
                        return VisitResponse::NEXT;
                    };
                    player_character.visit_items( visitable_counter );
                    player_character.inv->visit_items( visitable_counter );
                    const int num_items_pre_migration = items_visited.size();
                    items_visited.clear();

                    player_character.migrate_items_to_storage( true );
                    player_character.visit_items( visitable_counter );
                    const int num_items_post_migration = items_visited.size();
                    items_visited.clear();

                    if( num_items_pre_migration != num_items_post_migration ) {
                        failure cur_fail;
                        cur_fail.prof = player_character.prof->ident();
                        cur_fail.mut = player_character.get_mutations();
                        cur_fail.reason = string_format( "does not have enough space to store all items." );

                        failures.insert( cur_fail );
                    }
                    CAPTURE( player_character.prof->ident().c_str() );
                    CHECK( num_items_pre_migration == num_items_post_migration );
                } // all genders
            } // all profs
        } // all scens
    }
    std::stringstream failure_messages;
    for( const failure &f : failures ) {
        failure_messages << f.prof.c_str() << " " << f.mut <<
                         " " << f.item_name.str() << ": " << f.reason << "\n";
    }
    INFO( failure_messages.str() );
    REQUIRE( failures.empty() );
}
