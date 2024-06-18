#pragma once
#ifndef CATA_SRC_COLOR_LOADER_H
#define CATA_SRC_COLOR_LOADER_H

#include <array>
#include <map>
#include <string>

#include "debug.h"
#include "filesystem.h"
#include "json.h"
#include "json_loader.h"
#include "path_info.h"

template<typename ColorType>
class color_loader
{
    public:
        static constexpr size_t COLOR_NAMES_COUNT = 16;

    private:
        static ColorType from_rgb( int r, int g, int b );

        std::map<std::string, ColorType> consolecolors;

        // color names as read from the json file
        static const std::array<std::string, COLOR_NAMES_COUNT> &main_color_names() {
            static const std::array<std::string, COLOR_NAMES_COUNT> names{ { "BLACK", "RED", "GREEN",
                    "BROWN", "BLUE", "MAGENTA", "CYAN", "GRAY", "DGRAY", "LRED", "LGREEN", "YELLOW",
                    "LBLUE", "LMAGENTA", "LCYAN", "WHITE"
                } };
            return names;
        }

        void load_colors( const JsonObject &jsobj ) {
            for( const std::string &color : main_color_names() ) {
                JsonArray jsarr = jsobj.get_array( color );
                consolecolors[color] = from_rgb( jsarr.get_int( 0 ), jsarr.get_int( 1 ), jsarr.get_int( 2 ) );
            }
        }
        ColorType ccolor( const std::string &color ) const {
            const auto it = consolecolors.find( color );
            if( it == consolecolors.end() ) {
                throw std::runtime_error( std::string( "requested non-existing color " ) + color );
            }
            return it->second;
        }

        void load_colorfile( const cata_path &path ) {
            JsonValue jsin = json_loader::from_path( path );
            for( JsonObject jo : jsin.get_array() ) {
                // This isn't actually read (here), so just ignore it
                jo.get_string( "type" );
                load_colors( jo );
            }
        }

    public:
        /// @throws std::exception upon any kind of error.
        void load( std::array<ColorType, COLOR_NAMES_COUNT> &windowsPalette ) {
            const cata_path default_path = PATH_INFO::colors();
            const cata_path custom_path = PATH_INFO::base_colors();

            if( !file_exist( custom_path ) ) {
                assure_dir_exist( PATH_INFO::config_dir() );
                copy_file( default_path, custom_path );
            }

            try {
                load_colorfile( custom_path );
            } catch( const JsonError &err ) {
                debugmsg( "Failed to load color data from \"%s\": %s", custom_path.generic_u8string(), err.what() );

                // this should succeed, otherwise the installation is botched
                load_colorfile( default_path );
            }

            for( size_t c = 0; c < main_color_names().size(); c++ ) {
                windowsPalette[c] = ccolor( main_color_names()[c] );
            }
        }
};

#endif // CATA_SRC_COLOR_LOADER_H
