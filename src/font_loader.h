#pragma once
#ifndef CATA_SRC_FONT_LOADER_H
#define CATA_SRC_FONT_LOADER_H

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "debug.h"
#include "filesystem.h"
#include "json.h"
#include "path_info.h"
#include "cata_utility.h"

void ensure_unifont_loaded( std::vector<std::string> &font_list );

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
        void load_throws( const std::string &path );
        void save( const std::string &path ) const;

    public:
        /// @throws std::exception upon any kind of error.
        void load();
};

#endif // CATA_SRC_FONT_LOADER_H
