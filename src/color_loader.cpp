#include "color_loader.h"

#include <array>
#include <fstream>
#include <map>
#include <string>

#include "debug.h"
#include "filesystem.h"
#include "json.h"
#include "path_info.h"
#include "platform_win.h"
#if defined(TILES)
#include "sdl_wrappers.h"
#endif

template<typename ColorType>
void color_loader<ColorType>::load_colors( const JsonObject &jsobj )
{
    for( size_t c = 0; c < main_color_names().size(); c++ ) {
        const std::string &color = main_color_names()[c];
        JsonArray jsarr = jsobj.get_array( color );
        consolecolors[color] = from_rgb( jsarr.get_int( 0 ), jsarr.get_int( 1 ), jsarr.get_int( 2 ) );
    }
}

template<typename ColorType>
ColorType color_loader<ColorType>::ccolor( const std::string &color ) const
{
    const auto it = consolecolors.find( color );
    if( it == consolecolors.end() ) {
        throw std::runtime_error( std::string( "requested non-existing color " ) + color );
    }
    return it->second;
}

template<typename ColorType>
void color_loader<ColorType>::load_colorfile( const std::string &path )
{
    std::ifstream colorfile( path.c_str(), std::ifstream::in | std::ifstream::binary );
    JsonIn jsin( colorfile );
    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject jo = jsin.get_object();
        load_colors( jo );
        jo.finish();
    }
}

template<typename ColorType>
void color_loader<ColorType>::load( std::array<ColorType, COLOR_NAMES_COUNT> &windowsPalette )
{
    const std::string default_path = PATH_INFO::colors();
    const std::string custom_path = PATH_INFO::base_colors();

    if( !file_exist( custom_path ) ) {
        copy_file( default_path, custom_path );
    }

    try {
        load_colorfile( custom_path );
    } catch( const JsonError &err ) {
        debugmsg( "Failed to load color data from \"%s\": %s", custom_path, err.what() );

        // this should succeed, otherwise the installation is botched
        load_colorfile( default_path );
    }

    for( size_t c = 0; c < main_color_names().size(); c++ ) {
        windowsPalette[c] = ccolor( main_color_names()[c] );
    }
}

#if defined(TILES)
template void color_loader<SDL_Color>::load_colors( const JsonObject &jsobj );
template SDL_Color color_loader<SDL_Color>::ccolor( const std::string &color ) const;
template void color_loader<SDL_Color>::load_colorfile( const std::string &path );
template void color_loader<SDL_Color>::load( std::array<SDL_Color, COLOR_NAMES_COUNT>
        &windowsPalette );
#endif
#if defined(_WIN32)
template void color_loader<RGBQUAD>::load_colors( const JsonObject &jsobj );
template RGBQUAD color_loader<RGBQUAD>::ccolor( const std::string &color ) const;
template void color_loader<RGBQUAD>::load_colorfile( const std::string &path );
template void color_loader<RGBQUAD>::load( std::array<RGBQUAD, COLOR_NAMES_COUNT> &windowsPalette );
#endif
