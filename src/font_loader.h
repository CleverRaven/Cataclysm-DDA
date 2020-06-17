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
                std::ifstream stream( path.c_str(), std::ifstream::binary );
                JsonIn json( stream );
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

                // Ensure that unifont is always loaded as a fallback font to prevent users from shooting themselves in the foot
                auto ensure_unifont_loaded = []( std::vector<std::string> &font_list ) {
                    if( std::find( std::begin( font_list ), std::end( font_list ), "unifont" ) == font_list.end() ) {
                        font_list.emplace_back( "unifont" );
                    }
                };
                ensure_unifont_loaded( typeface );
                ensure_unifont_loaded( map_typeface );
                ensure_unifont_loaded( overmap_typeface );

            } catch( const std::exception &err ) {
                throw std::runtime_error( std::string( "loading font settings from " ) + path + " failed: " +
                                          err.what() );
            }
        }
        void save( const std::string &path ) const {
            try {
                write_to_file( path, [&]( std::ostream & stream ) {
                    JsonOut json( stream, true ); // pretty-print
                    json.start_object();
                    json.member( "typeface", typeface );
                    json.member( "map_typeface", map_typeface );
                    json.member( "overmap_typeface", overmap_typeface );
                    json.end_object();
                    stream << "\n";
                } );
            } catch( const std::exception &err ) {
                DebugLog( D_ERROR, D_SDL ) << "saving font settings to " << path << " failed: " << err.what();
            }
        }

    public:
        /// @throws std::exception upon any kind of error.
        void load() {
            const std::string fontdata = PATH_INFO::fontdata();
            if( file_exist( fontdata ) ) {
                load_throws( fontdata );
            } else {
                const std::string legacy_fontdata = PATH_INFO::legacy_fontdata();
                load_throws( legacy_fontdata );
                assure_dir_exist( PATH_INFO::config_dir() );
                save( fontdata );
            }
        }
};

#endif // CATA_SRC_FONT_LOADER_H
