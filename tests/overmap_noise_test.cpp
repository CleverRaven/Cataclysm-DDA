#include <string>

#include "cata_catch.h"
#include "coordinates.h"
#include "map_scale_constants.h"
#include "overmap_debug.h"
#include "overmap_noise.h"
#include "point.h"

TEST_CASE( "om_noise_layer_forest_export", "[.]" )
{
    const om_noise::om_noise_layer_forest f( point_abs_omt(), 1920237457 );
    om_debug::export_raw_noise( "forest-map-raw.pgm", f, OMAPX, OMAPY );
}

TEST_CASE( "om_noise_layer_floodplain_export", "[.]" )
{
    const om_noise::om_noise_layer_floodplain f( point_abs_omt(), 1920237457 );
    om_debug::export_raw_noise( "floodplain-map-raw.pgm", f, OMAPX, OMAPY );
}

TEST_CASE( "om_noise_layer_lake_export", "[.]" )
{
    const om_noise::om_noise_layer_lake f( point_abs_omt(), 1920237457 );
    om_debug::export_raw_noise( "lake-map-raw.pgm", f, OMAPX * 5, OMAPY * 5 );
    om_debug::export_interpreted_noise( "lake-map-interp.pgm", f, OMAPX * 5, OMAPY * 5, 0.25, false );
}
