#include <filesystem>
#include <fstream>
#include <string>

#include "cata_catch.h"
#include "coordinates.h"
#include "map_scale_constants.h"
#include "overmap_noise.h"
#include "point.h"

static void export_raw_noise( const std::string &filename, const om_noise::om_noise_layer &noise,
                              int width, int height )
{
    std::ofstream testfile;
    testfile.open( std::filesystem::u8path( filename ), std::ofstream::trunc );
    testfile << "P2" << std::endl;
    testfile << width << " " << height << std::endl;
    testfile << "255" << std::endl;

    for( int x = 0; x < width; x++ ) {
        for( int y = 0; y < height; y++ ) {
            testfile << static_cast<int>( noise.noise_at( {x, y} ) * 255 ) << " ";
        }
        testfile << std::endl;
    }

    testfile.close();
}

static void export_interpreted_noise(
    const std::string &filename, const om_noise::om_noise_layer &noise, int width, int height,
    float threshold )
{
    std::ofstream testfile;
    testfile.open( std::filesystem::u8path( filename ), std::ofstream::trunc );
    testfile << "P2" << std::endl;
    testfile << width << " " << height << std::endl;
    testfile << "255" << std::endl;

    for( int x = 0; x < width; x++ ) {
        for( int y = 0; y < height; y++ ) {
            if( noise.noise_at( {x, y} ) > threshold ) {
                testfile << 255 << " ";
            } else {
                testfile << 0 << " ";
            }
        }
        testfile << std::endl;
    }

    testfile.close();
}

TEST_CASE( "om_noise_layer_forest_export", "[.]" )
{
    const om_noise::om_noise_layer_forest f( point_abs_omt(), 1920237457 );
    export_raw_noise( "forest-map-raw.pgm", f, OMAPX, OMAPY );
}

TEST_CASE( "om_noise_layer_floodplain_export", "[.]" )
{
    const om_noise::om_noise_layer_floodplain f( point_abs_omt(), 1920237457 );
    export_raw_noise( "floodplain-map-raw.pgm", f, OMAPX, OMAPY );
}

TEST_CASE( "om_noise_layer_lake_export", "[.]" )
{
    const om_noise::om_noise_layer_lake f( point_abs_omt(), 1920237457 );
    export_raw_noise( "lake-map-raw.pgm", f, OMAPX * 5, OMAPY * 5 );
    export_interpreted_noise( "lake-map-interp.pgm", f, OMAPX * 5, OMAPY * 5, 0.25 );
}
