#pragma once
#ifndef CATA_SRC_FONT_LOADER_H
#define CATA_SRC_FONT_LOADER_H

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "debug.h"
#include "filesystem.h"
#include "json.h"
#include "path_info.h"
#include "cata_utility.h"

// Ensure that unifont is always loaded as a fallback font to prevent users from shooting themselves in the foot
static void ensure_unifont_loaded( std::vector<std::string> &font_list )
{
    if( std::find( std::begin( font_list ), std::end( font_list ), "unifont" ) == font_list.end() ) {
        font_list.emplace_back( PATH_INFO::fontdir() + "unifont.ttf" );
    }
}

class font_loader
{
    public:
        bool fontblending = false;
        std::vector<std::string> typeface;
        std::vector<std::string> map_typeface;
        std::vector<std::string> overmap_typeface;
        int fontwidth = 8;
        int fontheight = 16;
        int fontsize = 16;
        int map_fontwidth = 8;
        int map_fontheight = 16;
        int map_fontsize = 16;
        int overmap_fontwidth = 8;
        int overmap_fontheight = 16;
        int overmap_fontsize = 16;

    private:
        void load_throws( const std::string &path ) {
            try {
                cata_ifstream stream = std::move( cata_ifstream().binary( true ).open( path ) );
                JsonIn json( *stream );
                JsonObject config = json.get_object();
                if( config.has_string( "typeface" ) ) {
                    typeface.emplace_back( config.get_string( "typeface" ) );
                } else {
                    config.read( "typeface", typeface );
                }
                if( config.has_string( "map_typeface" ) ) {
                    map_typeface.emplace_back( config.get_string( "map_typeface" ) );
                } else {
                    config.read( "map_typeface", map_typeface );
                }
                if( config.has_string( "overmap_typeface" ) ) {
                    overmap_typeface.emplace_back( config.get_string( "overmap_typeface" ) );
                } else {
                    config.read( "overmap_typeface", overmap_typeface );
                }

                ensure_unifont_loaded( typeface );
                ensure_unifont_loaded( map_typeface );
                ensure_unifont_loaded( overmap_typeface );

            } catch( const std::exception &err ) {
                throw std::runtime_error( std::string( "loading font settings from " ) + path + " failed: " +
                                          err.what() );
            }
        }

    public:
        void load() {
            const std::string user_fontconfig = PATH_INFO::user_fontconfig();
            const std::string fontconfig = PATH_INFO::fontconfig();
            bool try_user = true;
            if( !file_exist( user_fontconfig ) ) {
                if( !copy_file( fontconfig, user_fontconfig ) ) {
                    try_user = false;
                    DebugLog( D_ERROR, D_SDL ) << "failed to create user font config file "
                                               << user_fontconfig;
                }
            }
            if( try_user ) {
                font_loader copy = *this;
                try {
                    load_throws( user_fontconfig );
                    return;
                } catch( const std::exception &e ) {
                    DebugLog( D_ERROR, D_SDL ) << e.what();
                }
                *this = copy;
            }
            try {
                load_throws( fontconfig );
            } catch( const std::exception &e ) {
                DebugLog( D_ERROR, D_SDL ) << e.what();
                abort();
            }
        }
};

#endif // CATA_SRC_FONT_LOADER_H
