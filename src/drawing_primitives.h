#pragma once
#ifndef CATA_SRC_DRAWING_PRIMITIVES_H
#define CATA_SRC_DRAWING_PRIMITIVES_H

#include <functional>

#include "coords_fwd.h"

struct rl_vec2d;

void draw_line( const std::function<void( const point_bub_ms & )> &set, const point_bub_ms &p1,
                const point_bub_ms &p2 );

void draw_square( const std::function<void( const point_bub_ms & )> &set, point_bub_ms p1,
                  point_bub_ms p2 );

void draw_rough_circle( const std::function<void( const point_bub_ms & )> &set,
                        const point_bub_ms &p, int rad );

void draw_circle( const std::function<void( const point_bub_ms & )> &set, const rl_vec2d &p,
                  double rad );

void draw_circle( const std::function<void( const point_bub_ms & )> &set, const point_bub_ms &p,
                  int rad );

#endif // CATA_SRC_DRAWING_PRIMITIVES_H
