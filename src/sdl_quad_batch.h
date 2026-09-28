#pragma once
#ifndef CATA_SRC_SDL_QUAD_BATCH_H
#define CATA_SRC_SDL_QUAD_BATCH_H

#if defined(TILES)

#include <cstddef>
#include <utility>
#include <vector>

#include "sdl_wrappers.h"

SDL_FColor to_fcolor( const SDL_Color &color );

// CPU-side quads for one SDL_RenderGeometry call, as SDL_Vertex so position,
// color and uv share one stride. A beacon-heavy minimap frame exceeds 65535
// vertices, hence int indices.
class quad_batch
{
    public:
        void append_quad( float x, float y, float w, float h, const SDL_FColor &color );
        // `src` in texels of a tex_w x tex_h texture
        void append_textured_quad( const SDL_Rect &dst, const SDL_Rect &src, int tex_w, int tex_h,
                                   const SDL_FColor &color );
        void clear();
        bool empty() const;
        int vertex_count() const;
        int index_count() const;
        const SDL_Vertex *vertex_data() const;
        const int *index_data() const;
        void reserve_quads( int quads );

    private:
        void append( float x, float y, float w, float h, const SDL_FColor &color,
                     float u0, float v0, float u1, float v1 );
        std::vector<SDL_Vertex> vertices_;
        std::vector<int> indices_;
};

void render_quad_batch( const SDL_Renderer_Ptr &renderer, const quad_batch &batch,
                        SDL_Texture *texture = nullptr );

// solid rects and glyph quads of one text pass. flush() draws every rect, then
// the glyph quads grouped by texture in first-use order. callers guarantee no
// glyph quad overlaps another texture's glyph quad or a rect appended after it,
// and flush first where one would
class text_batch
{
    public:
        void add_rect( const SDL_Rect &rect, const SDL_Color &color );
        void add_glyph( SDL_Texture *texture, int tex_w, int tex_h, const SDL_Rect &dst,
                        const SDL_Rect &src, const SDL_FColor &color );
        bool empty() const;
        void flush( const SDL_Renderer_Ptr &renderer );

    private:
        quad_batch rects_;
        // groups past active_groups_ are spare: texture reset to null, buffers
        // cleared but keeping their capacity for the next pass
        std::vector<std::pair<SDL_Texture *, quad_batch>> glyphs_;
        size_t active_groups_ = 0;
};

#endif // TILES
#endif // CATA_SRC_SDL_QUAD_BATCH_H
