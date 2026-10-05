#pragma once
#ifndef CATA_SRC_PIXEL_MINIMAP_GEOMETRY_H
#define CATA_SRC_PIXEL_MINIMAP_GEOMETRY_H

#if defined(TILES)

#include "sdl_wrappers.h"

class quad_batch;
struct point;

// The screen mapping of the minimap: screen = origin + scale * native,
// per axis, clipped to dest_rect. Axis scales differ because
// fit_rect_inside truncates each axis separately.
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
void append_beacon( quad_batch &batch, const SDL_Rect &rect,
                    const SDL_Color &color, int edge_divisor );

#endif // TILES
#endif // CATA_SRC_PIXEL_MINIMAP_GEOMETRY_H
