#include "overmap_debug.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "game.h"
#include "output.h"
#include "overmapbuffer.h"
#include "overmap_noise.h"
#include "point.h"
#include "regional_settings.h"
#include "string_id.h"
#include "translations.h"
#include "uilist.h"

namespace om_debug
{

void print_noise_maps( const tripoint_abs_omt &curs )
{
    constexpr int WIDTH = 900;
    point_rel_omt offset( WIDTH / 2, WIDTH / 2 );
    const region_settings &settings = overmap_buffer.get_settings( curs );
    om_noise::om_noise_layer_forest forest( curs.xy() - offset, g->get_seed() );
    om_noise::om_noise_layer_floodplain floodplain( curs.xy() - offset, g->get_seed() );
    om_noise::om_noise_layer_lake lake( curs.xy() - offset, g->get_seed() );
    om_noise::om_noise_layer_ocean ocean( curs.xy() - offset, g->get_seed() );
    std::vector<std::pair<om_noise::om_noise_layer *, std::string>> layers{
        {&forest, _( "Forest" ) },
        {&floodplain, _( "Floodplain" ) },
        {&lake, _( "Lake" ) },
        {&ocean, _( "Ocean" ) }
    };
    uilist menu;
    menu.text = _( "Which noise map?  (Centered at the cursor)" );
    for( size_t i = 0; i < layers.size(); ++i ) {
        menu.addentry( i, true, -1, layers[i].second );
    }
    menu.query();

    if( menu.ret < 0 || menu.ret >= static_cast<int>( layers.size() ) ) {
        return;
    }

    float threshold = 0.f;
    bool invert = false;
    if( menu.ret == 0 && settings.overmap_forest ) {
        threshold = settings.overmap_forest->obj().noise_threshold_forest;
    } else if( menu.ret == 1 && settings.place_swamps ) {
        threshold = settings.overmap_forest->obj().noise_threshold_swamp_isolated;
    } else if( menu.ret == 2 && settings.overmap_lake ) {
        if( settings.overmap_lake->obj().invert_lakes ) {
            invert = true;
        }
        threshold = settings.overmap_lake->obj().noise_threshold_lake;
    } else if( menu.ret == 3 && settings.overmap_ocean ) {
        threshold = settings.overmap_ocean->obj().noise_threshold_ocean;
    }


    std::string raw_name = layers[menu.ret].second + "-noise.pgm";
    std::string interp_name = layers[menu.ret].second + "-interp.pgm";

    export_raw_noise( raw_name, *layers[menu.ret].first, 900, 900 );
    export_interpreted_noise( interp_name, *layers[menu.ret].first, 900, 900, threshold, invert );
    popup( _( "Exported noise to %s (raw) and %s." ), raw_name, interp_name );
}

void export_raw_noise( const std::string &filename, const om_noise::om_noise_layer &noise,
                       int width, int height )
{
    std::ofstream testfile;
    testfile.open( std::filesystem::u8path( filename ), std::ofstream::trunc );
    testfile << "P2" << std::endl;
    testfile << width << " " << height << std::endl;
    testfile << "255" << std::endl;

    for( int y = 0; y < height; y++ ) {
        for( int x = 0; x < width; x++ ) {
            if( x == width / 2 && y == width / 2 ) {
                testfile << 255 << " ";
            } else {
                testfile << static_cast<int>( noise.noise_at( {x, y} ) * 255 ) << " ";
            }
        }
        testfile << std::endl;
    }

    testfile.close();
}

void export_interpreted_noise(
    const std::string &filename, const om_noise::om_noise_layer &noise, int width, int height,
    float threshold, bool invert )
{
    std::ofstream testfile;
    testfile.open( std::filesystem::u8path( filename ), std::ofstream::trunc );
    testfile << "P2" << std::endl;
    testfile << width << " " << height << std::endl;
    testfile << "255" << std::endl;

    // invert swaps what is empty and what is full
    const int full = invert ? 0 : 255;
    const int empty = invert ? 255 : 0;
    for( int y = 0; y < height; y++ ) {
        for( int x = 0; x < width; x++ ) {
            if( invert ^ ( noise.noise_at( {x, y} ) > threshold ) ) {
                testfile << full << " ";
            } else {
                testfile << empty << " ";
            }
        }
        testfile << std::endl;
    }

    testfile.close();
}

} // namespace om_debug
