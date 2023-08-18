#include <iosfwd>
#include <memory>
#include <vector>

#include "cata_catch.h"
#include "item_factory.h"
#include "itype.h"
#include "type_id.h"
#include "units.h"

TEST_CASE( "non-solid-comestibles-are-stackable", "[item]" )
{
    for( const itype *t :  item_controller->all() ) {
        if( t->comestible && t->phase != phase_id::SOLID ) {
            INFO( "liquid: " << t->get_id().str() )
            CHECK( t->count_by_charges() );
        }
    }
}
