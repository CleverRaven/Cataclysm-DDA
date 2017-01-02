#include "catch/catch.hpp"

#include "itype.h"
#include "item.h"
#include "player.h"
#include "profession.h"
#include "material.h"
#include "string_id.h"

#include <vector>
#include <string>
#include <algorithm>

TEST_CASE( "Ensure that starting items do not conflict with certain traits" ) {
    const std::vector<profession> &all_profs = profession::get_all();
    const std::vector<std::string> mutations = {
        "ANTIFRUIT", // Hates fruit
        "MEATARIAN", // Hates vegetables
        "ANTIJUNK", // Junkfood intolerance
        "LACTOSE", // Lactose intolerance
        "VEGETARIAN", // Meat intolerance
        "ANTIWHEAT", // Wheat allergy
        "WOOLALLERGY" // Wool allergy
    };

    struct failure {
        string_id<profession> prof;
        std::string mut;
        itype_id item_name;
        std::string reason;
    };
    std::vector<failure> failures;

    auto add_failure = [&]( const profession &prof, const std::string &mut, const std::string &item_name, const std::string &reason ) {
        if( std::any_of( failures.begin(), failures.end(), [&item_name]( const failure &f ) {
            return f.item_name == item_name;
            } ) ) {
            return;
        }
        failures.push_back( failure{ prof.ident(), mut, item_name, reason } );
    };

    for( const profession &prof : all_profs ) {
        std::vector<item> items = prof.items( true );
        const auto female = prof.items( false );
        items.insert( items.begin(), female.begin(), female.end() );
        for( const std::string &mut : mutations ) {
            player u;
            u.recalc_hp();
            for( int i = 0; i < num_hp_parts; i++ ) {
                u.hp_cur[i] = u.hp_max[i];
            }
            u.toggle_trait( mut );

            for( const item &it : items ) {
                // Seeds don't count- they're for growing things, not eating
                if( !it.is_seed() && it.is_food() && u.can_eat( it, false, false ) != EDIBLE ) {
                    add_failure( prof, mut, it.type->get_id(), "Couldn't eat it" );
                } else if( it.is_armor() && !u.can_wear( it, false ) ) {
                    add_failure( prof, mut, it.type->get_id(), "Couldn't wear it" );
                }
            }
        }
    }

    for( const failure &f : failures ) {
        printf( "%s, %s, %s: %s\n", f.prof.c_str(), f.mut.c_str(), f.item_name.c_str(), f.reason.c_str() );
    }
    REQUIRE( failures.empty() );
}

