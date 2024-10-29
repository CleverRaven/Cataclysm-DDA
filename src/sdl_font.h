#pragma once
#ifndef CATA_SRC_SDL_FONT_H
#define CATA_SRC_SDL_FONT_H

#if defined(TILES)

#include "cursesdef.h" // IWYU pragma: associated
#include "sdltiles.h" // IWYU pragma: associated

#include <array>
#include <map>
#include <memory>
#include <vector>
#include <string>

#include "sdl_geometry.h"
#include "color.h"
#include "color_loader.h"
#include "debug.h"
#include "point.h"
#include "hash_utils.h"
#include "sdl_wrappers.h"

using palette_array = std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT>;

/// Interface which is capable of rendering a single character on the screen.
class Font
{
    public:
        Font( int w, int h, const palette_array &palette ) :
            width( w ), height( h ), palette( palette ) { }
        virtual ~Font() = default;

        /// @return `true` if font is able to render utf-8 encoded symbol @p ch.
        virtual bool isGlyphProvided( const std::string &ch ) const = 0;

        /// Draw a single character using font's palette.
        /// @param ch Character to draw
        /// @param p Point on the screen where to draw character
        /// @param color Curses color to use when drawing
        /// @param opacity Optional opacity of the character
        virtual void OutputChar( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,
                                 const std::string &ch, const point &p,
                                 unsigned char color, float opacity = 1.0f ) = 0;

        /// Draw an ascii line using font's palette.
        /// @param line_id Character to draw
        /// @param point Point on the screen where to draw character
        /// @param color Curses color to use when drawing
        virtual void draw_ascii_lines( const SDL_Renderer_Ptr &renderer,
                                       const GeometryRenderer_Ptr &geometry,
                                       unsigned char line_id, const point &p, unsigned char color ) const;

        /// Try to load a font by typeface (Bitmap or Truetype).
        static std::unique_ptr<Font> load_font(
            SDL_Renderer_Ptr &renderer, SDL_PixelFormat_Ptr &format,
            const std::string &typeface, int fontsize, int fontwidth,
            int fontheight,
            const palette_array &palette,
            bool fontblending );

        // the width of the font, background is always this size.
        int width;
        // the height of the font, background is always this size.
        int height;
        // font palette.
        const palette_array &palette;
};
using Font_Ptr = std::unique_ptr<Font>;

/// Font implementation on a TrueType font. Its glyphs are cached.
class CachedTTFFont : public Font
{
    public:
        CachedTTFFont(
            int w, int h,
            const palette_array &palette,
            std::string typeface, int fontsize, bool fontblending );
        ~CachedTTFFont() override = default;

        bool isGlyphProvided( const std::string &ch ) const override;
        void OutputChar( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,
                         const std::string &ch,
                         const point &p,
                         unsigned char color, float opacity = 1.0f ) override;

    protected:
        SDL_Texture_Ptr create_glyph( const SDL_Renderer_Ptr &renderer, const std::string &ch,
                                      int &ch_width, int color );

        TTF_Font_Ptr font;
        // Maps (character code, color) to SDL_Texture*

        struct key_t {
            std::string   codepoints;
            unsigned char color;

            std::pair<std::string, unsigned char> as_pair() const {
                return std::make_pair( codepoints, color );
            }

            friend bool operator==( const key_t &lhs, const key_t &rhs ) noexcept {
                return lhs.as_pair() == rhs.as_pair();
            }
        };

        struct key_t_hash {
            size_t operator()( const key_t &k ) const {
                return cata::auto_hash<std::pair<std::string, unsigned char>>()( k.as_pair() );
            }
        };

        struct cached_t {
            SDL_Texture_Ptr texture;
            int          width;
        };

        std::unordered_map<key_t, cached_t, key_t_hash> glyph_cache_map;

        const bool fontblending;
};

/// A font created from a bitmap. Each character is taken from a
/// specific area of the source bitmap.
class BitmapFont : public Font
{
    public:
        BitmapFont(
            SDL_Renderer_Ptr &renderer, SDL_PixelFormat_Ptr &format,
            int w, int h,
            const palette_array &palette,
            const std::string &typeface_path );
        ~BitmapFont() override = default;

        bool isGlyphProvided( const std::string &ch ) const override;
        void OutputChar( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,
                         const std::string &ch,
                         const point &p,
                         unsigned char color, float opacity = 1.0f ) override;
        void OutputChar( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,  int t,
                         const point &p,
                         unsigned char color, float opacity = 1.0f );
        void draw_ascii_lines( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,
                               unsigned char line_id, const point &p, unsigned char color ) const override;
    protected:
        std::array<SDL_Texture_Ptr, color_loader<SDL_Color>::COLOR_NAMES_COUNT> ascii;
        int tilewidth;
};

/// Multiple fonts container. Tries to render character using font on the top,
/// if glyph is not supported, tries next font, and so on.
class FontFallbackList : public Font
{
    public:
        FontFallbackList(
            SDL_Renderer_Ptr &renderer, SDL_PixelFormat_Ptr &format,
            int w, int h,
            const palette_array &palette,
            const std::vector<std::string> &typefaces,
            int fontsize, bool fontblending );
        ~FontFallbackList() override = default;

        bool isGlyphProvided( const std::string &ch ) const override;
        void OutputChar( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,
                         const std::string &ch,
                         const point &p,
                         unsigned char color, float opacity = 1.0f ) override;
    protected:
        std::vector<std::unique_ptr<Font>> fonts;
        std::map<std::string, std::vector<std::unique_ptr<Font>>::iterator> glyph_font;
};

#endif // TILES

#endif // CATA_SRC_SDL_FONT_H
