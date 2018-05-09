#pragma once
#ifndef FONT_LOADER_H
#define FONT_LOADER_H

#include "json.h"
#include "path_info.h"
#include "filesystem.h"
#include "debug.h"

#include <string>
#include <fstream>
#include <stdexcept>

class font_loader
{
    public:
        bool fontblending = false;
        std::string typeface;
        std::string map_typeface;
        std::string overmap_typeface;
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
                config.read( "fontblending", fontblending );
                config.read( "fontwidth", fontwidth );
                config.read( "fontheight", fontheight );
                config.read( "fontsize", fontsize );
                config.read( "typeface", typeface );
                config.read( "map_fontwidth", map_fontwidth );
                config.read( "map_fontheight", map_fontheight );
                config.read( "map_fontsize", map_fontsize );
                config.read( "map_typeface", map_typeface );
                config.read( "overmap_fontwidth", overmap_fontwidth );
                config.read( "overmap_fontheight", overmap_fontheight );
                config.read( "overmap_fontsize", overmap_fontsize );
                config.read( "overmap_typeface", overmap_typeface );
            } catch( const std::exception &err ) {
                throw std::runtime_error( std::string( "loading font settings from " ) + path + " failed: " +
                                          err.what() );
            }
        }
        void save( const std::string &path ) const {
            std::ofstream stream( path.c_str(), std::ofstream::binary );
            JsonOut json( stream, true ); // pretty-print
            json.start_object();
            json.member( "fontblending", fontblending );
            json.member( "fontwidth", fontwidth );
            json.member( "fontheight", fontheight );
            json.member( "fontsize", fontsize );
            json.member( "typeface", typeface );
            json.member( "map_fontwidth", map_fontwidth );
            json.member( "map_fontheight", map_fontheight );
            json.member( "map_fontsize", map_fontsize );
            json.member( "map_typeface", map_typeface );
            json.member( "overmap_fontwidth", overmap_fontwidth );
            json.member( "overmap_fontheight", overmap_fontheight );
            json.member( "overmap_fontsize", overmap_fontsize );
            json.member( "overmap_typeface", overmap_typeface );
            json.end_object();
            stream << "\n";
            stream.close();
            if( !stream.good() ) {
                DebugLog( D_ERROR, D_SDL ) << "saving font settings to " << path << " failed";
            }
        }

    public:
        /// @throws std::exception upon any kind of error.
        void load() {
            const std::string fontdata = FILENAMES["fontdata"];
            const std::string legacy_fontdata = FILENAMES["legacy_fontdata"];
            if( file_exist( fontdata ) ) {
                load_throws( fontdata );
            } else {
                load_throws( legacy_fontdata );
                assure_dir_exist( FILENAMES["config_dir"] );
                save( fontdata );
            }
        }
};

#endif
