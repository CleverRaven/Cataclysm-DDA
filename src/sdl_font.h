#pragma once
#ifndef CATA_SRC_SDL_FONT_H
#define CATA_SRC_SDL_FONT_H
#include "font_loader.h"

#if defined(TILES)

#include "cursesdef.h" // IWYU pragma: associated
#include "sdltiles.h" // IWYU pragma: associated

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>

#include "color.h"
#include "color_loader.h"
#include "debug.h"
#include "glyph_shelf_packer.h"
#include "point.h"
#include "sdl_wrappers.h"

class text_batch;

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

        /// Queue a single character into @p batch; it is drawn when the batch flushes.
        /// @param ch Character to draw
        /// @param p Point on the screen where to draw character
        /// @param color Curses color to use when drawing
        /// @param opacity Optional opacity of the character
        virtual void queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                                 const std::string &ch, const point &p,
                                 unsigned char color, float opacity = 1.0f ) = 0;

        /// Queue an ascii line into @p batch using font's palette.
        /// @param line_id One of the LINE_*_C constants
        /// @param p Point on the screen where to draw character
        /// @param color Curses color to use when drawing
        virtual void queue_ascii_lines( text_batch &batch, unsigned char line_id,
                                        const point &p, unsigned char color ) const;

        /// Draw a single character now, through a one-glyph batch.
        void OutputChar( const SDL_Renderer_Ptr &renderer, const std::string &ch, const point &p,
                         unsigned char color, float opacity = 1.0f );

        /// Drop any GPU-side glyph textures owned by this font. Subclasses
        /// override to clear their own atlases or caches. Path and metadata
        /// kept so the font can be rebuilt against a fresh renderer.
        virtual void release_gpu_resources() {}

        /// Recreate GPU-side glyph textures against `renderer` after a device
        /// reset. Default is a no-op for fonts whose glyphs repopulate lazily
        /// on the next queue_char (the TTF glyph atlas).
        virtual void rebuild_for_renderer( const SDL_Renderer_Ptr &renderer ) {
            ( void )renderer;
        }

        /// Try to load a font by typeface (Bitmap or Truetype).
        static std::unique_ptr<Font> load_font(
            SDL_Renderer_Ptr &renderer, Uint32 pixel_format,
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

/// Font implementation on a TrueType font. Its glyphs are rendered white into
/// atlas pages and colored per draw.
class CachedTTFFont : public Font
{
    public:
        CachedTTFFont(
            int w, int h,
            const palette_array &palette,
            std::string typeface, int fontsize, bool fontblending );
        ~CachedTTFFont() override = default;

        bool isGlyphProvided( const std::string &ch ) const override;
        void queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                         const std::string &ch, const point &p,
                         unsigned char color, float opacity = 1.0f ) override;
        void release_gpu_resources() override;

        /// Entries in the glyph atlas; one per distinct string, whatever its colors.
        size_t atlas_glyph_count() const {
            return glyphs.size();
        }
        /// Atlas page textures created so far.
        size_t atlas_page_count() const {
            return pages.size();
        }
        /// Per-color glyph textures of the immediate path.
        size_t immediate_glyph_count() const {
            return immediate_glyphs.size();
        }

    protected:
        struct glyph_entry {
            // Atlas page with the glyph cell, or -1 if nothing to draw
            int page = -1;
            SDL_Rect cell = { 0, 0, 0, 0 };
        };

        struct immediate_glyph {
            SDL_Texture_Ptr texture;
            int width = 0;
        };

        // render `ch` in `color` into a cell_width x height surface in
        // create_surface_32's format, centred per axis, cropped from centre,
        // null on SDL_ttf failure or non-positive width
        SDL_Surface_Ptr render_glyph_cell( const std::string &ch, int &cell_width,
                                           const SDL_Color &color ) const;
        // texture of `ch` pre-colored with palette `color`, created on first use
        const immediate_glyph &immediate_glyph_for( const SDL_Renderer_Ptr &renderer,
                const std::string &ch, unsigned char color );
        // atlas entry for `ch`, render and upload on first use
        const glyph_entry &glyph_for( const SDL_Renderer_Ptr &renderer, const std::string &ch );
        // texture for `page`, created w x h, cleared when it's the next page;
        // false when that failed, leaving no page behind so a later glyph
        // retries it
        bool ensure_page( const SDL_Renderer_Ptr &renderer, int page, int w, int h );

        TTF_Font_Ptr font;
        std::unordered_map<std::string, glyph_entry> glyphs;
        std::vector<SDL_Texture_Ptr> pages;
        glyph_shelf_packer packer;
        // keyed by the glyph string followed by the palette index
        std::unordered_map<std::string, immediate_glyph> immediate_glyphs;

        const bool fontblending;
};

