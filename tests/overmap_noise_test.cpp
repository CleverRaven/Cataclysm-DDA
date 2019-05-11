#include <fstream>
#include <string>

#include "catch/catch.hpp"
#include "game_constants.h"
#include "overmap_noise.h"

void export_raw_noise( const std::string &filename, const om_noise::om_noise_layer &noise )
{
    std::ofstream testfile;
    testfile.open( filename, std::ofstream::trunc );
    testfile << "P2" << std::endl;
    testfile << "180 180" << std::endl;
    testfile << "255" << std::endl;

    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            testfile << static_cast<int>( noise.noise_at( {x, y} ) * 255 ) << " ";
        }
        testfile << std::endl;
    }

    testfile.close();
}

TEST_CASE( "om_noise_layer_forest_raw_export", "[.]" )
{
    const om_noise::om_noise_layer_forest f( {0, 0}, 1920237457 );
    export_raw_noise( "forest-map-raw.pgm", f );
}

TEST_CASE( "om_noise_layer_floodplain_raw_export", "[.]" )
{
    const om_noise::om_noise_layer_floodplain f( {0, 0}, 1920237457 );
    export_raw_noise( "floodplain-map-raw.pgm", f );
}