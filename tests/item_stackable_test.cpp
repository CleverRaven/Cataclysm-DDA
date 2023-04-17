#include <iosfwd>
#include <memory>
#include <vector>

#include "cata_catch.h"
#include "item_factory.h"
#include "itype.h"
#include "type_id.h"
#include "units.h"

TEST_CASE( "casings_are_stackable", "[item]" )
{
    for( const itype *t :  item_controller->all() ) {
        bool casing = t->get_id().str().find( "_casing" ) != std::string::npos &&
                      t->volume < 200_ml;

        if( casing ) {
            INFO( "casing: " << t->get_id().str() )
            CHECK( t->count_by_charges() );
        }
    }
}
