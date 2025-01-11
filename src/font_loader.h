#pragma once
#ifndef CATA_SRC_FONT_LOADER_H
#define CATA_SRC_FONT_LOADER_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_freetype.h>
#undef IMGUI_DEFINE_MATH_OPERATORS

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

// The font-configuration values modifiable by the user.
struct font_config {
    // Path to the font file.
    std::string path;
    // The type of hinting to apply.
#if defined(IMGUI)
    std::optional<ImGuiFreeTypeBuilderFlags> hinting = std::nullopt;
#endif
    // In practice, antialiasing will be ignored when hinting is set to FontHint::Bitmap.
    bool antialiasing = true;

    font_config() = default;

    explicit font_config( std::string path ) : path( std::move( path ) ) {}
#if defined(IMGUI)
    font_config( std::string path,
                 const std::optional<ImGuiFreeTypeBuilderFlags> hinting ) : path( std::move( path ) ),
        hinting( hinting ) {}
    font_config( std::string path, const std::optional<ImGuiFreeTypeBuilderFlags> hinting,
                 const bool antialiasing ) : path( std::move( path ) ), hinting( hinting ),
        antialiasing( antialiasing ) {}
#endif

    // Returns the font flags that should be passed to an ImFontConfig.
    unsigned int imgui_config() const;

    void deserialize( const JsonObject &jo );
};


extern void ensure_unifont_loaded( std::vector<font_config> &font_list );
extern void ensure_unifont_loaded( std::vector<std::string> &font_list );


class font_loader
{
    public:
        bool fontblending = false;
        std::vector<font_config> typeface;
        std::vector<font_config> map_typeface;
        std::vector<font_config> gui_typeface;
        std::vector<font_config> overmap_typeface;
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