/// A font created from a bitmap. Each character is taken from a
/// specific area of the source bitmap.
class BitmapFont : public Font
{
    public:
        BitmapFont(
            SDL_Renderer_Ptr &renderer, Uint32 pixel_format,
            int w, int h,
            const palette_array &palette,
            const std::string &typeface_path );
        ~BitmapFont() override = default;

        bool isGlyphProvided( const std::string &ch ) const override;
        void queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                         const std::string &ch, const point &p,
                         unsigned char color, float opacity = 1.0f ) override;
        void queue_ascii_lines( text_batch &batch, unsigned char line_id, const point &p,
                                unsigned char color ) const override;
        void release_gpu_resources() override;
        void rebuild_for_renderer( const SDL_Renderer_Ptr &renderer ) override;
    protected:
        // Queue CP437 glyph `t` from the atlas of `color`.
        void queue_index( text_batch &batch, int t, const point &p, unsigned char color,
                          float opacity ) const;

        // Build the per-color glyph atlases from typeface_path against
        // `renderer`, leaving ascii[] ready for OutputChar.
        void build_textures( const SDL_Renderer_Ptr &renderer, Uint32 pixel_format );

        std::array<SDL_Texture_Ptr, color_loader<SDL_Color>::COLOR_NAMES_COUNT> ascii;
        int tilewidth;
        // Atlas texture size in texels
        point atlas_size;
        // Retained for atlas rebuild after a GPU device-texture reset.
        std::string typeface_path;
        Uint32 source_pixel_format = 0;
};

/// Multiple fonts container. Tries to render character using font on the top,
/// if glyph is not supported, tries next font, and so on.
class FontFallbackList : public Font
{
    public:
        FontFallbackList(
            SDL_Renderer_Ptr &renderer, Uint32 pixel_format,
            int w, int h,
            const palette_array &palette,
            const std::vector<font_config> &typefaces,
            int fontsize, bool fontblending );
        ~FontFallbackList() override = default;

        bool isGlyphProvided( const std::string &ch ) const override;
        void queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                         const std::string &ch, const point &p,
                         unsigned char color, float opacity = 1.0f ) override;
        void release_gpu_resources() override;
        void rebuild_for_renderer( const SDL_Renderer_Ptr &renderer ) override;
    protected:
        std::vector<std::unique_ptr<Font>> fonts;
        std::map<std::string, std::vector<std::unique_ptr<Font>>::iterator> glyph_font;
};

// make the next `count` glyph atlas page creations fail, as a lost device or
// exhausted GPU memory would. tests only; inert until armed
void arm_glyph_page_create_failures( int count );

// whether text on `renderer` goes through the glyph atlas and batched geometry.
// the software renderer draws each glyph and rect immediately: its blitter is
// fastest on small pre-colored textures and opaque fills
bool text_atlas_enabled( const SDL_Renderer_Ptr &renderer );
// force text_atlas_enabled's answer; nullopt restores the renderer check. tests only
void override_text_atlas( std::optional<bool> enabled );

#endif // TILES

#endif // CATA_SRC_SDL_FONT_H
