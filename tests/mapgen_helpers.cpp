#include "mapgen_helpers.h"

#include <map>
#include <memory>
#include <string>

#include "calendar.h"
#include "coordinates.h"
#include "map.h"
#include "mapgen.h"
#include "mapgen_functions.h"
#include "mapgendata.h"
#include "overmapbuffer.h"
#include "weighted_list.h"

static const oter_str_id oter_field( "field" );

void common_mapgen_prep( tripoint_abs_omt const &pos, fprep_t const &prep )
{
    if( prep ) {
        tinymap tm;
        tm.load( pos, true );
        prep( *tm.cast_to_map() );
    }
}

void manual_update_mapgen( tripoint_abs_omt const &pos, update_mapgen_id const &id )
{
    // make sure we don't rotate the update
    overmap_buffer.ter_set( pos, oter_field.id() );
    run_mapgen_update_func( id, pos, {}, nullptr, false );
}

void manual_nested_mapgen( tripoint_abs_omt const &pos, nested_mapgen_id const &id )
{
    tinymap tm;
    tm.load( pos, true );
    mapgendata md( pos, *tm.cast_to_map(), 0.0f, calendar::turn, nullptr );
    const auto &ptr = nested_mapgens[id].funcs().pick();
    ( *ptr )->nest( md, tripoint_rel_ms::zero, "test" );
}
