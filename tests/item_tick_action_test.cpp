#include "cata_catch.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "type_id.h"

static const itype_id itype_chainsaw_off( "chainsaw_off" );
static const itype_id itype_chainsaw_on( "chainsaw_on" );

TEST_CASE( "tick_action_triggering", "[item]" )
{
    item chainsaw( itype_chainsaw_on );
    chainsaw.active = true;

    // The chainsaw has no fuel and turns off via its tick_action

    chainsaw.process( get_map(), nullptr, tripoint_bub_ms::zero );
    CHECK( chainsaw.typeId() == itype_chainsaw_off );
    CHECK( chainsaw.active == false );
}
