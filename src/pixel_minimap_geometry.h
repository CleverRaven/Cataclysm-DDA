#pragma once
#ifndef CATA_SRC_PIXEL_MINIMAP_GEOMETRY_H
#define CATA_SRC_PIXEL_MINIMAP_GEOMETRY_H

#if defined(TILES)

#include <vector>

#include "sdl_wrappers.h"

struct point;

// CPU-side vertex accumulator for one untextured SDL_RenderGeometryRaw
// call. Terrain alone peaks at 58564 vertices (121x121 tiles x 4), and
// beacons push the total past the 16-bit index range, so indices are 32-bit.
class minimap_vertex_batch
{
    public:
        void append_quad( float x, float y, float w, float h, const SDL_FColor &color );
        void clear();
        bool empty() const;
        int vertex_count() const;
        int index_count() const;
        const float *xy_data() const;
        const SDL_FColor *color_data() const;
        const Uint32 *index_data() const;
        void reserve_quads( int quads );

    private:
        std::vector<float> xy_;
        std::vector<SDL_FColor> colors_;
        std::vector<Uint32> indices_;
};

SDL_FColor to_fcolor( const SDL_Color &color );

// The screen mapping of the minimap: screen = origin + scale * native,
// per axis, clipped to dest_rect. scale_x and scale_y are computed
// independently because fit_rect_inside truncates width and height to int
// separately; a shared scale would underdraw one axis.
struct minimap_transform {
    SDL_Rect dest_rect = { 0, 0, 0, 0 };
    float origin_x = 0.0f;
    float origin_y = 0.0f;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
};

minimap_transform compute_minimap_transform( const point &native,
        const SDL_Rect &screen_rect, bool scale_to_fit );

// Screen pixel for a native-space coordinate on one axis.
int snap_to_pixel( float origin, float scale, int native );

// One 1x1 quad per diamond pixel; outline pixels are darkened by edge_divisor.
void append_beacon( minimap_vertex_batch &batch, const SDL_Rect &rect,
                    const SDL_Color &color, int edge_divisor );

void render_batch( const SDL_Renderer_Ptr &renderer, const minimap_vertex_batch &batch );

#endif // TILES
#endif // CATA_SRC_PIXEL_MINIMAP_GEOMETRY_H
