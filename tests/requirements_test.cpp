#include <map>
#include <set>
#include <string>
#include <utility>

#include "catch/catch.hpp"
#include "item.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "type_id.h"

TEST_CASE( "No book grant a recipe at skill levels above autolearn", "[recipe]" )
{
    for( const recipe *rec : recipe_dict.all_autolearn() ) {
        // Booklearning requires only one skill - the primary one
        // If autolearn requires any other skill, booklearn is not 100% useless
        if( rec->autolearn_requirements.size() > 1 ||
            rec->autolearn_requirements.count( rec->skill_used ) < 1 ) {
            continue;
        }
        for( const auto &p : rec->booksets ) {
            const std::string recipe_ident = rec->ident().str();
            const std::string item_type = rec->result();
            const std::string item_name = item::nname( rec->result() );
            const std::string bookname = item::nname( p.first );
            CAPTURE( recipe_ident );
            CAPTURE( item_type );
            CAPTURE( item_name );
            CAPTURE( bookname );
            CHECK( p.second < rec->autolearn_requirements.begin()->second );
        }
    }
}
