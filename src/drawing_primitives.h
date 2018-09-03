#pragma once
#ifndef DRAWING_PRIMITIVES_H
#define DRAWING_PRIMITIVES_H

#include <functional>

void draw_line( std::function<void( const int, const int )>set, int x1, int y1, int x2, int y2 );

void draw_square( std::function<void( const int, const int )>set, int x1, int y1, int x2, int y2 );

void draw_rough_circle( std::function<void( const int, const int )>set, int x, int y, int rad );

void draw_circle( std::function<void( const int, const int )>set, double x, double y, double rad );

void draw_circle( std::function<void( const int, const int )>set, int x, int y, int rad );

#endif
