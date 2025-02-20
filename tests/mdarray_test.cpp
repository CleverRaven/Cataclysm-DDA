#include "cata_catch.h"
#include "coordinates.h"
#include "map_scale_constants.h"
#include "mdarray.h"

TEST_CASE( "mdarray_default_size_sensible", "[mdarray]" )
{
    using cata::mdarray;
    static_assert( mdarray<int, point_bub_ms>::size_x == MAPSIZE_X );
    static_assert( mdarray<int, point_bub_ms>::size_y == MAPSIZE_Y );
    static_assert( mdarray<int, point_bub_sm>::size_x == MAPSIZE );
    static_assert( mdarray<int, point_bub_sm>::size_y == MAPSIZE );
    static_assert( mdarray<int, point_sm_ms>::size_x == SEEX );
    static_assert( mdarray<int, point_sm_ms>::size_y == SEEY );
    static_assert( mdarray<int, point_om_omt>::size_x == OMAPX );
    static_assert( mdarray<int, point_om_omt>::size_y == OMAPY );
}
