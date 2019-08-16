#pragma once
#ifndef DRAWING_PRIMITIVES_H
#define DRAWING_PRIMITIVES_H

#include <functional>

struct point;
struct rl_vec2d;

void draw_line( std::function<void( const int, const int )>set, int x1, int y1, int x2, int y2 );
void draw_line( std::function<void( const int, const int )>set, const point &p1, const point &p2 );

void draw_square( std::function<void( const int, const int )>set, int x1, int y1, int x2, int y2 );
void draw_square( std::function<void( const int, const int )>set, const point &p1,
                  const point &p2 );

void draw_rough_circle( std::function<void( const int, const int )>set, int x, int y, int rad );
void draw_rough_circle( std::function<void( const int, const int )>set, const point &p, int rad );

void draw_circle( std::function<void( const int, const int )>set, double x, double y, double rad );
void draw_circle( std::function<void( const int, const int )>set, const rl_vec2d &p, double rad );

void draw_circle( std::function<void( const int, const int )>set, int x, int y, int rad );
void draw_circle( std::function<void( const int, const int )>set, const point &p, int rad );

#endif
