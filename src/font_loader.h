#pragma once
#ifndef CATA_SRC_FONT_LOADER_H
#define CATA_SRC_FONT_LOADER_H

#if defined( TILES )

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "cata_utility.h"
#include "debug.h"
#include "filesystem.h"
#include "json.h"
#include "path_info.h"

extern void ensure_unifont_loaded( std::vector<std::string> &font_list );

class font_loader
{
    public:
        bool fontblending = false;
        std::vector<std::string> typeface;
        std::vector<std::string> gui_typeface;
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
        void load_throws( const cata_path &path );
        void save( const cata_path &path ) const;

    public:
        /// @throws std::exception upon any kind of error.
        void load();
};

#endif // TILES

#endif // CATA_SRC_FONT_LOADER_H
