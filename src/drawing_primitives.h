#pragma once
#ifndef CATA_SRC_DRAWING_PRIMITIVES_H
#define CATA_SRC_DRAWING_PRIMITIVES_H

#include <functional>

struct point;
struct rl_vec2d;

void draw_line( std::function<void( const point & )>set, const point &p1, const point &p2 );

void draw_square( std::function<void( const point & )>set, point p1, point p2 );

void draw_rough_circle( std::function<void( const point & )>set, const point &p, int rad );

void draw_circle( std::function<void( const point & )>set, const rl_vec2d &p, double rad );

void draw_circle( std::function<void( const point & )>set, const point &p, int rad );

#endif // CATA_SRC_DRAWING_PRIMITIVES_H
